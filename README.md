# TCPEchoingServer

A small TCP server (C++) that accepts multiple concurrent clients, prints
received messages to stdout, and appends them to a log file. Comes with a
helper script, [server.sh](server.sh), to manage it as a background process.

## Build

Compile `main.cpp` directly with g++ (C++23 is required for `std::expected` and `std::format`):

```bash
g++ -std=c++23 -O2 -o server main.cpp
```

## Running directly

```bash
./server 8080 --log-file=server_msgs.log
```

- First positional argument: port.
- `--log-file=<path>` (optional): where received messages are appended.
  Defaults to `./server_msgs.log`.

The server stops gracefully on `SIGINT` or `SIGTERM`.

## Managing with `server.sh`

[server.sh](server.sh) wraps start/status/stop as background-process operations.

```bash
# Start in the background; logs lifecycle to ./server.log and prints the PID
./server.sh start ./server 8080

# Check status of a running PID (uses `ps`)
./server.sh status <pid>

# Graceful stop: SIGINT first, escalates to SIGTERM if still alive
./server.sh stop <pid>
```

Override the lifecycle log location with `LOG_FILE=/path/to/file ./server.sh start ...`.

## Architecture

Single-threaded, event-driven server using `poll(2)`:

- **Bootstrapping** — `getaddrinfo` + `socket` + `bind` + `listen` set up a
  listening socket (IPv4/IPv6 agnostic, `SO_REUSEADDR`).
- **Event loop** — a `polling_file_descriptors` wrapper holds a `std::vector<pollfd>`.
  The listening socket plus every accepted client socket sit in the same poll
  set. Each iteration blocks on `poll(-1)` until something is readable.
- **Dispatch** — when the listening socket is ready, `accept` produces a new
  client fd and adds it to the poll set. When a client fd is ready, `recv`
  reads up to 256 bytes; the data is printed and appended to the log file
  stream. A zero-byte read or error closes and removes that fd.
- **Shutdown** — `SIGINT`/`SIGTERM` handlers flip an `std::atomic<bool>` flag;
  the loop exits cleanly on the next iteration and closes the listening socket.
