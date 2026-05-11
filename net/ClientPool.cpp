#include "net/ClientPool.h"

ClientPool::ClientPool(int server_fd, int initial_capacity)
    : server_fd_(server_fd) {
    poll_fds_.reserve(initial_capacity + 1);
    clients_.reserve(initial_capacity);
    poll_fds_.push_back(pollfd{server_fd_, POLLIN, 0});
}

void ClientPool::add(Client client) {
    poll_fds_.push_back(pollfd{client.fd(), POLLIN, 0});
    clients_.push_back(std::move(client));
}

int ClientPool::poll(int timeout_ms) {
    return ::poll(poll_fds_.data(), static_cast<nfds_t>(poll_fds_.size()), timeout_ms);
}

bool ClientPool::is_server_ready() const {
    return (poll_fds_[0].revents & (POLLIN | POLLHUP)) != 0;
}

void ClientPool::for_each_ready_client(std::function<bool(Client&)> fn) {
    std::vector<size_t> to_remove;

    for (size_t i = 0; i < clients_.size(); ++i) {
        if (poll_fds_[i + 1].revents & (POLLIN | POLLHUP)) {
            if (!fn(clients_[i])) {
                to_remove.push_back(i);
            }
        }
    }

    // Erase in reverse order so earlier indices stay valid.
    // Client destructor (via move assignment during erase) closes the fd.
    for (auto it = to_remove.rbegin(); it != to_remove.rend(); ++it) {
        auto idx = static_cast<ptrdiff_t>(*it);
        clients_.erase(clients_.begin() + idx);
        poll_fds_.erase(poll_fds_.begin() + idx + 1);
    }
}

size_t ClientPool::client_count() const {
    return clients_.size();
}
