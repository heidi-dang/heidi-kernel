# Rules (Non-negotiables)

1. WSL2-first: must build and run on WSL2 Ubuntu.
2. Near-zero idle CPU: no busy loops; periodic work must be timer-driven (typically >= 250ms).
3. Bounded memory: no unbounded queues/caches.
4. Backpressure > overload: bounded queues everywhere.
5. IPC via Unix domain socket.
6. Blocking I/O with timeouts OR epoll event loop.
7. No thread-per-request: fixed worker pool; bounded concurrency.
8. Deterministic under load: no unbounded retries; avoid global locks across I/O.
9. Clean shutdown: SIGINT/SIGTERM must stop safely; no orphan processes.
10. Process safety: only manage/terminate processes that are explicitly heidi-managed.
11. Minimal dependencies.
12. Logging must be structured and rate-limited on hot paths.
