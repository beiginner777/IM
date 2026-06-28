# core — 基础设施层

## 职责
整个项目的基石。包含程序入口、全局定义、主窗口外壳、共享数据结构。
本模块**不依赖**任何其他业务模块，是依赖树的根节点。

## 文件

| 文件 | 说明 |
|------|------|
| `main.cpp` | 应用程序入口，加载 `config.ini`，启动网络线程，创建 `MainWindow` |
| `global.h` / `global.cpp` | **全局头文件**（被几乎所有文件 include）。定义所有枚举（`REQUEST_ID`, `ERRORCODE`, `ChatUIMode` 等）、全局常量、全局变量（`Gate_Url_Prefix`）、工具函数（`showTip`, `registerMetaType`, `returnPixMapByUrl` 等） |
| `singleton.h` | 模板单例工具类 (`SingleTon<T>`)，被 `HttpManager`, `TcpMsg`, `FileUploadMsg`, `LoadLocalData`, `UserManager` 等使用 |
| `mainwindow.h/cpp/ui` | 主窗口容器，管理三个核心对话框的生命周期和切换：`LoginDialog` → `RegisterDialog` → `ChatDialog` |
| `userdata.h/cpp` | 共享数据结构定义：`SearchInfo`, `ApplyInfo`, `FriendInfo`, `UserInfo`, `ChatDataBase`, `TextChatData`, `ImageDataBase`, `ChatMessage`, `DownloadFileInfo` 等 |
| `config.ini` | 运行时配置（网关服务器地址、端口），由 `main.cpp` 在启动时读取 |

## 依赖

```
core
 └─ (无依赖，是顶层模块)
```

## 被依赖

```
widgets ──→ core/global.h
dialogs ──→ core/global.h, core/userdata.h
network ──→ core/global.h, core/userdata.h
data    ──→ core/global.h, core/userdata.h
```
