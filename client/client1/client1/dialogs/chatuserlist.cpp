#include "chatuserlist.h"
#include "data/loadlocaldata.h"
#include "data/usermanager.h"
#include "data/loadlocaldata.h"
#include "network/tcpmsg.h"
#include "data/fileuploadmsg.h"

ChatUserList::ChatUserList(QWidget *parent)
    : QListWidget(parent)
    , is_load_more_from_server_(true)
    , is_load_more_from_local_(true)
{
    Q_UNUSED(parent);

    model_ = new ChatUserModel();
    view_  = new ChatUserView(this);
    delegate_ = new ChatUserDelegate();

    view_->setModel(model_);
    view_->setItemDelegate(delegate_);

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0,0,0,0);
    layout->addWidget(view_);

    // 确定当前点击的是哪一个条目
    connect(view_,&ChatUserView::clicked,this,&ChatUserList::slotItemClicked);
    // 服务器回传的加载聊天用户
    connect(TcpMsg::GetInstance().get(),&TcpMsg::signalLoadChatList,this,&ChatUserList::slotLoadChatList);
    // 将从本地数据库加载的ChatUser数据加载到界面
    connect(LoadLocalData::GetInstance().get(),&LoadLocalData::signalDrawChatUserToList,this,&ChatUserList::slotDrawChatUserToList);
    // 头像下载完成
    connect(FileUploadMsg::GetInstance().get(),&FileUploadMsg::signalOtherIconDownloadFinished,this,&ChatUserList::slotFriendIconChange);
    // 在线状态下，好友的头像发生变更
    connect(FileUploadMsg::GetInstance().get(),&FileUploadMsg::signalOnlineFriendIconFinished,this,&ChatUserList::slotOnlineFriendIconFinished);
}

ChatUserList::~ChatUserList()
{
    if(model_){
        delete model_;
        model_ = nullptr;
    }
    if(view_){
        delete view_;
        view_ = nullptr;
    }
    if(delegate_){
        delete delegate_;
        delegate_ = nullptr;
    }
}

void ChatUserList::addChatUserWidget(std::shared_ptr<UserInfo> userInfo)
{
    icons_[userInfo->icon_] = userInfo->uid_;
    int peeruid = userInfo->uid_;
    int thread_id = UserManager::GetInstance()->GetThreadIdByUid(peeruid);
    std::shared_ptr<ChatThreadData> chat_data_base = UserManager::GetInstance()->GetChatThreadDataBuThreadID(thread_id);

    assert(chat_data_base != nullptr);

    std::shared_ptr<ChatDataBase> data = chat_data_base->GetLastMsg();

    ChatUserItem item;
    item.uid_ = userInfo->uid_;
    item.icon_ = userInfo->icon_;
    item.name_ = userInfo->name_;
    item.lastMessage_ = "";
    item.chat_time_ = QDateTime();
    item.unReadMessageCount_ = 0;

    if(data){
        item.lastMessage_ = data->GetContent();
        item.chat_time_ = data->GetChatTime();
    }

    // 保存当前选中的uid
    int selected_uid = -1;
    QModelIndex current = view_->currentIndex();
    if (current.isValid()) {
        selected_uid = current.data(ChatUserModel::ChatUserItemRoles::UidRole).toInt();
    }else{
        //qDebug() << "[DEBUG] Save Current Selected Item failed.";
        return;
    }
    // 添加ChatUserItem
    model_->addChatUserItem(item);
    // 恢复选中的条目
    if (selected_uid >= 0) {
        for (int row = 0; row < model_->GetItemSize(); ++row) {
            QModelIndex index = model_->index(row, 0);
            int currentData = model_->GetData(row, ChatUserModel::ChatUserItemRoles::UidRole).toInt();
            if (selected_uid == currentData) {
                view_->setCurrentIndex(index);
                break;
            }
        }
    }
}

bool ChatUserList::isChatUserExistByUid(int friend_id)
{
    return model_->IsChatUserItemExist(friend_id);
}

void ChatUserList::requestChatMessageToServer(int thread_id, int peerUid, int min_message_id, int max_message_id)
{
    QJsonObject obj;
    obj["uid"] = UserManager::GetInstance()->getUid();
    obj["peerUid"] = peerUid;
    obj["thread_id"] = thread_id;
    obj["min_message_id"] = min_message_id;
    obj["max_message_id"] = max_message_id;
    obj["page_size"] = CHATMESSAGE_LOAD_PAGESIZE;
    QJsonDocument doc(obj);
    QByteArray data = doc.toJson(QJsonDocument::Compact);
    emit TcpMsg::GetInstance()->signalSendData(ID_LOAD_CHAT_MSG_REQ,data);
}

