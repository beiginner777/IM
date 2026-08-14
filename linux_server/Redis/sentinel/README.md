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

## 集群创建流程

### 第一步：写配置文件（6 个角色，4 种配置）

```
redis-master.conf       Master 专属：appendonly yes, requirepass 123456
redis-slave.conf        Slave 共用：masterauth 123456, slave-read-only no
sentinel.conf           Sentinel 共用模板
sentinel-1/2/3.conf     从模板复制，RT 启动后每个 Sentinel 各自 CONFIG REWRITE
```

### 第二步：docker-compose up -d → 6 个容器启动

```
启动顺序（由 depends_on 控制）:
  ① redis-master      最先启动，作为初始 Master
  ② redis-slave-1     redis-slave-2     等 Master 就绪后启动，--slaveof redis-master 6379
  ③ sentinel-1        sentinel-2  sentinel-3  所有 Redis 就绪后启动
```

### 第三步：各角色自动互相发现

```
Slave 发现 Master:
  docker-compose.yml 启动命令中指定 --slaveof redis-master 6379
  Slave 主动 CONNECT Master + AUTH + PSYNC 全量同步

Sentinel 发现 Master:
  sentinel.conf 中写死 sentinel monitor mymaster redis-master 6379 2
  Sentinel 启动后主动 CONNECT Master 监控

Sentinel 发现 Slave:
  Sentinel 向 Master 发 INFO replication
  从 connected_slaves 列表拿到所有 Slave 的 IP:Port
  再逐个 CONNECT Slave 监控

Sentinel 互发现:
  所有 Sentinel 每 2s 向 __sentinel__:hello 频道 PUBLISH 自己的名片
  同时 SUBSCRIBE 该频道，收到其他 Sentinel 的名片后互相认识
```

### Sentinel 启动日志验证

```
1:X +monitor master mymaster 172.18.0.2 6379        ← 配置文件找到 Master
1:X * +slave slave 172.18.0.3:6379 ... @ mymaster   ← INFO 发现 Slave-1
1:X * +slave slave 172.18.0.5:6379 ... @ mymaster   ← INFO 发现 Slave-2
1:X * +sentinel sentinel a5523f...                   ← __sentinel__:hello 收到 Sentinel-2
1:X * +sentinel sentinel 0d4b9c...                   ← __sentinel__:hello 收到 Sentinel-3
```

> 此时 3 个 Sentinel 全部互相认识，Master 已知，所有 Slave 已知，集群就绪。

---

## 四种发现机制总结

| 角色 | 发现方式 | 命令 / 机制 |
|------|---------|------------|
| Slave → Master | 启动时强行指定 | `--slaveof redis-master 6379` |
| Sentinel → Master | 配置文件写死 | `sentinel monitor mymaster redis-master 6379 2` |
| Sentinel → Slave | 问 Master 的户口本 | `INFO replication` → `connected_slaves` |
| Sentinel → Sentinel | 公共聊天室广播 | `PUBLISH` / `SUBSCRIBE __sentinel__:hello`（每 2s） |

---

## 文件

| 文件 | 说明 |
|------|------|
| `docker-compose.yml` | 6 容器编排（1M + 2S + 3 Sentinel） |
| `redis-master.conf` | Master: AOF everysec + requirepass |
| `redis-slave.conf` | Slave: masterauth + read-only no |
| `sentinel.conf` | Sentinel 模板（监控 mymaster，30s 超时） |
| `sentinel-1.conf` ~ `sentinel-3.conf` | 各 Sentinel 独立运行时配置 |
| `start.bat` | 一键启动 |
| `data/` | Docker 卷挂载（持久化，gitignore） |

## 启动

```bash
# 前置：Docker Desktop 已启动

cd server/Redis/sentinel
docker-compose up -d

# 验证集群状态
redis-cli -p 26379 SENTINEL master mymaster      # Sentinel 视角
redis-cli -p 6380 -a 123456 INFO replication      # Master 复制状态
redis-cli -p 6381 -a 123456 INFO replication      # Slave 复制状态
```

## 故障切换

```bash
# 停掉 Master
docker stop redis-master

# 观察 Sentinel 日志（约 30s 后触发）
docker logs sentinel-1 -f

# 日志时间线:
# +sdown master mymaster 172.18.0.2 6379          ← 主观下线（30s 无响应）
# +odown master mymaster 172.18.0.2 6379 #quorum 3/2  ← 客观下线（投票通过）
# +switch-master mymaster 172.18.0.2 → 172.18.0.5    ← 提升 Slave 为新 Master

# 验证新 Master
redis-cli -p 26379 SENTINEL get-master-addr-by-name mymaster
# 旧 Master 重启后自动变成 Slave
docker start redis-master
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
