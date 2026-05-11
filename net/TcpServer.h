#pragma once
#include <string>
#include "net/Client.h"

class TcpServer {
public:
    explicit TcpServer(const std::string& port);
    ~TcpServer();

    TcpServer(const TcpServer&)            = delete;
    TcpServer& operator=(const TcpServer&) = delete;

    [[nodiscard]] int fd() const;
    [[nodiscard]] const std::string& port() const { return port_; }

    // Accepts the next pending connection. Throws std::runtime_error on failure.
    [[nodiscard]] Client accept_client() const;

private:
    int fd_ = -1;
    std::string port_;
};
