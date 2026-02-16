#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

INCLUDE_SUBMODULE="auto"

for arg in "$@"; do
  case "$arg" in
    --include-submodule=auto) INCLUDE_SUBMODULE="auto" ;;
    --include-submodule=on) INCLUDE_SUBMODULE="on" ;;
    --include-submodule=off) INCLUDE_SUBMODULE="off" ;;
    *) ;;
  esac
done

fail() { echo "FAIL: $*" >&2; exit 1; }
info() { echo "INFO: $*"; }

PATTERNS=(
  'ghp_[A-Za-z0-9]{36}'
  'github_pat_[A-Za-z0-9_]{22,}'
  '-----BEGIN (OPENSSH|RSA|EC|DSA) PRIVATE KEY-----'
  'sk-[A-Za-z0-9]{20,}'
  'AKIA[0-9A-Z]{16}'
)

scan_git_repo() {
  local label="$1"
  local dir="$2"

  if ! git -C "$dir" rev-parse --is-inside-work-tree >/dev/null 2>&1; then
    fail "Not a git repo: $dir"
  fi

  info "Secret scan: $label ($dir)"
  local found=0

  while IFS= read -r -d '' f; do
    local path="$dir/$f"
    [[ -f "$path" ]] || continue

    local size
    size="$(wc -c <"$path" 2>/dev/null || echo 0)"
    if [[ "$size" -gt 2000000 ]]; then
      continue
    fi

    for re in "${PATTERNS[@]}"; do
      if LC_ALL=C grep -n -I -E "$re" "$path" >/dev/null 2>&1; then
        echo "HIT: $label: $f matches /$re/"
        LC_ALL=C grep -n -I -E "$re" "$path" | head -n 1 | sed 's/:.*$/:<redacted>/'
        found=1
      fi
    done
  done < <(git -C "$dir" ls-files -z)

  if [[ "$found" == "1" ]]; then
    fail "Secret-like patterns detected in $label. Remove/redact before pushing."
  fi
}

scan_git_repo "repo" "$ROOT"

if [[ "$INCLUDE_SUBMODULE" == "on" ]]; then
  [[ -d ".local" && -d ".local/.git" ]] || fail ".local not checked out but include-submodule=on"
  scan_git_repo ".local" "$ROOT/.local"
elif [[ "$INCLUDE_SUBMODULE" == "auto" ]]; then
  if [[ -d ".local" && -d ".local/.git" ]]; then
    scan_git_repo ".local" "$ROOT/.local"
  else
    info "Skipping .local scan (not checked out)"
  fi
else
  info "Skipping .local scan (include-submodule=off)"
fi

echo "PASS: secret scan"
