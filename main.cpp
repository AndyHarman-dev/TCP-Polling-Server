#include <filesystem>
#include <iostream>
#include <string>
#include <unordered_map>
#include <netdb.h>
#include <expected>
#include <csignal>
#include <unistd.h>
#include <arpa/inet.h>
#include <poll.h>

std::unordered_map<std::string, std::string> parse_args(int argc, char* argv[]) {
    std::unordered_map<std::string, std::string> args;
    int positional = 0;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg.rfind("--", 0) == 0) {
            std::string body = arg.substr(2);
            auto eq = body.find('=');
            if (eq != std::string::npos) {
                args[body.substr(0, eq)] = body.substr(eq + 1);
            } else if (i + 1 < argc && std::string(argv[i + 1]).rfind("--", 0) != 0) {
                args[body] = argv[++i];
            } else {
                args[body] = "";
            }
        } else {
            args[std::to_string(positional++)] = arg;
        }
    }
    return args;
}

namespace config {
    std::filesystem::path glog_filepath = "./server_msgs.log";
    std::string gport = "0000";

    constexpr int GDEFAULT_POLL_SIZE = 5;
}

namespace app {
    std::atomic<bool> gstop{false};
    int errcode = 0;

    void stop_with_errcode(int code) {
        gstop.store(true);
        errcode = code;
    }
}

