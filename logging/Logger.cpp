#include "logging/Logger.h"
#include <iostream>
#include <stdexcept>

Logger::Logger(const std::filesystem::path& log_path)
    : file_(log_path, std::ios::app)
{
    if (!file_.is_open()) {
        throw std::runtime_error("Logger: failed to open log file: " + log_path.string());
    }
}

void Logger::info(std::string_view msg) {
    std::lock_guard lock(mutex_);
    std::cout << msg << '\n';
    file_ << msg << '\n';
    file_.flush();
}

void Logger::error(std::string_view msg) {
    std::lock_guard lock(mutex_);
    std::cerr << msg << '\n';
    file_ << msg << '\n';
    file_.flush();
}

void Logger::raw(const char* data, int n) {
    std::lock_guard lock(mutex_);
    file_.write(data, n);
    file_.flush();
}
