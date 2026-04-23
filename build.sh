#!/bin/bash
# build.sh — Build MapCompletionTracker.dll
# Run from the project root: ./build.sh

set -e

PROJECT_ROOT="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"

echo "==> Cleaning build directory..."
rm -rf "$BUILD_DIR"

echo "==> Configuring with CMake..."
cmake -B "$BUILD_DIR" \
    -DCMAKE_TOOLCHAIN_FILE="$PROJECT_ROOT/mingw-toolchain.cmake" \
    -DCMAKE_BUILD_TYPE=Release

echo "==> Building..."
cmake --build "$BUILD_DIR"

echo ""
echo "==> Done! Output: $BUILD_DIR/MapCompletionTracker.dll"
