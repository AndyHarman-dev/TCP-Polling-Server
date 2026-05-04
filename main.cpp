#include <filesystem>
#include <iostream>
#include <string>
#include <unordered_map>
#include <netdb.h>
#include <expected>
#include <csignal>
#include <fstream>
#include <unistd.h>
#include <arpa/inet.h>
#include <poll.h>
#include <vector>

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
    std::atomic<int> errcode = 0;

    void stop_with_errcode(int code) {
        gstop.store(true);
        errcode = code;
    }

    std::unique_ptr<std::ofstream> glog_file_stream = nullptr;
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
            close(file_descriptor);
            continue;
        }

        if (bind(file_descriptor, p->ai_addr, p->ai_addrlen) == -1) {
            close(file_descriptor);
            continue;
        }

        break;
    }

    if (p == nullptr) {
        freeaddrinfo(server_info);
        return std::unexpected(std::format("either socket() or bind() failed: {0}", strerror(errno)));
    }

    char _[INET6_ADDRSTRLEN];
    const auto listening_ip = inet_ntop(
        p->ai_family,
        get_in_addr(p->ai_addr),
        _, sizeof _
    );

    freeaddrinfo(server_info);
    if (listen(file_descriptor, SOMAXCONN) == -1) {
        return std::unexpected(std::format("listen() failed: {0}", strerror(errno)));
    }

    std::printf("Listening at %s\n", listening_ip);

    return file_descriptor;
}

void handle_sigterm(int) {
    std::cout << "Handled SIGTERM!";
    app::gstop.store(true);
}

void handle_sigint(int) {
    std::cout << "Handled SIGINT!";
    app::gstop.store(true);
}

bool setup_sigint_sigterm_bindigns() {
    struct sigaction sa_term;
    sa_term.sa_handler = handle_sigterm;
    sigemptyset(&sa_term.sa_mask);
    sa_term.sa_flags = SA_RESTART;

    if (sigaction(SIGTERM, &sa_term, nullptr) == -1) {
        std::cout << std::format("sigaction(SIGTERM) failed: {0}", strerror(errno)) << std::endl;
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
    std::vector<pollfd> inner_vec;

public:
    polling_file_descriptors(int size) {
        inner_vec.reserve(size);
    }

    // Interface functions
    void add(int new_fd, short events = POLLIN) {
        inner_vec.push_back(
            pollfd{
                new_fd,
                events,
                0
            }
        );
    }

    auto& at(int index) {
        return inner_vec[index];
    }

    void remove_at(int index) {
        inner_vec.erase(inner_vec.begin() + index);
    }

    [[nodiscard]] bool is_valid_entry(int index) const {
        return 0 <= index && index < get_count();
    }

    [[nodiscard]] bool is_ready_at(int index, short events) const {
        return is_valid_entry(index) && inner_vec[index].revents & (events);
    }

    [[nodiscard]] int poll(int timeout) {
        return ::poll(inner_vec.data(), get_count(), timeout);
    }

    [[nodiscard]] size_t get_count() const
    { return inner_vec.size(); }
};

void handle_new_connection(int listening_socket, polling_file_descriptors& pfds) {
    sockaddr_storage client_address;
    socklen_t addrlen = sizeof client_address;

    int new_file_desc = accept(listening_socket, (sockaddr*)&client_address, &addrlen);
    if (new_file_desc == -1) {
        std::cerr << std::format("accept() failed: {0}", strerror(errno)) << std::endl;
        return;
    }

    pfds.add(new_file_desc, POLLIN);
    std::printf("New connections on socket: %d\n", new_file_desc);
}

void handle_client_data(polling_file_descriptors & pfds, int& i) {
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
        std::printf("server: received from fd: %d: %.*s\n", pfds.at(i).fd, bytes_received, buffer);

        if (app::glog_file_stream) {
            app::glog_file_stream->write(buffer, bytes_received);
            app::glog_file_stream->flush();
        }
        else {
            std::cerr << "Log file stream is null!" << std::endl;
        }
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
        const auto file_path = args["log-file"];
        if (!file_path.empty()) config::glog_filepath = file_path;

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

    // Open log file stream
    app::glog_file_stream = std::make_unique<std::ofstream>(config::glog_filepath, std::ios::app);

    while (!app::gstop.load()) {
        auto poll_count = pfds.poll(-1);
        if (poll_count == -1) {
            if (errno == EINTR) {
                continue;
            }

            std::cerr << "Poll() error" << std::endl;
            app::stop_with_errcode(1);
            break;
        }

        for (int i = 0; i < pfds.get_count(); ++i) {
            if (pfds.is_ready_at(i, (POLLIN | POLLHUP))) {
                if (pfds.at(i).fd == listening_socket) {
                    handle_new_connection(listening_socket, pfds);
                }
                else {
                    handle_client_data(pfds, i);
                }
            }
        }
    }

    close(listening_socket);
    return app::errcode;
}
