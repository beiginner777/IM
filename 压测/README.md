# 压测客户端

本目录存放 IM 系统的压测客户端，覆盖 `E:\File\Obsidian\沉淀\压测技术方案.md` 里的全部 10 个场景。

## 文件清单

| 文件 | 场景 | 类型 | 用途 |
|---|---|---|---|
| `tcp_bench.cpp` | 场景 1 | C++ 客户端 | 单聊消息吞吐（TCP 长连接） |
| `reliable_upload.py` | 场景 2 | Python 客户端 | 文件分片可靠送达（20% 丢包重传） |
| `batch_insert_bench.sh` | 场景 3 | shell 脚本 | 批量写入 vs 单条写入 |
| `pay.lua` | 场景 4 | wrk Lua | 秒杀支付接口压测 |
| `buy.lua` | 场景 5 | wrk Lua | 秒杀购买接口压测 |
| `recharge.lua` | 场景 6 | wrk Lua | 充值接口（三层限流验证） |
| `sentinel_failover.sh` | 场景 7 | shell 脚本 | Redis 哨兵故障切换 |
| `upload_throughput.py` | 场景 8 | Python 客户端 | 文件传输吞吐 |
| `stability_monitor.sh` | 场景 9 | shell 脚本 | 长时稳定性监控 |
| `seckill_bench.py` | 场景 10 | Python 客户端 | 秒杀防超卖（两步流程） |

## 前置条件

1. 服务已部署（`sh run.sh`），端口映射见 `docker/docker-compose.yml`。
2. ChatServer 登录需 MySQL 有对应用户（uid 连续）。
3. JWT 密钥统一为 `im-jwt-secret-2026`。
4. 丢包模拟开关在 `ResourceServer/LogicWorker.cpp:178`（测完记得删）。

## 运行

### 场景 1：单聊吞吐（C++）

```bash
g++ -O2 -pthread tcp_bench.cpp -o tcp_bench -lboost_system -lssl -lcrypto
./tcp_bench <host> <port> <连接数> <时长秒> <起始uid> im-jwt-secret-2026 [target_uid]
# 例：100 连接压 60s
./tcp_bench 127.0.0.1 8090 100 60 1000 im-jwt-secret-2026
```

### 场景 2 / 8 / 10：Python 客户端

```bash
python3 reliable_upload.py                                      # 场景 2
python3 upload_throughput.py 127.0.0.1 9090 1000 10 im-jwt-secret-2026   # 场景 8（10MB）
python3 seckill_bench.py                                        # 场景 10
```

### 场景 3 / 7 / 9：shell 脚本

```bash
sh batch_insert_bench.sh          # 场景 3：批量 vs 单条写入（需 MySQL 客户端）
sh sentinel_failover.sh           # 场景 7：哨兵故障切换
sh stability_monitor.sh 6 600     # 场景 9：稳定性采样（6 次 × 10min）
```

### 场景 4 / 5 / 6：wrk Lua

先登录拿 token，填入对应 `.lua` 文件里的 `Bearer <token>` 占位：

```bash
wrk -t4 -c50  -d30s -s pay.lua      http://127.0.0.1:8101
wrk -t4 -c100 -d60s -s buy.lua      http://127.0.0.1:8101
wrk -t8 -c200 -d20s -s recharge.lua http://127.0.0.1:8101
```

## 端口速查

| 端口 | 服务 |
|---|---|
| 8100 | nginx 统一入口 |
| 8080 | authserver（登录拿 token） |
| 8101 | seckillserver（秒杀） |
| 8090 / 8091 | chatserver1 / chatserver2（聊天 TCP） |
| 9090 | resourceserver（文件传输 TCP） |
| 3307 / 3308 | mysql master / slave |
| 6380-6382 | redis master / slave1 / slave2 |
| 26379-26381 | sentinel 1/2/3 |
