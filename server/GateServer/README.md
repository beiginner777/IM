# GateServer — 网关服务器

## 职责
客户端的 HTTP 入口。负责：
- 用户注册、登录校验（HTTP API）
- 获取验证码
- 将登录成功的客户端重定向到对应的 ChatServer

## 文件

| 文件 | 说明 |
|------|------|
| `main.cpp` | 启动入口：初始化 io_context，启动 GateServer |
| `global.h` | 全局定义：`MAX_RECV_LENGTH=1024`、`HEART_CHECK_INTERVAL=60`、HTTP 相关 REQUEST_ID |
| `data.h` | 数据结构：`UserInfo`、`ApplyInfo`（精简版） |
| `GateServer.h/cpp` | 网关主逻辑：HTTP acceptor，创建 `HttpConnection` 处理请求 |
| `HttpConnection.h/cpp` | HTTP 连接处理：解析请求、路由到 LogicSystem |
| `LogicSystem.h/cpp` | URL 路由分发：`getHandles_` / `postHandles_` 映射，处理注册/登录/验证码请求 |
| `MysqlDao.h/cpp` | 数据库访问：`registerUser`、`userLogin` |
| `MysqlManager.h/cpp` | MySQL 连接池 |
| `StatusGrpcClient.h/cpp` | gRPC 客户端：向 StatusServer 查询 ChatServer 列表 |
| `VerifyGrpcClient.h` | gRPC 客户端：向 VerifyServer 发送验证码 |

## 依赖

```
GateServer
 ├── common.lib
 ├── MySQL（用户注册、登录校验）
 ├── Redis（验证码缓存、session）
 └── gRPC（调用 StatusServer、VerifyServer）
```

## 与其他服务的关系

```
Client ──HTTP──→ GateServer（注册/登录）
                   │
                   ├──gRPC──→ StatusServer（获取可用 ChatServer 列表）
                   │
                   └──返回 ChatServer 地址给 Client
                            │
                            └──→ Client 直连 ChatServer（TCP）
```
