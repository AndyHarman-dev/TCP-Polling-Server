#pragma once
#include <chrono>
#include <cstddef>
#include <functional>
#include <poll.h>
#include <vector>
#include "net/Client.h"

class ClientPool {
public:
    // wakeup_fd, when >= 0, is included in the poll set so external pipes/eventfds
    // can wake the main loop. The fd is observed only; this class never closes it.
    ClientPool(int server_fd, int initial_capacity, int wakeup_fd = -1);

    void add(Client client);

    // Returns ::poll() result directly: >= 0 on success, -1 on EINTR or error.
    int poll(int timeout_ms);

    [[nodiscard]] bool is_server_ready() const;

    // Calls fn(Client&) for each ready client.
    // fn must return true to keep the client, false to close and remove it.
    void for_each_ready_client(std::function<bool(Client&)> fn);

    [[nodiscard]] size_t client_count() const;

    // Removes and closes clients whose last_activity is older than threshold
    // relative to now. Returns the number of clients removed.
    size_t purge_idle(std::chrono::seconds threshold,
                      std::chrono::steady_clock::time_point now);

    [[nodiscard]] bool is_wakeup_ready() const;

private:
    // poll_fds_ layout:
    //   [0]                = server_fd_
    //   [1]                = wakeup_fd_ (only when wakeup_fd_ >= 0)
    //   [client_off_ ..]   = per-client fds, parallel with clients_
    int server_fd_;
    int wakeup_fd_;
    size_t client_off_;
    std::vector<pollfd> poll_fds_;
    std::vector<Client> clients_;
};
