# Resource Governor

The Resource Governor controls job admission, execution, and enforcement based on system resources and configured policies.

## Tick Contract

The JobRunner is driven by deterministic ticks. Each call to `tick(now_ms, metrics)` advances the state:

```cpp
void JobRunner::tick(uint64_t now_ms, 
                     const SystemMetrics& metrics,
                     size_t max_starts_per_tick = 5,
                     size_t max_limit_scans_per_tick = 10);
```

- **now_ms**: Deterministic timestamp (milliseconds since epoch). All enforcement decisions derive from this value.
- **metrics**: SystemMetrics containing CPU usage and memory state.
- **max_starts_per_tick**: Bounded work - maximum jobs to start per tick (default: 5)
- **max_limit_scans_per_tick**: Bounded work - maximum running jobs to scan per tick (default: 10)

## Budgets

| Budget | Default | Description |
|--------|---------|-------------|
| start_budget | 5 | Maximum jobs started per tick |
| scan_budget | 10 | Maximum running jobs scanned per tick |

These budgets ensure bounded CPU and I/O work per tick, preventing the governor from becoming a denial-of-service vector.

## Policy Structure

```cpp
struct GovernorPolicy {
    int max_running_jobs = 10;        // Concurrent job limit
    int max_queue_depth = 100;        // Queue size limit
    double cpu_high_watermark_pct = 85.0;  // CPU % to block new jobs
    double mem_high_watermark_pct = 90.0;  // Memory % to block new jobs
    uint64_t cooldown_ms = 1000;      // Cooldown after HOLD decision
    uint64_t min_start_gap_ms = 100; // Minimum gap between job starts
};
```

## Per-Job Limits

Jobs can have individual limits set via `JobLimits`:

```cpp
struct JobLimits {
    uint64_t max_runtime_ms = 3600000;    // 1 hour default
    uint64_t max_log_bytes = 1048576;      // 1MB default
    int max_child_processes = 0;           // 0 = no limit
    uint64_t kill_grace_ms = 5000;         // Grace period before SIGKILL
};
```

## Enforcement Order

During each tick, enforcement happens in this order:

1. **Read output** - Non-blocking read from stdout/stderr pipes
2. **Check waitpid** - Reap finished processes (WNOHANG)
3. **Check timeout** - Terminate jobs exceeding `max_runtime_ms`
4. **Check log cap** - Truncate logs exceeding `max_log_bytes`
5. **Check process cap** - Terminate jobs exceeding `max_child_processes`

## Job States

| State | Description |
|-------|-------------|
| QUEUED | Waiting in queue |
| STARTING | About to be spawned |
| RUNNING | Actively executing |
| COMPLETED | Finished successfully (exit code 0) |
| FAILED | Finished with non-zero exit |
| TIMEOUT | Exceeded max_runtime_ms |
| PROC_LIMIT | Exceeded max_child_processes |

## Decision Types

The governor returns one of:

```cpp
enum class GovernorDecision {
    START_NOW,      // Start queued jobs
    HOLD_QUEUE,     // Wait, retry later
    REJECT_QUEUE_FULL  // Reject new submissions
};
```

## Block Reasons

```cpp
enum class BlockReason {
    NONE,
    CPU_HIGH,       // CPU usage above watermark
    MEM_HIGH,       // Memory usage above watermark
    QUEUE_FULL,     // Queue at capacity
    RUNNING_LIMIT   // Max concurrent jobs reached
};
```

## Determinism Guarantees

All enforcement decisions are deterministic:

1. **No wall-clock time** - All time-related decisions use `now_ms` parameter
2. **No random numbers** - No randomization in scheduling or enforcement
3. **Bounded work** - Fixed budgets prevent unbounded loops
4. **Pure governor** - `ResourceGovernor::decide()` is a pure function (metrics in, decision out)

## Testing

### Deterministic Testing

Tests must use deterministic ticks:

```cpp
uint64_t now_ms = 1000;
job_runner_->tick(now_ms, metrics);  // Not: tick(metrics)

// Advance time deterministically
now_ms = 2000;
job_runner_->tick(now_ms, metrics);
```

### Example: Timeout Enforcement

```cpp
JobLimits limits;
limits.max_runtime_ms = 100;

std::string job_id = job_runner_->submit_job("sleep 60", limits);

uint64_t now_ms = 0;
job_runner_->tick(now_ms, metrics);  // Start at t=0

now_ms = 101;  // After timeout
job_runner_->tick(now_ms, metrics);

auto job = job_runner_->get_job_status(job_id);
EXPECT_EQ(job->status, JobStatus::TIMEOUT);
```

### Example: Process Cap Enforcement

```cpp
JobLimits limits;
limits.max_child_processes = 3;

std::string job_id = job_runner_->submit_job("forkbomb", limits);

// Set fake process count for testing
inspector_->set_process_count(job->process_group, 10);

uint64_t now_ms = 1000;
job_runner_->tick(now_ms, metrics);

auto job = job_runner_->get_job_status(job_id);
EXPECT_EQ(job->status, JobStatus::PROC_LIMIT);
```

## Persistence

Policy can be persisted to disk via PolicyStore:

- Atomic writes: temp file + fsync + rename
- Validation: invalid values fall back to defaults
- Load: strict parsing with error reporting

See `PolicyStore` class in `include/heidi-kernel/policy_store.h`.

## Running Integration Tests

Integration tests verify end-to-end behavior with real processes.

### Run All Integration Tests

```bash
# From project root
cd build
ctest -R '^IntegrationTest' --output-on-failure
```

### Run Specific Integration Test

```bash
ctest -R 'IT_Timeout_KillsProcessGroup' --output-on-failure
```

### Notes

- Integration tests are excluded from the default test run (via `ctest -E '^IntegrationTest'`)
- Some tests may be skipped in restricted environments (e.g., container limitations on process groups)
- Tests use deterministic tick-driven loops, no sleep-based assertions
