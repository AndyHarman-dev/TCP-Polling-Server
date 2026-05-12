#include <doctest/doctest.h>
#include <filesystem>
#include <stdexcept>
#include "app/App.h"
#include "config/Config.h"

namespace fs = std::filesystem;

TEST_CASE("App: construction succeeds with valid config") {
    Config cfg;
    cfg.port     = "0";
    cfg.log_path = fs::temp_directory_path() / "tcp_echo_app_test.log";
    CHECK_NOTHROW((App(cfg)));
    fs::remove(cfg.log_path);
}

TEST_CASE("App: construction throws with invalid port") {
    Config cfg;
    cfg.port     = "not_a_port";
    cfg.log_path = fs::temp_directory_path() / "tcp_echo_app_test2.log";
    CHECK_THROWS_AS((App(cfg)), std::runtime_error);
    fs::remove(cfg.log_path);
}
