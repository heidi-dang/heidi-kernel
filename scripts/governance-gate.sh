#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

fail() { echo "FAIL: $*" >&2; exit 1; }

# Gate 1: .local/INDEX.md must exist
if [[ ! -f "$ROOT/.local/INDEX.md" ]]; then
  fail ".local/INDEX.md is missing"
fi
echo "PASS: .local/INDEX.md exists"

# Gate 2: At least one file under .local/worklog/ must be modified
if ! git diff --cached --name-only | grep -q '^.*\.local/worklog/'; then
  if ! git diff --name-only | grep -q '^.*\.local/worklog/'; then
    fail "No worklog entry found. Add a dated entry to .local/worklog/<your_name>.md per PR"
  fi
fi
echo "PASS: worklog entry detected"

echo "PASS: governance gate"
