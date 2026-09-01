#!/bin/bash
# 启动所有 C++ 服务（裸机二进制，需先编译好 bin/）
# 用法: sh start-all.sh
# 前提: 先跑 sh docker-start.sh 启动 MySQL/Redis，且 bin/ 下有编译产物
# 配置: 每个服务用 IM_CONFIG 环境变量加载各自的 config.local.ini（裸机配置，与容器化 config.ini 独立）

set -e

# 切到脚本所在目录（linux-server/），保证相对路径（docker/certs/、upload/）正确
cd "$(dirname "$0")"

mkdir -p logs upload

# ---------- 等待基础设施就绪（docker 重启后容器恢复需要时间） ----------
echo "等待 MySQL 就绪..."
for i in $(seq 1 30); do
  if docker exec mysql-master mysqladmin ping -uroot -p123456 2>/dev/null | grep -q alive; then
    echo "MySQL 已就绪"; break
  fi
  sleep 2
done
echo "等待 Redis 就绪..."
for i in $(seq 1 15); do
  if docker exec redis-master redis-cli -a 123456 --no-auth-warning ping 2>/dev/null | grep -q PONG; then
    echo "Redis 已就绪"; break
  fi
  sleep 1
done

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
