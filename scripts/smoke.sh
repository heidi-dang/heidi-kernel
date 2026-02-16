#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "=== Building heidi-kernel ==="
cmake --preset debug
cmake --build --preset debug

echo ""
echo "=== Running heidi-kernel (will stop after 2 seconds) ==="
timeout 2 ./build/debug/heidi-kernel || true

echo ""
echo "=== Checking version flag ==="
./build/debug/heidi-kernel --version

echo ""
echo "=== Checking help flag ==="
./build/debug/heidi-kernel --help

echo ""
echo "=== Smoke test complete ==="
