#include <doctest/doctest.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string>
#include <chrono>
#include <thread>
#include "net/Client.h"

TEST_CASE("Client: receive reads data from peer") {
    int sv[2];
    REQUIRE(socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == 0);

    Client client(sv[0], sockaddr_storage{});
    write(sv[1], "hello", 5);

    int n = client.receive();
    REQUIRE(n == 5);
    CHECK(std::string(client.buffer(), n) == "hello");

    close(sv[1]);
    // sv[0] closed by Client destructor
}

TEST_CASE("Client: receive returns 0 on peer disconnect") {
    int sv[2];
    REQUIRE(socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == 0);

    Client client(sv[0], sockaddr_storage{});
    close(sv[1]);

    CHECK(client.receive() == 0);
}

TEST_CASE("Client: last_activity is set on construction") {
    using clock = std::chrono::steady_clock;
    auto before = clock::now();
    Client client(-1, sockaddr_storage{});
    auto after = clock::now();

    CHECK(client.last_activity() >= before);
    CHECK(client.last_activity() <= after);
}

TEST_CASE("Client: receive updates last_activity on data") {
    int sv[2];
    REQUIRE(socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == 0);

    Client client(sv[0], sockaddr_storage{});
    auto t0 = client.last_activity();

    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    write(sv[1], "x", 1);
    client.receive();

    CHECK(client.last_activity() > t0);

    close(sv[1]);
}

TEST_CASE("Client: receive does not update last_activity on disconnect") {
    int sv[2];
    REQUIRE(socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == 0);

    Client client(sv[0], sockaddr_storage{});
    auto t0 = client.last_activity();

    close(sv[1]);
    client.receive();  // returns 0

    CHECK(client.last_activity() == t0);
}

TEST_CASE("Client: move preserves last_activity") {
    int sv[2];
    REQUIRE(socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == 0);

    Client original(sv[0], sockaddr_storage{});
    auto t0 = original.last_activity();

    Client moved(std::move(original));

    CHECK(moved.last_activity() == t0);

    close(sv[1]);
}
