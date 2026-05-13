#include <doctest/doctest.h>
#include <chrono>
#include <sys/select.h>
#include "app/PurgeBridge.h"

using seconds = std::chrono::seconds;

TEST_CASE("PurgeBridge: read_fd is valid after construction") {
    PurgeBridge bridge;
    CHECK(bridge.read_fd() >= 0);
}

TEST_CASE("PurgeBridge: enqueue wakes the read fd") {
    PurgeBridge bridge;
    bridge.enqueue({seconds(60), [](size_t) {}});

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(bridge.read_fd(), &fds);
    timeval tv{0, 100'000};  // 100ms
    int ready = select(bridge.read_fd() + 1, &fds, nullptr, nullptr, &tv);

    CHECK(ready == 1);
    CHECK(FD_ISSET(bridge.read_fd(), &fds));
}

TEST_CASE("PurgeBridge: drain delivers threshold and result to on_complete") {
    PurgeBridge bridge;

    seconds captured_threshold{0};
    size_t received = 0;

    bridge.enqueue({seconds(30), [&](size_t count) {
        received = count;
    }});

    bridge.drain([&](PurgeBridge::Request& req) {
        captured_threshold = req.threshold;
        req.on_complete(42);
    });

    CHECK(captured_threshold == seconds(30));
    CHECK(received == 42);
}

TEST_CASE("PurgeBridge: drain handles multiple enqueued requests in order") {
    PurgeBridge bridge;

    int calls = 0;
    size_t r1 = 0, r2 = 0;

    bridge.enqueue({seconds(10), [&](size_t n) { r1 = n; }});
    bridge.enqueue({seconds(20), [&](size_t n) { r2 = n; }});

    bridge.drain([&](PurgeBridge::Request& req) {
        req.on_complete(static_cast<size_t>(++calls));
    });

    CHECK(calls == 2);
    CHECK(r1 == 1);
    CHECK(r2 == 2);
}
