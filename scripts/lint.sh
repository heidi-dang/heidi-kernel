#!/usr/bin/env bash
# Lint script - runs clang-tidy on C++ source files
# Requires compile_commands.json from a debug build

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"

cd "$ROOT_DIR"

echo "=== Running lint checks ==="

# Check for clang-tidy
if ! command -v clang-tidy >/dev/null 2>&1; then
  echo "WARN: clang-tidy not found; skipping" >&2
  exit 0
fi

# Check for compile_commands.json
CC_DB="$ROOT_DIR/build/debug/compile_commands.json"
if [[ ! -f "$CC_DB" ]]; then
  echo "WARN: compile_commands.json not found (build debug first); skipping clang-tidy" >&2
  echo "Run: cmake --preset debug && cmake --build --preset debug"
  exit 0
fi

# Find C++ source files
CPP_FILES=$(git ls-files "*.cpp" "*.cc" "*.cxx" 2>/dev/null || true)

if [[ -z "$CPP_FILES" ]]; then
  echo "No C++ source files found."
  exit 0
fi

# Run clang-tidy on a bounded set of files (can expand over time)
TUS=$(echo "$CPP_FILES" | head -n 50)
ERRORS=0

for tu in $TUS; do
  echo "Checking: $tu"
  if ! clang-tidy "$tu" -p "$ROOT_DIR/build/debug" 2>&1; then
    ERRORS=1
  fi
done

if [[ "$ERRORS" == "1" ]]; then
  echo "FAIL: clang-tidy found issues"
  exit 1
fi

echo "PASS: lint check complete"
exit 0
