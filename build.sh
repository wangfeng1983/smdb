#!/bin/bash
# Unix/Linux build script for MemoryDB

set -e

echo "===================================="
echo "MemoryDB Build Script"
echo "===================================="
echo ""

# Create build directory
mkdir -p build
cd build

# Configure with CMake
echo "Configuring with CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release "$@"

# Build
echo ""
echo "Building..."
cmake --build . --config Release -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

echo ""
echo "===================================="
echo "Build completed successfully!"
echo "Binaries are in: build/bin/"
echo "===================================="

cd ..
