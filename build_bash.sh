#!/bin/bash

set -e

check_package() {
    dpkg -s "$1" >/dev/null 2>&1
}

install_if_missing() {
    local pkg="$1"

    # -----------------------------------------
    # Skip installation when running in CI
    # -----------------------------------------
    if [[ "$IS_RUNNING_CI" == "1" ]]; then
        echo "CI mode: skip installing package [$pkg]"
        return
    fi

    if check_package "$pkg"; then
        echo "✔ [$pkg] is already installed."
        return
    fi

    echo "✘ [$pkg] not found. Installing..."
    apt update -y
    apt install -y "$pkg"
}

check_to_install_databento() {
    local INSTALL_PREFIX="/usr/local"
    local CMAKE_CONFIG_PATH=""
    local REPO_URL="https://github.com/databento/databento-cpp"
    local BUILD_DIR="databento-cpp"

    # Detect databentoConfig.cmake
    for path in \
        "$INSTALL_PREFIX/lib/cmake/databento/databentoConfig.cmake" \
        "$INSTALL_PREFIX/lib64/cmake/databento/databentoConfig.cmake"
    do
        if [ -f "$path" ]; then
            CMAKE_CONFIG_PATH="$path"
            break
        fi
    done

    if [ -n "$CMAKE_CONFIG_PATH" ]; then
        echo "✔ databento already installed: $CMAKE_CONFIG_PATH"
        return 0
    fi

    echo "databento not found, installing..."

    # Clean old source if exists
    rm -rf "$BUILD_DIR"

    # Clone
    git clone "$REPO_URL"
    cd "$BUILD_DIR"

    # Configure
    cmake -S . -B build \
        -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX"

    # Build
    cmake --build build --target databento --parallel

    # Install (requires sudo)
    cmake --install build

    cd ..
    rm -rf "$BUILD_DIR"

    echo "✔ databento installed successfully"
}

# Check & install packages if missing
install_if_missing build-essential
install_if_missing git
install_if_missing cmake
install_if_missing libssl-dev
install_if_missing libsasl2-dev
install_if_missing libzstd-dev
install_if_missing libspdlog-dev

check_to_install_databento

mkdir -p build
cd build/
cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_COMPILER=g++ \
  -DCMAKE_C_COMPILER=gcc \
  -DCMAKE_CXX_FLAGS="-O3 -march=native -mtune=native -flto=8 -ffast-math -fno-plt" \
  -DCMAKE_C_FLAGS="-O3 -march=native -mtune=native -flto=8 -ffast-math -fno-plt" \
  -DENABLE_UNITY_BUILD=OFF \
  -DBUILD_ONLY="core;sts;identitystore;s3;ec2"
cmake --build . -j 6
make -j
chmod 777 order_book_server_cpp
cp order_book_server_cpp ../