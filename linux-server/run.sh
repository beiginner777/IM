#!/bin/bash
# IM 系统一键部署脚本（docker-compose 全容器化）
# 用法: sh run.sh

set -e

# 检测 compose 命令（新版 docker compose 或旧版 docker-compose）
if docker compose version >/dev/null 2>&1; then
    COMPOSE="docker compose"
elif docker-compose version >/dev/null 2>&1; then
    COMPOSE="docker-compose"
else
    echo "错误：未安装 docker compose。"
    echo "安装: curl -fsSL https://get.docker.com | bash"
    exit 1
fi

# 进入 docker 目录
cd "$(dirname "$0")/docker"

echo "=== 1. 构建镜像并启动所有容器（$COMPOSE）==="
$COMPOSE up -d --build

echo ""
echo "=== 2. 等待服务就绪 ==="
sleep 5

echo ""
echo "=== 3. 容器状态 ==="
$COMPOSE ps

echo ""
echo "=== 4. 部署完成 ==="
echo "秒杀服务(统一入口):  curl http://localhost:8100/products"
echo "认证服务:           curl http://localhost:8080"
echo "Prometheus:         http://<公网IP>:19090"
echo "Grafana:            http://<公网IP>:3000  (admin/admin)"
echo ""
echo "查看日志:  $COMPOSE logs -f <服务名>"
