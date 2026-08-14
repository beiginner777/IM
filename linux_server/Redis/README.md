# Redis 配置

## 文件

| 文件 | 说明 |
|------|------|
| `redis_queue.conf` | 消息队列专用 Redis（AOF everysec 持久化） |
| `start_redis_persistent.bat` | 启动持久化 Redis |
| `sentinel/` | Docker Compose 哨兵集群（1M+2S+3 Sentinel） |

## 哨兵集群

详见 `sentinel/README.md`。

当前 ChatServer 通过 `config.ini` 中的 `SentinelHost`/`SentinelPort` 自动发现 Master。
