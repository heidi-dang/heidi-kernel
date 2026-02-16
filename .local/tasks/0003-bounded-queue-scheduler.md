keywords: implement, bounded_queues, backpressure, deterministic_under_load, no_thread_per_request
onboarding: acknowledged
acceptance:
  - All queues have hard caps
  - Backpressure behavior defined + tested

## Description

Implement bounded queue scheduler with backpressure.

## Scope

- Define queue capacity limits
- Implement backpressure policy (fail fast or block with timeout)
- Ensure deterministic behavior under load
- Use fixed worker pool (no thread-per-request)

## Implementation notes

- Each queue must have explicit max size
- When full: return error OR block with bounded timeout
- Never allocate unbounded memory during spikes
