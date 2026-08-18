#!/bin/bash
# IM 系统一键部署脚本（docker-compose 全容器化）
# 用法: sh run.sh
#
# 部署内容：
#   - 所有业务容器（nginx/auth/chat/resource/seckill/status + prometheus/grafana）
#   - Redis 主从 + 哨兵（slave 启动命令带 --slaveof，自动建主从）
#   - MySQL 主从（本脚本自动 mysqldump 同步 + CHANGE MASTER TO + START SLAVE）

set -e

# ============ Docker 环境初始化 ============
sudo apt update
sudo apt install -y docker.io

# 配置 Docker 镜像加速（registry-mirrors）
sudo mkdir -p /etc/docker
sudo tee /etc/docker/daemon.json > /dev/null <<'EOF'
{
  "builder": {
    "gc": {
      "defaultKeepStorage": "20GB",
      "enabled": true
    }
  },
  "experimental": false,
  "registry-mirrors": [
    "https://docker.m.daocloud.io",
    "https://docker.1panel.live",
    "https://hub.rat.dev"
  ]
}
EOF

sudo systemctl restart docker

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

# 等待 MySQL 容器就绪（mysqladmin ping 直到 alive，最多 90s）
wait_mysql() {
    local container="$1"
    for i in $(seq 1 45); do
        if $COMPOSE exec -T "$container" mysqladmin ping -uroot -p123456 --silent 2>/dev/null; then
            echo "$container 已就绪"
            return 0
        fi
        echo "  等待 $container 就绪... ($i/45)"
        sleep 2
    done
    echo "错误：$container 未在 90s 内就绪" >&2
    return 1
}

echo "=== 1. 构建镜像并启动所有容器（$COMPOSE）==="
$COMPOSE up -d --build

echo ""
echo "=== 2. 等待 MySQL 就绪 ==="
wait_mysql mysql-master
wait_mysql mysql-slave

echo ""
echo "=== 3. 配置 MySQL 主从复制 ==="
# 3.1 master 创建复制账号（IF NOT EXISTS 保证可重复执行）
$COMPOSE exec -T mysql-master mysql -uroot -p123456 -e \
    "CREATE USER IF NOT EXISTS 'repl'@'%' IDENTIFIED BY '123456'; \
     GRANT REPLICATION SLAVE ON *.* TO 'repl'@'%'; \
     FLUSH PRIVILEGES;"

# 3.2 从 master 导出 JerryChat 库：
#     --single-transaction  一致性快照（InnoDB，不锁表）
#     --master-data=2       在 dump 注释里记录 binlog 位置
#     --set-gtid-purged=OFF 避免 GTID 干扰（本项目未开 GTID）
$COMPOSE exec -T mysql-master mysqldump -uroot -p123456 \
    --databases JerryChat --single-transaction --master-data=2 --set-gtid-purged=OFF \
    > /tmp/im_master_dump.sql

# 3.3 从 dump 注释里提取 binlog 位置（--master-data=2 生成的行）
MASTER_LOG_FILE=$(grep -oP "MASTER_LOG_FILE='\K[^']+" /tmp/im_master_dump.sql | head -1)
MASTER_LOG_POS=$(grep -oP "MASTER_LOG_POS=\K[0-9]+" /tmp/im_master_dump.sql | head -1)
if [ -z "$MASTER_LOG_FILE" ] || [ -z "$MASTER_LOG_POS" ]; then
    echo "错误：未能从 dump 提取 binlog 位置" >&2
    exit 1
fi
echo "master binlog: $MASTER_LOG_FILE @ $MASTER_LOG_POS"

# 3.4 导入 slave（--databases 会生成 CREATE DATABASE + USE，slave 无需预建库）
$COMPOSE exec -T mysql-slave mysql -uroot -p123456 < /tmp/im_master_dump.sql

# 3.5 清除旧复制配置并建立主从关系（幂等，可重复执行）
$COMPOSE exec -T mysql-slave mysql -uroot -p123456 -e \
    "STOP SLAVE; RESET SLAVE ALL; \
     CHANGE MASTER TO \
       MASTER_HOST='mysql-master', \
       MASTER_USER='repl', \
       MASTER_PASSWORD='123456', \
       MASTER_LOG_FILE='$MASTER_LOG_FILE', \
       MASTER_LOG_POS=$MASTER_LOG_POS; \
     START SLAVE;"

# 3.6 验证复制状态
echo ""
echo "MySQL 主从复制状态："
$COMPOSE exec -T mysql-slave mysql -uroot -p123456 -e "SHOW SLAVE STATUS\G" \
    | grep -E "Slave_IO_Running|Slave_SQL_Running|Seconds_Behind_Master" || true

echo ""
echo "=== 4. 容器状态 ==="
$COMPOSE ps

echo ""
echo "=== 5. 部署完成 ==="
echo "秒杀服务(统一入口):  curl http://localhost:8100/products"
echo "认证服务:           curl http://localhost:8080"
echo "Prometheus:         http://<公网IP>:19090"
echo "Grafana:            http://<公网IP>:3000  (admin/admin)"
echo ""
echo "验证主从:  docker compose exec mysql-slave mysql -uroot -p123456 -e 'SHOW SLAVE STATUS\\G'"
echo "查看日志:  $COMPOSE logs -f <服务名>"
