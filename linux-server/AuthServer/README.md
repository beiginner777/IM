# AuthServer — 认证服务器

## 职责

客户端的 HTTP 入口。负责：
- 用户注册、登录校验（HTTP API）
- 获取验证码
- 签发 JWT token
- 将登录成功的客户端重定向到对应的 ChatServer / SeckillServer

## 文件

| 文件 | 说明 |
|------|------|
| `main.cpp` | 启动入口：初始化 io_context，启动 AuthServer |
| `global.h` | 全局定义：`MAX_RECV_LENGTH=1024`、`HEART_CHECK_INTERVAL=60`、HTTP 相关 REQUEST_ID |
| `data.h` | 数据结构：`UserInfo`、`ApplyInfo`（精简版） |
| `AuthServer.h/cpp` | 认证服务主逻辑：HTTP acceptor，创建 `HttpConnection` 处理请求 |
| `HttpConnection.h/cpp` | HTTP 连接处理：解析请求、路由到 LogicSystem |
| `LogicSystem.h/cpp` | URL 路由分发：`getHandles_` / `postHandles_` 映射，处理注册/登录/验证码请求 |
| `MysqlDao.h/cpp` | 数据库访问：`registerUser`、`userLogin` |
| `MysqlManager.h/cpp` | MySQL 连接池 |
| `StatusGrpcClient.h/cpp` | gRPC 客户端：向 StatusServer 查询 ChatServer 列表 |
| `VerifyGrpcClient.h` | gRPC 客户端：向 VerifyServer 发送验证码 |

## 依赖

```
AuthServer
 ├── common.lib
 ├── MySQL（用户注册、登录校验）
 ├── Redis（验证码缓存、JWT token）
 └── gRPC（调用 StatusServer、VerifyServer）
```

## 与其他服务的关系

```
Client ──HTTP──→ Nginx (:8100) ──→ AuthServer (:8080)（注册/登录）
                                       │
                                       ├──gRPC──→ StatusServer（获取可用 ChatServer 列表）
                                       │
                                       └──返回 ChatServer/SeckillServer 地址 + JWT token 给 Client
                                                │
                                                └──→ Client 直连 ChatServer（TCP）
