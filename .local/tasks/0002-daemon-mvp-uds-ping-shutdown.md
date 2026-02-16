keywords: implement, optimize_wsl2, wsl2_first, no_busy_loop, uds_ipc, clean_shutdown, minimal_deps
onboarding: acknowledged
acceptance:
  - Daemon starts and listens on UDS
  - Client ping returns OK
  - SIGINT/SIGTERM clean shutdown
  - Idle CPU near-zero

## Description

Create smallest runnable daemon MVP with UDS IPC and clean shutdown.

## Scope

- C++23 daemon skeleton
- UDS server with length-prefixed framing
- ping/status request handling
- Signal handling (SIGINT/SIGTERM) with clean shutdown
- Basic structured logging (rate-limited)
- Smoke test script

## Implementation notes

- Use blocking I/O or epoll (no busy loops)
- Minimal dependencies (std only + small libs if needed)
- Timer-driven periodic work >= 250ms if any
