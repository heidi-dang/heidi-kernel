#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "=== Building heidi-kernel (Debug) ==="
cmake --preset debug
cmake --build --preset debug

echo "=== Build complete ==="
