#pragma once
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string_view>

#include "app/ILogger.h"

class FileLogger : public ILogger {
public:
    explicit FileLogger(const std::filesystem::path& log_path);

    void info(std::string_view msg) override;
    void error(std::string_view msg) override;
    void raw(const char* data, int n) override;

private:
    std::ofstream file_;
    std::mutex mutex_;
};
