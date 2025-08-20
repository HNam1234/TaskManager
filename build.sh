#!/bin/bash
set -e

BUILD_DIR="build"
BUILD_TYPE="Release"

mkdir -p "$BUILD_DIR"

# Conan install -> generate conan_toolchain.cmake + deps
conan install . \
    --output-folder="$BUILD_DIR" \
    --build=missing \
    -s build_type=$BUILD_TYPE \
    -g CMakeDeps -g CMakeToolchain

# Configure cmake
cmake -B "$BUILD_DIR" -S . \
    -DCMAKE_TOOLCHAIN_FILE="$BUILD_DIR/conan_toolchain.cmake" \
    -DCMAKE_BUILD_TYPE=$BUILD_TYPE

# Build
cmake --build "$BUILD_DIR"
