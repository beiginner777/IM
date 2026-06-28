# common — 公共静态库

## 职责
所有 server 共享的基础代码。编译为 `common.lib`，静态链接到各 server 的 exe 中。

## 文件

| 文件 | 说明 |
|------|------|
| `SingleTon.h` | 模板单例工具类，被 `HttpManager`、`TcpMsg`、`UserManager` 等使用 |
| `ConfigManager.h/cpp` | 配置管理器（单例），读取 `config.ini`，提供各配置项的 getter |
| `RedisLocker.h/cpp` | Redis 分布式锁，基于 `SETNX` + TTL 实现 |
| `RedisManager.h/cpp` | Redis 连接池和命令封装（Get/Set/HGet/HSet/LPush/RPush/Del 等） |
| `AsioIOContextThreadPool.h/cpp` | Boost.Asio 线程池，管理多个 `io_context` 工作线程 |

## 依赖

```
common
 └──→ 各 server 的 global.h（通过 $(SolutionDir) 动态解析）
```

## 被依赖

```
ChatServer1 ──→ common.lib
ChatServer2 ──→ common.lib
GateServer  ──→ common.lib
ResourceServer ──→ common.lib
StatusServer ──→ common.lib
```

## 构建

本目录不独立编译。各 server 的 `.sln` 包含 common 项目，生成解决方案时自动先编译 common。
