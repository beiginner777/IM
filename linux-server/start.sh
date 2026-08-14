#!/bin/bash
# IM 系统一键编译 + 部署脚本
# 用法: sh start.sh

# ==================== 编译阶段 ====================
# 1. 装依赖
sh env.sh

# 2. 生成 protobuf 文件（Linux protoc 重新生成）
for dir in AuthServer StatusServer ChatServer1 ChatServer2 ResourceServer SeckillServer; do
    protoc --cpp_out=$dir --grpc_out=$dir \
        --plugin=protoc-gen-grpc=$(which grpc_cpp_plugin) \
        -I$dir $dir/message.proto
done

# 3. 编译
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
cd ..

# ==================== 启动阶段 ====================
echo "=== 编译完成，开始部署 ==="

# 4. 启动依赖（MySQL + Redis）
docker start mysql 2>/dev/null || docker run -d --name mysql -p 3307:3306 \
  -e MYSQL_ROOT_PASSWORD=123456 -e MYSQL_DATABASE=JerryChat mysql:8.0

docker start redis 2>/dev/null || docker run -d --name redis -p 6380:6379 \
  redis:7 redis-server --requirepass 123456

# 5. 等 MySQL 就绪
echo "等待 MySQL 启动..."
sleep 8

# 6. 导入初始数据（表已存在会报错，忽略）
docker exec -i mysql mysql -uroot -p123456 JerryChat < MySQL/dump.sql 2>/dev/null || true

# 7. 按顺序启动 6 个服务（后台运行，日志落盘）
mkdir -p logs

echo "启动 StatusServer（注册中心，最先）..."
nohup ./bin/StatusServer > logs/StatusServer.log 2>&1 &
sleep 2

echo "启动 AuthServer..."
nohup ./bin/AuthServer > logs/AuthServer.log 2>&1 &
sleep 1

echo "启动 ChatServer1 / ChatServer2..."
nohup ./bin/ChatServer1 > logs/ChatServer1.log 2>&1 &
nohup ./bin/ChatServer2 > logs/ChatServer2.log 2>&1 &
sleep 1

echo "启动 ResourceServer..."
nohup ./bin/ResourceServer > logs/ResourceServer.log 2>&1 &
sleep 1

echo "启动 SeckillServer..."
nohup ./bin/SeckillServer > logs/SeckillServer.log 2>&1 &

echo ""
echo "=== 全部部署完成 ==="
echo "查看进程:  ps aux | grep Server"
echo "查看日志:  tail -f logs/StatusServer.log"
echo "验证秒杀:  curl localhost:8100/products"
echo "监控指标:  curl localhost:9100/metrics"
