#include "app/App.h"
#include <cerrno>
#include <chrono>
#include <cstring>
#include <format>

#include "logging/DbLoggerProxy.h"
#include "logging/FileLogger.h"

// Member declaration order (see App.h) is load-bearing — members are
// constructed top-to-bottom and destroyed bottom-to-top. The order is:
//
//   cfg_         — plain data; no deps.
//   logger_      — constructed early so subsequent members can log if needed.
//   server_      — owns the listening socket; pool_ needs its fd.
//   shutdown_    — installs signal handlers.
//   bridge_      — owns the wakeup pipe; must be alive before pool_ is
//                  constructed (pool_ takes bridge_.read_fd()) and must
//                  outlive http_server_ (Drogon thread enqueues via it).
//   pool_        — needs server_.fd() and bridge_.read_fd().
//   http_server_ — optional; constructed only when cfg.http_port is set.
//                  Declared last so its destructor (drogon::quit + join)
//                  runs first, before bridge_ and pool_ are torn down.
App::App(const Config& cfg)
    : cfg_(cfg)
    , logger_([&cfg]() -> std::unique_ptr<ILogger> {
        if (cfg.db_dsn)
            return std::make_unique<DbLoggerProxy>(*cfg.db_dsn, "App");
        return std::make_unique<FileLogger>(cfg.log_path);
    }())
    , server_(cfg_.port)
    , shutdown_()
    , bridge_()
    , pool_(server_.fd(), cfg_.poll_size, bridge_.read_fd())
{
    if (cfg_.http_port) {
        http_server_.emplace(cfg_, bridge_, [this] { shutdown_.request(0); });
    }
}

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

        if (pool_.is_wakeup_ready()) {
            bridge_.drain([this](PurgeBridge::Request& req) {
                size_t count = pool_.purge_idle(req.threshold,
                                                std::chrono::steady_clock::now());
                logger_->info(std::format("purge: removed {} idle client(s)", count));
                req.on_complete(count);
            });
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
