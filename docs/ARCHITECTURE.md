# Architecture

heidi-kernel is an infrastructure daemon that provides deterministic scheduling, resource governance, and safe process management for developer workflows on WSL2.

## Goals

- Stable under load (bounded queues, backpressure)
- Near-zero idle CPU
- Deterministic behavior
- Clean shutdown
- Minimal dependencies
- Unix domain socket IPC

## Non-goals

- UI
- Web backend
- Remote orchestration
- Heavy plugin frameworks

## High-level components

1. **IPC Server (UDS)**
   - Listens on a Unix domain socket.
   - Accepts requests to start/stop jobs, query status, and stream events.

2. **Scheduler**
   - Owns bounded queues.
   - Applies backpressure when saturated.
   - Runs tasks via a small fixed worker pool (or event loop + workers).

3. **Job Runtime**
   - Represents a job (invocation) with:
     - Job id
     - Limits (time, process count, spawn rate, memory/cpu caps if available)
     - Process container (cgroup v2 or process group)

4. **Process Manager**
   - Spawns and tracks child processes.
   - Ensures clean termination.
   - Implements the reaper policy (see `PROCESS_POLICY.md`).

5. **Cache (optional, later)**
   - Disk-backed by default.
   - Explicit size limits.

6. **Metrics/Tracing (minimal)**
   - Basic counters and durations.
   - Rate-limited logging.

## IPC wire framing (recommended)

- Transport: Unix domain socket
- Messages: length-prefixed frames
  - `uint32_le length` + `length bytes payload`
- Payload: JSON (initially) or compact binary later
- Response: same framing; include request id

## Concurrency model

- One accept thread (or event loop)
- Bounded worker pool for CPU work
- Blocking I/O with timeouts, or epoll loop for socket I/O

## Backpressure policy

- Each queue has a fixed cap.
- If full:
  - Return an error immediately OR
  - Block with a bounded timeout
- Never allocate unbounded memory during spikes.

## Clean shutdown

On SIGINT/SIGTERM:

1. Stop accepting new IPC connections.
2. Stop accepting new jobs.
3. Drain in-flight work up to a timeout.
4. Terminate heidi-managed process trees.
5. Exit with a clear status code.
