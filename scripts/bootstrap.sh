#!/bin/bash
set -e

echo "=== heidi-kernel bootstrap ==="

if [ -d ".git" ]; then
    echo "Updating submodules..."
    git submodule update --init --recursive
else
    echo "Not a git repository"
    exit 1
fi

echo ""
echo "=== Next steps ==="
echo "1. Build: cmake --preset debug && cmake --build --preset debug"
echo "2. Run kernel: ./build/debug/heidi-kernel"
echo "3. Query status: printf 'STATUS\\n' | socat - UNIX-CONNECT:/tmp/heidi-kernel.sock"
echo "4. Run dashboard: ./build/debug/heidi-dashd"
echo "5. Open: http://127.0.0.1:7778"
