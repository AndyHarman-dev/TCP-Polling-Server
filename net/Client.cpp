#include "net/Client.h"
#include <unistd.h>

Client::Client(int fd, sockaddr_storage addr,
               std::chrono::steady_clock::time_point last_activity)
    : fd_(fd), addr_(addr), last_activity_(last_activity) {}

Client::~Client() {
    if (fd_ >= 0) close(fd_);
}

Client::Client(Client&& other) noexcept
    : fd_(other.fd_), addr_(other.addr_), buffer_(other.buffer_),
      last_activity_(other.last_activity_) {
    other.fd_ = -1;
}

Client& Client::operator=(Client&& other) noexcept {
    if (this != &other) {
        if (fd_ >= 0) close(fd_);
        fd_            = other.fd_;
        addr_          = other.addr_;
        buffer_        = other.buffer_;
        last_activity_ = other.last_activity_;
        other.fd_ = -1;
    }
    return *this;
}

int Client::receive() {
    auto n = static_cast<int>(recv(fd_, buffer_.data(), buffer_.size(), 0));
    if (n > 0) last_activity_ = std::chrono::steady_clock::now();
    return n;
}

std::chrono::steady_clock::time_point Client::last_activity() const {
    return last_activity_;
}

const char* Client::buffer() const {
    return buffer_.data();
}

int Client::fd() const {
    return fd_;
}
