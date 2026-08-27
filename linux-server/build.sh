#!/bin/bash
# 编译 C++ 服务（裸机部署）
# 用法: sh build.sh
# 前提: 已跑 sudo sh docker/setup_wsl.sh 装好依赖

set -e

echo "=== 1. 生成 protobuf 文件 ==="
for dir in AuthServer StatusServer ChatServer1 ChatServer2 ResourceServer SeckillServer; do
    protoc --cpp_out=$dir --grpc_out=$dir \
        --plugin=protoc-gen-grpc=$(which grpc_cpp_plugin) \
        -I$dir $dir/message.proto
done

echo "=== 2. 编译 ==="
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
cd ..

echo ""
echo "=== 编译完成，二进制在 bin/ 目录 ==="
ls bin/ 2>/dev/null || echo "（未见 bin/，检查编译是否成功）"
