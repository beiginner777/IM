# 一、总体架构图

![](drawio/1782684161650.drawio.svg)

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

## 4.2 抢红包 (Redis Lua 原子操作)
![](drawio/1782684406499.drawio.svg)

## 4.3 通讯录匹配 + 布隆过滤器
![](drawio/1782684446469.drawio.svg)
