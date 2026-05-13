#pragma once
#include <chrono>
#include <functional>
#include <mutex>
#include <queue>

// Thread-safe bridge between the Drogon HTTP thread and the main poll() loop.
// Drogon enqueues a request and returns immediately; the main loop drains and
// calls on_complete(count) from its own thread — Drogon's cb is thread-safe.
class PurgeBridge {
public:
    struct Request {
        std::chrono::seconds threshold;
        std::function<void(size_t)> on_complete;
    };

    PurgeBridge();
    ~PurgeBridge();

    PurgeBridge(const PurgeBridge&)            = delete;
    PurgeBridge& operator=(const PurgeBridge&) = delete;

    // Called from any thread: enqueues a request and wakes the main loop.
    void enqueue(Request req);

    // Called from the main thread when read_fd is readable: drains all pending
    // requests, calling handler(req) for each. Handler must call req.on_complete.
    void drain(std::function<void(Request&)> handler);

    // The read end of the wakeup pipe; add this fd to your poll set.
    [[nodiscard]] int read_fd() const;

private:
    int pipe_[2]{-1, -1};
    std::mutex mutex_;
    std::queue<Request> queue_;
};
