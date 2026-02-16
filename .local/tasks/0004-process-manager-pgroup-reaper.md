keywords: implement, managed_processes_only, clean_shutdown, deterministic_under_load
onboarding: acknowledged
acceptance:
  - Spawns child into process group
  - Kill job terminates entire group
  - Never touches non-managed processes

## Description

Implement process manager with job container and reaper.

## Scope

- Spawn children into process group (or cgroup v2)
- Track PIDs per job
- Implement SIGTERM -> SIGKILL termination sequence
- Set PR_SET_CHILD_SUBREAPER to reap orphaned descendants

## Implementation notes

- Only manage/terminate heidi-managed processes
- Never scan or kill arbitrary system processes
- Log all termination actions (rate-limited)