void ChatUserList::setCurrentChatUserItem(int friend_id)
{
    view_->setCurrentChatUserItem(friend_id);
}

void ChatUserList::slotItemClicked(const QModelIndex &index)
{
    QString name = index.data(ChatUserModel::ChatUserItemRoles::NameRole).toString();
    qDebug() << "点击了联系人：" << name;

    // 模拟读取消息，清除未读数
    QModelIndex sourceIndex = index;
    ChatUserItem item;
    item.uid_ = index.data(ChatUserModel::ChatUserItemRoles::UidRole).toInt();
    item.name_ = index.data(ChatUserModel::ChatUserItemRoles::NameRole).toString();
    item.icon_ = index.data(ChatUserModel::ChatUserItemRoles::IconRole).toString();
    item.lastMessage_ = index.data(ChatUserModel::ChatUserItemRoles::LastMessageRole).toString();
    item.chat_time_ = index.data(ChatUserModel::ChatUserItemRoles::ChatTimeRole).toDateTime();
    item.unReadMessageCount_ = 0;  // 清除未读

    model_->addChatUserItem(item);

    // 通知ChatDialog
    emit signalChangeChatUser(item.uid_);
}

void ChatUserList::updateItemLastMsg(int uid, QString last_msg, QDateTime chat_time)
{
    // 保存当前选中的uid
    int selected_uid = -1;
    QModelIndex current = view_->currentIndex();
    if (current.isValid()) {
        selected_uid = current.data(ChatUserModel::ChatUserItemRoles::UidRole).toInt();
    }else{
        //qDebug() << "[DEBUG] Save Current Selected Item failed.";
    }
    // 修改ChatUserItem
    model_->updateItemLastMsg(uid,last_msg,chat_time);
    // 恢复选中的条目
    if (selected_uid >= 0) {
        for (int row = 0; row < model_->GetItemSize(); ++row) {
            QModelIndex index = model_->index(row, 0);
            int currentData = model_->GetData(row, ChatUserModel::ChatUserItemRoles::UidRole).toInt();
            if (selected_uid == currentData) {
                view_->setCurrentIndex(index);
                break;
            }
        }
    }
}

void ChatUserList::updateItemUnreadMsgCount(int uid, QString last_msg, QDateTime chat_time, int unReadMsgCount)
{
    // 保存当前选中的uid
    int selected_uid = -1;
    QModelIndex current = view_->currentIndex();
    if (current.isValid()) {
        selected_uid = current.data(ChatUserModel::ChatUserItemRoles::UidRole).toInt();
    }else{
        //qDebug() << "[DEBUG] Save Current Selected Item failed.";
    }
    // 修改ChatUserItem
    model_->updateItemUnreadMsgCount(uid,last_msg,chat_time,unReadMsgCount);
    // 恢复选中的条目
    if (selected_uid >= 0) {
        for (int row = 0; row < model_->GetItemSize(); ++row) {
            QModelIndex index = model_->index(row, 0);
            int currentData = model_->GetData(row, ChatUserModel::ChatUserItemRoles::UidRole).toInt();
            if (selected_uid == currentData) {
                view_->setCurrentIndex(index);
                break;
            }
        }
    }
}

