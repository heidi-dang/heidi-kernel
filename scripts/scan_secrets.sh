#!/usr/bin/env bash
# Secret scan - checks for leaked tokens/keys in tracked files
# Exits 0 if no secrets found, exits 1 if secrets detected

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"

cd "$ROOT_DIR"

echo "=== Scanning for secrets ==="

# Common secret patterns (best-effort)
PATTERNS=(
  "ghp_[a-zA-Z0-9]{36}"
  "github_pat_[a-zA-Z0-9_]{22,}"
  "[a-zA-Z0-9_-]*_TOKEN"
  "[a-zA-Z0-9_-]*_KEY"
  "AKIA[0-9A-Z]{16}"
  "sk-[a-zA-Z0-9]{20,}"
  "xox[baprs]-[0-9a-zA-Z]{10,}"
)

# Files to exclude from scanning
EXCLUDE_PATTERNS=(
  ".git/"
  "build/"
  ".local/"
  "docs/examples/"
)

EXCLUDE_ARGS=""
for pattern in "${EXCLUDE_PATTERNS[@]}"; do
  EXCLUDE_ARGS="$EXCLUDE_ARGS --exclude='*$pattern*'"
done

FOUND=0

for pattern in "${PATTERNS[@]}"; do
  # Use git grep for efficiency
  RESULT=$(git grep -l -E "$pattern" -- ':!*' 2>/dev/null || true)
  if [[ -n "$RESULT" ]]; then
    echo "FAIL: Potential secret pattern '$pattern' found in:"
    echo "$RESULT"
    FOUND=1
  fi
done

if [[ "$FOUND" == "1" ]]; then
  echo ""
  echo "FAIL: Secrets detected. Review and remove them before committing."
  exit 1
fi

echo "PASS: No obvious secrets detected"
exit 0
