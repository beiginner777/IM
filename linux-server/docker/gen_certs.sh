#!/bin/bash
# gen_certs.sh —— 生成自签 CA + 服务器证书（TLS 全链路加密用）
#
# 用法: ./gen_certs.sh [公网IP]
#   例: ./gen_certs.sh 1.2.3.4    把公网IP加入证书 SAN（客户端通过公网IP访问时需要）
#       ./gen_certs.sh            不填则只覆盖 localhost / 127.0.0.1 / docker 容器名
#
# 前置: openssl (>= 1.1.1，支持 -addext)
# 产物: ./certs/{ca.key,ca.crt,server.key,server.crt}
# 注意: 私钥（*.key）只在服务器本地生成，不入 git，不随镜像分发
set -euo pipefail

CERT_DIR="$(cd "$(dirname "$0")" && pwd)/certs"
mkdir -p "$CERT_DIR"
cd "$CERT_DIR"

SERVER_IP="${1:-}"
DAYS=3650   # 有效期 10 年（自签学习/演示用，正式环境应缩短）

echo "==> [1/3] 生成 CA 根证书"
openssl genrsa -out ca.key 2048
openssl req -x509 -new -nodes -key ca.key -sha256 -days "$DAYS" \
    -subj "/C=CN/O=IM-System/CN=IM-CA" \
    -out ca.crt

echo "==> [2/3] 生成服务器私钥 + 证书请求"
openssl genrsa -out server.key 2048
openssl req -new -key server.key \
    -subj "/C=CN/O=IM-System/CN=im-server" \
    -out server.csr

# SAN：本机 + 所有 docker 服务容器名（gRPC 服务间用容器名互连）+ 公网IP（可选）
SAN="DNS:localhost,DNS:nginx,DNS:authserver,DNS:seckillserver,DNS:chatserver1,DNS:chatserver2,DNS:resourceserver,DNS:statusserver,IP:127.0.0.1"
if [ -n "$SERVER_IP" ]; then
    SAN="$SAN,IP:$SERVER_IP"
fi

echo "==> [3/3] 用 CA 签发服务器证书"
openssl x509 -req -in server.csr -CA ca.crt -CAkey ca.key -CAcreateserial \
    -out server.crt -days "$DAYS" -sha256 \
    -addext "subjectAltName=$SAN"

rm -f server.csr ca.srl

echo ""
echo "✅ 证书生成完毕: $CERT_DIR"
echo "   ca.crt       CA 根证书（客户端 / 服务间 gRPC 校验服务端用）"
echo "   ca.key       CA 私钥（仅签发证书时用，部署不需要）"
echo "   server.crt   服务器证书（各服务 TLS 用）"
echo "   server.key   服务器私钥（各服务 TLS 用）"
echo "   SAN: $SAN"
echo ""
if [ -z "$SERVER_IP" ]; then
    echo "⚠️  未指定公网IP，证书只覆盖 localhost/127.0.0.1/容器名。"
    echo "   若客户端通过公网IP访问，请重新执行: ./gen_certs.sh <公网IP>"
fi
