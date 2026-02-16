# CI Overview

## Workflows

### CI (pull_request / push)

Runs on every PR and push to main.

**Triggers:**
- `pull_request` - Runs on PR commits
- `push` to `main` - Runs on main pushes
- `workflow_dispatch` - Manual trigger

**Jobs:**
1. **Gitlink validation** - Checks `.local` is a gitlink (submodule) in tree
2. **Submodule pointer gate** - For PRs, detects if `.local` pointer changed
3. **Emoji check** - Fails if 4-byte UTF-8 emojis in commit messages (protects cross-platform compatibility)
4. **Governance gate** - Local verification script
5. **Secret scan** - Scans for obvious leaked credentials
6. **Build** - Runs only when `CMakeLists.txt` exists in repo
7. **Lint** - clang-format + clang-tidy
8. **Test** - Runs only when `CMakeLists.txt` exists in repo

**Platform matrix:** ubuntu-latest, macos-latest, windows-latest

**Concurrency:** Cancels older runs when new commits are pushed.

**Note:** The `.local` submodule contains private governance/code and is not fetched in CI. Only the gitlink entry is validated.

### Nightly Sanitizers

Runs daily at 3 AM UTC (or manually via workflow_dispatch).

**Matrix:**
- ubuntu-latest: asan-ubsan, tsan
- macos-latest: asan-ubsan

**Limitations:**
- Windows sanitizers not enabled (clang-msan not available on windows-latest)
- tsan on macOS not enabled due to LLVM version constraints

**Purpose:** Detect memory errors, undefined behavior, and threading issues.

### Release

Runs on git tags (`v*`) or manually via workflow_dispatch.

**Artifacts:**
- `heidi-kernel-ubuntu-x64.tar.gz`
- `heidi-kernel-macos-x64.tar.gz`
- `heidi-kernel-windows-x64.zip`

**Note:** Build and packaging are skipped if no `CMakeLists.txt` exists (governance-only commits).

## Status Checks

All checks must pass before merging to main:
- `ci / build-test` (required)
- All jobs must be green

## Running CI Manually

If checks don't attach to your PR:
1. Go to Actions tab
2. Select the workflow
3. Click "Run workflow"

## Emoji Check

The emoji check prevents 4-byte UTF-8 sequences that can cause issues in some git tools:
- Blocks: `\U0001F600-\U0001F64F` (emoticons), `\U0001F300-\U0001F5FF` (symbols), `\U0001F680-\U0001F6FF` (transport), `\U0001F1E0-\U0001F1FF` (flags)
- Does NOT block: ASCII characters, 2-byte UTF-8 (e.g., accented letters), emoji in other contexts
