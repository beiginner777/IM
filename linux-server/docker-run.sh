#!/bin/bash
# Docker 基础设施启动脚本（MySQL + Redis + 可选监控）
# 用法: sh docker-run.sh

echo "=== 启动 Docker 基础设施 ==="

# ---------- 1. MySQL ----------
echo "启动 MySQL 容器..."
docker start mysql 2>/dev/null || docker run -d --name mysql \
  -p 3307:3306 \
  -e MYSQL_ROOT_PASSWORD=123456 \
  -e MYSQL_DATABASE=JerryChat \
  mysql:8.0

# ---------- 2. Redis ----------
echo "启动 Redis 容器..."
docker start redis 2>/dev/null || docker run -d --name redis \
  -p 6380:6379 \
  redis:7 redis-server --requirepass 123456

# ---------- 3. 等 MySQL 就绪（healthcheck 轮询，比 sleep 可靠） ----------
echo "等待 MySQL 就绪..."
for i in $(seq 1 30); do
    if docker exec mysql mysqladmin ping -uroot -p123456 2>/dev/null | grep -q "alive"; then
        echo "MySQL 已就绪"
        break
    fi
    echo "  MySQL 启动中... ($i/30)"
    sleep 2
done

# ---------- 4. 导入初始数据（表已存在会报错，忽略） ----------
echo "导入初始数据..."
docker exec -i mysql mysql -uroot -p123456 JerryChat < MySQL/dump.sql 2>/dev/null || true

# ---------- 5. Nginx 统一 HTTP 入口（host 网络，直接监听 8100） ----------
docker start nginx 2>/dev/null || docker run -d --name nginx \
  --network host \
  -v $(pwd)/docker/nginx.conf:/etc/nginx/conf.d/default.conf:ro \
  nginx:alpine

# ---------- 6. 可选：Prometheus + Grafana 监控 ----------
# 取消注释以启用
# docker start prometheus 2>/dev/null || docker run -d --name prometheus \
#   -p 9090:9090 \
#   -v $(pwd)/docker/prometheus.yml:/etc/prometheus/prometheus.yml:ro \
#   prom/prometheus
#
# docker start grafana 2>/dev/null || docker run -d --name grafana \
#   -p 3000:3000 \
#   -e GF_SECURITY_ADMIN_PASSWORD=admin \
#   grafana/grafana

# ---------- 7. 状态 ----------
echo ""
echo "=== Docker 容器状态 ==="
docker ps --format "table {{.Names}}\t{{.Status}}\t{{.Ports}}"
