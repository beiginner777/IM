# 一、总体架构图


![](drawio/1782684161650.drawio.svg)

---

# 二、各服务架构图

## 2.1 GateServer
![1782687112256.drawio](drawio/1782687112256.drawio.svg)

---

## 2.2 StatusServer
![1782687211724.drawio](drawio/1782687211724.drawio.svg)

---

## 2.3 ChatServer

![1782687278041.drawio](drawio/1782687278041.drawio.svg)

--- 

## 2.4 ResourceServer

![1782687310607.drawio](drawio/1782687310607.drawio.svg)

---

# 三、存储层

![](drawio/1782684256881.drawio.svg)

---

# 四、核心时序

## 4.1 消息发送 + ACK
![](drawio/1782684360406.drawio.svg)

---

## 4.2 抢红包 (Redis Lua 原子操作)
![](drawio/1782684406499.drawio.svg)

---

## 4.3 通讯录匹配 + 布隆过滤器
![](drawio/1782684446469.drawio.svg)

---

## 4.4 分布式限流（令牌桶）

![](drawio/1782688803442.drawio.svg)

---

## 4.5 消息入库（批量写入 + 异步重试）

![](drawio/1782689043975.drawio.svg)

---

## 4.6 用户登录 + 服务发现

![](drawio/1782689075187.drawio.svg)

---

## 4.7 服务注册与心跳摘除

![](drawio/1782689115635.drawio.svg)

---

## 4.8 文件分片上传 + 断点续传

![](drawio/1782689152458.drawio.svg)

---

## 4.9 Client→ChatServer（一致性哈希路由 + 节点扩缩容）

![1782690636108.drawio](drawio/1782690636108.drawio.svg)

---

## 4.10 ChatServer→ResourceServer（一致性哈希文件路由）

![1782690658441.drawio](drawio/1782690658441.drawio.svg)

---

## 4.11 MySQL 分表（一致性哈希分片 + 扩容）

![1782690690788.drawio](drawio/1782690690788.drawio.svg)
