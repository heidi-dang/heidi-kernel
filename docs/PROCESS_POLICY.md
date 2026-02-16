# Process Policy

This document defines what heidi-kernel may start, track, and terminate.

## Definitions

- **heidi-managed process**: a process spawned by heidi-kernel (directly or as a descendant) and tracked as part of a job.
- **job container**: the mechanism used to group processes for a job:
  - Preferred: **cgroup v2** (pids controller)
  - Fallback: **process group** (+ explicit PID tracking)

## Invariants (must always hold)

1. heidi-kernel **must never** scan and kill arbitrary system processes.
2. heidi-kernel **may only** terminate processes that are heidi-managed.
3. Every job must have a job container (cgroup or process group).
4. All termination actions must be logged (rate-limited) with job id and reason.

## Process creation

- All child processes are spawned through the Process Manager.
- On spawn:
  - Assign to job container.
  - Record PID, start time.
  - Start spawn-rate accounting.

## Limits (recommended defaults)

- **Max processes per job**: 200
- **Spawn rate**: 30 new processes per 10 seconds
- **Max runtime per job**: optional, off by default

If a limit is exceeded:

- Terminate the entire job container (all processes for that job).

## Idle cleanup

If there are **no active jobs**, and any heidi-managed containers remain after a grace window (default 5 minutes):

- Terminate remaining heidi-managed containers.

## Subreaper (recommended)

Set `PR_SET_CHILD_SUBREAPER` so that if a parent dies, heidi-kernel can adopt and reap descendants of heidi-managed trees, preventing unreaped dead processes.

Note: a true Linux "zombie" (state Z) cannot be killed; it disappears when the parent reaps it. Subreaper behavior helps ensure reaping occurs for heidi-managed trees.

## Termination sequence

When terminating a job:

1. Send SIGTERM to the job container.
2. Wait up to `T_TERM` (e.g., 2s).
3. If any remain, send SIGKILL.
4. Confirm job container is empty.

## What is forbidden

- Killing processes not in heidi-managed containers
- Infinite retries on spawn failure
- Polling loops with tight intervals