void ChatUserList::slotLoadChatList(QJsonObject obj)
{
    // 解析数据，并且渲染到ChatDataList（聊天记录列表）
    int uid = obj["uid"].toInt();
    is_load_more_from_server_ = obj["load_more"].toBool();
    int last_thread_id = obj["max_thread_id"].toInt();
    auto threadArrary = obj["threads"].toArray();
    UserManager::GetInstance()->setLastChatThreadID(last_thread_id);
    for(const QJsonValue& value : threadArrary)
    {
        auto info = std::make_shared<ChatThreadInfo>();
        info->threadId_ = value["thread_id"].toInt();
        info->threadType_ = value["thread_type"].toInt();
        info->user1_id_ = value["user1_id"].toInt();
        info->user2_id_ = value["user2_id"].toInt();
        int peerUid = 0;
        if(info->user1_id_ == UserManager::GetInstance()->getUid()){
            peerUid = info->user2_id_;
        }else{
            peerUid = info->user1_id_;
        }
        // 添加到缓存
        UserManager::GetInstance()->set_threadId_min_message_id(info->threadId_,0);
        UserManager::GetInstance()->set_threadId_max_message_id(info->threadId_,INT_MAX);
        // 添加到数据库
        emit LoadLocalData::GetInstance()->signalmodifyMsgId(info->threadId_,0,INT_MAX);
        // 创建新的ChatThreadData 并且 添加到UserManager
        std::shared_ptr<ChatThreadData> chatThreadData = std::make_shared<ChatThreadData>(peerUid,info->threadId_);
        UserManager::GetInstance()->addChatThreadData(chatThreadData);
        // 将数据插入到本地的数据库(同时设置最大的last_thread_id到表load_info)
        emit LoadLocalData::GetInstance()->signalStoreThreadInfo(info);

        // 获取好友的信息
        auto friendInfo = UserManager::GetInstance()->getFriendById(peerUid);
        icons_[friendInfo->icon_] = friendInfo->uid_;

        // 将聊天线程条目渲染到界面上
        ChatUserItem item;
        item.uid_ = peerUid;
        item.icon_ = friendInfo->icon_;
        item.name_ = friendInfo->name_;
        item.lastMessage_ = "";
        item.chat_time_ = QDateTime();
        item.unReadMessageCount_ = 0;

        model_->addChatUserItem(item);
    }
    if(is_load_more_from_server_) {
        emit signalRequestMoreFromServer();
    }else{
        // 发送加载聊天消息的请求
        auto thread_ids = UserManager::GetInstance()->GetAllThreadID();
        for(int thread_id : thread_ids){
            int peerUid  = UserManager::GetInstance()->GetUidByThreadId(thread_id);
            int min_message_id = UserManager::GetInstance()->get_threadId_min_message_id(thread_id);
            int max_message_id = UserManager::GetInstance()->get_threadId_max_message_id(thread_id);
            requestChatMessageToServer(thread_id,peerUid,min_message_id,max_message_id);
        }
    }
}

void ChatUserList::slotDrawChatUserToList(std::vector<ChatThreadInfo> thread_infos, bool is_load_more)
{
    //qDebug() << "Receive Load ChatUserItem Data ...";

    is_load_more_from_local_ = is_load_more;
    view_->setIsLoading(is_load_more_from_local_);

    // 渲染到 当前这个界面上
    for(auto info : thread_infos){       
        int peerUid = 0;
        if(info.user1_id_ == UserManager::GetInstance()->getUid()){
            peerUid = info.user2_id_;
        }else{
            peerUid = info.user1_id_;
        }
        if(model_->IsChatUserItemExist(peerUid)) {
            qDebug() << "ChatUserItem(" << peerUid << ") exist.";
            continue;
        } 
        // 将聊天记录条目渲染到界面上
        auto friendInfo = UserManager::GetInstance()->getFriendById(peerUid);
        icons_[friendInfo->icon_] = friendInfo->uid_;

        ChatUserItem item;
        item.uid_ = peerUid;
        item.icon_ = friendInfo->icon_;
        item.name_ = friendInfo->name_;
        item.lastMessage_ = "";
        item.chat_time_ = QDateTime();
        item.unReadMessageCount_ = 0;

        std::shared_ptr<ChatDataBase> data = UserManager::GetInstance()->GetChatThreadDataBuThreadID(info.threadId_)->GetLastMsg();
        if(data){
            item.lastMessage_ = data->GetContent();
            item.chat_time_ = data->GetChatTime();
        }

        // 获取最后一个消息内容 和 时间
//        int thread_id = info.threadId_;
//        auto chat_thread_data = UserManager::GetInstance()->GetChatThreadDataBuThreadID(thread_id);
//        assert(chat_thread_data != nullptr);
//        auto last_msg = chat_thread_data->GetLastMsg();
//        if(last_msg){
//            item.lastMessage_ = last_msg->GetContent();
//            item.chat_time_ = last_msg->GetChatTime();
//        }
        model_->addChatUserItem(item);

        // 去本地获取聊天数据内容
        emit LoadLocalData::GetInstance()->signalLoadChatMessage(info.threadId_);
    }
}

void ChatUserList::slotFriendIconChange(QString icon)
{
    auto iter = icons_.find(icon);
    if(iter == icons_.end()){
        qDebug() << "[ERROR]: " << __FILE__ << ":" << __LINE__;
        return;
    }
    int friend_id = icons_[icon];  
    model_->friendIconDownloadFinished(friend_id, icon);
}

