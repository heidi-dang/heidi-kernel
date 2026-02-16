#!/usr/bin/env bash
# Local verification script - mirrors CI governance checks
# Run this before pushing to ensure your PR will pass CI gates

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

pass() { echo -e "${GREEN}PASS${NC}: $*"; }
fail() { echo -e "${RED}FAIL${NC}: $*" >&2; exit 1; }
warn() { echo -e "${YELLOW}WARN${NC}: $*"; }

echo "=== Local Governance Verification ==="

# Gate 1: .local must be a gitlink (submodule)
echo "Checking .local is a gitlink..."
MODE=$(git ls-tree -r HEAD | grep '.local$' | awk '{print $1}')
if [[ "$MODE" != "160000" ]]; then
  fail ".local must be a gitlink (submodule), not a regular directory. Found mode: $MODE"
fi
pass ".local is a gitlink (submodule)"

# Gate 2: Check for emojis in recent commits
echo "Checking for emojis in commits..."
FOUND=0
for commit in $(git log --format=%H -5 HEAD 2>/dev/null || echo "HEAD"); do
  MSG=$(git log -1 --format=%B "$commit" 2>/dev/null || echo "")
  if echo "$MSG" | grep -q $'[\xf0\x9f\x98\x80-\xf0\x9f\xbf\xbf]' 2>/dev/null; then
    warn "Emoji found in commit $commit"
    FOUND=1
  fi
done
if [[ "$FOUND" == "1" ]]; then
  fail "Emojis found in commits - remove them before pushing"
fi
pass "No emojis in recent commits"

# Gate 3: Worklog entry check (if running on a branch with a PR)
if [[ -n "${GITHUB_ACTOR:-}" ]] && [[ "$GITHUB_ACTOR" != "github-actions" ]]; then
  if git diff --name-only 2>/dev/null | grep -q '^.*\.local/worklog/'; then
    pass "Worklog entry detected"
  else
    warn "No worklog entry detected in changed files"
  fi
fi

echo ""
echo "=== All local checks passed ==="
