# IPC Protocol v1

The `heidi-kernel` daemon uses Unix domain sockets for IPC. This document describes the Protocol v1 contract.

## Protocol Specification

- **Transport**: Unix Domain Socket (SOCK_STREAM)
- **Framing**: Line-delimited (commands and responses end with `\n`)
- **Default Path**: `/run/heidi-kernel/heidi-kernel.sock` (falls back to `$XDG_RUNTIME_DIR/heidi-kernel.sock` if set)
- **Override**:
  - Environment variable: `HEIDI_KERNEL_SOCK`
  - Command line: `--socket-path <path>`

## Commands and Responses

### `ping`
Checks if the daemon is alive.
- **Request**: `ping`
- **Response**: `PONG`

### `status` / `status/json`
Returns a JSON snapshot of the daemon's current state.
- **Request**: `status` or `status/json`
- **Response**: A single line of JSON containing:
  - `protocol_version`: Integer (currently 1)
  - `version`: Daemon version string
  - `pid`: Process ID
  - `uptime_ms`: Uptime in milliseconds
  - `rss_kb`: Resident Set Size in KB
  - `threads`: Number of threads
  - `queue_depth`: Current job queue depth

### `version`
Returns the protocol and daemon version.
- **Request**: `version`
- **Response**: `PROTOCOL 1 DAEMON <version>`

### `governor/policy`
Returns the current resource governor policy.
- **Request**: `governor/policy`
- **Response**: Multiline key-value pairs.

### `governor/diagnostics`
Returns governor tick diagnostics.
- **Request**: `governor/diagnostics`
- **Response**: Multiline key-value pairs.

### `metrics latest`
Returns the latest system metrics sample.
- **Request**: `metrics latest`
- **Response**: JSON snapshot of metrics.

### `metrics tail <n>`
Returns the last `<n>` metrics samples.
- **Request**: `metrics tail <n>`
- **Response**: CSV or JSON list of metrics.

## Error Format

If a command fails or is unrecognized, the daemon returns a single-line error message:

```
ERR <code> <message>
```

Common error codes:
- `UNKNOWN_COMMAND`: The command was not recognized.
- `INVALID_ARGUMENT`: Command arguments are invalid.
- `INTERNAL_ERROR`: An internal error occurred while processing the command.

## Unix Socket API (Direct)

The kernel daemon listens on `/run/heidi-kernel/heidi-kernel.sock` (default), or `$XDG_RUNTIME_DIR/heidi-kernel.sock` if set.

### Request Types

#### `ping`
Returns `PONG\n`.

#### `status` / `status/json` / `STATUS`
Returns a JSON snapshot with protocol version, daemon version, pid, uptime, rss, threads, queue depth, etc.

#### `governor/policy`, `governor/policy_update <json>`, `governor/diagnostics`, `metrics latest`, `metrics tail <n>`
See main protocol above. All operate over the Unix domain socket only in this configuration.

## Response Format
Most commands return JSON or plain text. HTTP/dashboard APIs are no longer present in daemon scope.

