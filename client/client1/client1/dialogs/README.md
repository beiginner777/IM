# dialogs — 对话框与页面

## 职责
所有用户可见的窗口、对话框、页面，以及它们配套的列表控件和列表项。
这是最大的模块，按功能分为 4 个区域：

## 文件

### 登录/注册流程
| 文件 | 说明 |
|------|------|
| `logindialog.h/cpp/ui` | 登录对话框，输入账号密码，获取验证码 |
| `registerdialog.h/cpp/ui` | 注册对话框，填写资料创建账号 |
| `loadingdialog.h/cpp/ui` | 加载中覆盖层，异步操作时显示 |
| `logineduserlist.h/cpp` | 历史登录用户列表模型 |
| `logineduserwidget.h/cpp/ui` | 历史登录用户的列表项 |

### 聊天页面
| 文件 | 说明 |
|------|------|
| `chatdialog.h/cpp/ui` | **聊天主窗口**（最复杂的对话框），容器包含侧边栏、聊天区、联系人区、设置页 |
| `chatinterface.h/cpp/ui` | 聊天消息展示区 + 输入区 |
| `offlinedialog.h/cpp/ui` | 掉线通知对话框 |
| `settingpage.h/cpp/ui` | 设置页面（聊天窗口内嵌） |
| `imageviewerdialog.h/cpp` | 图片全屏查看器 |
| `chatuserlist.h/cpp` | 聊天用户列表（左侧最近聊天列表） |
| `chatuserwidget.h/cpp/ui` | 聊天用户列表的单项 |
| `searchlist.h/cpp` | 搜索结果列表 |
| `adduseritem.h/cpp/ui` | 向服务端发起搜素请求的点击框 |

### 好友管理
| 文件 | 说明 |
|------|------|
| `applyfriend.h/cpp/ui` | 搜索用户并发起好友申请 |
| `applyfriendpage.h/cpp/ui` | 好友申请列表页面（待处理的申请） |
| `applyfrienditem.h/cpp/ui` | 好友申请列表的单项 |
| `authfriendapply.h/cpp/ui` | 通过/拒绝好友申请的对话框 |
| `findfaildialog.h/cpp/ui` | "未找到用户"提示对话框 |
| `findsuccessdialog.h/cpp/ui` | "找到用户，发送申请"确认对话框 |
| `friendinfointerface.h/cpp/ui` | 好友详情/个人信息展示页 |
| `friendlabel.h/cpp/ui` | 好友昵称/标签控件 |

### 联系人
| 文件 | 说明 |
|------|------|
| `contactuserlist.h/cpp` | 联系人（好友）列表（聊天窗口右侧） |
| `contactuserwidget.h/cpp/ui` | 联系人列表的单项 |

## 依赖

```
dialogs
 ├──→ core/global.h, core/userdata.h
 ├──→ widgets/（clickbutton, clicklabel, customizeedit, listitembase, statewidget, grouptipitem, chatitembase, messagetextedit, timerbtn）
 ├──→ network/httpmanager.h, network/tcpmsg.h
 └──→ data/usermanager.h, data/fileuploadmsg.h, data/loadlocaldata.h, data/sqlitemanager.h, data/chatdatalist.h, data/chatthreaddata.h
```

## 被谁使用

```
core/mainwindow.h  →  管理 LoginDialog / RegisterDialog / ChatDialog 的创建和切换
network/tcpmsg.cpp →  直接引用 ChatUserWidget, ContactUserWidget 发送信号更新列表
```
