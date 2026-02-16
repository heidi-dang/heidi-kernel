#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT="$ROOT/.local/dashboard/snapshot.json"

branch="$(git -C "$ROOT" rev-parse --abbrev-ref HEAD 2>/dev/null || echo unknown)"
commit="$(git -C "$ROOT" rev-parse --short HEAD 2>/dev/null || echo unknown)"
dirty="false"
if [ -n "$(git -C "$ROOT" status --porcelain 2>/dev/null || true)" ]; then dirty="true"; fi

mkdir -p "$(dirname "$OUT")"
cat >"$OUT" <<JSON
{
  "branch": "$branch",
  "commit": "$commit",
  "dirty": $dirty,
  "ci": "see GitHub Actions"
}
JSON

echo "Wrote $OUT"
