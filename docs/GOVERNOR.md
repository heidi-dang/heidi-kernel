# Governor (Process Resource Governor)

The process resource governor enforces per-process resource policy (CPU/affinity/memory ceilings) with deterministic rules, bounded queues, and near-zero idle cost.

## Architecture

- **Ingress**: Bounded queue (256 messages) with backpressure
- **Tracking**: pidfd for lifecycle (with procfs start_time fallback)
- **Apply**: Single-threaded deterministic apply loop
- **Events**: Non-blocking event emission via callback

## GOV_APPLY Message Schema

Message type: `GOV_APPLY`, length-prefixed, max 512 bytes.

```json
{
  "pid": 1234,
  "cpu": {
    "affinity": "0-3",
    "nice": 10,
    "max_pct": 80
  },
  "mem": {
    "max_bytes": 8589934592
  },
  "pids": {
    "max": 256
  },
  "rlim": {
    "nofile_soft": 1024,
    "nofile_hard": 4096,
    "core_soft": 0,
    "core_hard": 0
  },
  "oom_score_adj": 500
}
```

### Fields

| Field | Type | Description |
|-------|------|-------------|
| `pid` | int (required) | Process ID |
| `cpu.affinity` | string | CPU mask (e.g., "0-3", "0,2,4") |
| `cpu.nice` | int8 | Nice value (-20 to 19) |
| `cpu.max_pct` | uint8 | Max CPU % (for cgroup) |
| `mem.max_bytes` | uint64 | Max memory (for cgroup) |
| `pids.max` | uint32 | Max pids (for cgroup) |
| `rlim.nofile_soft` | uint64 | RLIMIT_NOFILE soft |
| `rlim.nofile_hard` | uint64 | RLIMIT_NOFILE hard |
| `rlim.core_soft` | uint64 | RLIMIT_CORE soft |
| `rlim.core_hard` | uint64 | RLIMIT_CORE hard |
| `oom_score_adj` | int | -1000 to 1000 |

## ACK/NACK Codes

| Code | Description |
|------|-------------|
| `ACK` | Success |
| `NACK_INVALID_PAYLOAD` | Invalid JSON or payload > 512B |
| `NACK_INVALID_PID` | PID <= 0 |
| `NACK_INVALID_RANGE` | Value out of valid range |
| `NACK_PARSE_ERROR` | Malformed JSON |
| `NACK_UNKNOWN_FIELD` | Unknown field in JSON |
| `NACK_QUEUE_FULL` | Ingress queue full |
| `NACK_PROCESS_DEAD` | Target process not found |

## Event Types

| Event | Code | Description |
|-------|------|-------------|
| `GOV_APPLIED` | 0 | Policy applied successfully |
| `GOV_FAILED` | 1 | Policy application failed |
| `GOV_PID_EXIT` | 2 | Tracked process exited |
| `GOV_EVICTED` | 3 | Rule evicted due to capacity |

## Feature Flags

- `HEIDI_CGROUP=1`: Enable cgroup v2 enforcement (optional, off by default)

## Cgroup v2

When enabled (`HEIDI_CGROUP=1`), the governor creates `/sys/fs/cgroup/heidi/<pid>/` and applies:
- `cpu.max`: CPU quota (requires cpu controller)
- `memory.max`: Memory limit (requires memory controller)  
- `pids.max`: PIDs limit (requires pids controller)

If cgroup v2 is unavailable, degrades gracefully to non-cgroup apply mechanisms.

## Performance

- **Idle CPU**: <0.3% on WSL2 (no clients)
- **RSS**: <42MB after 10k GOV_APPLY messages
- **Ingress**: 10k msgs/sec sustained
- **p99 apply latency**: <5ms (non-cgroup operations)
