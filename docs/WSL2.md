# WSL2 Notes

heidi-kernel targets WSL2 Ubuntu as the primary environment.

## Requirements

- Must run with near-zero idle CPU.
- Must shutdown cleanly.
- Must not assume full system services are available.

## cgroup v2

- Prefer cgroup v2 for job containment when available.
- If unavailable, fall back to process groups.

## File watching

- Avoid excessive filesystem watching.
- Avoid scanning large trees at startup.

## Networking

- IPC is local via Unix domain socket.
- Do not require privileged ports or root.
