#pragma once
#include <atomic>
#include <csignal>
#include <functional>
#include <vector>

class ShutdownManager {
public:
    // Throws std::logic_error if another instance already exists.
    ShutdownManager();
    ~ShutdownManager();

    ShutdownManager(const ShutdownManager&)            = delete;
    ShutdownManager& operator=(const ShutdownManager&) = delete;

    [[nodiscard]] bool requested() const;

    // Sets the stop flag and records the exit code.
    // Safe to call from the event loop; NOT from a signal handler.
    void request(int exit_code = 0);

    // Hooks fire in reverse registration order when the manager is destroyed.
    void register_hook(std::function<void()> hook);

    [[nodiscard]] int exit_code() const;

private:
    std::atomic<bool> stop_{false};
    std::atomic<int>  exit_code_{0};
    std::vector<std::function<void()>> hooks_;
    struct sigaction prev_sigint_{};
    struct sigaction prev_sigterm_{};

    // Only sets stop_ — async-signal-safe.
    static void handle_signal(int sig);
    static std::atomic<ShutdownManager*> instance_;
};
