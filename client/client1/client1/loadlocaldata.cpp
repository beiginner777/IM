#include "loadlocaldata.h"
#include "userdata.h"
#include "loadingdialog.h"
#include "usermanager.h"

LoadLocalData::LoadLocalData()
{
    connect(this,&LoadLocalData::signalAddUser,this,&LoadLocalData::slotAddUser);
    connect(this,&LoadLocalData::signalLoadChatUserList,this,&LoadLocalData::slotloadChatUserList);
    connect(this,&LoadLocalData::signalloadFriendList,this,&LoadLocalData::slotloadFriendList);
    connect(this,&LoadLocalData::signalloadFriendApplyList,this,&LoadLocalData::slotloadFriendApplyList);
    connect(this,&LoadLocalData::signalloadMessgaeThroughThreadId,this,&LoadLocalData::slotloadMessgaeThroughThreadId);
    connect(this,&LoadLocalData::signalLoadLocalInfo,this,&LoadLocalData::slotLoadLocalInfo);
    connect(this,&LoadLocalData::signalStoreThreadInfo,this,&LoadLocalData::slotStoreThreadInfo);
    connect(this,&LoadLocalData::signalStoreFriends,this,&LoadLocalData::slotStoreFriends);
    connect(this,&LoadLocalData::signalGetLoginedUser,this,&LoadLocalData::slotGetLoginedUser);
    connect(this,&LoadLocalData::signalupdateSelfIcon,this,&LoadLocalData::slotupdateSelfIcon);
    connect(this,&LoadLocalData::signalupdateFriendIcon,this,&LoadLocalData::slotupdateFriendIcon);
    connect(this,&LoadLocalData::signalStoreFriendApply,this,&LoadLocalData::slotStoreFriendApply);
    connect(this,&LoadLocalData::signalCheckLocalChatUser,this,&LoadLocalData::slotCheckLocalChatUser);
    connect(this,&LoadLocalData::signalLoadspecifiedChatUser,this,&LoadLocalData::slotLoadspecifiedChatUser);
    connect(this,&LoadLocalData::signalmodifyMsgId,this,&LoadLocalData::slotmodifyMsgId);
    connect(this,&LoadLocalData::signalstoreMessages,this,&LoadLocalData::slotstoreMessages);
    connect(this,&LoadLocalData::signalLoadChatMessage,this,&LoadLocalData::slotLoadChatMessage);
    connect(this,&LoadLocalData::signalSetFriendApplyStatus,this,&LoadLocalData::slotSetFriendApplyStatus);
    connect(this,&LoadLocalData::signalSaveTransferToDb, this, &LoadLocalData::slotSaveTransferToDb);
    connect(this,&LoadLocalData::signalAddTransferToDb,this,&LoadLocalData::slotAddTransferToDb);
}

LoadLocalData::~LoadLocalData()
{
}

void LoadLocalData::onThreadStarted()
{
    qDebug() << "Database Thread: " << QThread::currentThread();
    sqlite_ = new SqliteManager();
    connect(sqlite_,&SqliteManager::signalLoadLocalInfoFinish,this,&LoadLocalData::slotLoadLocalInfoFinish);
    connect(sqlite_,&SqliteManager::signalDrawChatUserToList,this,&LoadLocalData::slotDrawChatUserToList);
    connect(sqlite_,&SqliteManager::signaloadHistortUserFinish,this,&LoadLocalData::slotloadHistortUserFinish);
    connect(sqlite_,&SqliteManager::signalDrawFriendApplyToList,this,&LoadLocalData::slotDrawFriendApplyToList);
    connect(sqlite_,&SqliteManager::signalChatDialogSwitchFromConnToChat,this,&LoadLocalData::slotChatDialogSwitchFromConnToChat);
    connect(sqlite_,&SqliteManager::signalSpecifiedChatUserLoadFinished,this,&LoadLocalData::slotSpecifiedChatUserLoadFinished);
    connect(sqlite_,&SqliteManager::signalNotifyIsLoadMoreMsg,this,&LoadLocalData::slotNotifyIsLoadMoreMsg);
}

void LoadLocalData::slotAddUser(std::shared_ptr<UserInfo> userInfo)
{
    sqlite_->addUser(userInfo);
}

void LoadLocalData::slotloadChatUserList()
{
    sqlite_->loadChatUserList();
}

