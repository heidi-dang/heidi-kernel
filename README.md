# heidi-kernel

A native C++23 runtime daemon for WSL2-first environments. Bounded queues, backpressure, deterministic scheduling, and clean shutdown.

## Quickstart

```bash
# Clone with submodules
git clone --recurse-submodules https://github.com/heidi-dang/heidi-kernel
cd heidi-kernel

# Build
cmake --preset debug
cmake --build --preset debug

# Run kernel (terminal 1)
./build/debug/heidi-kernel

# Query status (terminal 2)
printf "STATUS\n" | socat - UNIX-CONNECT:/tmp/heidi-kernel.sock

# Run dashboard (terminal 3)
./build/debug/heidi-dashd

# Open browser
open http://127.0.0.1:7778
```

## Components

- **heidi-kernel**: Daemon with Unix socket for status queries
- **heidi-dashd**: HTTP bridge serving dashboard on port 7778

## Socket Protocol

Request: `STATUS\n`
Response: JSON with version, pid, uptime_ms, rss_kb, threads, queue_depth

## License

MIT License. See LICENSE.
