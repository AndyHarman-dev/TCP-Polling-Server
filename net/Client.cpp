#include "net/Client.h"
#include <unistd.h>

Client::Client(int fd, sockaddr_storage addr)
    : fd_(fd), addr_(addr) {}

Client::~Client() {
    if (fd_ >= 0) close(fd_);
}

Client::Client(Client&& other) noexcept
    : fd_(other.fd_), addr_(other.addr_), buffer_(other.buffer_) {
    other.fd_ = -1;
}

Client& Client::operator=(Client&& other) noexcept {
    if (this != &other) {
        if (fd_ >= 0) close(fd_);
        fd_     = other.fd_;
        addr_   = other.addr_;
        buffer_ = other.buffer_;
        other.fd_ = -1;
    }
    return *this;
}

int Client::receive() {
    return static_cast<int>(recv(fd_, buffer_.data(), buffer_.size(), 0));
}

const char* Client::buffer() const {
    return buffer_.data();
}

int Client::fd() const {
    return fd_;
}
