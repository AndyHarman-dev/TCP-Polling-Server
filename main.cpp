#include <atomic>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <format>
#include <iostream>
#include <memory>
#include <string>
#include "config/ArgParser.h"
#include "config/Config.h"
#include "logging/Logger.h"
#include "net/Client.h"
#include "net/ClientPool.h"
#include "net/TcpServer.h"

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

    std::unique_ptr<TcpServer> server;
    try {
        server = std::make_unique<TcpServer>(cfg.port);
    } catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    std::printf("Listening on port %s\n", cfg.port.c_str());
    app::glogger = std::make_unique<Logger>(cfg.log_path);
    ClientPool pool(server->fd(), cfg.poll_size);

    while (!app::gstop.load()) {
        int poll_count = pool.poll(-1);
        if (poll_count == -1) {
            if (errno == EINTR) continue;
            std::cerr << "poll() failed" << std::endl;
            app::stop_with_errcode(1);
            break;
        }

        if (pool.is_server_ready()) {
            try {
                pool.add(server->accept_client());
            } catch (const std::runtime_error& e) {
                std::cerr << e.what() << std::endl;
            }
        }

        pool.for_each_ready_client([](Client& client) -> bool {
            int n = client.receive();
            if (n == 0) {
                std::printf("Client fd %d disconnected\n", client.fd());
                return false;
            }
            if (n < 0) {
                std::cerr << std::format("recv() failed for fd {}: {}\n", client.fd(), strerror(errno));
                return false;
            }
            std::printf("server: received from fd %d: %.*s\n", client.fd(), n, client.buffer());
            if (app::glogger) app::glogger->raw(client.buffer(), n);
            return true;
        });
    }

    return app::errcode;
}
