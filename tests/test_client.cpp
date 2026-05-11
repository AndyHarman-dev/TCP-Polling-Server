#include <doctest/doctest.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string>
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
