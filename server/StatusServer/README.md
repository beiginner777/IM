# StatusServer — 状态服务器

## 职责
管理和监控所有 ChatServer 实例的状态。负责：
- 接收 ChatServer 的心跳上报
- 维护可用 ChatServer 列表（Redis 中）
- 供 GateServer 查询可用 ChatServer 地址
- 检测 ChatServer 超时下线

## 文件

| 文件 | 说明 |
|------|------|
| `main.cpp` | 启动入口：gRPC server（StatusServiceImpl）+ TCP acceptor + io_context |
| `global.h/cpp` | 全局定义：`MAX_RECV_LENGTH=1024`、`Server_Info` 结构体、`ChatServer` 结构体、`generate_unique_string()` |
| `CServer.h/cpp` | TCP acceptor：接受 ChatServer 的连接，超时检测 |
| `CSession.h/cpp` | TCP 连接管理：`SetServerInfo()`、`GetServerInfo()`、心跳检测 |
| `LogicSystem.h/cpp` | 业务逻辑：`registerService()`、`heartCheck()`、`storeServerInfoInRedis()` |
| `MsgNode.h/cpp` | 消息节点（协议层，使用 `short` 类型） |
| `StatusServiceImpl.h/cpp` | gRPC 服务端实现：供 GateServer 查询 ChatServer 列表 |
| `config.ini` | 配置文件 |

## 依赖

```
StatusServer
 ├── common.lib
 ├── Redis（存储 ChatServer 列表和心跳时间）
 └── gRPC（供 GateServer 查询）
```

## 与其他服务的关系

```
ChatServer1 ──TCP──→ StatusServer（心跳上报）
ChatServer2 ──TCP──→ StatusServer（心跳上报）
GateServer  ──gRPC─→ StatusServer（查询可用 ChatServer 列表）
```

## 注意
- 不使用 MySQL，只有 Redis 依赖
- 没有 `data.h`（不需要用户/好友等数据结构）
- `HEAD_TOTOL_LEN=4`，与其他 ChatServer 相同
