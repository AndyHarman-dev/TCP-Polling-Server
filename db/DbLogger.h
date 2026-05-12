//
// Created by King Andrés on 12/05/2026.
//

#ifndef TCPECHOINGSERVER_DBLOGGER_H
#define TCPECHOINGSERVER_DBLOGGER_H
#include <memory>
#include <mutex>
#include <string_view>
#include <pqxx/pqxx>

class DbLogger {
    std::unique_ptr<pqxx::connection> conn_;
    std::mutex mtx_;

    void ensure_schema();
public:

    enum Level {
        Info,
        Warning,
        Error,
        Trace,
        Debug,
        Fatal
    };

    explicit DbLogger(std::string_view conn_str);

    void log(
        Level l,
        std::string_view source,
        std::string_view message,
        std::optional<std::string> client = std::nullopt,
        std::optional<std::string> json_obj = std::nullopt
    );

    static std::string level_str(Level l);
};

#endif //TCPECHOINGSERVER_DBLOGGER_H
