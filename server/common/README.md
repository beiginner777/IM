# common — 公共静态库

## 职责
所有 server 共享的基础代码。编译为 `common.lib`，静态链接到各 server 的 exe 中。

## 文件

| 文件 | 说明 |
|------|------|
| `SingleTon.h` | 模板单例工具类 |
| `ConfigManager.h/cpp` | 配置管理器（单例），读取 `config.ini` |
| `RedisLocker.h/cpp` | Redis 分布式锁，基于 `SETNX` + TTL |
| `RedisManager.h/cpp` | Redis 连接池 + Sentinel 发现 + 分布式 ID 生成 |
| `AsioIOContextThreadPool.h/cpp` | Boost.Asio 线程池 |
| `Defer.h` | RAII 延迟执行器（Go `defer` 的 C++ 实现） |
| `MessageDeduplicator.h/cpp` | 消息去重器（Redis 后端，TTL 自动过期） |
| `crypto/BCryptHasher.h/cpp` | bcrypt 密码哈希（OpenBSD 参考实现） |
| `crypto/bcrypt_impl.c/h` | bcrypt 核心算法（EksBlowfish） |
| `tests/` | Google Test 单元测试和集成测试 |

## 关键特性

### RedisManager
- Sentinel 感知：启动时查询 Sentinel 发现 Master 地址
- 分布式 ID：`generateMsgId()` — Redis `INCR` + `WAIT`（主路径），Snowflake 降级（备路径）
- 连接池：8 个长连接，自动健康检查和重连
- 命令封装：Get/Set/Expire/LPush/RPush/LPop/RPop/HGet/HSet/Del/Exists

### MessageDeduplicator
- Redis `msg:dedup:{uuid}` → TTL 300s 自动过期
- `isDuplicate()` / `cacheResult()` / `getCachedAck()` / `remove()`

### 测试（tests/）
| 文件 | 说明 |
|------|------|
| `MessageDeduplicator_test.cpp` | 去重器 20 个 gtest 用例（单元 + 异常 + 压测） |
| `TestEnvironment.cpp` | gtest 全局初始化（Redis 连接） |
| `BatchInsertBenchmark.cpp` | MySQL 批量 vs 单条 INSERT 压测 |
| `gtest_msgdedup.vcxproj` | gtest 项目文件 |
| `benchmark.vcxproj` | benchmark 项目文件 |
| `config.ini` | 测试用 Redis/MySQL 配置 |

## 依赖

```
common
 ├── hiredis (Redis C 客户端)
 ├── OpenSSL (bcrypt 底层)
 ├── Boost (filesystem, property_tree)
 └──→ 各 server 的 global.h
```

## 构建

本目录不独立编译。各 server 的 `.sln` 包含 common 项目。
