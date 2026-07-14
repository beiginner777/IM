# widgets — 可复用基础控件

## 职责
纯 UI 控件层，**不包含业务逻辑**。提供被 `dialogs/` 复用的基础组件。

## 文件

| 文件 | 说明 |
|------|------|
| `clickbutton.h/cpp` | 可点击按钮，跟踪点击状态（normal/select），支持 QSS 动态样式 |
| `clicklabel.h/cpp` | 可点击标签（QLabel 子类），带状态跟踪，用于侧边栏图标、好友标签等 |
| `clickoncelabel.h/cpp` | 单次点击标签变体 |
| `timerbtn.h/cpp` | 带倒计时功能的按钮，用于注册页验证码按钮 |
| `customizeedit.h/cpp` | 自定义输入框（QLineEdit 子类），带 placeholder 提示行为 |
| `messagetextedit.h/cpp` | 聊天消息输入框（QTextEdit 子类），支持回车发送 |
| `listitembase.h/cpp` | 列表项抽象基类，所有列表 item（聊天用户/联系人/搜索/好友申请）都继承它 |
| `statewidget.h/cpp` | 状态指示控件，用于侧边栏切换图标（搜索/聊天/联系人/设置模式） |
| `grouptipitem.h/cpp/ui` | 分组提示分割线，用于列表中的分组标题行 |
| `chatitembase.h/cpp` | 聊天气泡消息项的基类 |

## 依赖

```
widgets
 └──→ core/global.h
```

## 被谁使用

```
dialogs/
 ├──→ 登录/注册/聊天/好友 等所有对话框页面
 ├──→ 列表控件（ChatUserList, ContactUserList, SearchList 等）
 └──→ 列表项控件（ChatUserWidget, ContactUserWidget, ApplyFriendItem 等）
```
