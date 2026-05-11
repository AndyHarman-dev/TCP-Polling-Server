#include <filesystem>
#include <iostream>
#include <string>
#include <netdb.h>
#include <expected>
#include <csignal>
#include <unistd.h>
#include <arpa/inet.h>
#include <poll.h>
#include <vector>
#include "util/sockets.h"
#include "logging/Logger.h"
#include "config/Config.h"
#include "config/ArgParser.h"

//TODO: app related fields could also be encapsulated in App object
namespace app {
    std::atomic<bool> gstop{false};
    std::atomic<int> errcode = 0;

    // TODO: Given app's looped nature we should have a united shutdown scalable mechanism so that we can gracefully handle any shutdown specifics of this program
    void stop_with_errcode(int code) {
        gstop.store(true);
        errcode = code;
    }

    // TODO: Move into App object in Phase 6
    std::unique_ptr<Logger> glogger = nullptr;
}

// TODO: Replace with TcpServer class in Phase 4
std::expected<int, std::string> get_listen_socket(const std::string& port) {

    addrinfo hints, *server_info;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // auto ip

    if (int rv; (rv = getaddrinfo(nullptr, port.c_str(), &hints, &server_info)) != 0) {
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
        util::get_in_addr(p->ai_addr),
        _, sizeof _
    );

    freeaddrinfo(server_info);
    if (listen(file_descriptor, SOMAXCONN) == -1) {
        return std::unexpected(std::format("listen() failed: {0}", strerror(errno)));
    }

    std::printf("Listening at %s\n", listening_ip);

    return file_descriptor;
}

// TODO: Handling functions go to app or shutdown mechanism
void handle_sigterm(int) {
    std::cout << "Handled SIGTERM!";
    app::gstop.store(true);
}

void handle_sigint(int) {
    std::cout << "Handled SIGINT!";
    app::gstop.store(true);
}

// TODO: To app
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
// TODO: Only exists because I used raw memory first, std::vector is enough though. Probably in the new architecture wouldn't exist at all!
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

// TODO: that should exist in the app probably, too but I'm unsure whether or not to give their proper objects for a client to store the socket id
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

//TODO: Here, too we could have the encapsulate the data receiving from the client object instead of having it in the app
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

        if (app::glogger) {
            app::glogger->raw(buffer, bytes_received);
        }
        else {
            std::cerr << "Logger is null!" << std::endl;
        }
    }
}

int main(int argc, char* argv[]) {

    Config cfg;
    try {
        ArgParser(argc, argv).apply(cfg);
    } catch (const std::invalid_argument& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    std::printf("Logging to %s\n", cfg.log_path.string().c_str());

    if (!setup_sigint_sigterm_bindigns()) {
        return 1;
    }

    auto expected_socket = get_listen_socket(cfg.port);
    if (!expected_socket.has_value()) {
        std::cerr << expected_socket.error() << std::endl;
        return 1;
    }

    auto listening_socket = expected_socket.value();

    polling_file_descriptors pfds(cfg.poll_size);
    pfds.add(listening_socket, POLLIN);

    app::glogger = std::make_unique<Logger>(cfg.log_path);

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
