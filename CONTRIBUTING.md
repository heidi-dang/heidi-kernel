# Contributing to heidi-kernel

## Project structure

- **Public repo**: source code, CI, scripts, public docs
- **Private repo** (`.local` submodule): rules, tasks, worklogs

## Getting started

1. Clone with submodules: `git clone --recurse-submodules git@github.com:heidi-dang/heidi-kernel.git`
2. Sync before starting work:
   ```
   git checkout main
   git pull --rebase origin main
   git submodule update --init --recursive
   ```

## Workflow

1. Create a feature branch from main
2. Make changes following the rules in `.local/`
3. Update the `.local` submodule pointer (required for every PR)
4. Open a PR with updated pointer

## PR requirements

- Every PR must update the `.local` submodule pointer
- PR descriptions must include: What changed, What did not change, Bugs fixed, New behavior, Validation, Risk
- No emojis in PR titles, bodies, or commit messages

## Build & test

See `CMakeLists.txt` and `scripts/` for build instructions.
