#pragma once
#include <filesystem>
#include <string>

struct Config {
    std::string port               = "0000";
    std::filesystem::path log_path = "./server_msgs.log";
    int poll_size                  = 5;
};
