# Contributing to heidi-kernel

heidi-kernel is a native C++23 runtime daemon for WSL2-first environments. It exists to keep developer workflows stable under load: bounded queues, backpressure, deterministic scheduling, and clean shutdown.

## Mandatory onboarding

**Before starting any work, read `./.local/INDEX.md`.**

First-time contributors must create an acknowledgement file at `./.local/ack/<your_name>.md` using the template in `./.local/ack/_template.md`. PRs may be rejected if this acknowledgement is missing for new developers.

## Worklog (mandatory per PR)

Every contributor must add a dated entry to their personal worklog for each PR:

- File: `.local/worklog/<your_name>.md`
- Template: `.local/worklog/_template.md`

See `./.local/WORKLOG_RULE.md` for details. CI will fail if no worklog entry is added.

## Non-negotiables (reject PR if violated)

1. **WSL2-first**: Must build and run on WSL2 Ubuntu.
2. **Near-zero idle CPU**: No busy loops. Any periodic work must be timer-driven and typically **>= 250ms**.
3. **Bounded memory**: No unbounded queues/caches. Prefer disk-backed cache over large RAM caches.
4. **No thread-per-request**: Use a small fixed thread pool and/or event-driven I/O.
5. **IPC**: Unix domain socket only. Use blocking I/O or epoll-based event loop.
6. **Backpressure > overload**: Bounded queues everywhere. When full, apply backpressure or fail fast.
7. **Deterministic under load**: Avoid global locks held across I/O, avoid unbounded retries.
8. **Clean shutdown**: SIGINT/SIGTERM must stop accepting new work, drain/stop safely, and release resources.
9. **Process safety**: Only manage/kill processes that are explicitly heidi-managed.
10. **Minimal dependencies**: Prefer standard library + small header-only libs if needed. No heavy frameworks.

## Performance budgets

- **Idle CPU**: near-zero (no polling loops)
- **Idle RSS**: keep small; justify anything > 50MB
- **Startup**: fast; no heavy init on boot
- **Under load**: bounded queues; no unbounded memory growth

## Coding standards

- Language: **C++23**
- Formatting: `clang-format` (enforced in CI)
- Warnings: treat as errors in CI
- Logging: structured, rate-limited for noisy paths
- Timeouts: all blocking ops must have timeouts or cancellation

## Local workflow

### Build

```bash
cmake --preset debug
cmake --build --preset debug
```

### Test

```bash
./scripts/test.sh
```

### Lint/Format

```bash
./scripts/lint.sh
```

## PR checklist

- [ ] No new busy loops or high-frequency polling
- [ ] All queues/caches bounded with clear limits
- [ ] No thread-per-request; concurrency is bounded
- [ ] Clean shutdown behavior verified
- [ ] If touching processes: only heidi-managed trees; see `docs/PROCESS_POLICY.md`
- [ ] Tests updated/added; CI passes
- [ ] Docs updated if behavior changed

## Definition of Done

A change is done only if:

- It has clear **acceptance criteria** (pass/fail)
- It includes tests or a verifiable smoke path
- It does not violate any non-negotiables

## Security

- Never commit secrets or tokens.
- Any files that contain secrets must be `0600` perms when created.
