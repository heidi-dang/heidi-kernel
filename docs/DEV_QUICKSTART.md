# Developer Quickstart

## Prerequisites

- **OS**: Linux (WSL2 recommended), macOS, or Windows
- **Tools**:
  - Git
  - CMake 3.22+
  - Ninja
  - Clang (clang, clang++)
  - Optional: clang-format, clang-tidy

## Clone and Setup

```bash
# Clone with submodules
git clone --recurse-submodules https://github.com/heidi-dang/heidi-kernel
cd heidi-kernel

# Or if already cloned:
git submodule update --init --recursive
```

## Build

```bash
# Debug build
cmake --preset debug
cmake --build --preset debug

# Release build
cmake --preset release
cmake --build --preset release
```

## Test

```bash
# Run tests
ctest --preset debug
# Or use the script
./scripts/test.sh
```

## Format

```bash
# Check formatting (Linux/macOS)
./scripts/format.sh

# Fix formatting
clang-format -i --style=file <files>

# Windows
powershell -ExecutionPolicy Bypass -File scripts/format.ps1
```

## Lint

```bash
# Requires debug build first for compile_commands.json
cmake --preset debug
cmake --build --preset debug

# Run lint
./scripts/lint.sh

# Windows
powershell -ExecutionPolicy Bypass -File scripts/lint.ps1
```

## Local Verification

Run the local governance verification script before pushing:

```bash
./scripts/verify_local.sh
```

## Common Issues

### "No C++ source files found"
This is expected if the project has no source code yet.

### "compile_commands.json not found"
Run `cmake --preset debug` first.

### "clang-tidy not found"
Install clang-tidy: `sudo apt install clang-tidy` (Linux) or via brew (macOS).