void ChatUserList::slotOnlineFriendIconFinished(int icon_uid, QString icon)
{
    qDebug() << "icon_uid = " << icon_uid << " icon = " << icon ;
    auto iter = icons_.begin();
    for(iter = icons_.begin(); iter != icons_.end(); iter++){
        qDebug() << "icon = " << iter.key() << " uid = " << iter.value();
        if(iter.value() == icon_uid){
            icons_.erase(iter);
            break;
        }
    }
    if(iter == icons_.end()){
        qDebug() << "ChatUserWidget not load yet.";
        return;
    }
    icons_[icon] = icon_uid;

    int friend_id = icons_[icon];
    model_->friendIconDownloadFinished(friend_id, icon);
}

ChatUserModel::ChatUserModel()
{
}

ChatUserModel::~ChatUserModel()
{
}

QVariant ChatUserModel::GetData(int row, int role)
{
    if (row < 0 || row >= items_.size()){
        return QVariant();
    }
    const ChatUserItem& item = items_[row];
    switch (role) {
        case ChatUserItemRoles::UidRole :
            return item.uid_;
        case ChatUserItemRoles::IconRole :
            return item.icon_;
        case ChatUserItemRoles::NameRole :
            return item.name_;
        case ChatUserItemRoles::LastMessageRole :
            return item.lastMessage_;
        case ChatUserItemRoles::UnReadMessageCountRole :
            return item.unReadMessageCount_;
        case ChatUserItemRoles::ChatTimeRole :
            return item.chat_time_;
    default:
        return QVariant();
    }
}

void ChatUserModel::addChatUserItem(ChatUserItem item)
{
    //qDebug() << "Add ChatUserItem(uid = " << item.uid_ << ").";
    id_items_[item.uid_] = item;
    for(int i = 0; i < items_.size(); ++i){
        if(items_[i].uid_ == item.uid_){
            items_[i] = item;
            sortAndNotify();
            return;
        }
    }
    beginInsertRows(QModelIndex(), items_.size(), items_.size());
    items_.append(item);
    endInsertRows();
    sortAndNotify();
    Q_EMIT layoutChanged();
}

void ChatUserModel::addChatUserItems(QList<ChatUserItem> items)
{
    for(int i = 0; i < items.size(); ++i){
        addChatUserItem(items[i]);
    }
}

void ChatUserModel::updateItemLastMsg(int friend_id, QString lastMessage, QDateTime chat_time)
{
    auto iter = id_items_.find(friend_id);
    if(iter == id_items_.end()){
        qDebug() << "Find ChatUserItem In QMap(id_items_) failed.";
        return;
    }else{
        ChatUserItem item = iter.value();
        if(item.uid_ == 2){
            //qDebug() << "Old Time: " << item.chat_time_.toString() << ", New Time: " << chat_time;
        }
        if(item.chat_time_ < chat_time){
            item.lastMessage_ = lastMessage;
            item.chat_time_ = chat_time;
            this->addChatUserItem(item);
            //qDebug() << "Update ChatUserItem(friend_id = " << friend_id << "), last_msg = " << lastMessage << " chat_time = " << chat_time << "success.";
        }
    }
}

void ChatUserModel::friendIconDownloadFinished(int friend_id, QString icon)
{
    auto iter = id_items_.find(friend_id);
    if(iter == id_items_.end()){
        qDebug() << "Find ChatUserItem In QMap(id_items_) failed.";
        return;
    }
    ChatUserItem item = iter.value();
    item.icon_ = icon;
    this->addChatUserItem(item);
    qDebug() << "Update ChatUserItem(uid = " << friend_id << "), icon = " << icon << " success.";
}

void ChatUserModel::updateItemUnreadMsgCount(int friend_id, QString lastMessage, QDateTime chat_time, int unReadMsgCount)
{
    auto iter = id_items_.find(friend_id);
    if(iter == id_items_.end()){
        qDebug() << "Find ChatUserItem In QMap(id_items_) failed.";
        return;
    }
    ChatUserItem item = iter.value();
    item.lastMessage_ = lastMessage;
    item.chat_time_ = chat_time;
    item.unReadMessageCount_ += unReadMsgCount;
    this->addChatUserItem(item);
    qDebug() << "Update ChatUserItem(uid = " << friend_id << "), last_msg,chat_time,unreadMsgCount" << " success.";
}

