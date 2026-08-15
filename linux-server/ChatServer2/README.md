# ChatServer2 — 聊天服务器（第二实例）

## 职责
与 ChatServer1 完全相同的聊天服务。独立部署，用于：
- 水平扩展：分流更多客户端连接
- gRPC 互通：ChatServer1 和 ChatServer2 之间通过 gRPC 转发跨服消息

## 文件结构
与 ChatServer1 完全相同。详见 `../ChatServer1/README.md`。

## 与 ChatServer1 的差异
- `main.cpp` 启动时将 Redis 登录计数重置为 0
- `config.ini` 中 `SelfServer/Name = ChatServer2`，`SelfServer/Port = 8091`
- `BatchMessageWriter` 通过共享 Redis 队列与 ChatServer1 协同消费

## 部署
独立 exe，部署到不同机器或同一机器的不同端口。
