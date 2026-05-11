#include <doctest/doctest.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdexcept>
#include <string>
#include "net/TcpServer.h"

TEST_CASE("TcpServer: port 0 binds and listens successfully") {
    TcpServer server("0");
    CHECK(server.fd() >= 0);
}

TEST_CASE("TcpServer: client can connect") {
    TcpServer server("0");

    // Discover the port the OS assigned
    sockaddr_storage addr{};
    socklen_t len = sizeof(addr);
    REQUIRE(getsockname(server.fd(), reinterpret_cast<sockaddr*>(&addr), &len) == 0);
    int port = ntohs(reinterpret_cast<sockaddr_in*>(&addr)->sin_port);

    addrinfo hints{}, *res;
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    REQUIRE(getaddrinfo("127.0.0.1", std::to_string(port).c_str(), &hints, &res) == 0);

    int client_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    REQUIRE(client_fd >= 0);
    int rc = ::connect(client_fd, res->ai_addr, res->ai_addrlen);
    freeaddrinfo(res);
    close(client_fd);

    CHECK(rc == 0);
}

TEST_CASE("TcpServer: throws on invalid port") {
    CHECK_THROWS_AS(TcpServer("not_a_port"), std::runtime_error);
}
