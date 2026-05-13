#pragma once
#include <functional>
#include <thread>
#include "app/PurgeBridge.h"
#include "config/Config.h"

// Runs Drogon on a dedicated thread, exposes the /purge admin endpoint.
// Constructed only when cfg.http_port has a value.
//
// on_shutdown is called when SIGINT or SIGTERM reaches the Drogon handler
// (which happens because Drogon overwrites the signal handlers installed by
// ShutdownManager). The callback must be safe to invoke from a signal handler
// context — ShutdownManager::request() only does atomic stores, so it qualifies.
class HttpServer {
public:
    HttpServer(const Config& cfg, PurgeBridge& bridge,
               std::function<void()> on_shutdown);
    ~HttpServer();

    HttpServer(const HttpServer&)            = delete;
    HttpServer& operator=(const HttpServer&) = delete;

private:
    std::thread thread_;
};
