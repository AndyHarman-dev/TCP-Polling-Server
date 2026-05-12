//
// Created by King Andrés on 12/05/2026.
//

#ifndef TCPECHOINGSERVER_ILOGGER_H
#define TCPECHOINGSERVER_ILOGGER_H
#include <string_view>

class ILogger {

public:
    virtual ~ILogger() = default;

    virtual void info(std::string_view msg) = 0;
    virtual void error(std::string_view msg) = 0;
    virtual void raw(const char* data, int n) = 0;
};

#endif //TCPECHOINGSERVER_ILOGGER_H
