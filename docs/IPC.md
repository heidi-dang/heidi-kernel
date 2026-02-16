# IPC Protocol

The heidi-kernel daemon uses Unix domain sockets for IPC. Messages are length-prefixed JSON.

## Message Format
- 4 bytes: big-endian uint32 length of JSON payload
- JSON payload: UTF-8 encoded

## Request Types
- `{"type": "ping"}` -> `{"type": "pong"}`
- `{"type": "status"}` -> `{"type": "status", "version": "0.1.0", "uptime": 123, "queue_depth": 0}`

## Error Codes
- `{"type": "error", "code": 400, "message": "invalid request"}`