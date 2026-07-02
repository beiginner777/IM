#include "usermanager.h"

UserManager::~UserManager()
{

}

void UserManager::setUserInfo(std::shared_ptr<UserInfo> user_info) {
    userInfo_ = user_info;
}

void UserManager::setToken(QString token)
{
    token_ = token;
}

int UserManager::getUid()
{
    return userInfo_->uid_;
}

QString UserManager::getName()
{
    return userInfo_->name_;
}

QString UserManager::getNick()
{
    return userInfo_->nick_;
}

QString UserManager::getIcon()
{
    if(userInfo_ != nullptr && userInfo_->icon_ != nullptr){
        return userInfo_->icon_;
    }else{
        qDebug() << "nullptr";
    }
}

QString UserManager::getDesc()
{
    return userInfo_->desc_;
}

std::shared_ptr<UserInfo> UserManager::getUserInfo()
{
    return userInfo_;
}

void UserManager::appendFriendList(std::vector<std::shared_ptr<FriendInfo>> friends)
{
    for(auto info : friends){
        int uid = info->uid_;
        friendList_.push_back(info);
        friendMap_.insert(uid, info);
    }
}

void UserManager::appendApplyList(QJsonArray array)
{
    // 遍历 QJsonArray 并输出每个元素
    for (const QJsonValue &value : array) {
        auto name = value["name"].toString();
        auto desc = value["desc"].toString();
        auto icon = value["icon"].toString();
        auto nick = value["nick"].toString();
        auto sex = value["sex"].toInt();
        auto uid = value["uid"].toInt();
        auto status = value["status"].toInt();
        auto info = std::make_shared<ApplyInfo>(uid, name,
                           desc, icon, nick, sex, status);
        applyList_.push_back(info);
    }
}

void UserManager::appendApplyList(std::vector<std::shared_ptr<ApplyInfo>> apply_list)
{
    for(auto apply : apply_list){
        applyList_.push_back(apply);
    }
}

void UserManager::appendFriendList(QJsonArray array) {
    // 遍历 QJsonArray 并输出每个元素
    for (const QJsonValue& value : array) {
        auto name = value["name"].toString();
        auto desc = value["desc"].toString();
        auto icon = value["icon"].toString();
        auto nick = value["nick"].toString();
        auto sex = value["sex"].toInt();
        auto uid = value["uid"].toInt();
        auto back = value["back"].toString();

        auto info = std::make_shared<FriendInfo>(uid, name,
            nick, icon, sex, desc, back);
        friendList_.push_back(info);
        friendMap_.insert(uid, info);
    }
}

std::vector<std::shared_ptr<ApplyInfo> > UserManager::getApplyList()
{
    return applyList_;
}

std::vector<std::shared_ptr<FriendInfo> > UserManager::getFriendList()
{
    return friendList_;
}

void UserManager::addApplyList(std::shared_ptr<ApplyInfo> app)
{
    applyList_.push_back(app);
}

bool UserManager::alreadyApply(int uid)
{
    for(auto& apply: applyList_){
        if(apply->uid_ == uid){
            return true;
        }
    }
    return false;
}

std::vector<std::shared_ptr<FriendInfo>> UserManager::getChatListPerPage() {

    std::vector<std::shared_ptr<FriendInfo>> friend_list;
    int begin = chatLoaded_;
    int end = begin + CHAT_COUNT_PER_PAGE;

    if (begin >= friendList_.size()) {
        return friend_list;
    }

    if (end > friendList_.size()) {
        friend_list = std::vector<std::shared_ptr<FriendInfo>>(friendList_.begin() + begin, friendList_.end());
        return friend_list;
    }


    friend_list = std::vector<std::shared_ptr<FriendInfo>>(friendList_.begin() + begin, friendList_.begin()+ end);
    return friend_list;
}


std::vector<std::shared_ptr<FriendInfo>> UserManager::getConListPerPage() {
    std::vector<std::shared_ptr<FriendInfo>> friend_list;
    int begin = contactLoaded_;
    int end = begin + CHAT_COUNT_PER_PAGE;

    if (begin >= friendList_.size()) {
        return friend_list;
    }

    if (end > friendList_.size()) {
        friend_list = std::vector<std::shared_ptr<FriendInfo>>(friendList_.begin() + begin, friendList_.end());
        return friend_list;
    }


    friend_list = std::vector<std::shared_ptr<FriendInfo>>(friendList_.begin() + begin, friendList_.begin() + end);
    return friend_list;
}


UserManager::UserManager():userInfo_(nullptr), chatLoaded_(0),contactLoaded_(0),lastChatThreadID_(0)
{
    //int uid, QString name, QString nick, QString icon, int sex, QString last_msg = "", QString desc=""
    userInfo_ = std::make_shared<UserInfo>(4,"Jerry","hahaha77",":/res/default.jpeg",1,"G","Hello,world");
}

