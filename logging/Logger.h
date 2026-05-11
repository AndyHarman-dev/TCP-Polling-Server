#pragma once
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string_view>

class Logger {
public:
    explicit Logger(const std::filesystem::path& log_path);

    void info(std::string_view msg);
    void error(std::string_view msg);
    void raw(const char* data, int n);

private:
    std::ofstream file_;
    std::mutex mutex_;
};
