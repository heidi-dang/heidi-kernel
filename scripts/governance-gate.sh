#!/usr/bin/env bash
# Governance gate - validates .local is a submodule (gitlink)
# This runs in CI without fetching submodule contents

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

fail() { echo "FAIL: $*" >&2; exit 1; }

# Gate 1: .local must be a gitlink (submodule)
MODE=$(git ls-tree -q --mode HEAD .local 2>/dev/null | awk '{print $1}')
if [[ "$MODE" != "160000" ]]; then
  fail ".local must be a gitlink (submodule), not a regular directory. Found mode: $MODE"
fi
echo "PASS: .local is a gitlink (submodule)"

echo "PASS: governance gate"
