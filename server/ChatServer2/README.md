# ChatServer2 — 聊天服务器（第二实例）

## 职责
与 ChatServer1 完全相同的聊天服务。独立部署，用于：
- 水平扩展：分流更多客户端连接
- gRPC 互通：ChatServer1 和 ChatServer2 之间通过 gRPC 转发跨服消息

## 文件结构
与 ChatServer1 完全相同。详见 `../ChatServer1/README.md`。

## 与 ChatServer1 的微小差异
- `main.cpp` 启动时会将 Redis 登录计数重置为 0
- `CSession.cpp` Close() 中日志消息略有不同

## 部署
独立 exe，部署到不同机器或同一机器的不同端口。
