#ifndef CHATITEMBASE_H
#define CHATITEMBASE_H

#include "core/global.h"

class ChatItemBase : public QWidget
{
    Q_OBJECT
public:
    ChatItemBase(ChatRole role, QWidget* parent = nullptr);
    // 设置聊天消息的名字
    void setUserName(const QString& name);
    // 设置聊天消息的头像
    void setUserIcon(const QPixmap& icon);
    // 设置聊天消息的消息框（包括文字）
    void setWidget(QWidget* widget);
    // 设置消息的状态
    void setStatus(int status);
private:
    // 区分消息是自己的，还是别人的
    ChatRole role_;

    QLabel* nameLabel_;
    QLabel* iconLabel_;
    QWidget* bubble_;
    QLabel* statusLabel_;
};

#endif // CHATITEMBASE_H
