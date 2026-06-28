# data — 业务逻辑与持久化层

## 职责
管理应用数据和状态：用户缓存、好友列表缓存、聊天记录缓存、本地 SQLite 数据库操作、文件传输状态跟踪。

## 文件

| 文件 | 说明 |
|------|------|
| `usermanager.h/cpp` | **用户状态管理中心**（单例）。缓存当前用户信息、好友列表、聊天线程映射、文件传输状态。提供 `getUid()`, `getToken()`, `isFriendByUid()` 等接口 |
| `sqlitemanager.h/cpp` | SQLite 数据库管理（单例）。负责本地数据库的 CRUD：用户信息、好友列表、聊天记录、文件传输记录的持久化 |
| `loadlocaldata.h/cpp` | **本地数据加载协调器**（单例）。在独立线程中运行，负责：启动时加载缓存的聊天列表、好友列表、好友申请；运行时增量存储新数据；定时同步 |
| `chatdatalist.h/cpp` | 聊天消息数据模型/控制器。管理单个会话中的消息列表，支持分页加载（每页 20 条），区分文本/图片/文件消息 |
| `chatthreaddata.h/cpp` | 聊天会话（线程）数据结构。每个 `ChatThreadData` 对应一个私聊/群聊会话，包含会话 ID、对方 UID、未读计数等 |
| `fileuploadmsg.h/cpp` | **文件传输管理**（单例）。与 ResourceServer 通信，管理文件/图片的上传和下载，支持断点续传。在独立线程中运行 |

## 数据流

```
Server ──→ network/tcpmsg ──→ data/LoadLocalData ──→ SQLite (本地持久化)
                                    │
                                    └──→ data/UserManager (内存缓存)
                                              │
                                              └──→ dialogs/ (UI 渲染)
```

## 依赖

```
data
 ├──→ core/global.h, core/userdata.h
 ├──→ dialogs/chatuserwidget.h, dialogs/loadingdialog.h
 └──→ network/tcpmsg.h（间接，通过信号槽）
```

## 被谁使用

```
core/global.cpp  →  UserManager, FileUploadMsg, LoadLocalData（处理头像下载等）
core/mainwindow  →  LoadLocalData（启动时加载本地数据）
network/         →  TcpMsg 解析服务器消息后存入 UserManager 缓存
dialogs/         →  所有页面通过信号槽读写 UserManager 和 LoadLocalData
```
