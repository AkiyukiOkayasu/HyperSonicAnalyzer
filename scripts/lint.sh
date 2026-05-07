#!/bin/bash

# Lint and format script for HyperSonicAnalyzer
# Run this after making changes to check and fix issues

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

cd "$PROJECT_DIR"

echo "=== Running clang-format ==="
for file in Source/*.cpp Source/*.h; do
    if [ -f "$file" ]; then
        clang-format -i "$file"
        echo "Formatted: $file"
    fi
done

echo ""
echo "=== Building with strict warnings ==="
if [ -d "build" ]; then
    cd build
    # Force rebuild of our source files to see warnings
    touch ../Source/*.cpp ../Source/*.h 2>/dev/null || true
    cmake --build . --config Release 2>&1 | grep -E "Source/.*:(warning|error)" || echo "No warnings or errors in Source files."
else
    echo "Build directory not found. Run:"
    echo "  mkdir build && cd build && cmake .. -DCMAKE_BUILD_TYPE=Release"
fi
