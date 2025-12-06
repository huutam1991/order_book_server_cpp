#!/bin/bash

# Check & install cmake if missing
if ! dpkg -s cmake >/dev/null 2>&1; then
    echo "cmake not found. Installing..."
    apt update -y
    apt install -y cmake
else
    echo "Check dependency: "
    echo " * [cmake] is already installed."
fi

# Check & install OpenSSL dev package
if ! dpkg -s libssl-dev >/dev/null 2>&1; then
    echo "libssl-dev not found. Installing..."
    apt update -y
    apt install -y libssl-dev libsasl2-dev
else
    echo "Check dependency: "
    echo " * [libssl-dev] is already installed."
fi

# Check & install spdlog if missing
if ! dpkg -s libspdlog-dev >/dev/null 2>&1; then
    echo "libspdlog-dev not found. Installing..."
    apt update -y
    apt install -y libspdlog-dev
else
    echo "Check dependency: "
    echo " * [libspdlog-dev] is already installed."
fi

mkdir build
cd build/
cmake .. -DCMAKE_CXX_COMPILER=g++ -DCMAKE_C_COMPILER=gcc -DENABLE_UNITY_BUILD=OFF -DBUILD_ONLY="core;sts;identitystore;s3;ec2"
cmake --build . -j 6
make -j
chmod 777 order_book_server_cpp
cp order_book_server_cpp ../