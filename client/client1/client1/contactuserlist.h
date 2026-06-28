#ifndef CONTACTUSERLIST_H
#define CONTACTUSERLIST_H

#include "global.h"
#include "contactuserwidget.h"

class ChatUserWidget;
class ContactUserWidget;

class ContactUserList : public QListWidget
{
    Q_OBJECT
    friend class ChatDialog;
public:
    explicit ContactUserList(QWidget* parent = nullptr);
    void showRedPoint(bool bshow = true);

protected:
    bool eventFilter(QObject* watched,QEvent* event) override;

public slots:
    void slotItemClick(QListWidgetItem* item);
    void slotLoadConnList(QJsonObject obj);
    void slotAddNewConnUserWidget(std::shared_ptr<FriendInfo> friend_info);
    void slotFriendIconChange(QString icon);
    void slotOnlineFriendIconFinished(int icon_uid, QString icon);

private:
    void DrawUserToList(); // 将UserManager中所有的好友数据渲染到列表上
    void initContactUserList(); // 添加默认条目
    ContactUserWidget* addFriendItem_; // 提示条目
    // QListWidgetItem* groupItem_; // 提示条目

    QMap<int, QListWidgetItem*> ConnAddedItem_; // 聊天记录条目(对方id,对应的ChatUserWidget)
    QMap<QString,int> icons_;

signals:
    void signalLoadingMoreMessage();
    void signalSwitchToFriendChatInterface(ContactUserWidget* item);
    void signalSwitchToFriendApplyInterface();
};

#endif // CONTACTUSERLIST_H
