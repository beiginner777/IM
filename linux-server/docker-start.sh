#!/bin/bash
# Docker 基础设施启动脚本 —— 裸机部署（C++ 服务跑宿主机，Docker 只跑基础设施）
# 用法: sh docker-start.sh（在 linux-server/ 目录下执行）
# 启动: MySQL 主从(2) + Redis 主从(3) + Redis 哨兵(3) + Nginx(1)
# 注: 复用 docker-compose.yml 的配置文件和端口映射

set -e

# ---------- 网络：容器间通过服务名（redis-master 等）互访 ----------
docker network create im-net 2>/dev/null || true

# ==================== MySQL 主从 ====================
echo "=== 启动 MySQL 主从 ==="

# master (宿主机 3307) —— 挂 master.cnf + dump.sql 首次初始化
docker start mysql-master 2>/dev/null || docker run -d --name mysql-master --network im-net \
  -p 3307:3306 \
  -e MYSQL_ROOT_PASSWORD=123456 \
  -e MYSQL_DATABASE=JerryChat \
  -v $(pwd)/MySQL/master.cnf:/etc/mysql/conf.d/master.cnf:ro \
  -v $(pwd)/MySQL/dump.sql:/docker-entrypoint-initdb.d/dump.sql:ro \
  mysql:8.0 --default-authentication-plugin=mysql_native_password

# slave (宿主机 3308) —— 只挂 slave.cnf，不挂 dump.sql（数据靠 binlog 复制，避免"No database selected"报错）
docker start mysql-slave 2>/dev/null || docker run -d --name mysql-slave --network im-net \
  -p 3308:3306 \
  -e MYSQL_ROOT_PASSWORD=123456 \
  -v $(pwd)/MySQL/slave.cnf:/etc/mysql/conf.d/slave.cnf:ro \
  mysql:8.0 --default-authentication-plugin=mysql_native_password

# ==================== Redis 主从 ====================
echo "=== 启动 Redis 主从 ==="

docker start redis-master 2>/dev/null || docker run -d --name redis-master --network im-net \
  -p 6380:6379 \
  -v $(pwd)/Redis/sentinel/redis-master.conf:/usr/local/etc/redis/redis.conf:ro \
  redis:7-alpine redis-server /usr/local/etc/redis/redis.conf

docker start redis-slave-1 2>/dev/null || docker run -d --name redis-slave-1 --network im-net \
  -p 6381:6379 \
  -v $(pwd)/Redis/sentinel/redis-slave.conf:/usr/local/etc/redis/redis.conf:ro \
  redis:7-alpine redis-server /usr/local/etc/redis/redis.conf --slaveof redis-master 6379 --masterauth 123456

docker start redis-slave-2 2>/dev/null || docker run -d --name redis-slave-2 --network im-net \
  -p 6382:6379 \
  -v $(pwd)/Redis/sentinel/redis-slave.conf:/usr/local/etc/redis/redis.conf:ro \
  redis:7-alpine redis-server /usr/local/etc/redis/redis.conf --slaveof redis-master 6379 --masterauth 123456

# ==================== Redis 哨兵 ====================
echo "=== 启动 Redis 哨兵 ==="

docker start sentinel-1 2>/dev/null || docker run -d --name sentinel-1 --network im-net \
  -p 26379:26379 \
  -v $(pwd)/Redis/sentinel/sentinel-1.conf:/usr/local/etc/redis/sentinel.conf \
  redis:7-alpine redis-sentinel /usr/local/etc/redis/sentinel.conf

docker start sentinel-2 2>/dev/null || docker run -d --name sentinel-2 --network im-net \
  -p 26380:26379 \
  -v $(pwd)/Redis/sentinel/sentinel-2.conf:/usr/local/etc/redis/sentinel.conf \
  redis:7-alpine redis-sentinel /usr/local/etc/redis/sentinel.conf

docker start sentinel-3 2>/dev/null || docker run -d --name sentinel-3 --network im-net \
  -p 26381:26379 \
  -v $(pwd)/Redis/sentinel/sentinel-3.conf:/usr/local/etc/redis/sentinel.conf \
  redis:7-alpine redis-sentinel /usr/local/etc/redis/sentinel.conf

# ==================== Nginx（host 网络，代理宿主机 C++ 服务） ====================
echo "=== 启动 Nginx ==="

docker start nginx 2>/dev/null || docker run -d --name nginx \
  --network host \
  -v $(pwd)/docker/nginx.conf:/etc/nginx/conf.d/default.conf:ro \
  nginx:alpine

# ==================== 等待就绪 ====================
echo "=== 等待 MySQL master 就绪 ==="
for i in $(seq 1 30); do
  if docker exec mysql-master mysqladmin ping -uroot -p123456 2>/dev/null | grep -q alive; then
    echo "MySQL master 已就绪"; break
  fi
  echo "  MySQL 启动中... ($i/30)"; sleep 2
done

echo "=== 等待 Redis master 就绪 ==="
for i in $(seq 1 15); do
  if docker exec redis-master redis-cli -a 123456 ping 2>/dev/null | grep -q PONG; then
    echo "Redis master 已就绪"; break
  fi
  echo "  Redis 启动中... ($i/15)"; sleep 1
done

echo ""
echo "=== 容器状态 ==="
docker ps --format "table {{.Names}}\t{{.Status}}\t{{.Ports}}"

echo ""
echo "=== 提示 ==="
echo "MySQL 主从复制需手动 CHANGE MASTER TO（脚本只铺好 server-id/log-bin/read-only 配置）"
echo "验证哨兵:  docker exec sentinel-1 redis-cli -p 26379 SENTINEL master mymaster"
echo "验证主从:  docker exec redis-master redis-cli -a 123456 INFO replication | grep connected_slaves"
echo "改动配置后需先 docker rm 旧容器再重跑本脚本（docker start 不会应用新配置）"
