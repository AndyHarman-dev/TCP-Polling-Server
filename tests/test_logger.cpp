#include <doctest/doctest.h>
#include <filesystem>
#include <fstream>
#include <string>
#include "logging/Logger.h"

namespace fs = std::filesystem;

static std::string read_file(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), {}};
}

TEST_CASE("Logger::raw writes bytes to file") {
    auto path = fs::temp_directory_path() / "tcp_echo_test_raw.log";
    fs::remove(path);

    {
        Logger logger(path);
        logger.raw("hello", 5);
    }

    REQUIRE(fs::exists(path));
    CHECK(read_file(path) == "hello");

    fs::remove(path);
}

TEST_CASE("Logger throws when file cannot be opened") {
    CHECK_THROWS_AS(Logger("/no/such/directory/test.log"), std::runtime_error);
}

TEST_CASE("Logger::raw appends across instances") {
    auto path = fs::temp_directory_path() / "tcp_echo_test_append.log";
    fs::remove(path);

    {
        Logger logger(path);
        logger.raw("foo", 3);
    }
    {
        Logger logger(path);
        logger.raw("bar", 3);
    }

    CHECK(read_file(path) == "foobar");

    fs::remove(path);
}
