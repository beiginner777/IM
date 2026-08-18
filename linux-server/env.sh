#!/bin/bash
# WSL Ubuntu 24.04 环境搭建脚本
# 用法: chmod +x setup_wsl.sh && sudo ./setup_wsl.sh

set -e

echo "=== 1. 安装编译工具链 ==="
apt-get update
apt-get install -y build-essential cmake g++ pkg-config

echo "=== 2. 安装依赖库 ==="
apt-get install -y \
    libboost-all-dev \
    libssl-dev \
    libjsoncpp-dev \
    libprotobuf-dev protobuf-compiler \
    libgrpc++-dev protobuf-compiler-grpc \
    libmysqlcppconn-dev \
    libhiredis-dev \
    libspdlog-dev \
    libfmt-dev

echo "=== 3. 验证关键库 ==="
echo -n "Boost:    "; dpkg -s libboost-system-dev | grep Version
echo -n "OpenSSL:  "; openssl version
echo -n "Protobuf: "; protoc --version
echo -n "gRPC:     "; dpkg -s libgrpc++-dev | grep Version
echo -n "gRPC插件: "; which grpc_cpp_plugin
echo -n "JsonCpp:  "; dpkg -s libjsoncpp-dev | grep Version
echo -n "MySQL:    "; dpkg -s libmysqlcppconn-dev | grep Version
echo -n "hiredis:  "; dpkg -s libhiredis-dev | grep Version
echo -n "spdlog:   "; dpkg -s libspdlog-dev | grep Version
echo -n "fmt:      "; dpkg -s libfmt-dev | grep Version

echo "=== 4. 安装依赖的python包 ==="
sudo apt install python3-pymysql -y

echo "=== 5. 环境安装成功 ==="

