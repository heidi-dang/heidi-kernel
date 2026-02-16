#!/usr/bin/env bash
# Format check - runs clang-format on all C++ source files
# Exit 0 if files are formatted correctly, exit 1 if formatting needed

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"

cd "$ROOT_DIR"

echo "=== Running clang-format check ==="

# Find C++ source files
CPP_FILES=$(find . -type f \( -name "*.cpp" -o -name "*.hpp" -o -name "*.h" -o -name "*.cc" -o -name "*.cxx" \) -not -path "./build/*" -not -path "./.git/*")

if [[ -z "$CPP_FILES" ]]; then
  echo "No C++ source files found."
  exit 0
fi

# Check formatting
FORMATTED=$(clang-format --style=file "$CPP_FILES" 2>/dev/null || echo "")
UNFORMATTED=false

for file in $CPP_FILES; do
  ORIGINAL=$(cat "$file")
  CHECKED=$(clang-format --style=file "$file" 2>/dev/null || echo "$ORIGINAL")
  if [[ "$ORIGINAL" != "$CHECKED" ]]; then
    echo "Formatting needed: $file"
    UNFORMATTED=true
  fi
done

if [[ "$UNFORMATTED" == "true" ]]; then
  echo ""
  echo "FAIL: Some files need formatting. Run:"
  echo "  clang-format -i --style=file <files>"
  exit 1
fi

echo "PASS: All files formatted correctly"
exit 0
