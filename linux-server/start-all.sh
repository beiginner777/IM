#!/bin/bash
# 启动所有 C++ 服务（裸机二进制，需先编译好 bin/）
# 用法: sh start-all.sh
# 前提: 先跑 sh docker-start.sh 启动 MySQL/Redis，且 bin/ 下有编译产物

set -e

mkdir -p logs

# ---------- 按顺序启动（StatusServer 注册中心最先） ----------

echo "启动 StatusServer..."
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
echo "=== 全部服务已启动 ==="
echo "查看进程:  ps aux | grep -E 'bin/.*Server'"
echo "查看日志:  tail -f logs/StatusServer.log"
echo "验证秒杀:  curl http://localhost:8100/products"
