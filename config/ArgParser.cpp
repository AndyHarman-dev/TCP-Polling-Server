#include "config/ArgParser.h"
#include "util/duration.h"
#include <charconv>
#include <stdexcept>

ArgParser::ArgParser(int argc, char* argv[]) {
    int positional = 0;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg.rfind("--", 0) == 0) {
            std::string body = arg.substr(2);
            auto eq = body.find('=');
            if (eq != std::string::npos) {
                args_[body.substr(0, eq)] = body.substr(eq + 1);
            } else if (i + 1 < argc && std::string(argv[i + 1]).rfind("--", 0) != 0) {
                args_[body] = argv[++i];
            } else {
                args_[body] = "";
            }
        } else {
            args_[std::to_string(positional++)] = arg;
        }
    }
}

void ArgParser::apply(Config& cfg) const {
    auto port_it = args_.find("0");
    if (port_it == args_.end() || port_it->second.empty()) {
        throw std::invalid_argument("Usage: server <port> [--log-file=<path>] [--db-dsn=<connstr>]");
    }
    cfg.port = port_it->second;

    auto log_it = args_.find("log-file");
    if (log_it != args_.end() && !log_it->second.empty()) {
        cfg.log_path = log_it->second;
    }

    auto db_it = args_.find("db-dsn");
    if (db_it != args_.end() && !db_it->second.empty()) {
        cfg.db_dsn = db_it->second;
    }

    auto idle_it = args_.find("idle-timeout");
    if (idle_it != args_.end() && !idle_it->second.empty()) {
        cfg.idle_timeout = util::parse_duration(idle_it->second);
    }

    auto http_it = args_.find("http-port");
    if (http_it != args_.end() && !http_it->second.empty()) {
        int port = 0;
        auto [ptr, ec] = std::from_chars(http_it->second.data(),
                                         http_it->second.data() + http_it->second.size(),
                                         port);
        if (ec != std::errc{} || ptr != http_it->second.data() + http_it->second.size()
            || port < 1 || port > 65535) {
            throw std::invalid_argument("invalid http-port: " + http_it->second);
        }
        cfg.http_port = static_cast<uint16_t>(port);
    }
}