void* get_in_addr(sockaddr* sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

std::expected<int, std::string> get_listen_socket() {

    addrinfo hints, *server_info;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // auto ip

    if (int rv; (rv = getaddrinfo(nullptr, config::gport.c_str(), &hints, &server_info)) != 0) {
        std::cerr << "getaddrinfo() failed: " << gai_strerror(rv) << std::endl;
        return std::unexpected(std::format("getaddrinfo() failed: {0}", gai_strerror(rv)));
    }

    addrinfo *p;
    int file_descriptor = -1;
    for (p = server_info; p != nullptr; p = p->ai_next) {

        if ((file_descriptor = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            continue;
        }

        // Clear socket
        int yes = 1;
        if (setsockopt(file_descriptor, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            return std::unexpected(std::format("setsockopt() failed: {0}", strerror(errno)));
        }

        if (bind(file_descriptor, p->ai_addr, p->ai_addrlen) == -1) {
            return std::unexpected(std::format("bind() failed: {0}", strerror(errno)));
        }

        break;
    }

    if (file_descriptor == -1) {
        return std::unexpected(std::format("get_listen_socket() failed: {0}", strerror(errno)));
    }

    if (listen(file_descriptor, SOMAXCONN) == -1) {
        return std::unexpected(std::format("listen() failed: {0}", strerror(errno)));
    }

    char _[INET6_ADDRSTRLEN];
    const auto listening_ip = inet_ntop(
        p->ai_family,
        get_in_addr(p->ai_addr),
        _, sizeof _
    );

    freeaddrinfo(server_info);

    std::printf("Listening at %s", listening_ip);
    return file_descriptor;
}

void handle_sigchld(int) {
    while (waitpid(-1, nullptr, WNOHANG) > 0);
}

void handle_sigint(int) {
    app::gstop.store(true);
}

bool setup_sigint_sigterm_bindigns() {
    struct sigaction sa_chld;
    sa_chld.sa_handler = handle_sigchld;
    sigemptyset(&sa_chld.sa_mask);
    sa_chld.sa_flags = SA_RESTART;

    if (sigaction(SIGCHLD, &sa_chld, nullptr) == -1) {
        std::cout << std::format("sigaction(SIGCHLD) failed: {0}", strerror(errno)) << std::endl;
        return false;
    }

    struct sigaction sa_int;
    sa_int.sa_handler = handle_sigint;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = 0;
    if (sigaction(SIGINT, &sa_int, nullptr) == -1) {
        std::cout << std::format("sigaction(SIGINT) failed: {0}", strerror(errno)) << std::endl;
        return false;
    }

    return true;
}

// Wrapper for pollfd with auto cleaning and polling
class polling_file_descriptors {
    int size{5};
    int count{0};
    pollfd* inner_ptr = nullptr;

public:
    polling_file_descriptors(int size) : size(size) {
        inner_ptr = (pollfd*)malloc(sizeof *inner_ptr * size);
    }

    ~polling_file_descriptors() {
        if (inner_ptr != nullptr) {
            free(inner_ptr);
        }
    }

    // Interface functions
    void add(int new_fd, short events = POLLIN) {
        if (count == size) {
            size *= 2;
            inner_ptr = (pollfd*)realloc(inner_ptr, sizeof *inner_ptr * size);
        }

        inner_ptr[count].fd = new_fd;
        inner_ptr[count].events = events;
        inner_ptr[count].revents = 0;
        ++count;
    }

    auto& at(int index) {
        return inner_ptr[index];
    }

    void remove_at(int index) {
        at(index) = at(count - 1);
        --count;
    }

    [[nodiscard]] bool is_valid_entry(int index) const {
        return 0 <= index && index < count;
    }

    [[nodiscard]] bool is_ready_at(int index, short events) const {
        return is_valid_entry(index) && inner_ptr[index].revents & (events);
    }

    [[nodiscard]] int poll(int timeout) const {
        return ::poll(inner_ptr, count, timeout);
    }

    [[nodiscard]] int get_count() const
    { return count; }

    [[nodiscard]] int get_size() const
    { return size; }
};

void handle_new_connection(int listening_socket, polling_file_descriptors& pfds) {
    sockaddr_storage client_address;
    socklen_t addrlen = sizeof client_address;;

    int new_file_desc = accept(listening_socket, (sockaddr*)&client_address, &addrlen);
    if (new_file_desc == -1) {
        std::cerr << std::format("accept() failed: {0}", strerror(errno)) << std::endl;
        return;
    }

    pfds.add(new_file_desc, POLLIN);
    std::printf("New connections on socket: %d\n", new_file_desc);
}

void handle_client_data(int listening_socket, polling_file_descriptors & pfds, int& i) {
    char buffer[256]; // data;

    int bytes_received = recv(pfds.at(i).fd, buffer, sizeof(buffer), 0);
    if (bytes_received <= 0) {
        // The case of client hanging up
        if (bytes_received == 0) {
            std::cerr << "Client" << pfds.at(i).fd << "hung up" << std::endl;
        }
        else {
            std::cerr << std::format("recv() failed: {0}", strerror(errno)) << std::endl;
        }

        // Close a broken connection
        close(pfds.at(i).fd);
        pfds.remove_at(i);

        i--; // acount for the removal for the loop's index.
    }
    else {
        std::printf("server: received from fd: %d: %s", pfds.at(i).fd, buffer);

        // broadcast to all the clinets received data
    }
}

int main(int argc, char* argv[]) {

    auto args = parse_args(argc, argv);
    if (args.empty()) {
        std::cerr << "Usage: server <port> [options]" << std::endl;
        return 1;
    }

    config::gport = args["0"]; // first argument is the port.

    if (args.size() == 1) {
        std::printf("No log output file was provided. Dumping locally at %s\n", config::glog_filepath.string().c_str());
    }
    else if (args.size() == 2) {
        config::glog_filepath = args["log-file"];
        std::printf("%s\n", config::glog_filepath.c_str());
    }

    if (!setup_sigint_sigterm_bindigns()) {
        return 1;
    }

    auto expected_socket = get_listen_socket();
    if (!expected_socket.has_value()) {
        std::cerr << expected_socket.error() << std::endl;
        return 1;
    }

    auto listening_socket = expected_socket.value();

    polling_file_descriptors pfds(config::GDEFAULT_POLL_SIZE);
    pfds.add(listening_socket, POLLIN); // acount for listening socket

    while (!app::gstop.load()) {
        auto poll_count = pfds.poll(-1);
        if (poll_count == -1) {
            std::cerr << "Poll() error" << std::endl;
            app::stop_with_errcode(1);
            break;
        }

        for (int i = 0; i < pfds.get_count(); ++i) {
            if (pfds.is_ready_at(i, (POLLIN | POLLHUP))) {
                handle_new_connection(listening_socket, pfds);
            }
            else {
                handle_client_data(listening_socket, pfds, i);
            }
        }
    }

    close(listening_socket);
    return app::errcode;
}
