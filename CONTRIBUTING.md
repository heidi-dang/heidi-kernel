# Contributing to heidi-kernel

heidi-kernel is a native C++23 runtime daemon for WSL2-first environments. It exists to keep developer workflows stable under load: bounded queues, backpressure, deterministic scheduling, and clean shutdown.

## Project structure

- **Public repo** (`heidi-kernel`): contains source code, CI, scripts, public docs.
- **Private repo** (`.local` submodule): contains rules, tasks, context, worklogs, acknowledgements.

## Mandatory onboarding

**Before starting any work, sync with latest main and read `.local/INDEX.md`.**

If you have access to the private `.local` submodule:
- First-time contributors must create an acknowledgement file at `.local/ack/<your_name>.md`
- See `.local/SYNC_POLICY.md` for the sync workflow

## Submodule workflow

### Clone once

```bash
git clone --recurse-submodules git@github.com:heidi-dang/heidi-kernel.git
cd heidi-kernel
```

### Daily start sync

```bash
git checkout main
git pull --rebase origin main
git submodule update --init --recursive
```

### Start a feature branch

```bash
git checkout -b feat/<short-name>
git submodule update --init --recursive
```

### Update rules/tasks/worklog

1. Commit + push to private `.local` repo
2. Bump the `.local` submodule pointer in public repo:

```bash
cd .local
git checkout main
git pull --rebase origin main
# edit files (rules/tasks/worklog)
git commit -am "chore(local): <summary>"
git push origin main
cd ..
git add .local
git commit -m "chore(local): bump .local submodule"
git push -u origin HEAD
```

### Before opening PR

```bash
git fetch origin
git rebase origin/main
git submodule update --init --recursive
git status
```

**Every PR must update the `.local` submodule pointer.**

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

## Security

- Never commit secrets or tokens.
- Any files that contain secrets must be `0600` perms when created.
