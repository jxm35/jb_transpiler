#!/bin/bash
set -e

echo "Building and running JBLang tests..."

mkdir -p build
cd build

cmake ..
cmake --build .

if [ -f "tests" ]; then
    echo "Running unit tests..."
    ./tests
    echo "Tests completed"
else
    echo "Test executable not found"
    exit 1
fi