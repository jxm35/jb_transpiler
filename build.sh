#!/bin/bash

set -e

TEST_FILE=${1:-"tests/examples/test.jb"}

if [ ! -f "$TEST_FILE" ]; then
    echo "Error: Test file '$TEST_FILE' not found."
    echo "Usage: $0 <test-file>"
    exit 1
fi

mkdir -p build

cd build
cmake ..
cmake --build .

echo "Running file '$TEST_FILE'..."
./transpiler "../$TEST_FILE" -o ../output
cd ..

echo "----------   Generated Code   ----------"
cat output.c
echo "----------   Generated Code   ----------"

./output