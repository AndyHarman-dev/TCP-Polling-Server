#include <doctest/doctest.h>
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