void LoadLocalData::slotloadFriendList()
{
    sqlite_->loadFriendList();
}

void LoadLocalData::slotloadFriendApplyList()
{
    sqlite_->loadFriendApplyList();
}

void LoadLocalData::slotloadMessgaeThroughThreadId(int thread_id)
{
    sqlite_->loadMessgaeThroughThreadId(thread_id);
}

void LoadLocalData::slotLoadLocalInfo()
{
    sqlite_->loadLocalInfo();
}

void LoadLocalData::slotGetLoginedUser()
{
    sqlite_->GetLoginedUser();
}

void LoadLocalData::slotloadHistortUserFinish(std::vector<std::shared_ptr<UserInfo>> history_users)
{
    emit signalloadHistortUserFinish(history_users);
}

void LoadLocalData::slotStoreThreadInfo(std::shared_ptr<ChatThreadInfo> thread_info)
{
    sqlite_->storeThreadInfo(thread_info);
}

void LoadLocalData::slotStoreFriends(std::vector<std::shared_ptr<FriendInfo>> friends)
{
    sqlite_->storeFriends(friends);
}

void LoadLocalData::slotStoreFriendApply(std::vector<std::shared_ptr<ApplyInfo> > apply_list)
{
    sqlite_->storeFriendApply(apply_list);
}

void LoadLocalData::slotupdateSelfIcon(QString icon)
{
    sqlite_->updateSelfIcon(icon);
}

void LoadLocalData::slotupdateFriendIcon(int friend_id, QString icon)
{
    sqlite_->updateFriendIcon(friend_id,icon);
}

void LoadLocalData::slotCheckLocalChatUser(int uid)
{
    sqlite_->CheckLocalChatUser(uid);
}

void LoadLocalData::slotChatDialogSwitchFromConnToChat(int uid)
{
    emit signalNotifyChatDialogSwitchFromConnToChat(uid);
}

void LoadLocalData::slotLoadspecifiedChatUser(int uid)
{
    sqlite_->loadspecifiedChatUser(uid);
}

void LoadLocalData::slotmodifyMsgId(int thread_id, int min_message_id, int max_message_id)
{
    sqlite_->modifyMsgId(thread_id,min_message_id,max_message_id);
}

void LoadLocalData::slotstoreMessages(std::vector<std::shared_ptr<ChatDataBase> > messages)
{
    sqlite_->storeMessages(messages);
}

void LoadLocalData::slotLoadChatMessage(int thread_id)
{
    sqlite_->loadChatMessage(thread_id);
}

void LoadLocalData::slotSetFriendApplyStatus(int peeruid, FRIEND_APPLY status)
{
    sqlite_->setFriendApplyStatus(peeruid,status);
}

void LoadLocalData::slotSaveTransferToDb(QString unique_name, int trans_size, int total_size, int trans_status, int trans_type, QString error)
{
    sqlite_->saveTransferToDb(unique_name, trans_size, total_size, trans_status, trans_type, error);
}

void LoadLocalData::slotAddTransferToDb(QString unique_name, int trans_size, int total_size, QString local_path, int state, int trans_type, QString error)
{
    sqlite_->addTransferToDb(unique_name, trans_size, total_size, local_path, state, trans_type, error);
}

void LoadLocalData::slotSpecifiedChatUserLoadFinished(std::shared_ptr<ChatThreadInfo> thread_info)
{
    emit singnalSpecifiedChatUserLoadFinished(thread_info);
}

void LoadLocalData::slotNotifyIsLoadMoreMsg(int thread_id, bool is_more, std::vector<std::shared_ptr<ChatDataBase> > chat_messages)
{
    emit signalNotifyIsLoadMoreMsg(thread_id,is_more);
    emit signalNotifyLoadMoreMsgFromLocal(thread_id,chat_messages);
}

void LoadLocalData::slotLoadLocalInfoFinish()
{
    emit signalLoadLocalInfoFinish();
}

void LoadLocalData::slotDrawChatUserToList(std::vector<ChatThreadInfo> thread_infos, bool is_load_more)
{
    emit signalDrawChatUserToList(thread_infos,is_load_more);
}

void LoadLocalData::slotDrawFriendApplyToList(std::vector<std::shared_ptr<ApplyInfo> > apply_list, bool is_load_more)
{
    emit signalDrawFriendApplyToList(apply_list, is_load_more);
}

