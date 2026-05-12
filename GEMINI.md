# TCPEchoingServer Project Guidelines

This project is a small, event-driven TCP echoing server written in modern C++ (C++23). It accepts multiple concurrent clients, logs messages to a file, and handles graceful shutdowns via signals.

## Project Overview

*   **Architecture:** Single-threaded, event-driven using `poll(2)`. All production code lives in the `tcp_echo_core` static library; `main.cpp` is a thin 19-line entry point.
*   **Technologies:**
    *   Language: C++23 (`std::format`, `std::filesystem`).
    *   Build System: CMake (requires 3.14+).
    *   Dependencies: `doctest` v2.4.11 (automatically fetched via FetchContent).
    *   Platforms: Primarily POSIX (uses `<sys/socket.h>`, `<poll.h>`, etc.).

## Key Components

`tcp_echo_core` static library:

*   `config/Config.h`: Plain struct — port, log path, poll size (with defaults).
*   `config/ArgParser`: Parses `--key=value`, `--key value`, and positional args; `apply(Config&)` throws `std::invalid_argument` if port is absent.
*   `logging/Logger`: Mutex-guarded; `info`/`error` write to stdout/stderr and log file; `raw()` writes bytes to file only.
*   `net/TcpServer`: RAII listener — constructor runs `getaddrinfo`→`bind`→`listen`, throws `std::runtime_error` on failure; `accept_client()` returns a `Client`.
*   `net/Client`: RAII move-only fd wrapper; `receive()` calls `recv` into a 256-byte internal buffer (returns bytes, 0=disconnect, -1=error).
*   `net/ClientPool`: Parallel `vector<pollfd>` + `vector<Client>`; `for_each_ready_client(fn)` removes clients where `fn` returns `false` via mark-and-sweep reverse erase.
*   `app/ShutdownManager`: Installs SIGINT/SIGTERM via `sigaction`; enforces single-instance with `compare_exchange_strong` (throws `std::logic_error` if violated); hooks fire in reverse registration order on destruction.
*   `app/App`: Composes all subsystems as value members; owns the `poll(2)` event loop.

Member declaration order in `App` is load-bearing: `cfg_` → `logger_` → `server_` → `shutdown_` → `pool_` (pool requires `server_.fd()`).

## Building and Running

### Build Instructions

```bash
cmake -B cmake-build-debug
cmake --build cmake-build-debug
```

### Running the Server

```bash
./cmake-build-debug/TCPEchoingServer 8080 --log-file=server_msgs.log
```

*   **Port:** The first positional argument (required).
*   **Log File:** `--log-file=<path>` (optional, defaults to `./server_msgs.log`).

### Running Tests

```bash
cmake --build cmake-build-debug --target tests
ctest --test-dir cmake-build-debug --output-on-failure
```

## Development Workflow: Test-Driven Development

This project strictly follows TDD. **Every behavioral change starts with a failing test.** Do not write production code without a covering test that currently fails.

The TDD Loop:
1.  **Red:** Write the smallest test that captures the new behavior or reproduces a bug. Confirm it fails for the expected reason.
2.  **Green:** Write the minimum production code needed to make the test pass.
3.  **Refactor:** Clean up production and test code while ensuring the suite stays green.

Tests live under `tests/` and use the **doctest** framework (`TEST_CASE` / `CHECK` / `REQUIRE`). When fixing a bug, include a regression test. When adding a feature, include the test in the same change.

## Development Conventions

*   **Modern C++:** Use C++23 features where appropriate (`std::format`, structured bindings, etc.).
*   **Error Handling:** Throw standard exceptions (`std::runtime_error`, `std::invalid_argument`, `std::logic_error`). Do not use `std::expected`.
*   **RAII:** All resource ownership (fds, signal handlers) must be managed by destructors. No manual cleanup in application code.
*   **Graceful Shutdown:** The server handles `SIGINT` and `SIGTERM` via `ShutdownManager`. Signal handlers must be async-signal-safe (flag-only).
