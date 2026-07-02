#ifndef LOADLOCALDATA_H
#define LOADLOCALDATA_H

#include "core/global.h"
#include "chatthreaddata.h"
#include "sqlitemanager.h"

class UserInfo;

class LoadLocalData : public QObject,
                      public SingleTon<LoadLocalData>
{
    Q_OBJECT
public:
    friend class SingleTon<LoadLocalData>;
    ~LoadLocalData();
public slots:
    void onThreadStarted();
    void slotAddUser(std::shared_ptr<UserInfo> userInfo);
    void slotloadChatUserList();
    void slotloadFriendList();
    void slotloadFriendApplyList();
    void slotloadMessgaeThroughThreadId(int thread_id);
    void slotLoadLocalInfo();
    void slotGetLoginedUser();
    void slotloadHistortUserFinish(std::vector<std::shared_ptr<UserInfo>> history_users);
    void slotStoreThreadInfo(std::shared_ptr<ChatThreadInfo> thread_info);
    void slotStoreFriends(std::vector<std::shared_ptr<FriendInfo>> friends);
    void slotStoreFriendApply(std::vector<std::shared_ptr<ApplyInfo>> apply_list);
    void slotupdateSelfIcon(QString icon);
    void slotupdateFriendIcon(int friend_id,QString icon);
    void slotCheckLocalChatUser(int uid);
    void slotChatDialogSwitchFromConnToChat(int uid);
    void slotLoadspecifiedChatUser(int uid);
    void slotmodifyMsgId(int thread_id,int min_message_id,int max_message_id);
    void slotstoreMessages(std::vector<std::shared_ptr<ChatDataBase>> messages);
    void slotLoadChatMessage(int thread_id);
    void slotSetFriendApplyStatus(int peeruid,FRIEND_APPLY status);
    void slotSaveTransferToDb(QString unique_name, int trans_size, int total_size, int trans_status,int trans_type, QString error);
    void slotAddTransferToDb(QString unique_name, int trans_size, int total_size, QString local_path, int state, int trans_type, QString error);

signals:
    void signalAddUser(std::shared_ptr<UserInfo> userInfo); // 添加历史用户
    void signalLoadChatUserList(); // 加载聊天用户列表
    void signalLoadspecifiedChatUser(int); // 加载指定的聊天线程
    void signalloadFriendList(); // 加载好友列表
    void signalloadFriendApplyList(); // 加载好友申请列表
    void signalloadMessgaeThroughThreadId(int thread_id);
    void signalLoadLocalInfo(); // 加载本地初始化信息
    void signalGetLoginedUser(); // 加载历史用户
    void signalloadHistortUserFinish(std::vector<std::shared_ptr<UserInfo>> history_users); // 加载历史用户成功
    void signalStoreThreadInfo(std::shared_ptr<ChatThreadInfo> thread_info); // 存储聊天用户信息
    void signalStoreFriends(std::vector<std::shared_ptr<FriendInfo>> friends); // 存储好友信息
    void signalStoreFriendApply(std::vector<std::shared_ptr<ApplyInfo>> apply_list); // 存储好友申请信息
    void signalupdateSelfIcon(QString icon); // 修改自己的头像信息
    void signalupdateFriendIcon(int friend_id,QString icon); // 修改好友的头像信息
    void signalCheckLocalChatUser(int uid);
    void signalmodifyMsgId(int thread_id,int min_message_id,int max_message_id);
    void signalstoreMessages(std::vector<std::shared_ptr<ChatDataBase>> messages);
    void signalLoadChatMessage(int thread_id);
    void signalSetFriendApplyStatus(int peeruid, FRIEND_APPLY status);
    void signalSaveTransferToDb(QString unique_name, int trans_size, int total_size, int trans_status,int trans_type, QString error);
    void signalAddTransferToDb(QString unique_name, int trans_size, int total_size, QString local_path, int state, int trans_type, QString error);

// 用来连接SqliteManager和除了LoadLocalData的一些信号和槽函数
signals:
    void signalLoadLocalInfoFinish(); // 初始化本地信息完成
    void signalDrawChatUserToList(std::vector<ChatThreadInfo> thread_infos,bool is_load_more); // 通知聊天用户列表去渲染数据
    void signalDrawFriendApplyToList(std::vector<std::shared_ptr<ApplyInfo>> apply_list, bool is_load_more); // 通知好友申请列表去渲染数据
    void signalNotifyChatDialogSwitchFromConnToChat(int);
    void singnalSpecifiedChatUserLoadFinished(std::shared_ptr<ChatThreadInfo> thread_info);
    void signalNotifyIsLoadMoreMsg(int thread_id, bool is_more);
    void signalNotifyLoadMoreMsgFromLocal(int,std::vector<std::shared_ptr<ChatDataBase>>);

public slots:
    void slotLoadLocalInfoFinish();
    void slotDrawChatUserToList(std::vector<ChatThreadInfo> thread_infos,bool is_load_more);
    void slotDrawFriendApplyToList(std::vector<std::shared_ptr<ApplyInfo>> apply_list, bool is_load_more);
    void slotSpecifiedChatUserLoadFinished(std::shared_ptr<ChatThreadInfo> thread_info);
    void slotNotifyIsLoadMoreMsg(int thread_id, bool is_more, std::vector<std::shared_ptr<ChatDataBase>> chat_messages);

private:
    LoadLocalData();
//    bool is_load_more_chatuser_;
//    bool is_load_more_friend_;
//    bool is_load_more_friendapply_;
//    bool is_load_more_message_through_threadid_;
    SqliteManager* sqlite_;
};

#endif // LOADLOCALDATA_H
