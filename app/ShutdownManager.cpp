#include "app/ShutdownManager.h"
#include <stdexcept>

std::atomic<ShutdownManager*> ShutdownManager::instance_{nullptr};

ShutdownManager::ShutdownManager() {
    ShutdownManager* expected = nullptr;
    if (!instance_.compare_exchange_strong(expected, this, std::memory_order_relaxed)) {
        throw std::logic_error("Only one ShutdownManager may exist at a time");
    }

    struct sigaction sa{};
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGINT,  &sa, &prev_sigint_);
    sigaction(SIGTERM, &sa, &prev_sigterm_);
}

ShutdownManager::~ShutdownManager() {
    sigaction(SIGINT,  &prev_sigint_,  nullptr);
    sigaction(SIGTERM, &prev_sigterm_, nullptr);

    for (auto it = hooks_.rbegin(); it != hooks_.rend(); ++it) {
        (*it)();
    }

    instance_.store(nullptr, std::memory_order_relaxed);
}

bool ShutdownManager::requested() const {
    return stop_.load(std::memory_order_relaxed);
}

void ShutdownManager::request(int code) {
    stop_.store(true, std::memory_order_relaxed);
    exit_code_.store(code, std::memory_order_relaxed);
}

void ShutdownManager::register_hook(std::function<void()> hook) {
    hooks_.push_back(std::move(hook));
}

int ShutdownManager::exit_code() const {
    return exit_code_.load(std::memory_order_relaxed);
}

void ShutdownManager::handle_signal(int) {
    if (auto* mgr = instance_.load(std::memory_order_relaxed)) {
        mgr->stop_.store(true, std::memory_order_relaxed);
    }
}
