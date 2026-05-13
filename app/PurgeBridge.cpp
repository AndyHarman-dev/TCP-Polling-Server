#include "app/PurgeBridge.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdexcept>

PurgeBridge::PurgeBridge() {
    if (::pipe(pipe_) != 0)
        throw std::runtime_error("PurgeBridge: pipe() failed");
    // Non-blocking read end so drain() can loop without blocking.
    ::fcntl(pipe_[0], F_SETFL, ::fcntl(pipe_[0], F_GETFL) | O_NONBLOCK);
}

PurgeBridge::~PurgeBridge() {
    if (pipe_[0] >= 0) ::close(pipe_[0]);
    if (pipe_[1] >= 0) ::close(pipe_[1]);
}

void PurgeBridge::enqueue(Request req) {
    {
        std::lock_guard lock(mutex_);
        queue_.push(std::move(req));
    }
    // Write a single byte to wake the main poll() loop.
    // Ignore EINTR / EAGAIN — the byte is best-effort; drain loops anyway.
    char byte = 1;
    (void)::write(pipe_[1], &byte, 1);
}

void PurgeBridge::drain(std::function<void(Request&)> handler) {
    // Drain all wakeup bytes first so the fd goes back to non-readable.
    char buf[64];
    while (::read(pipe_[0], buf, sizeof(buf)) > 0) {}

    std::queue<Request> local;
    {
        std::lock_guard lock(mutex_);
        std::swap(local, queue_);
    }

    while (!local.empty()) {
        handler(local.front());
        local.pop();
    }
}

int PurgeBridge::read_fd() const {
    return pipe_[0];
}
