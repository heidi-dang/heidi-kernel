# C++ Strict Rules (C++23)

These rules are mandatory for heidi-kernel.

## 1) Toolchain + build flags (mandatory)

- Compiler: clang (preferred)
- Standard: C++23
- Warnings: treat as errors

**Required warnings (minimum):**
- `-Wall -Wextra -Wpedantic -Werror`
- `-Wshadow -Wconversion -Wsign-conversion -Wformat=2 -Wundef`

**Debug hardening (recommended):**
- `-D_GLIBCXX_ASSERTIONS`
- `-fno-omit-frame-pointer`

**Sanitizers (CI required for Debug):**
- AddressSanitizer + UndefinedBehaviorSanitizer

## 2) Formatting + static analysis (mandatory)

- `clang-format` is required (CI gate)
- `clang-tidy` runs on a bounded set of TUs in CI (expand over time)

## 3) Error handling (mandatory)

- No exceptions across module boundaries.
- Core runtime code should be **non-throwing**.
- Prefer `std::expected<T, Err>` (or explicit error enums) over throwing.
- Every syscall / IO call must check return values and handle `errno`.

## 4) Forbidden patterns

- No `new`/`delete` (use smart pointers or arenas).
- No `std::make_unique`/`make_shared` in hot paths (prefer pool allocation).
- No unbounded containers in any kernel path (`std::vector` with unbounded growth, `std::map` without bounds).
- No busy loops. Use timers (`std::chrono`) or event-driven I/O.
- No floating point in scheduling/timing paths.
- No global locks held across I/O boundaries.
- No unbounded retries. Always use bounded retry with timeout.
- No `printf`/`scanf`. Use structured logging or `fmt`.
- No `sleep` in event loops. Use `epoll_wait` or timerfd.

## 5) Concurrency model (mandatory)

- Use a **small fixed thread pool** (size must be justified, typically 2-8).
- No thread-per-request model.
- All shared state must use proper synchronization (`std::mutex`, `std::atomic`, or lock-free for simple cases).
- Avoid lock contention: partition data, use per-thread buffers.
- Use `std::jthread` for cooperative cancellation.

## 6) Resource management (mandatory)

- All resources must have explicit lifetime: RAII wrappers for FDs, file handles, memory.
- Bounded queues everywhere. Hard cap + backpressure policy.
- Memory: pre-allocate or use memory pools for hot paths.
- File descriptors: always `close()` in destructor or via RAII.
- Timeouts: every blocking operation must have a timeout.

## 7) API boundaries

- Public APIs must be C++23 with explicit `[[nodiscard]]`.
- Return `std::expected<T, E>` for fallible operations.
- Use `std::span` instead of raw pointers + size.
- Use `std::string_view` for read-only string parameters.

## 8) Testing

- Unit tests: use `doctest` or `catch2`.
- Integration tests: minimal shell scripts.
- Property-based testing for core data structures (bounded queues, state machines).
- Fuzzing for parser/unserializer code.

## 9) CI gates

- **Every commit**: builds with clang, warnings as errors.
- **Every commit**: clang-format check passes.
- **Debug builds**: sanitizers enabled (ASan + USan).
- **PR**: clang-tidy on changed files (at minimum).
- **PR**: no regressions in existing tests.

## 10) Performance budgets

- Idle CPU: near-zero (no polling).
- Startup: < 500ms to ready.
- Per-request latency: bounded, no unbounded queuing.
- Memory: justified RSS; anything > 50MB must be reviewed.
