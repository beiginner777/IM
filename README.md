# 一、总体架构图

![](drawio/1782684161650.drawio.svg)

# 二、存储层
![](drawio/1782684256881.drawio.svg)

---

# 三、核心时序

## 3.1 消息发送 + ACK
![](drawio/1782684360406.drawio.svg)

## 3.2 抢红包 (Redis Lua 原子操作)
![](drawio/1782684406499.drawio.svg)

## 3.3 通讯录匹配 + 布隆过滤器
![](drawio/1782684446469.drawio.svg)