void UserManager::slotAddFriendRsp(std::shared_ptr<AuthRsp> rsp)
{
    addFriend(rsp);
}

void UserManager::slotAddFriendAuth(std::shared_ptr<AuthInfo> auth)
{
    addFriend(auth);
}

bool UserManager::isLoadChatFin() {
    if (chatLoaded_ >= friendList_.size()) {
        return true;
    }

    return false;
}

void UserManager::updateChatLoadedCount() {
    int begin = chatLoaded_;
    int end = begin + CHAT_COUNT_PER_PAGE;

    if (begin >= friendList_.size()) {
        return ;
    }

    if (end > friendList_.size()) {
        chatLoaded_ = friendList_.size();
        return ;
    }

    chatLoaded_ = end;
}

void UserManager::updateContactLoadedCount() {
    int begin = contactLoaded_;
    int end = begin + CHAT_COUNT_PER_PAGE;

    if (begin >= friendList_.size()) {
        return;
    }

    if (end > friendList_.size()) {
        contactLoaded_ = friendList_.size();
        return;
    }

    contactLoaded_ = end;
}

bool UserManager::isLoadConFin()
{
    if (contactLoaded_ >= friendList_.size()) {
        return true;
    }

    return false;
}

bool UserManager::checkFriendById(int uid)
{
    auto iter = friendMap_.find(uid);
    if(iter == friendMap_.end()){
        return false;
    }

    return true;
}

void UserManager::addFriend(std::shared_ptr<AuthRsp> auth_rsp)
{
    auto friend_info = std::make_shared<FriendInfo>(auth_rsp);
    friendMap_[friend_info->uid_] = friend_info;
}

void UserManager::addFriend(std::shared_ptr<AuthInfo> auth_info)
{
    auto friend_info = std::make_shared<FriendInfo>(auth_info);
    friendMap_[friend_info->uid_] = friend_info;
}

std::shared_ptr<FriendInfo> UserManager::getFriendById(int uid)
{
    auto find_it = friendMap_.find(uid);
    if(find_it == friendMap_.end()){
        return nullptr;
    }

    return *find_it;
}

void UserManager::appendFriendChatMsg(int friend_id,std::vector<std::shared_ptr<TextChatData> > msgs)
{
    auto find_iter = friendMap_.find(friend_id);
    if(find_iter == friendMap_.end()){
        qDebug()<<"append friend uid  " << friend_id << " not found";
        return;
    }
    find_iter.value()->AppendChatMsgs(msgs);
}

std::shared_ptr<ChatThreadData> UserManager::GetChatThreadDataBuThreadID(int thread_id)
{
    auto iter = _chat_map.find(thread_id);
    if(iter == _chat_map.end()){
        return nullptr;
    }else{
        return iter.value();
    }
}

int UserManager::GetThreadIdByUid(int uid)
{
    auto iter = uid_to_thread_id_.find(uid);
    if(iter != uid_to_thread_id_.end()){
        qDebug() << "GetThreadIdByUid::Get thread_id = " << iter.value() << " other_id = " << uid << " success.";
        return iter.value();
    }
    return -1;
}

int UserManager::GetUidByThreadId(int thread_id)
{
    auto iter = thread_ids_.find(thread_id);
    if(iter != thread_ids_.end()){
        //qDebug() << "GetUidByThreadId::Get thread_id = " << thread_id << " other_id = " << iter.value() << " success.";
        return iter.value();
    }
    return -1;
}

void UserManager::addChatThreadData(std::shared_ptr<ChatThreadData> chatThreadData)
{
    int thread_id = chatThreadData->GetThreadId();
    int other_id = chatThreadData->GetOtherId();
    if(_chat_map.find(thread_id) != _chat_map.end()){
        qDebug() << "thread_id = " << thread_id << " other_id = " << other_id << " ChatThreadData exist.";
        return;
    }
    _chat_map[thread_id] = chatThreadData;
    threadIds_.push_back(thread_id);
    if(chatThreadData->GetOtherId() != 0){
        // 私聊
        thread_ids_[thread_id] = other_id;
        uid_to_thread_id_[other_id] = thread_id;
    }
}

std::vector<int> UserManager::GetAllThreadID()
{
    return threadIds_;
}

void UserManager::set_last_msg_id_for_thread_id(int thread_id, int msg_id)
{
    thread_id_to_msg_id_[thread_id] = msg_id;
}

int UserManager::getLastMsgIdByThreadId(int thread_id)
{
    return thread_id_to_msg_id_[thread_id];
}

QString UserManager::get_file_path(QString md5)
{
   if(files_path_.count(md5)){
       return files_path_[md5];
   }
   return "";
}

