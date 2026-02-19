# IPC Protocol

The heidi-kernel daemon uses Unix domain sockets for IPC. Messages are line-delimited text.

## Unix Socket API (Direct)

The kernel daemon listens on `/tmp/heidi-kernel.sock` (default).

### Request Types

#### `ping`
Returns `pong\n`.

#### `status`
Returns multiline status including version and resource usage.

Response example:
```text
status
version: 0.1.0
cpu: 4.5%
mem_total: 8150108
mem_free: 7100324
running_jobs: 0
queued_jobs: 0
rejected_jobs: 0
blocked_reason: none
retry_after_ms: 0
cpu_pct: 4.5
mem_pct: 12.8
```

#### `governor/policy`
Returns the current resource governor policy.

Response example:
```text
governor/policy
max_running_jobs: 10
max_queue_depth: 100
cpu_high_watermark_pct: 85.0
mem_high_watermark_pct: 90.0
cooldown_ms: 1000
min_start_gap_ms: 100
```

#### `governor/policy_update <json>`
Updates the governor policy. Body must be a JSON object (partial updates allowed).

Example: `governor/policy_update {"max_running_jobs": 20}`

#### `governor/diagnostics`
Returns detailed diagnostics from the last governor tick.

Response example:
```text
governor/diagnostics
last_decision: START_NOW
last_block_reason: NONE
last_retry_after_ms: 0
last_tick_now_ms: 1708267594000
last_tick_running: 5
last_tick_queued: 2
jobs_started_this_tick: 3
jobs_scanned_this_tick: 10
```

#### `STATUS` (heidi-kernel binary only)
The minimal `heidi-kernel` binary responds to `STATUS\n` with a JSON snapshot.

Response:
```json
{"version":"0.1.0","pid":1234,"uptime_ms":1000,"rss_kb":4096,"threads":3,"queue_depth":0}
```

## Response Format
Most commands return multiline text with `key: value` pairs, unless otherwise specified.