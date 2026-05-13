#include "http/HttpServer.h"
#include "util/duration.h"
#include <drogon/drogon.h>
#include <stdexcept>

static drogon::HttpResponsePtr json_error(drogon::HttpStatusCode code, const std::string& msg) {
    Json::Value body;
    body["error"] = msg;
    auto resp = drogon::HttpResponse::newHttpJsonResponse(body);
    resp->setStatusCode(code);
    return resp;
}

HttpServer::HttpServer(const Config& cfg, PurgeBridge& bridge,
                       std::function<void()> on_shutdown) {
    if (!cfg.http_port) return;

    uint16_t port = *cfg.http_port;
    std::chrono::seconds default_threshold = cfg.idle_timeout;

    // Drogon installs its own SIGINT/SIGTERM handlers inside run(), overwriting
    // ShutdownManager's. We redirect them so both the Drogon event loop and the
    // main poll() loop shut down cleanly.
    drogon::app()
        .setIntSignalHandler([on_shutdown] {
            drogon::app().quit();
            on_shutdown();
        })
        .setTermSignalHandler([on_shutdown] {
            drogon::app().quit();
            on_shutdown();
        });

    drogon::app()
        .addListener("127.0.0.1", port)
        .setThreadNum(1)
        .registerHandler(
            "/purge",
            [&bridge, default_threshold](
                const drogon::HttpRequestPtr& req,
                std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
                std::chrono::seconds threshold = default_threshold;

                auto threshold_param = req->getParameter("threshold");
                if (!threshold_param.empty()) {
                    try {
                        threshold = util::parse_duration(threshold_param);
                    } catch (const std::invalid_argument& e) {
                        cb(json_error(drogon::k400BadRequest,
                                      std::string("Bad threshold: ") + e.what()));
                        return;
                    }
                }

                // Enqueue and return — main loop calls cb from its thread.
                // Drogon's cb is thread-safe: it posts the response to the
                // connection's I/O loop regardless of which thread calls it.
                bridge.enqueue({threshold, [cb](size_t count) {
                    Json::Value body;
                    body["purged"] = static_cast<Json::UInt64>(count);
                    cb(drogon::HttpResponse::newHttpJsonResponse(body));
                }});
            },
            {drogon::Get});

    thread_ = std::thread([] { drogon::app().run(); });
}

HttpServer::~HttpServer() {
    // Only tear drogon down if we actually started it.
    if (thread_.joinable()) {
        drogon::app().quit();
        thread_.join();
    }
}
