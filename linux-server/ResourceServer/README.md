# ResourceServer — 资源服务器

## 职责
文件/图片的上传和下载。负责：
- 头像上传/下载
- 聊天图片传输
- 文件上传/下载（支持断点续传）
- 文件分片管理

## 文件

| 文件 | 说明 |
|------|------|
| `main.cpp` | 启动入口：io_context + CServer + AsioIOContextThreadPool |
| `global.h` | 全局定义：`HEAD_TOTOL_LEN=6`、`MAX_RECV_LENGTH=4096`、`MAX_SENDQUEUE_SIZE=200000`、文件相关 REQUEST_ID |
| `data.h/cpp` | 数据结构：`UserInfo`（精简版） |
| `CServer.h/cpp` | TCP acceptor（精简版，无 StatusServer 连接、无定时器） |
| `CSession.h/cpp` | TCP 连接管理（使用 `int` 协议头，与其他 server 的 `short` 不同） |
| `LogicSystem.h/cpp` | 业务逻辑分发：将请求按 UUID hash 路由到 `LogicWorker` |
| `LogicWorker.h/cpp` | 工作线程：处理文件操作请求 |
| `MsgNode.h/cpp` | 消息节点（协议层，使用 `int` 类型） |
| `FileSystem.h/cpp` | 文件系统抽象（单例）：管理文件读写 |
| `FileWorker.h/cpp` | 文件上传工作线程 |
| `DownloadWorker.h/cpp` | 文件下载工作线程 |
| `MysqlDao.h/cpp` | 数据库访问：`getUserByUid`、`updateUserIcon`、`GetFriendList` |
| `MysqlManager.h/cpp` | MySQL 连接池 |
| `ResouceServerClient.h/cpp` | gRPC 客户端：与其他服务通信 |
| `config.ini` | 配置文件：`[Server] Port=9090` 等 |

## 依赖

```
ResourceServer
 ├── common.lib
 ├── MySQL（用户头像信息）
 ├── Redis（文件传输状态、离线消息队列）
 └── gRPC（与其他服务通信）
```

## 与其他服务的关系

```
Client ──TCP──→ ChatServer（发送文件消息）
                   │
                   └──→ ResourceServer（上传文件）
                            │
                            └──→ 返回文件 URL 给 ChatServer
                                      │
                                      └──→ Client 从 ResourceServer 下载
```

## 注意
- 协议编码与 ChatServer 不同：`HEAD_DATA_LEN=4`（int），其他 server 使用 `HEAD_DATA_LEN=2`（short）。MsgNode 因此未提取到 common。
- 端口默认为 9090（其他 server 为 8080）。
