# 压测客户端

本目录存放 IM 系统的压测客户端，覆盖 `E:\File\Obsidian\压测方案.md` 里的全部 10 个场景。

## 文件清单

| 文件 | 场景 | 类型 | 用途 |
|---|---|---|---|
| `tcp_bench.cpp` | 场景 1 | C++ 客户端 | 单聊消息吞吐（TCP 长连接） |
| `reliable_upload.py` | 场景 2 | Python 客户端 | 文件分片可靠送达（20% 丢包重传） |
| `batch_insert_bench.sh` | 场景 3 | shell 脚本 | 批量写入 vs 单条写入 |
| `recharge_bench.py` | 场景 6 | Python 客户端 | 充值接口三层限流验证 |
| `sentinel_failover.sh` | 场景 7 | shell 脚本 | Redis 哨兵故障切换 |
| `upload_throughput.py` | 场景 8 | Python 客户端 | 文件传输吞吐 |
| `stability_monitor.sh` | 场景 9 | shell 脚本 | 长时稳定性监控 |
| `seckill_bench.py` | 场景 10 | Python 客户端 | 秒杀防超卖（两步流程） |
| `create_users.py` | 辅助 | Python 脚本 | 预置压测用户（先跑这个） |

## 前置条件

1. 服务已部署（`sh run.sh`），端口映射见 `docker/docker-compose.yml`。
2. ChatServer 登录需 MySQL 有对应用户（uid 连续）。
3. JWT 密钥统一为 `im-jwt-secret-2026`。
4. 丢包模拟开关在 `ResourceServer/LogicWorker.cpp:178`（测完记得删）。

## 运行

### 0. 先创建压测用户（必做）

ChatServer 登录、秒杀登录都要查 MySQL `user` 表，且密码是 bcrypt(cost=10)。先预置用户：

```bash
pip install pymysql bcrypt   # 一次性装依赖

python3 create_users.py 200 1000 123456   # 场景 1：uid 1000~1199（配合 tcp_bench 起始 uid）
python3 create_users.py 3 1 123456        # 场景 10：uid 1/2/3（配合 seckill_bench 的 UIDS=[1,2,3]）
```

> ⚠️ **布隆过滤器**：AuthServer 启动时会从 MySQL 构建用户名布隆过滤器缓存到 Redis（`bloom:user_search`）。如果服务已启动、后补的用户，布隆过滤器不含新用户，登录会被误判「用户不存在」。两种解法：
> 1. **推荐**：先建用户，再 `sh run.sh` 启动整套服务。
> 2. 已启动的话：清 Redis 布隆键 + 重启 authserver：
>    ```bash
>    docker compose exec redis-master redis-cli -a 123456 DEL bloom:user_search
>    docker compose restart authserver
>    ```

### 场景 1：单聊吞吐（C++）

```bash
g++ -O2 -pthread tcp_bench.cpp -o tcp_bench -lboost_system -lssl -lcrypto
./tcp_bench <host> <port> <连接数> <时长秒> <起始uid> im-jwt-secret-2026 [target_uid]
# 例：100 连接压 60s
./tcp_bench 127.0.0.1 8090 100 60 1000 im-jwt-secret-2026
```

### 场景 2 / 6 / 8 / 10：Python 客户端

```bash
python3 reliable_upload.py                                      # 场景 2
python3 recharge_bench.py 20 30 test1,test2,test3               # 场景 6（三层限流验证）
python3 upload_throughput.py 127.0.0.1 9090 1000 10 im-jwt-secret-2026   # 场景 8（10MB）
python3 seckill_bench.py                                        # 场景 10
```

### 场景 3 / 7 / 9：shell 脚本

```bash
sh batch_insert_bench.sh          # 场景 3：批量 vs 单条写入（需 MySQL 客户端）
sh sentinel_failover.sh           # 场景 7：哨兵故障切换
sh stability_monitor.sh 6 600     # 场景 9：稳定性采样（6 次 × 10min）
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
