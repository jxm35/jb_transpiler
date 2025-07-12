#!/bin/bash

set -e

TEST_FILE=${1:-"tests/examples/test.jb"}
ALLOCATOR=${2:-"reference_count"}
DEBUG_FLAG=""

for arg in "$@"; do
    if [ "$arg" = "--debug" ]; then
        DEBUG_FLAG="--debug"
        break
    fi
done

if [ ! -f "$TEST_FILE" ]; then
    echo "Error: Test file '$TEST_FILE' not found."
    echo "Usage: $0 <test-file> [allocator-type] [--debug]"
    echo "Allocator types: simple, reference_count, mark_sweep"
    exit 1
fi

echo "Using allocator: $ALLOCATOR"
if [ -n "$DEBUG_FLAG" ]; then
    echo "Debug mode enabled"
fi

mkdir -p build

cd build
cmake ..
cmake --build .

echo "Running file '$TEST_FILE' with $ALLOCATOR allocator..."
./transpiler "../$TEST_FILE" -o ../output -a "$ALLOCATOR" $DEBUG_FLAG
cd ..

echo "----------   Generated Code   ----------"
cat output.c
echo "----------   Generated Code   ----------"

./output