#include "net/TcpServer.h"
#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <format>
#include <netdb.h>
#include <stdexcept>
#include <unistd.h>

TcpServer::TcpServer(const std::string& port) : port_(port) {
    addrinfo hints{}, *server_info;
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags    = AI_PASSIVE;

    if (int rv = getaddrinfo(nullptr, port.c_str(), &hints, &server_info); rv != 0) {
        throw std::runtime_error(std::format("getaddrinfo() failed: {}", gai_strerror(rv)));
    }

    addrinfo* p = nullptr;
    int fd = -1;
    for (p = server_info; p != nullptr; p = p->ai_next) {
        fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (fd == -1) continue;

        int yes = 1;
        if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
            close(fd);
            continue;
        }

        if (bind(fd, p->ai_addr, p->ai_addrlen) == -1) {
            close(fd);
            continue;
        }

        break;
    }

    freeaddrinfo(server_info);

    if (p == nullptr) {
        throw std::runtime_error(std::format("socket() or bind() failed: {}", strerror(errno)));
    }

    if (listen(fd, SOMAXCONN) == -1) {
        close(fd);
        throw std::runtime_error(std::format("listen() failed: {}", strerror(errno)));
    }

    fd_ = fd;
}

TcpServer::~TcpServer() {
    if (fd_ >= 0) close(fd_);
}

int TcpServer::fd() const {
    return fd_;
}
