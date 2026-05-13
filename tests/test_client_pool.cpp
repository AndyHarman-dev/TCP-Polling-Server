#include <doctest/doctest.h>
#include <sys/socket.h>
#include <unistd.h>
#include "net/Client.h"
#include "net/ClientPool.h"

static std::pair<int, int> make_pipe() {
    int p[2];
    REQUIRE(pipe(p) == 0);
    return {p[0], p[1]};  // {read_end, write_end}
}

// Returns {our_fd, peer_fd} — caller owns peer_fd and must close it;
// our_fd is to be transferred to a Client.
static std::pair<int, int> make_pair() {
    int sv[2];
    REQUIRE(socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == 0);
    return {sv[0], sv[1]};
}

TEST_CASE("ClientPool: is_server_ready when server fd has data") {
    auto [srv, srv_trigger] = make_pair();

    ClientPool pool(srv, 4);
    write(srv_trigger, "x", 1);
    pool.poll(100);

    CHECK(pool.is_server_ready());

    close(srv);
    close(srv_trigger);
}

TEST_CASE("ClientPool: for_each_ready_client invokes fn for a ready client") {
    auto [srv, srv_trigger] = make_pair();
    auto [cli, cli_trigger] = make_pair();  // cli transferred to Client

    ClientPool pool(srv, 4);
    pool.add(Client(cli, {}));

    write(cli_trigger, "hi", 2);
    pool.poll(100);

    int calls = 0;
    pool.for_each_ready_client([&](Client& c) -> bool {
        ++calls;
        c.receive();
        return true;
    });

    CHECK(calls == 1);
    CHECK(pool.client_count() == 1);

    close(srv);
    close(srv_trigger);
    close(cli_trigger);
    // cli closed by ClientPool → Client destructor
}

TEST_CASE("ClientPool: for_each_ready_client removes client when fn returns false") {
    auto [srv, srv_trigger] = make_pair();
    auto [cli, cli_trigger] = make_pair();

    ClientPool pool(srv, 4);
    pool.add(Client(cli, {}));

    write(cli_trigger, "x", 1);
    pool.poll(100);

    pool.for_each_ready_client([](Client&) -> bool { return false; });

    CHECK(pool.client_count() == 0);

    close(srv);
    close(srv_trigger);
    close(cli_trigger);
}

TEST_CASE("ClientPool: is_wakeup_ready when wakeup fd has data") {
    auto [srv, srv_trigger] = make_pair();
    auto [rfd, wfd]         = make_pipe();

    ClientPool pool(srv, 4, rfd);

    write(wfd, "x", 1);
    pool.poll(100);

    CHECK(pool.is_wakeup_ready());

    close(srv); close(srv_trigger); close(rfd); close(wfd);
}

TEST_CASE("ClientPool: for_each_ready_client works correctly with wakeup fd set") {
    auto [srv, srv_trigger] = make_pair();
    auto [cli, cli_trigger] = make_pair();
    auto [rfd, wfd]         = make_pipe();

    ClientPool pool(srv, 4, rfd);
    pool.add(Client(cli, {}));

    write(cli_trigger, "hi", 2);
    pool.poll(100);

    int calls = 0;
    pool.for_each_ready_client([&](Client& c) -> bool {
        ++calls;
        c.receive();
        return true;
    });

    CHECK(calls == 1);
    CHECK(pool.client_count() == 1);

    close(srv); close(srv_trigger); close(cli_trigger); close(rfd); close(wfd);
}