std::shared_ptr<MsgInfo> UserManager::get_trans_file(QString name)
{
    if(trans_files_.count(name)){
        return trans_files_[name];
    }
    return nullptr;
}

void UserManager::add_download_file(QString file, std::shared_ptr<DownloadFileInfo> file_info)
{
    download_files_[file] = file_info;
}

std::shared_ptr<DownloadFileInfo> UserManager::get_download_file(QString file)
{
    if(download_files_.count(file)){
        return download_files_[file];
    }
    return nullptr;
}

bool UserManager::remove_download_file(QString file)
{
    auto iter = download_files_.find(file);
    if(iter != download_files_.end()){
        download_files_.erase(iter);
        return true;
    }
    return false;
}

void UserManager::AddUniqueNameByLocalPath(QString local_path, QString unique_name)
{
    unqiue_name_by_local_path_[local_path] = unique_name;
}

QString UserManager::GetUniqueNameByLocalPath(QString local_path)
{
    auto iter = unqiue_name_by_local_path_.find(local_path);
    if(iter != unqiue_name_by_local_path_.end()){
        return iter.value();
    }
    return "";
}

bool UserManager::PauseTransFileByName(QString unique_name, TRANSFER_STATE trans_state)
{
    auto iter = trans_files_.find(unique_name);
    if(iter != trans_files_.end()) {
        iter->second->_state = TRANSFER_STATE::Paused;
        //qDebug() << "将文件设置为暂停状态成功: unique_name = " << unique_name;
        return true;
    }
    //qDebug() << "将文件设置为暂停状态失败: unique_name = " << unique_name << " file not exist.";
    return false;
}

bool UserManager::ResumeTransFileByName(QString unique_name)
{
    auto iter = trans_files_.find(unique_name);
    if(iter != trans_files_.end()) {
        iter->second->_state = TRANSFER_STATE::Uploading;
        qDebug() << "将文件设置为上传状态成功: unique_name = " << unique_name;
        return true;
    }
    qDebug() << "将文件设置为上传状态失败: unique_name = " << unique_name << " file not exist.";
    return false;
}

bool UserManager::isFriendByUid(int uid)
{
    auto iter = friendMap_.find(uid);
    if(iter != friendMap_.end()){
        return true;
    }
    return false;
}

bool UserManager::isCreateThreadByThreadID(int thread_id)
{
    auto iter = _chat_map.find(thread_id);
    if(iter != _chat_map.end()){
        return true;
    }
    return false;
}

bool UserManager::changeFriendIconByuid(int uid, QString icon)
{
     auto iter = friendMap_.find(uid);
     if(iter != friendMap_.end()){
         iter.value()->icon_ = icon;
         qDebug() << "Change friend_id = " << uid << " icon = " << icon << " success.";
         return true;
     }
     qDebug() << "Change friend_id = " << uid << " icon = " << icon << " failed.";
     return false;
}

bool UserManager::add_chat_image(QString unique_name, std::shared_ptr<ImageDataBase> image)
{
    if(chat_images_.count(unique_name) != 0 ){
        return false;
    }
    chat_images_[unique_name] = image;
}

std::shared_ptr<ImageDataBase> UserManager::get_chat_image(QString unique_name)
{
    if(chat_images_.count(unique_name) != 0){
        return chat_images_[unique_name];
    }
    return nullptr;
}

bool UserManager::rm_chat_image(QString unique_name)
{
    auto it = chat_images_.find(unique_name);
    if(it != chat_images_.end()){
        chat_images_.erase(it);
        return true;
    }
    return false;
}

void UserManager::addCachedMsg(int thread_id, std::shared_ptr<ChatDataBase> data)
{
    cache_.insert(thread_id,data);
}

void UserManager::rmCacheMsg(int thread_id)
{
    cache_.remove(thread_id);
}

std::vector<std::shared_ptr<ChatDataBase> > UserManager::GetCacheMsg(int thread_id)
{
    std::vector<std::shared_ptr<ChatDataBase>> datas;
    auto range = cache_.equal_range(thread_id);
    for (auto it = range.first; it != range.second; ++it) {
        datas.push_back(it.value());
    }
    return datas;
}

int UserManager::get_threadId_min_message_id(int thread_id)
{
    return min_message_ids_[thread_id];
}

int UserManager::get_threadId_max_message_id(int thread_id)
{
    return max_message_ids_[thread_id];
}

void UserManager::set_threadId_min_message_id(int thread_id,int min_message_id)
{
    min_message_ids_[thread_id] = min_message_id;
}

void UserManager::set_threadId_max_message_id(int thread_id,int max_message_id)
{
    max_message_ids_[thread_id] = max_message_id;
}
