#!/bin/bash

set -e

check_package() {
    dpkg -s "$1" >/dev/null 2>&1
}

install_if_missing() {
    local pkg="$1"

    if check_package "$pkg"; then
        echo " * [$pkg] is already installed."
        return
    fi

    echo " * [$pkg] not found. Installing..."
    apt update -y
    apt install -y "$pkg"
}

# Check & install packages if missing
install_if_missing build-essential
install_if_missing git
install_if_missing cmake
install_if_missing libssl-dev
install_if_missing libsasl2-dev
install_if_missing libzstd-dev
install_if_missing libspdlog-dev

mkdir -p build
cd build/
cmake .. -DCMAKE_CXX_COMPILER=g++ -DCMAKE_C_COMPILER=gcc -DENABLE_UNITY_BUILD=OFF -DBUILD_ONLY="core;sts;identitystore;s3;ec2"
cmake --build . -j 6
make -j
chmod 777 order_book_server_cpp
cp order_book_server_cpp ../