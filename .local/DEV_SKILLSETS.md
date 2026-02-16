# Dev Skillsets (Keyword-Controlled)

Use **comma-separated keywords** at the top of every task.

## Task header (required)

```text
keywords: <comma-separated keywords>
onboarding: acknowledged
acceptance:
  - <pass/fail checks>
```

## Work-Mode keywords
plan_phase, translate_for_dev, implement, refactor_safe, review, audit, lock_scope, optimize_wsl2, ci_gates, docs_only, tests_only

## Constraint keywords
wsl2_first, no_busy_loop, bounded_memory, bounded_queues, backpressure, uds_ipc, blocking_or_epoll, no_thread_per_request, disk_cache_preferred, deterministic_under_load, clean_shutdown, managed_processes_only, minimal_deps, warnings_as_errors, rate_limited_logging

## Kernel default bundle
```text
wsl2_first, no_busy_loop, bounded_memory, bounded_queues, backpressure, uds_ipc, blocking_or_epoll, no_thread_per_request, deterministic_under_load, clean_shutdown, managed_processes_only, minimal_deps, rate_limited_logging
```
