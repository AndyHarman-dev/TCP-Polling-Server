//
// Created by King Andrés on 12/05/2026.
//

#ifndef TCPECHOINGSERVER_DBLOGGERPROXY_H
#define TCPECHOINGSERVER_DBLOGGERPROXY_H
#include "app/ILogger.h"
#include "db/DbLogger.h"


class DbLoggerProxy : public ILogger {
    DbLogger inner_;
    std::string source_;
    std::optional<std::string> client_;
public:
    explicit DbLoggerProxy(
        std::string_view connection_string,
        std::string_view source,
        std::optional<std::string> client = {}
    );

    void info(std::string_view msg) override;
    void error(std::string_view msg) override;
    void raw(const char *data, int n) override;
};


#endif //TCPECHOINGSERVER_DBLOGGERPROXY_H
