#!/bin/bash
# Docker 环境初始化脚本（安装 Docker + 配置镜像加速）
# 用法: sh setup_docker.sh   （首次部署前执行一次）

set -e

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

echo "Docker 环境初始化完成"
