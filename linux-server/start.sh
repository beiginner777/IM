# 1. 装依赖
sh env.sh

# 2. 生成 protobuf 文件（Linux protoc 重新生成）
for dir in AuthServer StatusServer ChatServer1 ChatServer2 ResourceServer SeckillServer; do
    protoc --cpp_out=$dir --grpc_out=$dir \
        --plugin=protoc-gen-grpc=$(which grpc_cpp_plugin) \
        -I$dir $dir/message.proto
done
 
# 3. 编译
mkdir build && cd build

# 4. 构建项目
cmake .. -DCMAKE_BUILD_TYPE=Release

# 5.编译项目
make -j$(nproc)


