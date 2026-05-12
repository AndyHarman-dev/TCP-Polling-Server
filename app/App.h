#pragma once
#include "ILogger.h"
#include "app/ShutdownManager.h"
#include "config/Config.h"
#include "logging/FileLogger.h"
#include "net/ClientPool.h"
#include "net/TcpServer.h"

class App {
public:
    // Constructs all subsystems. Throws std::runtime_error if the server
    // cannot bind (e.g. invalid port), or std::logic_error if a
    // ShutdownManager already exists.
    explicit App(const Config& cfg);

    // Runs the event loop until a signal or internal error requests shutdown.
    // Returns the exit code.
    int run();

private:
    Config cfg_;
    std::unique_ptr<ILogger> logger_;
    TcpServer server_;
    ShutdownManager shutdown_;  // must come after server_ (no deps, but logically last setup)
    ClientPool pool_;           // needs server_.fd() — declared after server_
};
