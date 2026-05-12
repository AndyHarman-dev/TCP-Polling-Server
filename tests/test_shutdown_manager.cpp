#include <doctest/doctest.h>
#include <csignal>
#include <vector>
#include "app/ShutdownManager.h"

TEST_CASE("ShutdownManager: not requested initially") {
    ShutdownManager mgr;
    CHECK(!mgr.requested());
    CHECK(mgr.exit_code() == 0);
}

TEST_CASE("ShutdownManager: request sets requested and exit code") {
    ShutdownManager mgr;
    mgr.request(42);
    CHECK(mgr.requested());
    CHECK(mgr.exit_code() == 42);
}

TEST_CASE("ShutdownManager: hook is called on destruction") {
    bool called = false;
    {
        ShutdownManager mgr;
        mgr.register_hook([&called] { called = true; });
    }
    CHECK(called);
}

TEST_CASE("ShutdownManager: hooks fire in reverse registration order") {
    std::vector<int> order;
    {
        ShutdownManager mgr;
        mgr.register_hook([&order] { order.push_back(1); });
        mgr.register_hook([&order] { order.push_back(2); });
        mgr.register_hook([&order] { order.push_back(3); });
    }
    REQUIRE(order.size() == 3);
    CHECK(order[0] == 3);
    CHECK(order[1] == 2);
    CHECK(order[2] == 1);
}

TEST_CASE("ShutdownManager: SIGINT triggers shutdown") {
    ShutdownManager mgr;
    raise(SIGINT);
    CHECK(mgr.requested());
    // destructor restores the previous SIGINT handler
}

TEST_CASE("ShutdownManager: throws if a second instance is created") {
    ShutdownManager mgr;
    CHECK_THROWS_AS(ShutdownManager{}, std::logic_error);
}
