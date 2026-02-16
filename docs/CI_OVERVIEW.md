# CI Overview

## Workflows

### CI (pull_request / push)

Runs on every PR and push to main.

**Triggers:**
- `pull_request` - Runs on PR commits
- `push` to `main` - Runs on main pushes
- `workflow_dispatch` - Manual trigger

**Jobs:**
1. **Validation** - Checks `.local` is a gitlink (submodule)
2. **Emoji check** - Fails if emojis in commit messages
3. **Governance gate** - Local verification script
4. **Build** - Debug and Release builds on ubuntu-latest
5. **Lint** - clang-format + clang-tidy
6. **Test** - Runs test suite

**Concurrency:** Cancels older runs when new commits are pushed.

### Nightly Sanitizers

Runs daily at 3 AM UTC (or manually).

**Matrix:**
- ubuntu-latest: asan-ubsan, tsan
- macos-latest: asan-ubsan

**Purpose:** Detect memory errors, undefined behavior, and threading issues.

### Release

Runs on git tags (`v*`).

**Artifacts:**
- `heidi-kernel-ubuntu-x64.tar.gz`
- `heidi-kernel-macos-x64.tar.gz`
- `heidi-kernel-windows-x64.zip`

## Status Checks

All checks must pass before merging to main:
- `ci / build-test` (required)
- All jobs must be green

## Running CI Manually

If checks don't attach to your PR:
1. Go to Actions tab
2. Select the workflow
3. Click "Run workflow"
