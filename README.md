# TCPEchoingServer

A small TCP server (C++) that accepts multiple concurrent clients, prints
received messages to stdout, and appends them to a log file. Comes with a
helper script, [server.sh](server.sh), to manage it as a background process.

## Build

CMake (C++23 required):

```bash
cmake -B cmake-build-debug && cmake --build cmake-build-debug
```

The resulting binary is `cmake-build-debug/TCPEchoingServer`.

## Running

```bash
./cmake-build-debug/TCPEchoingServer 8080 --log-file=server_msgs.log
```

- First positional argument: port.
- `--log-file=<path>` (optional): where received messages are appended.
  Defaults to `./server_msgs.log`.

The server stops gracefully on `SIGINT` or `SIGTERM`.

## Tests

```bash
cmake --build cmake-build-debug --target tests
ctest --test-dir cmake-build-debug --output-on-failure
```

## Managing with `server.sh`

[server.sh](server.sh) wraps start/status/stop as background-process operations.

```bash
# Start in the background; logs lifecycle to ./server.log and prints the PID
./server.sh start ./cmake-build-debug/TCPEchoingServer 8080

# Check status of a running PID (uses `ps`)
./server.sh status <pid>

# Graceful stop: SIGINT first, escalates to SIGTERM if still alive
./server.sh stop <pid>
```

Override the lifecycle log location with `LOG_FILE=/path/to/file ./server.sh start ...`.

## Architecture

Single-threaded, event-driven server using `poll(2)`. The codebase is split
into a static library (`tcp_echo_core`) linked by both the server binary and
the test executable.

### Components

- **`config/Config`** — plain struct holding port, log path, and poll size with
  defaults. Populated by `ArgParser` from `argc`/`argv`.

- **`config/ArgParser`** — parses `--key=value`, `--key value`, and positional
  arguments (keyed `"0"`, `"1"`, …). `apply(Config&)` throws
  `std::invalid_argument` if no port is given.

- **`logging/Logger`** — thread-safe (mutex-guarded) logger that writes `info`
  and `error` lines to stdout/stderr and appends to a log file. `raw()` writes
  bytes directly to the file.

- **`net/TcpServer`** — RAII TCP listener. Constructor runs `getaddrinfo` →
  `socket` → `setsockopt(SO_REUSEADDR)` → `bind` → `listen`, throwing
  `std::runtime_error` at each failure. Destructor closes the fd.
  `accept_client()` returns a `Client` or throws.

- **`net/Client`** — RAII move-only wrapper for an accepted client fd.
  `receive()` calls `recv` into a 256-byte internal buffer and returns the byte
  count (0 = disconnect, -1 = error). Destructor closes the fd; move constructor
  nulls the source fd to prevent double-close.

- **`net/ClientPool`** — holds parallel `vector<pollfd>` (index 0 = server fd)
  and `vector<Client>`. `poll(timeout_ms)` wraps `::poll`.
  `for_each_ready_client(fn)` iterates ready clients and removes those where
  `fn` returns `false` (mark-and-sweep reverse-order erase, no index
  arithmetic required).

- **`app/ShutdownManager`** — installs `SIGINT`/`SIGTERM` handlers via
  `sigaction`. Only one instance may exist at a time; the constructor throws
  `std::logic_error` if a second is attempted. The signal handler only sets an
  `std::atomic<bool>` (async-signal-safe). The destructor restores previous
  handlers and fires registered hooks in reverse registration order.

- **`app/App`** — composes all subsystems as value members in the order
  `Config → Logger → TcpServer → ShutdownManager → ClientPool`. `run()` owns
  the `poll(2)` event loop: accepts new connections, dispatches received data
  to the logger, and exits cleanly when `ShutdownManager::requested()` is true.
