# Minimum CMake version required
cmake_minimum_required(VERSION 3.10)

# Project name and language
project(MyCProject C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

file(GLOB SOURCES "*.c")

include_directories(${CMAKE_SOURCE_DIR})

# Add the executable target
add_executable(Runtime ${SOURCES})
