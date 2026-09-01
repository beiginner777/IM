#!/bin/bash
# IM 系统裸机一键部署脚本
# 从零部署：装 Docker → 装依赖 → 生成证书 → 起基础设施 → 编译 → 起服务
# 用法: sh deploy.sh [公网IP]   （公网IP 加入证书 SAN，客户端通过公网 IP 访问时需要）
# 幂等：已安装 / 已生成 / 已编译的步骤会自动跳过

set -e

cd "$(dirname "$0")"

echo "=========================================="
echo "  IM 系统裸机一键部署"
echo "=========================================="

# 0. 清理 dpkg 锁（防系统自动更新 unattended-upgrades 阻塞 apt）
echo ""
echo "=== [0/6] 清理 dpkg 锁 ==="
pkill -f unattended-upgr 2>/dev/null || true
sleep 3

# 1. 安装 Docker
echo ""
echo "=== [1/6] 安装 Docker ==="
if command -v docker >/dev/null 2>&1; then
    echo "Docker 已安装: $(docker --version)"
else
    sh setup_docker.sh
fi

# 2. 安装编译依赖
echo ""
echo "=== [2/6] 安装编译依赖 ==="
if command -v cmake >/dev/null 2>&1; then
    echo "编译依赖已安装: $(cmake --version | head -1)"
else
    sh docker/setup_wsl.sh
    # setup_wsl.sh 缺 libcrypt-dev（CMakeLists 需要 crypt 库）
    apt-get install -y libcrypt-dev
fi

# 3. 生成 TLS 证书
echo ""
echo "=== [3/6] 生成 TLS 证书 ==="
if [ -f docker/certs/server.crt ]; then
    echo "证书已存在，跳过"
else
    (cd docker && bash gen_certs.sh "${1:-}")
fi

# 4. 启动基础设施（MySQL 主从 + Redis 哨兵 + Nginx）
echo ""
echo "=== [4/6] 启动基础设施 ==="
sh docker-start.sh

# 5. 编译 C++ 服务
echo ""
echo "=== [5/6] 编译 C++ 服务 ==="
if [ -f bin/StatusServer ] && [ -f bin/ChatServer1 ] && [ -f bin/ResourceServer ]; then
    echo "二进制已存在，跳过编译（如需重新编译: sh build.sh）"
else
    sh build.sh
fi

# 6. 启动所有服务
echo ""
echo "=== [6/6] 启动所有服务 ==="
sh start-all.sh

echo ""
echo "=========================================="
echo "  部署完成！"
echo "  验证秒杀:  curl http://localhost:8100/products"
echo "  查看日志:  tail -f logs/StatusServer.log"
echo "  查看进程:  ps aux | grep bin/"
echo "=========================================="
