#include "app/App.h"
#include <cerrno>
#include <cstring>
#include <format>

App::App(const Config& cfg)
    : cfg_(cfg)
    , logger_(std::make_unique<FileLogger>(cfg_.log_path))
    , server_(cfg_.port)
    , shutdown_()
    , pool_(server_.fd(), cfg_.poll_size)
{}

int App::run() {
    logger_->info(std::format("Logging to {}", cfg_.log_path.string()));
    logger_->info(std::format("Listening on port {}", server_.port()));

    while (!shutdown_.requested()) {
        int poll_count = pool_.poll(-1);
        if (poll_count == -1) {
            if (errno == EINTR) continue;
            logger_->error("poll() failed");
            shutdown_.request(1);
            break;
        }

        if (pool_.is_server_ready()) {
            try {
                pool_.add(server_.accept_client());
            } catch (const std::runtime_error& e) {
                logger_->error(e.what());
            }
        }

        pool_.for_each_ready_client([this](Client& client) -> bool {
            int n = client.receive();
            if (n == 0) {
                logger_->info(std::format("Client fd {} disconnected", client.fd()));
                return false;
            }
            if (n < 0) {
                logger_->error(std::format("recv() failed for fd {}: {}", client.fd(), strerror(errno)));
                return false;
            }
            logger_->info(std::format("server: received from fd {}", client.fd()));
            logger_->raw(client.buffer(), n);
            return true;
        });
    }

    return shutdown_.exit_code();
}
