#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

fail() { echo "FAIL: $*" >&2; exit 1; }

if ! command -v clang-format >/dev/null 2>&1; then
  fail "clang-format not found"
fi

# Format check
FILES=$(git ls-files "*.cpp" "*.cc" "*.c" "*.hpp" "*.hh" "*.h")
if [[ -n "${FILES}" ]]; then
  clang-format -i ${FILES}
  if ! git diff --quiet; then
    echo "clang-format produced changes. Please commit formatted output." >&2
    git --no-pager diff
    exit 1
  fi
fi

# clang-tidy (optional but enabled if compile_commands exists)
CC_DB="$ROOT/build/debug/compile_commands.json"
if [[ -f "$CC_DB" ]]; then
  if command -v clang-tidy >/dev/null 2>&1; then
    echo "Running clang-tidy (may take a bit)..."
    # Run on a bounded set of translation units to avoid huge runtime early on.
    # Developers can expand coverage as the codebase grows.
    TUS=$(git ls-files "*.cpp" "*.cc" | head -n 50)
    for tu in $TUS; do
      clang-tidy "$tu" -p "$ROOT/build/debug" || exit 1
    done
  else
    echo "clang-tidy not found; skipping" >&2
  fi
else
  echo "compile_commands.json not found (build debug first); skipping clang-tidy" >&2
fi

echo "PASS: lint"
