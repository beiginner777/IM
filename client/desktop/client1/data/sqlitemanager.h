#ifndef SQLITEMANAGER_H
#define SQLITEMANAGER_H

#include "core/global.h"
#include "core/userdata.h"

class UserInfo;
class ChatThreadInfo;
class FriendInfo;

class SqliteManager : public QObject
{
    Q_OBJECT
public:
    SqliteManager();
    ~SqliteManager();
    void addUser(std::shared_ptr<UserInfo> userInfo); // 插入登录过的用户
    void loadChatUserList(); // 增量/全部加载-->先加载服务器的数据，再加载客户端的数据（因此这里可以做成增量加载）
    void loadFriendList(); // 全部加载
    void loadFriendApplyList(); // 增量加载
    void loadMessgaeThroughThreadId(int thread_id); // 增量/全部加载-->先加载服务器的数据，再加载客户端的数据（因此这里可以做成增量加载）
    void GetLoginedUser();
    void loadLocalInfo();
    void storeThreadInfo(std::shared_ptr<ChatThreadInfo> thread_info);
    void storeFriends(std::vector<std::shared_ptr<FriendInfo>>);
    void updateSelfIcon(QString icon);
    void updateFriendIcon(int friend_id,QString icon);
    void storeFriendApply(std::vector<std::shared_ptr<ApplyInfo>> apply_list);
    void CheckLocalChatUser(int uid);
    void loadspecifiedChatUser(int uid);
    void modifyMsgId(int thread_id,int min_message_id,int max_message_id);
    void storeMessages(std::vector<std::shared_ptr<ChatDataBase>> messages);
    void loadChatMessage(int thread_id);
    void setFriendApplyStatus(int peeruid,FRIEND_APPLY status);
    void addTransferToDb(QString unique_name, int trans_size, int total_size, QString local_path, int state, int trans_type, QString error);
    void saveTransferToDb(QString unique_name, int trans_size, int total_size, int trans_status, int trans_type, QString error);

private:
    std::shared_ptr<MsgInfo> queryTransferMsgInfo(int operator_uid,
                                                                 int status,
                                                                 const QString& unique_name);

    QSqlDatabase db_;
    int last_thread_id_;
    int chat_thread_left_count_;
    int last_friend_id_;
    int last_friend_apply_id_;
    QMap<int,int> last_chat_message_id_;

    bool is_load_more_chatUser_;
    bool is_load_more_ConnUser_;
    bool is_load_more_FriendApply_;

    QMap<int,bool> is_load_more_chatmessage_; // 是否可以加载更多聊天记录

signals:
    void signalLoadLocalInfoFinish();
    void signalDrawChatUserToList(std::vector<ChatThreadInfo> thread_infos ,bool is_load_more);
    void signaloadHistortUserFinish(std::vector<std::shared_ptr<UserInfo>>);
    void signalDrawFriendApplyToList(std::vector<std::shared_ptr<ApplyInfo>>,bool);
    void signalChatDialogSwitchFromConnToChat(int);
    void signalSpecifiedChatUserLoadFinished(std::shared_ptr<ChatThreadInfo>);
    void signalNotifyIsLoadMoreMsg(int thread_id, bool is_more, std::vector<std::shared_ptr<ChatDataBase>>);
};

#endif // SQLITEMANAGER_H
