@echo off
:: 启动带 AOF 持久化的 Redis（消息队列专用）
:: 使用场景: ChatServer 全挂了也不丢队列中的消息

title Redis 6380 (AOF persistent)

echo === Starting Redis with AOF persistence on port 6380 ===
echo     Strategy: AOF everysec (max 1s data loss)
echo     Auto-rewrite: 100%% growth, min 64MB

:: 创建数据目录
if not exist "%~dp0data" mkdir "%~dp0data"

:: 启动 Redis（使用本目录的配置文件）
redis-server "%~dp0redis_queue.conf" --port 6380 --requirepass 123456

pause
