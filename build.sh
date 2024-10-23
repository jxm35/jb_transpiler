#!/bin/bash

# Exit on any error
set -e

# Create build directory if it doesn't exist
mkdir -p build

# Generate build files
cd build
cmake ..

# Build the project
cmake --build .

# Run a test if provided
if [ -f "../test/test.jb" ]; then
    echo "Running test file..."
    ./transpiler ../test/test.jb > ../output.c
    cd ..
    echo "----------   Generated Code   ----------"
    cat output.c
    echo "----------   Generated Code   ----------"
    gcc output.c -o test_program
    ./test_program
fi