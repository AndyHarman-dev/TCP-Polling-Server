#pragma once
#include <functional>
#include <poll.h>
#include <vector>
#include "net/Client.h"

class ClientPool {
public:
    ClientPool(int server_fd, int initial_capacity);

    void add(Client client);

    // Returns ::poll() result directly: >= 0 on success, -1 on EINTR or error.
    int poll(int timeout_ms);

    [[nodiscard]] bool is_server_ready() const;

    // Calls fn(Client&) for each ready client.
    // fn must return true to keep the client, false to close and remove it.
    void for_each_ready_client(std::function<bool(Client&)> fn);

    [[nodiscard]] size_t client_count() const;

private:
    int server_fd_;
    std::vector<pollfd> poll_fds_;  // [0] = server_fd_, [1+] = client fds
    std::vector<Client> clients_;   // parallel to poll_fds_[1+]
};
