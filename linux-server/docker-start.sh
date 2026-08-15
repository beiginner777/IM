#!/bin/bash
# Docker 基础设施启动脚本（MySQL + Redis + Nginx）
# 用法: sh docker-start.sh

echo "=== 启动 Docker 基础设施 ==="

# ---------- MySQL ----------
echo "启动 MySQL 容器..."
docker start mysql 2>/dev/null || docker run -d --name mysql \
  -p 3307:3306 \
  -e MYSQL_ROOT_PASSWORD=123456 \
  -e MYSQL_DATABASE=JerryChat \
  mysql:8.0

# ---------- Redis ----------
echo "启动 Redis 容器..."
docker start redis 2>/dev/null || docker run -d --name redis \
  -p 6380:6379 \
  redis:7 redis-server --requirepass 123456

# ---------- 等 MySQL 就绪 ----------
echo "等待 MySQL 就绪..."
for i in $(seq 1 30); do
    if docker exec mysql mysqladmin ping -uroot -p123456 2>/dev/null | grep -q "alive"; then
        echo "MySQL 已就绪"
        break
    fi
    echo "  MySQL 启动中... ($i/30)"
    sleep 2
done

# ---------- 导入初始数据 ----------
echo "导入初始数据..."
docker exec -i mysql mysql -uroot -p123456 JerryChat < MySQL/dump.sql 2>/dev/null || true

# ---------- Nginx（host 网络，代理宿主机 C++ 服务） ----------
echo "启动 Nginx 容器..."
docker start nginx 2>/dev/null || docker run -d --name nginx \
  --network host \
  -v $(pwd)/docker/nginx.conf:/etc/nginx/conf.d/default.conf:ro \
  nginx:alpine

echo ""
echo "=== Docker 容器状态 ==="
docker ps --format "table {{.Names}}\t{{.Status}}\t{{.Ports}}"
