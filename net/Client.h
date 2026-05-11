#pragma once
#include <array>
#include <sys/socket.h>

class Client {
public:
    static constexpr int BUFFER_SIZE = 256;

    Client(int fd, sockaddr_storage addr);
    ~Client();

    Client(const Client&)            = delete;
    Client& operator=(const Client&) = delete;
    Client(Client&&) noexcept;
    Client& operator=(Client&&) noexcept;

    // Returns bytes received (> 0), 0 on disconnect, -1 on error.
    int receive();

    [[nodiscard]] const char* buffer() const;
    [[nodiscard]] int fd() const;

private:
    int fd_ = -1;
    sockaddr_storage addr_{};
    std::array<char, BUFFER_SIZE> buffer_{};
};
