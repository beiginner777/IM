# Redis Sentinel 集群

## 架构

```
                  ┌─ Sentinel-1 (:26379) ─┐
                  ├─ Sentinel-2 (:26380) ─┤  监控 Master，quorum=2
                  └─ Sentinel-3 (:26381) ─┘
                           │
              Master 宕机 → 选举 Slave 提升
                           │
          ┌────────────────┼────────────────┐
          ▼                ▼                ▼
    Redis Master     Redis Slave-1    Redis Slave-2
    (:6380)          (:6381)          (:6382)
```

## 文件

| 文件 | 说明 |
|------|------|
| `docker-compose.yml` | 6 容器编排（1M + 2S + 3 Sentinel） |
| `redis-master.conf` | Master: AOF everysec + requirepass |
| `redis-slave.conf` | Slave: masterauth + read-only no |
| `sentinel.conf` | Sentinel 模板（监控 mymaster，30s 超时） |
| `start.bat` | 一键启动 |
| `data/` | Docker 卷挂载（持久化） |

## 启动

```bash
# 前置：Docker Desktop 已启动

cd server/Redis/sentinel
docker-compose up -d

# 验证
redis-cli -p 26379 SENTINEL master mymaster
redis-cli -p 6380 -a 123456 INFO replication
```

## 故障切换

```bash
# 停掉 Master
docker stop redis-master

# 观察 Sentinel 日志（约 30s 后触发）
docker logs sentinel-1 -f
# +sdown → +odown → +switch-master

# 验证新 Master
redis-cli -p 26379 SENTINEL get-master-addr-by-name mymaster
```

## 端口映射

| 容器 | 内部 | 宿主机 | 用途 |
|------|:--:|:--:|------|
| redis-master | 6379 | 6380 | 写入 |
| redis-slave-1 | 6379 | 6381 | 只读 / 故障切换候选 |
| redis-slave-2 | 6379 | 6382 | 只读 / 故障切换候选 |
| sentinel-1 | 26379 | 26379 | 哨兵 |
| sentinel-2 | 26379 | 26380 | 哨兵 |
| sentinel-3 | 26379 | 26381 | 哨兵 |
