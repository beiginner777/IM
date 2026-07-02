#ifndef CHATUSERWIDGET_H
#define CHATUSERWIDGET_H

#include "core/global.h"
#include "widgets/listitembase.h"
#include "core/userdata.h"

namespace Ui {
class ChatUserWidget;
}

class ChatUserWidget : public ListItemBase
{
    Q_OBJECT

public:
    explicit ChatUserWidget(QWidget *parent = nullptr);
    ~ChatUserWidget();

    QSize sizeHint() const override{
        return QSize(350,80);
    }

    void setInfo(QString head,QString name,QString msg);
    void setUserInfo(std::shared_ptr<UserInfo> userInfo);
    void setUserInfo(std::shared_ptr<FriendInfo> friendInfo);
    void setIcon(QString icon);


    std::shared_ptr<UserInfo> getUserInfo() {
        return userInfo_;
    }

    void updateLastMsg(QString lastMsg,QString chat_time);
private:
    Ui::ChatUserWidget *ui;

    std::shared_ptr<UserInfo> userInfo_;
    QString name_;
    QString head_;
    QString msg_;
    QString chat_time_;
};

#endif // CHATUSERWIDGET_H
