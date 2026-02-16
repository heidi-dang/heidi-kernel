#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

fail() { echo "FAIL: $*" >&2; exit 1; }
info() { echo "INFO: $*"; }

if ! git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
  fail "Not inside a git repo"
fi

MODE="$(git ls-tree HEAD .local 2>/dev/null | awk '{print $1}' || true)"
if [[ "$MODE" != "160000" ]]; then
  fail ".local must be a gitlink submodule (mode 160000). Found: '${MODE:-empty}'"
fi

info "Initializing submodules..."
git submodule sync --recursive
git submodule update --init --recursive

if [[ ! -f ".local/INDEX.md" ]]; then
  fail "Submodule initialized but .local/INDEX.md not found. Check submodule repo content."
fi

echo ""
echo "NEXT:"
echo "  1) Read: .local/INDEX.md"
echo "  2) Rules: .local/RULES.md + .local/RULES_CPP.md"
echo ""
echo "Build (when code exists):"
echo "  cmake --preset debug"
echo "  cmake --build --preset debug"
