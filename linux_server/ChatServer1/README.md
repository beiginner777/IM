# ChatServer1 — 聊天服务器

## 职责
即时通讯的核心服务。负责：
- 维持与所有在线客户端的 TCP 长连接
- 消息路由（私聊消息转发）
- 好友管理（搜索、申请、通过/拒绝）
- 向 StatusServer 上报心跳和在线状态
- 通过 gRPC 与其他 ChatServer 实例通信

## 文件

| 文件 | 说明 |
|------|------|
| `main.cpp` | 启动入口：初始化 TCP acceptor + gRPC server + BatchMessageWriter |
| `global.h` | 全局定义：协议常量、REQUEST_ID 枚举、错误码 |
| `data.h` | 数据结构：`UserInfo`、`ChatMessage`、`ChatThreadInfo` 等 |
| `CServer.h/cpp` | TCP acceptor：接受连接、创建 `CSession`、心跳上报 |
| `CSession.h/cpp` | TCP 连接管理：收发、编解码、心跳、断线 |
| `LogicSystem.h/cpp` | 业务逻辑：20+ 种消息类型回调，异步处理 |
| `MsgNode.h/cpp` | 消息节点（协议层）：RecvNode / SendNode |
| **`BatchMessageWriter.h/cpp`** | **异步批量写入器（Redis 队列 → MySQL）** |
| `ChatServiceImpl.h/cpp` | gRPC 服务端 |
| `ChatGrpcClient.h/cpp` | gRPC 客户端 |
| `UserManager.h/cpp` | 用户状态管理 |
| `MysqlDao.h/cpp` | 数据库访问层（批量 INSERT） |
| `MysqlManager.h/cpp` | MySQL 连接池 |
| `utils.h/cpp` | 工具函数 |

## 关键特性

### 消息 ID 生成
- Redis `INCR msg_id_counter` + `WAIT 1 100`（全局严格递增）
- Snowflake 降级：`timestamp(42) + server_id(5) + sequence(17)`（Redis 不可用时自动切换）

### 异步批量写入
```
Client → ChatServer → generateMsgId() → enqueue Redis → ACK (1ms)
                                              ↓
                              BatchMessageWriter::flushWorker()
                                  100ms/100条 → 批量 INSERT MySQL
                                  失败 → 备份队列（最多重试3次） → 死信队列
                                  恢复线程每10s重试
```

| 指标 | 单条 INSERT | 批量 INSERT | 提升 |
|------|:--:|:--:|:--:|
| 吞吐 | 677 msg/s | 8726 msg/s | 12.9x |
| 延迟 | 1478 us | 115 us | 12.9x |

### 配置（config.ini）
```ini
[Redis]
Host = 127.0.0.1
Port = 6380
Password = 123456
SentinelHost = 127.0.0.1       # 哨兵地址（可选）
SentinelPort = 26379,26380,26381
```

## 依赖

```
ChatServer1
 ├── common.lib
 ├── MySQL（用户数据、好友关系、聊天记录）
 ├── Redis（在线状态、消息队列、去重缓存、ID 生成）
 └── gRPC（与其他 ChatServer 通信）
```

## 与其他服务的关系

```
Client ──TCP──→ ChatServer1 ──gRPC──→ ChatServer2
                   │
                   └──TCP──→ StatusServer（心跳上报）
```
