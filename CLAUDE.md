# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build

CMake (C++23 required):

```bash
cmake -B cmake-build-debug && cmake --build cmake-build-debug
```

## Running

```bash
./cmake-build-debug/TCPEchoingServer <port> [--log-file=<path>]
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

Single-threaded, event-driven using `poll(2)`. All production code lives in the
`tcp_echo_core` static library; `main.cpp` is a thin 19-line entry point.

### Component map

| Component | Files | Responsibility |
|---|---|---|
| `Config` | `config/Config.h` | Plain struct: port, log path, poll size |
| `ArgParser` | `config/ArgParser` | Parses argv into Config; throws `std::invalid_argument` if port missing |
| `Logger` | `logging/Logger` | Mutex-guarded; `info`/`error` → stdout/stderr + file; `raw()` → file only |
| `TcpServer` | `net/TcpServer` | RAII listener: `getaddrinfo`→`bind`→`listen`; `accept_client()` returns `Client` |
| `Client` | `net/Client` | RAII move-only fd wrapper; `receive()` → recv into 256-byte buffer |
| `ClientPool` | `net/ClientPool` | Parallel `vector<pollfd>` + `vector<Client>`; mark-and-sweep removal via `for_each_ready_client(fn)` |
| `ShutdownManager` | `app/ShutdownManager` | Installs SIGINT/SIGTERM via `sigaction`; single-instance enforced with `compare_exchange_strong`; hooks fire in reverse order on destruction |
| `App` | `app/App` | Composes all subsystems; owns the `poll(2)` event loop |

`App` member declaration order matters for initialization: `cfg_` → `logger_` → `server_` → `shutdown_` → `pool_` (pool needs `server_.fd()`).

C++23 features in use: `std::format`.
