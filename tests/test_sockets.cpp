#include <doctest/doctest.h>
#include <netinet/in.h>
#include "util/sockets.h"

TEST_CASE("get_in_addr: IPv4 returns pointer to sin_addr") {
    sockaddr_in addr{};
    addr.sin_family = AF_INET;

    void* result = util::get_in_addr(reinterpret_cast<sockaddr*>(&addr));
    CHECK(result == static_cast<void*>(&addr.sin_addr));
}

TEST_CASE("get_in_addr: IPv6 returns pointer to sin6_addr") {
    sockaddr_in6 addr{};
    addr.sin6_family = AF_INET6;

    void* result = util::get_in_addr(reinterpret_cast<sockaddr*>(&addr));
    CHECK(result == static_cast<void*>(&addr.sin6_addr));
}
