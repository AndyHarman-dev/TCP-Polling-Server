#include <doctest/doctest.h>
#include <sys/socket.h>
#include <unistd.h>
#include <chrono>
#include "net/Client.h"
#include "net/ClientPool.h"

using steady = std::chrono::steady_clock;
using seconds = std::chrono::seconds;

static std::pair<int, int> make_socketpair() {
    int sv[2];
    REQUIRE(socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == 0);
    return {sv[0], sv[1]};
}

TEST_CASE("ClientPool::purge_idle: removes nothing when all clients are fresh") {
    auto [srv, srv_peer] = make_socketpair();
    auto [cli, cli_peer] = make_socketpair();

    ClientPool pool(srv, 4);
    pool.add(Client(cli, {}));

    auto now = steady::now();
    size_t removed = pool.purge_idle(seconds(60), now);

    CHECK(removed == 0);
    CHECK(pool.client_count() == 1);

    close(srv); close(srv_peer); close(cli_peer);
}

TEST_CASE("ClientPool::purge_idle: removes client whose last_activity exceeds threshold") {
    auto [srv, srv_peer] = make_socketpair();
    auto [cli, cli_peer] = make_socketpair();

    ClientPool pool(srv, 4);
    pool.add(Client(cli, {}));

    // Advance 'now' past the threshold so the client looks idle
    auto now = steady::now() + seconds(120);
    size_t removed = pool.purge_idle(seconds(60), now);

    CHECK(removed == 1);
    CHECK(pool.client_count() == 0);

    close(srv); close(srv_peer); close(cli_peer);
}

TEST_CASE("ClientPool::purge_idle: only removes idle clients, keeps fresh ones") {
    auto [srv, srv_peer]     = make_socketpair();
    auto [idle, idle_peer]   = make_socketpair();
    auto [fresh, fresh_peer] = make_socketpair();

    auto now = steady::now();
    ClientPool pool(srv, 4);
    pool.add(Client(idle,  {}, now - seconds(300)));  // 5 min ago — idle
    pool.add(Client(fresh, {}, now));                  // just now — fresh

    size_t removed = pool.purge_idle(seconds(60), now);

    CHECK(removed == 1);
    REQUIRE(pool.client_count() == 1);

    close(srv); close(srv_peer); close(idle_peer); close(fresh_peer);
}

TEST_CASE("ClientPool::purge_idle: returns count of removed clients") {
    auto [srv, srv_peer] = make_socketpair();
    auto [c1, p1] = make_socketpair();
    auto [c2, p2] = make_socketpair();
    auto [c3, p3] = make_socketpair();

    ClientPool pool(srv, 4);
    pool.add(Client(c1, {}));
    pool.add(Client(c2, {}));
    pool.add(Client(c3, {}));

    auto now = steady::now() + seconds(120);
    size_t removed = pool.purge_idle(seconds(60), now);

    CHECK(removed == 3);
    CHECK(pool.client_count() == 0);

    close(srv); close(srv_peer);
    close(p1); close(p2); close(p3);
}
