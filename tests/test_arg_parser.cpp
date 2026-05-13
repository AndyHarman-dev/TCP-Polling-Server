#include <doctest/doctest.h>
#include <chrono>
#include <stdexcept>
#include "config/Config.h"
#include "config/ArgParser.h"

TEST_CASE("Config has sane defaults") {
    Config cfg;
    CHECK(cfg.port == "0000");
    CHECK(cfg.log_path == "./server_msgs.log");
    CHECK(cfg.poll_size == 5);
}

TEST_CASE("ArgParser: applies port from first positional arg") {
    const char* argv[] = {"server", "8080"};
    ArgParser parser(2, const_cast<char**>(argv));
    Config cfg;
    parser.apply(cfg);
    CHECK(cfg.port == "8080");
}

TEST_CASE("ArgParser: --log-file= sets log path") {
    const char* argv[] = {"server", "8080", "--log-file=/tmp/test.log"};
    ArgParser parser(3, const_cast<char**>(argv));
    Config cfg;
    parser.apply(cfg);
    CHECK(cfg.log_path == "/tmp/test.log");
}

TEST_CASE("ArgParser: --log-file <space> sets log path") {
    const char* argv[] = {"server", "8080", "--log-file", "/tmp/test2.log"};
    ArgParser parser(4, const_cast<char**>(argv));
    Config cfg;
    parser.apply(cfg);
    CHECK(cfg.log_path == "/tmp/test2.log");
}

TEST_CASE("ArgParser: default log path preserved when not specified") {
    const char* argv[] = {"server", "9090"};
    ArgParser parser(2, const_cast<char**>(argv));
    Config cfg;
    parser.apply(cfg);
    CHECK(cfg.log_path == "./server_msgs.log");
    CHECK(cfg.poll_size == 5);
}

TEST_CASE("ArgParser: throws when port is missing") {
    const char* argv[] = {"server"};
    ArgParser parser(1, const_cast<char**>(argv));
    Config cfg;
    CHECK_THROWS_AS(parser.apply(cfg), std::invalid_argument);
}

TEST_CASE("Config: idle_timeout defaults to 10 minutes") {
    Config cfg;
    CHECK(cfg.idle_timeout == std::chrono::seconds(600));
}

TEST_CASE("Config: http_port is absent by default") {
    Config cfg;
    CHECK_FALSE(cfg.http_port.has_value());
}

TEST_CASE("ArgParser: --idle-timeout=10m sets 600s") {
    const char* argv[] = {"server", "8080", "--idle-timeout=10m"};
    ArgParser parser(3, const_cast<char**>(argv));
    Config cfg;
    parser.apply(cfg);
    CHECK(cfg.idle_timeout == std::chrono::seconds(600));
}

TEST_CASE("ArgParser: --idle-timeout=30s sets 30s") {
    const char* argv[] = {"server", "8080", "--idle-timeout=30s"};
    ArgParser parser(3, const_cast<char**>(argv));
    Config cfg;
    parser.apply(cfg);
    CHECK(cfg.idle_timeout == std::chrono::seconds(30));
}

TEST_CASE("ArgParser: --idle-timeout=1h sets 3600s") {
    const char* argv[] = {"server", "8080", "--idle-timeout=1h"};
    ArgParser parser(3, const_cast<char**>(argv));
    Config cfg;
    parser.apply(cfg);
    CHECK(cfg.idle_timeout == std::chrono::seconds(3600));
}

TEST_CASE("ArgParser: --idle-timeout with invalid suffix throws") {
    const char* argv[] = {"server", "8080", "--idle-timeout=5x"};
    ArgParser parser(3, const_cast<char**>(argv));
    Config cfg;
    CHECK_THROWS_AS(parser.apply(cfg), std::invalid_argument);
}

TEST_CASE("ArgParser: --http-port sets port") {
    const char* argv[] = {"server", "8080", "--http-port=8081"};
    ArgParser parser(3, const_cast<char**>(argv));
    Config cfg;
    parser.apply(cfg);
    REQUIRE(cfg.http_port.has_value());
    CHECK(cfg.http_port.value() == 8081);
}

TEST_CASE("ArgParser: --http-port with invalid value throws") {
    const char* argv[] = {"server", "8080", "--http-port=notanumber"};
    ArgParser parser(3, const_cast<char**>(argv));
    Config cfg;
    CHECK_THROWS_AS(parser.apply(cfg), std::invalid_argument);
}
