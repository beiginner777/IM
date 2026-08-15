@echo off
:: 启动 Nginx HTTPS 反代，加密 GateServer 的 HTTP 明文传输
:: 前置条件: GateServer 在 8080 端口运行, Docker Desktop 已启动

echo === Starting Nginx HTTPS Proxy ===
echo GateServer HTTP :8080 → Nginx HTTPS :443

docker run -d ^
  --name im-nginx-https ^
  -p 443:443 ^
  -v %~dp0nginx.conf:/etc/nginx/nginx.conf:ro ^
  -v %~dp0server.crt:/etc/nginx/certs/server.crt:ro ^
  -v %~dp0server.key:/etc/nginx/certs/server.key:ro ^
  nginx:alpine

if %errorlevel% equ 0 (
  echo === Nginx HTTPS Proxy started ===
  echo Client now uses https://localhost instead of http://localhost:8080
) else (
  echo ERROR: Failed to start Nginx. Check that Docker Desktop is running.
)

pause
