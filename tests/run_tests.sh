#!/bin/bash
# Run all kcomp tests

set -e

cd "$(dirname "$0")/.."

SRCS="src/models/ppm.cpp src/models/bwt.cpp src/models/lz77.cpp src/models/lzopt.cpp \
      src/models/lzx.cpp src/models/cm.cpp src/models/dict.cpp src/models/lzma.cpp \
      src/models/mixer.cpp src/models/model257.cpp src/models/rle.cpp \
      src/core/range_coder.cpp src/io/file_io.cpp"

build_test() {
    local name=$1
    local src=$2
    if [ ! -f "build/$name" ] || [ "$src" -nt "build/$name" ]; then
        echo "Building $name..."
        g++ -std=c++17 -O2 -I. "$src" $SRCS -o "build/$name"
    fi
}

echo "Building kcomp..."
make build > /dev/null 2>&1

echo ""
echo "Running shell tests..."
./test.sh

echo ""
echo "=== C++ Unit Tests ==="

echo ""
echo "Running roundtrip tests..."
build_test test_roundtrip tests/test_roundtrip.cpp
./build/test_roundtrip

echo ""
echo "Running BWT bug regression tests..."
g++ -std=c++17 -O2 -I. tests/test_bwt_bug.cpp src/models/bwt.cpp -o build/test_bwt_bug 2>/dev/null || true
./build/test_bwt_bug

echo ""
echo "Running compression mode tests..."
build_test test_modes tests/test_modes.cpp
./build/test_modes

echo ""
echo "Running edge case tests..."
build_test test_edge_cases tests/test_edge_cases.cpp
./build/test_edge_cases

echo ""
echo "Running transform tests..."
build_test test_transforms tests/test_transforms.cpp
./build/test_transforms

echo ""
echo "Running stress tests..."
build_test test_stress tests/test_stress.cpp
./build/test_stress

echo ""
echo "Running file format tests..."
build_test test_file_formats tests/test_file_formats.cpp
./build/test_file_formats

echo ""
echo "Running corruption detection tests..."
build_test test_corruption tests/test_corruption.cpp
./build/test_corruption

echo ""
echo "Running performance tests..."
build_test test_performance tests/test_performance.cpp
./build/test_performance

echo ""
echo "=========================================="
echo "All tests completed successfully!"
echo "=========================================="
