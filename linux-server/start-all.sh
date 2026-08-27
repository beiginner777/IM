#!/bin/bash
# 启动所有 C++ 服务（裸机二进制，需先编译好 bin/）
# 用法: sh start-all.sh
# 前提: 先跑 sh docker-start.sh 启动 MySQL/Redis，且 bin/ 下有编译产物
# 配置: 每个服务用 IM_CONFIG 环境变量加载各自的 config.local.ini（裸机配置，与容器化 config.ini 独立）

set -e

# 切到脚本所在目录（linux-server/），保证相对路径（docker/certs/、upload/）正确
cd "$(dirname "$0")"

mkdir -p logs upload

# ---------- 按顺序启动（StatusServer 注册中心最先） ----------

echo "启动 StatusServer..."
IM_CONFIG=StatusServer/config.local.ini nohup ./bin/StatusServer > logs/StatusServer.log 2>&1 &
sleep 2

echo "启动 AuthServer..."
IM_CONFIG=AuthServer/config.local.ini nohup ./bin/AuthServer > logs/AuthServer.log 2>&1 &
sleep 1

echo "启动 ChatServer1 / ChatServer2..."
IM_CONFIG=ChatServer1/config.local.ini nohup ./bin/ChatServer1 > logs/ChatServer1.log 2>&1 &
IM_CONFIG=ChatServer2/config.local.ini nohup ./bin/ChatServer2 > logs/ChatServer2.log 2>&1 &
sleep 1

echo "启动 ResourceServer..."
IM_CONFIG=ResourceServer/config.local.ini nohup ./bin/ResourceServer > logs/ResourceServer.log 2>&1 &
sleep 1

echo "启动 SeckillServer..."
IM_CONFIG=SeckillServer/config.local.ini nohup ./bin/SeckillServer > logs/SeckillServer.log 2>&1 &

echo ""
echo "=== 全部服务已启动 ==="
echo "查看进程:  ps aux | grep -E 'bin/.*Server'"
echo "查看日志:  tail -f logs/StatusServer.log"
echo "验证秒杀:  curl http://localhost:8100/products"
