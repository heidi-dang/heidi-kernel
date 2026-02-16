# IPC Protocol

The heidi-kernel daemon uses Unix domain sockets for IPC. Messages are line-delimited text.

## Request Types
- `ping` -> `pong`
- `status` -> multiline status with version, cpu, mem
- `metrics latest` -> latest cpu/mem metrics
- `metrics tail <n>` -> last n metrics from disk

## Response Format
Multiline text with key: value pairs or CSV for tail.