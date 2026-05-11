# TCPEchoingServer Project Guidelines

This project is a small, event-driven TCP echoing server written in modern C++ (C++23). It accepts multiple concurrent clients, logs messages to a file, and handles graceful shutdowns via signals.

## Project Overview

*   **Architecture:** Single-threaded, event-driven using `poll(2)`.
*   **Technologies:** 
    *   Language: C++23 (`std::expected`, `std::format`, `std::filesystem`).
    *   Build System: CMake (requires 3.14+).
    *   Dependencies: `doctest` (automatically fetched via FetchContent).
    *   Platforms: Primarily POSIX (uses `<sys/socket.h>`, `<poll.h>`, etc.).

## Key Components

*   `main.cpp`: The entry point and main event loop. Manages socket lifecycle and client data dispatch.
*   `tcp_echo_core`: A static library containing:
    *   `logging/Logger.h/cpp`: Thread-safe file logging.
    *   `util/sockets.h/cpp`: Low-level socket utility functions.
*   `config/Config.h` & `ArgParser.h`: Configuration management and CLI argument parsing.
*   `tests/`: Unit tests for core components using `doctest`.

## Building and Running

### Build Instructions

```bash
# Using CMake (Recommended)
cmake -B build
cmake --build build

# Direct Compilation (Minimum requirements)
g++ -std=c++23 -O2 -o server main.cpp
```

### Running the Server

```bash
./build/TCPEchoingServer 8080 --log-file=server_msgs.log
```

*   **Port:** The first positional argument (required).
*   **Log File:** `--log-file=<path>` (optional, defaults to `./server_msgs.log`).

### Running Tests

```bash
# Using CMake/CTest
cmake --build build --target tests
ctest --test-dir build --output-on-failure
```

## Development Workflow: Test-Driven Development

This project strictly follows TDD. **Every behavioral change starts with a failing test.** Do not write production code without a covering test that currently fails.

The TDD Loop:
1.  **Red:** Write the smallest test that captures the new behavior or reproduces a bug. Confirm it fails for the expected reason.
2.  **Green:** Write the minimum production code needed to make the test pass.
3.  **Refactor:** Clean up production and test code while ensuring the suite stays green.

Tests live under `tests/` and use the **doctest** framework. When fixing a bug, include a regression test. When adding a feature, include the test in the same change.

## Development Conventions

*   **Modern C++:** Use C++23 features whenever appropriate.
*   **Error Handling:** Prefer `std::expected` for functions that can fail, especially in the socket layer.
*   **Testing:** All new core logic should be accompanied by unit tests in the `tests/` directory.
*   **Graceful Shutdown:** The server should handle `SIGINT` and `SIGTERM` to clean up resources.
*   **Encapsulation:** The project is in the process of refactoring towards better encapsulation (e.g., introducing an `App` object and persistent settings). Follow this direction for new features.
