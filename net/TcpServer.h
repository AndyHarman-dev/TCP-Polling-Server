#pragma once
#include <string>

class TcpServer {
public:
    explicit TcpServer(const std::string& port);
    ~TcpServer();

    TcpServer(const TcpServer&)            = delete;
    TcpServer& operator=(const TcpServer&) = delete;

    [[nodiscard]] int fd() const;
    [[nodiscard]] const std::string& port() const {
        return port_;
    }

private:
    int fd_ = -1;
    std::string port_;
};
