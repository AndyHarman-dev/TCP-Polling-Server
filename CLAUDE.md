# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build

Compile directly with g++ (preferred for quick iteration):

```bash
g++ -std=c++23 -O2 -o server main.cpp
```

Or via CMake (C++23 required):

```bash
cmake -B cmake-build-debug && cmake --build cmake-build-debug
```

## Running

```bash
./server <port> [--log-file=<path>]
```

The first positional argument is the port. `--log-file` defaults to `./server_msgs.log`. Stop with `Ctrl-C` (SIGINT) or SIGTERM.

## Background process management (`server.sh`)

```bash
./server.sh start ./server 8080   # launches in background, prints PID
./server.sh status <pid>
./server.sh stop <pid>            # SIGINT first, escalates to SIGTERM
```

Override the lifecycle log with `LOG_FILE=/path/to/file ./server.sh start ...`.

## Development workflow: Test-Driven Development

This project follows TDD. **Every behavioral change starts with a failing test.** Do not write production code without a covering test that currently fails.

The loop:

1. **Red** — write the smallest test that captures the new behavior or reproduces the bug. Build and run it; confirm it fails *for the reason you expect* (not a compile error or unrelated assertion).
2. **Green** — write the minimum production code needed to make the test pass. Resist generalizing; just satisfy the test.
3. **Refactor** — clean up production and test code with the now-green suite as a safety net. Run tests after each non-trivial edit.

Tests live under `tests/`, built as a separate executable that links against the production static library (`tcp_echo_core`). The framework is **doctest** (`TEST_CASE` / `CHECK` / `REQUIRE`).

Run tests:

```bash
cmake --build cmake-build-debug --target tests
ctest --test-dir cmake-build-debug --output-on-failure
```

When fixing a reported bug, the first commit (or the first hunk of the fix commit) should be the failing regression test. When adding a feature, no production-code commit should land without an accompanying test in the same change.

## Architecture

Single file (`main.cpp`), single-threaded, event-driven using `poll(2)`.

- **`polling_file_descriptors`** — thin RAII wrapper around `std::vector<pollfd>`. Holds the listening socket and all accepted client sockets in one poll set.
- **Event loop** — blocks on `poll(-1)`; dispatches to `handle_new_connection` (listening socket ready) or `handle_client_data` (client fd ready). Client removal during iteration decrements the loop index to stay correct.
- **`handle_client_data`** — reads up to 256 bytes, writes to stdout and appends to the log `std::ofstream`. A zero-byte read means the client hung up; any error also closes and removes the fd.
- **Shutdown** — `SIGINT`/`SIGTERM` handlers set `app::gstop` (`std::atomic<bool>`); the loop exits on the next iteration and closes the listening socket.
- **`get_listen_socket`** — uses `getaddrinfo` + `SO_REUSEADDR`; returns `std::expected<int, std::string>`.
- **`parse_args`** — handles `--key=value`, `--key value`, and positional arguments (keyed by `"0"`, `"1"`, …).

C++23 features in use: `std::expected`, `std::format`.
