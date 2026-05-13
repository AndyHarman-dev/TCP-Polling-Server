#pragma once
#include <array>
#include <chrono>
#include <sys/socket.h>

class Client {
public:
    static constexpr int BUFFER_SIZE = 256;

    Client(int fd, sockaddr_storage addr,
           std::chrono::steady_clock::time_point last_activity = std::chrono::steady_clock::now());
    ~Client();

    Client(const Client&)            = delete;
    Client& operator=(const Client&) = delete;
    Client(Client&&) noexcept;
    Client& operator=(Client&&) noexcept;

    // Returns bytes received (> 0), 0 on disconnect, -1 on error.
    int receive();

    [[nodiscard]] const char* buffer() const;
    [[nodiscard]] int fd() const;
    [[nodiscard]] std::chrono::steady_clock::time_point last_activity() const;

private:
    int fd_ = -1;
    sockaddr_storage addr_{};
    std::array<char, BUFFER_SIZE> buffer_{};
    std::chrono::steady_clock::time_point last_activity_;
};