bool ChatUserModel::IsChatUserItemExist(int friend_id)
{
    auto iter = id_items_.find(friend_id);
    if(iter != id_items_.end()){
        return true;
    }
    return false;
}

int ChatUserModel::rowCount(const QModelIndex &parent) const
{
    return items_.size();
}

QVariant ChatUserModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= items_.size()){
        return QVariant();
    }
    const ChatUserItem& item = items_[index.row()];
    switch (role) {
        case ChatUserItemRoles::UidRole :
            return item.uid_;
        case ChatUserItemRoles::IconRole :
            return item.icon_;
        case ChatUserItemRoles::NameRole :
            return item.name_;
        case ChatUserItemRoles::LastMessageRole :
            return item.lastMessage_;
        case ChatUserItemRoles::UnReadMessageCountRole :
            return item.unReadMessageCount_;
        case ChatUserItemRoles::ChatTimeRole :
            return item.chat_time_;
    default:
        return QVariant();
    }
}

void ChatUserModel::sortAndNotify()
{
    std::sort(items_.begin(), items_.end());
    Q_EMIT dataChanged(index(0), index(items_.size() - 1));
}

ChatUserView::ChatUserView(QWidget *parent)
    : QListView(parent),is_load_more_from_local_(true)
{
    this->installEventFilter(this);
    this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

ChatUserView::~ChatUserView()
{
}

void ChatUserView::setCurrentChatUserItem(int friend_id)
{
    for (int i = 0; i < this->model()->rowCount(); ++i) {
        QModelIndex index = this->model()->index(i, 0);
        if (index.data(ChatUserModel::ChatUserItemRoles::UidRole).toInt() == friend_id) {
            this->setCurrentIndex(index);
            qDebug() << "Set QListView select ChatUserItem(friend_id = " << friend_id << ") success.";
            return;
        }
    }
    qDebug() << "[ERROR] Set QListView select ChatUserItem(friend_id = " << friend_id << ") failed.";
}

bool ChatUserView::eventFilter(QObject *watched, QEvent *event)
{
    // 检查事件是否是鼠标悬浮进入或者离开
    if(watched == this){
       if(event->type() == QEvent::Enter){
           this->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
       }else if(event->type() == QEvent::Leave){
           this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
       }
    }
    if(watched == this && event->type() == QEvent::Wheel){
        QWheelEvent* wheelEvent = static_cast<QWheelEvent*>(event);
        int numDegrees = wheelEvent->angleDelta().y() / 8;
        int numSteps = numDegrees / 15;
        this->verticalScrollBar()->setValue(this->verticalScrollBar()->value() - numSteps);
        if(this->verticalScrollBar())
        {
            QScrollBar* scrollBar = this->verticalScrollBar();
            int maxScrollValue = scrollBar->maximum();
            int currentValue = scrollBar->value();
            if(maxScrollValue - currentValue <= 0){
                if(is_load_more_from_local_){
                    qDebug() << "loading more ChatUser";
                    //emit LoadLocalData::GetInstance()->signalLoadChatUserList();
                }
            }
        }
        return true;
    }

    return QListView::eventFilter(watched,event);
}

ChatUserDelegate::ChatUserDelegate()
{
}

ChatUserDelegate::~ChatUserDelegate()
{
}

QSize ChatUserDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    return QSize(option.rect.width(), 80);
}

void ChatUserDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    painter->save();

    // 设置抗锯齿
    painter->setRenderHint(QPainter::Antialiasing);

    // 绘制选中/悬停背景
    if (option.state & QStyle::State_Selected) {
        painter->fillRect(option.rect, option.palette.highlight());
    } else if (option.state & QStyle::State_MouseOver) {
        painter->fillRect(option.rect, option.palette.alternateBase());
    } else {
        painter->fillRect(option.rect, option.palette.base());
    }

    // 1. 绘制头像
    const int margin = 10;
    const int avatarSize = 50;
    QRect avatarRect(option.rect.left() + margin,
                    option.rect.top() + (option.rect.height() - avatarSize) / 2,
                    avatarSize, avatarSize);

    QString icon = index.data(ChatUserModel::ChatUserItemRoles::IconRole).toString();
    drawAvatar(painter, avatarRect, index, icon);

    // 绘制头像文字（名字首字母）
    QString name = index.data(ChatUserModel::ChatUserItemRoles::NameRole).toString();

    // 2. 绘制未读消息数
    int unreadCount = index.data(ChatUserModel::ChatUserItemRoles::UnReadMessageCountRole).toInt();
    if (unreadCount > 0) {
        QRect unreadRect(avatarRect.right() - 10, avatarRect.top() - 5, 20, 20);
        painter->setBrush(Qt::red);
        painter->drawEllipse(unreadRect);

        painter->setPen(Qt::white);
        painter->setFont(QFont("Arial", 9, QFont::Bold));
        QString unreadText = unreadCount > 99 ? "99+" : QString::number(unreadCount);
        painter->drawText(unreadRect, Qt::AlignCenter, unreadText);
    }

    // 3. 绘制文本区域
    int textLeft = avatarRect.right() + margin;
    int textWidth = option.rect.width() - textLeft - margin - 80;  // 预留时间区域

    // 名称
    QRect nameRect(textLeft, option.rect.top() + margin, textWidth, 25);
    painter->setPen(option.palette.text().color());
    painter->setFont(QFont("Microsoft YaHei", 11, QFont::Bold));

    QString displayName = name;
    /*if (index.data(ChatUserModel::ChatUserItemRoles::IsStickyRole).toBool()) {
        displayName = "[置顶] " + name;
    }*/

    painter->drawText(nameRect, Qt::AlignLeft | Qt::AlignTop,
                     painter->fontMetrics().elidedText(displayName, Qt::ElideRight, nameRect.width()));

    // 最后消息
    QRect msgRect(textLeft, nameRect.bottom() + 5, textWidth, 20);
    painter->setFont(QFont("Microsoft YaHei", 9));
    painter->setPen(QColor(100, 100, 100));

    QString lastMsg = index.data(ChatUserModel::ChatUserItemRoles::LastMessageRole).toString();
    painter->drawText(msgRect, Qt::AlignLeft | Qt::AlignTop,
                     painter->fontMetrics().elidedText(lastMsg, Qt::ElideRight, msgRect.width()));

    // 4. 绘制时间
    QDateTime lastTime = index.data(ChatUserModel::ChatUserItemRoles::ChatTimeRole).toDateTime();
    QString timeText = formatTime(lastTime);

    QRect timeRect(option.rect.right() - margin - 80, nameRect.top(), 80, 25);
    painter->setFont(QFont("Microsoft YaHei", 9));
    painter->setPen(QColor(150, 150, 150));
    painter->drawText(timeRect, Qt::AlignRight | Qt::AlignTop, timeText);

    // 5. 绘制分隔线
    painter->setPen(QColor(240, 240, 240));
    painter->drawLine(option.rect.bottomLeft(), option.rect.bottomRight());

    painter->restore();
}

void ChatUserDelegate::drawAvatar(QPainter *painter,
                                     const QRect &rect,
                                     const QModelIndex &index, const QString icon) const {
    const int avatarSize = rect.width();
    // 1. 加载头像图片
    QPixmap originalAvatar = returnPixMapByUrl(icon);

    // 2. 如果图片加载成功
    if (!originalAvatar.isNull()) {
        // 缩放图片（保持宽高比）
        QPixmap scaledAvatar = originalAvatar.scaled(
            avatarSize, avatarSize,
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation
        );

        // 3. 绘制圆形头像
        // 使用剪裁路径（简单）
        QPainterPath clipPath;
        clipPath.addEllipse(rect);
        painter->save();
        painter->setClipPath(clipPath);

        // 居中绘制（因为保持宽高比）
        int offsetX = (avatarSize - scaledAvatar.width()) / 2;
        int offsetY = (avatarSize - scaledAvatar.height()) / 2;
        painter->drawPixmap(rect.left() + offsetX,
                           rect.top() + offsetY,
                           scaledAvatar);
        painter->restore();

        // 4. 绘制圆形边框
        painter->setPen(QPen(QColor("#D0D0D0"), 1));
        painter->setBrush(Qt::NoBrush);
        painter->drawEllipse(rect);

    } else {
        // 5. 图片加载失败：使用原来的绘制方案
        painter->setBrush(QColor("#4CAF50"));  // 绿色背景
        painter->setPen(Qt::NoPen);
        painter->drawEllipse(rect);
        // 绘制名字首字母
        QString name = index.data(ChatUserModel::ChatUserItemRoles::NameRole).toString();
        QString firstLetter = name.isEmpty() ? "?" : name.left(1).toUpper();
        painter->setPen(Qt::white);
        painter->setFont(QFont("Arial", avatarSize / 2.5, QFont::Bold));
        painter->drawText(rect, Qt::AlignCenter, firstLetter);
    }
}
