#ifndef CHATUSERLIST_H
#define CHATUSERLIST_H

#include "core/global.h"
#include "chatuserwidget.h"
#include "core/userdata.h"

// EventFilter > event > 子类 > 父类

class ChatItemBase;
class ChatThreadInfo;

struct ChatUserItem
{
    ChatUserItem() = default;
    ChatUserItem(int uid, QString icon,QString name,QString lastMessage,int unReadMessageCount,QDateTime chat_time)
        : uid_(uid), icon_(icon), name_(name),lastMessage_(lastMessage),unReadMessageCount_(unReadMessageCount), chat_time_(chat_time)
    {
    }

    int uid_; // 用户uid
    QString icon_; // 用户头像路径
    QString name_; // 用户名字
    QString lastMessage_; // 最后一条消息
    int unReadMessageCount_; // 未读的消息数量
    QDateTime chat_time_; // 最后一个消息的时间

    bool operator < (const ChatUserItem& item){
        // 再按时间倒序（新的在前）
        return chat_time_ > item.chat_time_ || (chat_time_ == item.chat_time_ && name_ > item.name_);
    }
};

class ChatUserModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum ChatUserItemRoles
    {
        UidRole = Qt::UserRole + 1,
        IconRole,
        NameRole,
        LastMessageRole,
        UnReadMessageCountRole,
        ChatTimeRole
    };

    ChatUserModel();
    ~ChatUserModel();

    QVariant GetData(int row, int role);
    int GetItemSize() { return items_.size(); }

    void addChatUserItem(ChatUserItem item);
    void addChatUserItems(QList<ChatUserItem> items);

    void updateItemLastMsg(int friend_id, QString lastMessage, QDateTime chat_time);
    void friendIconDownloadFinished(int friend_id, QString icon);
    void updateItemUnreadMsgCount(int friend_id, QString lastMessage, QDateTime chat_time, int unReadMsgCount);

    bool IsChatUserItemExist(int friend_id);

protected:
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

private:
    void sortAndNotify();

    QMap<int,ChatUserItem> id_items_;
    QList<ChatUserItem> items_;
};

class ChatUserView : public QListView
{
    Q_OBJECT
public:
    ChatUserView(QWidget* parent = nullptr);
    ~ChatUserView();

    void setIsLoading(bool flag) { is_load_more_from_local_ = flag; }
    void setCurrentChatUserItem(int friend_id);

protected:
    bool eventFilter(QObject* watched,QEvent* event) override;

private:
    bool is_load_more_from_local_;
};

class ChatUserDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    ChatUserDelegate();
    ~ChatUserDelegate();

protected:
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    void paint(QPainter *painter,
               const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    void drawAvatar(QPainter *painter,const QRect &rect,const QModelIndex &index, const QString icon) const;
};

class ChatUserList : public QListWidget
{
    Q_OBJECT
public:
    ChatUserList(QWidget* parent = nullptr);
    ~ChatUserList();
    // 添加Item
    void addChatUserWidget(std::shared_ptr<UserInfo> userInfo);
    // 更新Item的最后一条消息
    void updateItemLastMsg(int uid, QString last_msg, QDateTime chat_time);
    // 更新Item的未读消息的数量
    void updateItemUnreadMsgCount(int uid, QString last_msg, QDateTime chat_time, int unReadMsgCount);
    // 确认这个Item是否存在（调用Model的自定义接口）
    bool isChatUserExistByUid(int friend_id);
    // 向服务器请求数据
    void requestChatMessageToServer(int thread_id,int peerUid, int min_message_id,int max_message_id);
    // 设置当前选中的条目
    void setCurrentChatUserItem(int friend_id);

public slots:
    // 点击条目
    void slotItemClicked(const QModelIndex &index);
    // 收到服务器返回的数据（解析完成之后，添加给 Model）
    void slotLoadChatList(QJsonObject obj);
    // 收到本地返回的数据（解析完成之后，添加给 Model）
    void slotDrawChatUserToList(std::vector<ChatThreadInfo> thread_infos,bool is_load_more);
    // 收到好友头像下载完成的消息（调用上述Model接口）
    void slotFriendIconChange(QString);
    // 收到在线好友头像更改的消息（调用上述Mode接口）
    void slotOnlineFriendIconFinished(int,QString);

signals:
    void signalChangeChatUser(int); // 改变选中的条目
    void signalRequestMoreFromServer(); // 向服务器请求更多的数据

private:
    bool is_load_more_from_server_; // 是否可以从服务器加载更多数据
    bool is_load_more_from_local_; // 是否可以从本地加载更多数据
    QMap<QString,int> icons_; // icon : uid
    ChatUserModel* model_;
    ChatUserView* view_;
    ChatUserDelegate* delegate_;
};

#endif // CHATUSERLIST_H
