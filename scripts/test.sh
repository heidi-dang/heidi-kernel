#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

fail() { echo "FAIL: $*" >&2; exit 1; }

cmake --preset debug
cmake --build --preset debug

# Unit tests (if any)
if [[ -f "$ROOT/build/debug/CTestTestfile.cmake" ]]; then
  ctest --preset debug
else
  echo "No CTest tests configured yet; skipping" >&2
fi

echo "PASS: test"
