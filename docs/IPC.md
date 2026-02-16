# IPC Protocol

The heidi-kernel daemon uses Unix domain sockets for IPC. Messages are line-delimited text.

## HTTP API (Dashboard)

The dashboard daemon (heidi-dashd) provides a REST API over HTTP.

### Endpoints

#### GET /api/status
Returns kernel status as JSON.

#### GET /api/governor/policy
Returns the current governor policy as JSON.

Request: GET /api/governor/policy

Response:
```json
{
  "max_running_jobs": 10,
  "max_queue_depth": 100,
  "cpu_high_watermark_pct": 85.0,
  "mem_high_watermark_pct": 90.0,
  "cooldown_ms": 1000,
  "min_start_gap_ms": 100
}
```

#### PUT /api/governor/policy
Updates the governor policy. Accepts partial updates. Unknown fields are ignored.

Request: PUT /api/governor/policy
Body: JSON with fields to update.

Response (200 OK):
```json
{
  "max_running_jobs": 20,
  "max_queue_depth": 200,
  "cpu_high_watermark_pct": 80.0,
  "mem_high_watermark_pct": 90.0,
  "cooldown_ms": 1000,
  "min_start_gap_ms": 100
}
```

Response (400 Validation Error):
```json
{
  "max_running_jobs": "must be between 1 and 1000",
  "cpu_high_watermark_pct": "must be between 0 and 100"
}
```

#### GET /api/governor/diagnostics
Returns tick diagnostics as JSON.

Request: GET /api/governor/diagnostics

Response:
```json
{
  "last_decision": "START_NOW",
  "last_block_reason": "NONE",
  "last_retry_after_ms": 0,
  "last_tick_now_ms": 1000,
  "last_tick_running": 5,
  "last_tick_queued": 2,
  "jobs_started_this_tick": 3,
  "jobs_scanned_this_tick": 10
}
```

## Unix Socket API (Direct)

## Request Types
- `ping` -> `pong`
- `status` -> multiline status with version, cpu, mem
- `metrics latest` -> latest cpu/mem metrics
- `metrics tail <n>` -> last n metrics from disk
- `governor/policy` -> multiline policy
- `governor/diagnostics` -> multiline diagnostics

## Response Format
Multiline text with key: value pairs or CSV for tail.