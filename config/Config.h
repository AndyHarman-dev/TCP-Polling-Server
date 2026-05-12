#pragma once
#include <filesystem>
#include <optional>
#include <string>

struct Config {
    std::string port               = "0000";
    std::filesystem::path log_path = "./server_msgs.log";
    int poll_size                  = 5;
    std::optional<std::string> db_dsn;
};
