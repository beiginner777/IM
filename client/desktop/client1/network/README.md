# network — 网络通信层

## 职责
所有与服务器的网络交互，与 UI 完全解耦。包括 HTTP 短连接（登录/注册 API）和 TCP 长连接（实时聊天消息）。

## 文件

| 文件 | 说明 |
|------|------|
| `httpmanager.h/cpp` | HTTP 请求管理器（单例）。负责向 GateServer 发送 POST 请求：获取验证码、注册用户、登录校验。结果通过信号异步返回 |
| `tcpmsg.h/cpp` | **TCP 消息核心**（单例）。与 ChatServer 维持长连接，处理消息收发、编解码、回调分发。注册了 20+ 种消息类型的回调函数。支持断线重连、心跳检测 |
| `tcpthread.h/cpp` | 网络线程管理。启动 3 个独立 QThread：TCP 聊天线程 (`TcpMsg`)、文件传输线程 (`FileUploadMsg`)、本地数据库线程 (`LoadLocalData`)，确保网络 IO 不阻塞 UI |

## 消息流

```
UI (dialogs/)                          Server
    │                                     │
    ├─→ signalSendData ──→ TcpMsg ──→ TCP Socket
    │                                     │
    ├─← signalSwitchToChat ←── ID_CHAT_LOGIN_RSP
    ├─← signalReceiveNewTextMsg ←─ ID_NOTIFY_TEXT_CHAT_MSG_REQ
    ├─← signalSearchUser ←─ ID_SEARCH_USER_RSP
    ├─← signalNotifyOffLine ←─ ID_NOTIFY_OFFLINE
    └─← ...（共 20+ 种消息信号）
```

## 依赖

```
network
 ├──→ core/global.h, core/userdata.h
 ├──→ dialogs/contactuserwidget.h（tcpmsg.cpp 解析数据后更新联系人列表）
 └──→ data/usermanager.h, data/fileuploadmsg.h, data/loadlocaldata.h
```

## 被谁使用

```
core/main.cpp  →  创建 TcpThread 实例，启动所有网络线程
dialogs/       →  登录/注册/聊天等页面通过 TcpMsg::signalSendData 发送消息
data/          →  接收到的数据通过 LoadLocalData 持久化到本地
```
