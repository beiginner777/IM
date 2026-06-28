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
| `main.cpp` | 启动入口：初始化 TCP acceptor + gRPC server，注册信号处理 |
| `global.h` | 全局定义：协议常量（`HEAD_TOTOL_LEN=4`、`MAX_RECV_LENGTH=4096`）、全部 REQUEST_ID 枚举、错误码、数据结构声明 |
| `data.h` | 共享数据结构：`UserInfo`、`ApplyInfo`、`ChatThreadInfo`、`ChatMessage`、`ChatDataBase`、`TextChatData`、`ImageDataBase` |
| `CServer.h/cpp` | TCP acceptor：接受连接、创建 `CSession`、超时检测、连接 StatusServer 并心跳上报 |
| `CSession.h/cpp` | 单条 TCP 连接管理：消息收发、编解码、心跳检测、断线处理 |
| `LogicSystem.h/cpp` | 业务逻辑分发：注册 20+ 种消息类型的回调函数，通过线程池异步处理 |
| `MsgNode.h/cpp` | 消息节点（协议层）：`RecvNode` 接收缓冲区、`SendNode` 发送队列 |
| `ChatServiceImpl.h/cpp` | gRPC 服务端实现：处理其他 ChatServer 发来的请求 |
| `ChatGrpcClient.h/cpp` | gRPC 客户端：向其他 ChatServer 发送通知（好友申请、消息转发等） |
| `UserManager.h/cpp` | 用户状态管理（单例）：在线用户 session 映射、好友列表缓存 |
| `MysqlDao.h/cpp` | 数据库访问层：用户注册、好友关系、聊天记录的 CRUD |
| `MysqlManager.h/cpp` | MySQL 连接池封装 |
| `utils.h/cpp` | 工具函数 |

## 依赖

```
ChatServer1
 ├── common.lib（SingleTon, ConfigManager, RedisManager, RedisLocker, AsioIOContextThreadPool）
 ├── MySQL（用户数据、好友关系、聊天记录）
 ├── Redis（在线状态、消息缓存）
 └── gRPC（与其他 ChatServer 通信）
```

## 与其他服务的关系

```
Client ──TCP──→ ChatServer1 ──gRPC──→ ChatServer2（跨服消息）
                   │
                   └──TCP──→ StatusServer（心跳上报）
```
