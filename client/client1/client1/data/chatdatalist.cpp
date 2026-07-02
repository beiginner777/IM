#include "chatdatalist.h"
#include "loadlocaldata.h"
#include "usermanager.h"
#include "fileuploadmsg.h"
#include "dialogs/imageviewerdialog.h"

ChatDataList::ChatDataList(QWidget *parent)
   : QWidget(parent)
   , isAppended(false)
   , is_loading_(false)
{
    model_ = new ChatDataModel();
    view_ = new ChatDataView();
    delegate_ = new ChatDataDelegate(view_);

    view_->setModel(model_);
    view_->setItemDelegate(delegate_);

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(15,15,15,15);
    layout->addWidget(view_);

    connect(model_, &ChatDataModel::signalAppendChatDataItem,[=]() {
         view_->scrollToBottom();
    });

    connect(LoadLocalData::GetInstance().get(),&LoadLocalData::signalNotifyIsLoadMoreMsg,this,&ChatDataList::slotNotifyIsLoadMoreMsg);

    connect(FileUploadMsg::GetInstance().get(), &FileUploadMsg::signalUpdateDownloadProgress, model_, &ChatDataModel::slotUpdateDownloadStatus);
    connect(FileUploadMsg::GetInstance().get(), &FileUploadMsg::signalUpdateUploadProgress, model_, &ChatDataModel::slotUpdateUploadStatus);

    connect(delegate_, &ChatDataDelegate::signalChangeTransStatus, model_, &ChatDataModel::slotChangeTransStatus);
}

ChatDataList::~ChatDataList()
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

void ChatDataList::appendChatItem(std::shared_ptr<ChatDataBase> data)
{
    model_->appendChatDataItem(data);
}

void ChatDataList::prependChatItem(std::shared_ptr<ChatDataBase> data)
{
    model_->insertChatDataItem(data, false);
}

void ChatDataList::prependChatItems(std::vector<std::shared_ptr<ChatDataBase> > datas)
{
    model_->insertChatDataItems(datas);
}

void ChatDataList::resetAllItems(std::shared_ptr<ChatThreadData> cur_thread_data)
{
    model_->resetChatDataItems(cur_thread_data);
}

void ChatDataList::updateMsgStatus(QString unique_id, MsgStatus status)
{
    model_->updateItemStatus(unique_id, status);
}

void ChatDataList::updateMsgChatTime(QString unique_id, QDateTime chat_time)
{
    model_->updateMsgChatTime(unique_id, chat_time);
}

void ChatDataList::slotNotifyIsLoadMoreMsg(int thread_id, bool is_more)
{
    is_load_more_chatMsg_[thread_id] = is_more;
    view_->SetIsLoadingMoreMsg(thread_id,is_more);
}

ChatDataModel::ChatDataModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

ChatDataModel::~ChatDataModel()
{
}

void ChatDataModel::insertChatDataItem(std::shared_ptr<ChatDataBase> data, bool is_show_time)
{
    qDebug() << " ============================= INSERT INTO CHATVIEW ====================================== ";

    beginInsertRows(QModelIndex(), 0, 0);

    const int row = 0;
    int diff_seconds_ = 0;
    if(items_.size() > 0){
        ChatDataItem front_item = items_[0];
        diff_seconds_ = data->GetChatTime().secsTo(front_item.chat_time_);
    }
    ChatDataItem item;
    ChatRole role;
    int self_uid = UserManager::GetInstance()->getUid();
    int send_uid = data->GetSendUid();
    //qDebug() << "self_uid = " << self_uid << ", send_uid = " << send_uid;
    if(self_uid == send_uid){
        item.role_ = ChatRole::self;
        item.name_ = UserManager::GetInstance()->getName();
        item.icon_ = UserManager::GetInstance()->getIcon();
        item.msg_type_ = data->GetMsgType();
        item.content_ = data->GetContent(); // 文件标识符
        item.status_ = (MsgStatus)data->GetStatus();
        item.chat_time_ = data->GetChatTime();
        item.diffSeconds_ = diff_seconds_;
    }else{
        auto friend_info = UserManager::GetInstance()->getFriendById(send_uid);
        item.role_ = ChatRole::Other;
        item.name_ = friend_info->name_;
        item.icon_ = friend_info->icon_;
        item.msg_type_ = data->GetMsgType();
        item.content_ = data->GetContent(); // 文件的标识符
        item.status_ = (MsgStatus)data->GetStatus();
        item.chat_time_ = data->GetChatTime();
        item.diffSeconds_ = diff_seconds_;
    }
    QString unique_id = data->GetUniqueId();
    if(unique_id != ""){
        row_by_id_[unique_id] = row;
        item.unique_id_ = unique_id;
        if(data->GetMsgType() != CHAT_MSG_TYPE::TEXT_MSG){
            uuid_by_content_[data->GetContent()] = data->GetUniqueId(); // 文件唯一标识符 ：文件的uuid
            persist_[data->GetContent()] = PersistState();
        }
        //qDebug() << "[DEBUG] unique_name = " << data->GetContent() << ", unique_id = " << data->GetUniqueId() << " , row = " << row;
    }
    // 上传
    if(self_uid == send_uid && data->GetMsgType() != CHAT_MSG_TYPE::TEXT_MSG){
        FileTransferInfo trans_file_info;
        std::shared_ptr<ImageDataBase> file = std::dynamic_pointer_cast<ImageDataBase>(data);
        std::shared_ptr<MsgInfo> msg_info = file->msg_info_;
        // to do ... 需要去获取这个文件是否传输完成
        trans_file_info.transfer_status_ = msg_info->_state;

        trans_file_info.trans_size_ = msg_info->current_size_;
        trans_file_info.total_size_ = msg_info->total_size_;
// trans_file_info.progress_ = trans_file_info.trans_size_ * 100 / trans_file_info.total_size_;
        trans_file_info.local_path_ = msg_info->text_or_url_;
        // to do ... mime_type_ =

        item.file_ = trans_file_info;

        if(msg_info->_state == TRANSFER_STATE::Uploading){
            emit FileUploadMsg::GetInstance()->signalContinueUploadFile(data->GetContent());
        }

    }
    // 下载
    else if(self_uid != send_uid && data->GetMsgType() != CHAT_MSG_TYPE::TEXT_MSG){
        //auto download_file = UserManager::GetInstance()->get_download_file(data->GetContent());
        FileTransferInfo trans_file_info;
        std::shared_ptr<ImageDataBase> file = std::dynamic_pointer_cast<ImageDataBase>(data);
        std::shared_ptr<MsgInfo> msg_info = file->msg_info_;
        //msg_info->_state = download_file->state_;
        //msg_info->current_size_ = download_file->trans_size_;
        //msg_info->total_size_  = download_file->total_size_;

        // to do ... 需要去获取这个文件是否下载完成
        trans_file_info.transfer_status_ = msg_info->_state;
        //        if(msg_info->_state == TRANSFER_STATE::Download_Finish){
//            trans_file_info.transfer_status_ = TRANSFER_STATE::Download_Finish;
//        }else{
//            trans_file_info.transfer_status_ = TRANSFER_STATE::Downloading; // to do ... 也有可能是暂停状态
//        }
        trans_file_info.trans_size_ = msg_info->current_size_;
        trans_file_info.total_size_ = msg_info->total_size_;
//        trans_file_info.progress_ = trans_file_info.trans_size_ * 100 / trans_file_info.total_size_;
        trans_file_info.local_path_ = msg_info->text_or_url_;
        // to do ... mime_type_ =

        item.file_ = trans_file_info;

        if(msg_info->_state == TRANSFER_STATE::Downloading){
            emit FileUploadMsg::GetInstance()->signalContinueDownloadFile(data->GetContent());
        }
    }
    if(is_show_time){
        item.diffSeconds_ = INT_MAX;
    }
    items_.push_front(item);

    // 头插导致所有已有 row +1：简单做法是整体平移一次
    for (auto it = row_by_id_.begin(); it != row_by_id_.end(); ++it) {
        it.value() += 1;
    }
    endInsertRows();
}

void ChatDataModel::insertChatDataItems(std::vector<std::shared_ptr<ChatDataBase> > datas)
{
    for(int i = 0; i < datas.size(); ++i){
        bool is_show_time = false;
        if(i == 0){
            is_show_time = true;
        }
        this->insertChatDataItem(datas[i], is_show_time);
    }
}

void ChatDataModel::appendChatDataItem(std::shared_ptr<ChatDataBase> data)
{
    // qDebug() << " ============================= APPEND INTO CHATVIEW ====================================== ";

    const int row = int(items_.size());
    beginInsertRows(QModelIndex(), row, row);

    int diff_seconds_ = 0;
    if(items_.size() > 0){
        ChatDataItem last_item = items_[items_.size() - 1];
        diff_seconds_ = last_item.chat_time_.secsTo(data->GetChatTime());
    }
    ChatDataItem item;
    ChatRole role;
    int self_uid = UserManager::GetInstance()->getUid();
    int send_uid = data->GetSendUid();
    //qDebug() << "self_uid = " << self_uid << ", send_uid = " << send_uid;
    if(self_uid == send_uid){
        item.role_ = ChatRole::self;
        item.name_ = UserManager::GetInstance()->getName();
        item.icon_ = UserManager::GetInstance()->getIcon();
        item.msg_type_ = data->GetMsgType();
        item.content_ = data->GetContent(); // 文件标识符
        item.status_ = (MsgStatus)data->GetStatus();
        item.chat_time_ = data->GetChatTime();
        item.diffSeconds_ = diff_seconds_;
    }else{
        auto friend_info = UserManager::GetInstance()->getFriendById(send_uid);
        item.role_ = ChatRole::Other;
        item.name_ = friend_info->name_;
        item.icon_ = friend_info->icon_;
        item.msg_type_ = data->GetMsgType();
        item.content_ = data->GetContent(); // 文件的标识符
        item.status_ = (MsgStatus)data->GetStatus();
        item.chat_time_ = data->GetChatTime();
        item.diffSeconds_ = diff_seconds_;
    }
    QString unique_id = data->GetUniqueId();
    if(unique_id != ""){
        row_by_id_[unique_id] = row;
        item.unique_id_ = unique_id;
        if(data->GetMsgType() != CHAT_MSG_TYPE::TEXT_MSG){
            uuid_by_content_[data->GetContent()] = data->GetUniqueId(); // 文件唯一标识符 ：文件的uuid
            persist_[data->GetContent()] = PersistState();
        }
        //qDebug() << "[DEBUG] unique_name = " << data->GetContent() << ", unique_id = " << data->GetUniqueId() << " , row = " << row;
    }
    // 上传的图片/文件
    if(self_uid == send_uid && data->GetMsgType() != CHAT_MSG_TYPE::TEXT_MSG){
        FileTransferInfo trans_file_info;
        std::shared_ptr<ImageDataBase> file = std::dynamic_pointer_cast<ImageDataBase>(data);
        std::shared_ptr<MsgInfo> msg_info = file->msg_info_;
        // to do ... 需要去获取这个文件是否传输完成
        trans_file_info.transfer_status_ = msg_info->_state;

        trans_file_info.trans_size_ = msg_info->current_size_;
        trans_file_info.total_size_ = msg_info->total_size_;
// trans_file_info.progress_ = trans_file_info.trans_size_ * 100 / trans_file_info.total_size_;
        trans_file_info.local_path_ = msg_info->text_or_url_;
        // to do ... mime_type_ =

        item.file_ = trans_file_info;

        if(msg_info->_state == TRANSFER_STATE::Uploading){
            emit FileUploadMsg::GetInstance()->signalContinueUploadFile(data->GetContent());
        }

    }
    // 下载的图片/文件
    else if(self_uid != send_uid && data->GetMsgType() != CHAT_MSG_TYPE::TEXT_MSG){
//        auto download_file = UserManager::GetInstance()->get_download_file(data->GetContent());
//        FileTransferInfo trans_file_info;
//        std::shared_ptr<ImageDataBase> file = std::dynamic_pointer_cast<ImageDataBase>(data);
//        std::shared_ptr<MsgInfo> msg_info = file->msg_info_;
//        msg_info->_state = download_file->state_;
//        msg_info->current_size_ = download_file->trans_size_;
//        msg_info->total_size_  = download_file->total_size_;

//        std::shared_ptr<MsgInfo> msg_info = UserManager::GetInstance()->get_trans_file(data->GetContent());
//        msg_info->current_size_ = download_file->trans_size_;
//        msg_info->total_size_  = download_file->total_size_;

        FileTransferInfo trans_file_info;
        std::shared_ptr<ImageDataBase> file = std::dynamic_pointer_cast<ImageDataBase>(data);
        std::shared_ptr<MsgInfo> msg_info = file->msg_info_;
        // to do ... 需要去获取这个文件是否下载完成
        trans_file_info.transfer_status_ = msg_info->_state;
        //        if(msg_info->_state == TRANSFER_STATE::Download_Finish){
//            trans_file_info.transfer_status_ = TRANSFER_STATE::Download_Finish;
//        }else{
//            trans_file_info.transfer_status_ = TRANSFER_STATE::Downloading; // to do ... 也有可能是暂停状态
//        }
        trans_file_info.trans_size_ = msg_info->current_size_;
        trans_file_info.total_size_ = msg_info->total_size_;
//        trans_file_info.progress_ = trans_file_info.trans_size_ * 100 / trans_file_info.total_size_;
        trans_file_info.local_path_ = msg_info->text_or_url_;
        // to do ... mime_type_ =

        item.file_ = trans_file_info;

        if(msg_info->_state == TRANSFER_STATE::Downloading){
            emit FileUploadMsg::GetInstance()->signalContinueDownloadFile(data->GetContent());
        }

    }
    items_.push_back(item);
    endInsertRows();
    emit signalAppendChatDataItem();
}

void ChatDataModel::appendChatDataItems(QVector<std::shared_ptr<ChatDataBase> > datas)
{
    for(int i = 0; i < datas.size(); ++i){
        this->appendChatDataItem(datas[i]);
    }
}

void ChatDataModel::resetChatDataItems(std::shared_ptr<ChatThreadData> thread_data)
{
    items_.clear();
    row_by_id_.clear();

    beginResetModel();
    // 服务器回复的消息
    auto thread_data_map = thread_data->GetMsgMap();
    auto thread_data_vector = thread_data_map.values().toVector();
    appendChatDataItems(thread_data_vector);
    // 服务器还未回复的消息
    auto un_thread_data_map = thread_data->GetUnrspMsg();
    auto un_thread_data_vector = un_thread_data_map.values().toVector();
    appendChatDataItems(un_thread_data_vector);
    endResetModel();
}

void ChatDataModel::updateItemStatus(QString unique_id, MsgStatus status)
{
    auto it = row_by_id_.find(unique_id);
    if (it == row_by_id_.end()){
        qDebug() << "[ERROR]: can not find ChatDataItem by unique_id.";
        return;
    }
    const int row = it.value();
    if (row < 0 || row >= int(items_.size())){
        qDebug() << "[ERROR]: row is illegal.";
        return;
    }
    ChatDataItem &item = items_[row];
    item.status_ = status;
    const QModelIndex idx = index(row, 0);
    emit dataChanged(idx, idx, { StatusRole });
}

void ChatDataModel::updateMsgChatTime(QString unique_id, QDateTime chat_time)
{
    auto it = row_by_id_.find(unique_id);
    if (it == row_by_id_.end()){
        qDebug() << "[ERROR]: can not find ChatDataItem by unique_id.";
        return;
    }
    const int row = it.value();
    if (row < 0 || row >= int(items_.size())){
        qDebug() << "[ERROR]: row is illegal.";
        return;
    }
    ChatDataItem &item = items_[row];
    item.chat_time_ = chat_time;
    const QModelIndex idx = index(row, 0);
    emit dataChanged(idx, idx, { ChatTimeRole });
}

void ChatDataModel::updateFileTransfer(const QString &unique_id, qint64 trans, qint64 total, TRANSFER_STATE st, QString error_reason)
{
    auto it = row_by_id_.find(unique_id);
    if (it == row_by_id_.end()){
        qDebug() << "[ERROR]: can not find ChatDataItem by unique_id.";
        return;
    }
    const int row = it.value();
    if (row < 0 || row >= int(items_.size())){
        qDebug() << "[ERROR]: row is illegal.";
        return;
    }
    ChatDataItem &item = items_[row];
    item.file_.trans_size_ = trans;
    item.file_.total_size_ = total;
    item.file_.transfer_status_ = st;
    int percent = (total > 0) ? int(double(trans) * 100.0 / double(total)) : -1;
    item.file_.progress_ = std::clamp(percent, 0, 100);
    if(st == TRANSFER_STATE::Upload_Failed){
        item.file_.error_ = error_reason;
    }
    const QModelIndex idx = index(row, 0);
    emit dataChanged(idx, idx);
}

void ChatDataModel::slotUpdateUploadStatus(QString unique_name)
{
    // 获取文件的uuid
    QString unique_id = uuid_by_content_[unique_name];
    // 获取文件的行号
    int file_row = row_by_id_[unique_id];
    // 获取传输的信息
    std::shared_ptr<MsgInfo> trans_info = UserManager::GetInstance()->get_trans_file(unique_name);
    int trans_size = trans_info->current_size_;
    int total_size = trans_info->total_size_;
    TRANSFER_STATE trans_status = trans_info->_state;
    //to do ... ERROR_REASON
    QString error = "";

    // 更新这个图标
    this->updateFileTransfer(unique_id, trans_size, total_size, trans_status, error);

    const QModelIndex idx = index(file_row, 0);

    // 决定是否写库（节流）
    int percent = (total_size > 0) ? int(double(trans_size) * 100.0 / double(total_size)) : 0;
    percent = std::clamp(percent, 0, 100);
    auto &ps = persist_[unique_id];
    qint64 nowMs = QDateTime::currentMSecsSinceEpoch();

    bool statusChanged = (trans_status != ps.lastStatus);
    bool timeDue = (nowMs - ps.lastSavedMs >= 1000);
    bool percentDue = (ps.lastSavedPercent < 0) || (percent - ps.lastSavedPercent >= 2);

    if (statusChanged || timeDue || percentDue) {
        // 写数据库（建议放到工作线程）
        emit LoadLocalData::GetInstance()->signalSaveTransferToDb(unique_name, trans_size, total_size, int(trans_status),int(TRANSFER_TYPE::Upload),error);
        ps.lastSavedMs = nowMs;
        ps.lastSavedPercent = percent;
        ps.lastStatus = trans_status;
    }
}

void ChatDataModel::slotUpdateDownloadStatus(QString unique_name)
{
    qDebug() << "Update Download File Progress . . .";

    // 获取文件的uuid
    QString unique_id = uuid_by_content_[unique_name];
    // 获取文件的行号
    int file_row = row_by_id_[unique_id];
    // 获取传输的信息
//    std::shared_ptr<DownloadFileInfo> trans_info = UserManager::GetInstance()->get_download_file(unique_name);
//    int trans_size = trans_info->trans_size_;
//    int total_size = trans_info->total_size_;
//    TRANSFER_STATE trans_status = trans_info->state_;
    std::shared_ptr<MsgInfo> trans_info = UserManager::GetInstance()->get_trans_file(unique_name);
    int trans_size = trans_info->current_size_;
    int total_size = trans_info->total_size_;
    TRANSFER_STATE trans_status = trans_info->_state;
    QString error = "";
    // 更新这个图标
    this->updateFileTransfer(unique_id, trans_size, total_size, trans_status, error);
    const QModelIndex idx = index(file_row, 0);
    emit dataChanged(idx, idx);

    // 决定是否写库（节流）
    int percent = (total_size > 0) ? int(double(trans_size) * 100.0 / double(total_size)) : 0;
    percent = std::clamp(percent, 0, 100);
    auto &ps = persist_[unique_id];
    qint64 nowMs = QDateTime::currentMSecsSinceEpoch();

    bool statusChanged = (trans_status != ps.lastStatus);
    bool timeDue = (nowMs - ps.lastSavedMs >= 1000);
    bool percentDue = (ps.lastSavedPercent < 0) || (percent - ps.lastSavedPercent >= 2);

    if (statusChanged || timeDue || percentDue) {
        // 写数据库（建议放到工作线程）
        emit LoadLocalData::GetInstance()->signalSaveTransferToDb(unique_name, trans_size, total_size, int(trans_status),int(TRANSFER_TYPE::Download),error);
        ps.lastSavedMs = nowMs;
        ps.lastSavedPercent = percent;
        ps.lastStatus = trans_status;
    }
}

void ChatDataModel::slotChangeTransStatus(QString unique_name)
{
    // 获取文件消息框的uuid
    QString unique_id = uuid_by_content_[unique_name];
    // 获取文件消息框的行号
    int file_row = row_by_id_[unique_id];
    ChatDataItem &item = items_[file_row];
    TRANSFER_STATE state = item.file_.transfer_status_;
    TRANSFER_STATE new_state = TRANSFER_STATE::None;
    if(state == TRANSFER_STATE::Download_Finish || TRANSFER_STATE::Upload_Finish){
        qDebug() << "文件：" << unique_name << " 已经传输完成.";
    }
    if(state == TRANSFER_STATE::Downloading){
        qDebug() << "文件：" << unique_name << " 暂停下载";
        new_state = TRANSFER_STATE::Download_Paused;
    }
    if(state == TRANSFER_STATE::Uploading){
        qDebug() << "文件：" << unique_name << " 暂停上传.";
        new_state = TRANSFER_STATE::Upload_Paused;
    }
    if(state == TRANSFER_STATE::Download_Failed){
        qDebug() << "文件：" << unique_name << " 重新下载.";
        new_state = TRANSFER_STATE::Downloading;
    }
    if(state == TRANSFER_STATE::Upload_Failed){
        qDebug() << "文件: " << unique_name << " 重新上传";
        new_state = TRANSFER_STATE::Uploading;
    }
    if(state == TRANSFER_STATE::Download_Paused){
        qDebug() << "文件：" << unique_name << " 继续下载";
        new_state = TRANSFER_STATE::Downloading;
    }
    if(state == TRANSFER_STATE::Upload_Paused){
        qDebug() << "文件：" << unique_name << " 继续上传";
        new_state = TRANSFER_STATE::Uploading;
    }

    // 修改UserManager传输状态
    std::shared_ptr<MsgInfo> trans_info = UserManager::GetInstance()->get_trans_file(unique_name);
    trans_info->_state = new_state;
    // 修改Model传输状态
    item.file_.transfer_status_ = new_state;
    const QModelIndex idx = index(file_row, 0);
    emit dataChanged(idx, idx);

    if(new_state == TRANSFER_STATE::Uploading){
        // 继续上传
        emit FileUploadMsg::GetInstance()->signalContinueUploadFile(unique_name);
    }else if(new_state == TRANSFER_STATE::Downloading){
        // 继续下载
        emit FileUploadMsg::GetInstance()->signalContinueDownloadFile(unique_name);
    }
}

int ChatDataModel::rowCount(const QModelIndex &parent) const
{
    return items_.size();
}

QVariant ChatDataModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid() || index.row() >= items_.size()){
        return QVariant();
    }

    const ChatDataItem& item = items_[index.row()];

    switch (role) {
        case ChatDataModel::ChatDataItemRoles::ChatUserRole:
            return item.role_;
        case ChatDataModel::ChatDataItemRoles::NameRole:
            return item.name_;
        case ChatDataModel::ChatDataItemRoles::IconRole:
            return item.icon_;
        case ChatDataModel::ChatDataItemRoles::MsgTypeRole:
            return item.msg_type_;
        case ChatDataModel::ChatDataItemRoles::ContentRole:
            return item.content_;
        case ChatDataModel::ChatDataItemRoles::StatusRole:
            return item.status_;
        case ChatDataModel::ChatDataItemRoles::ChatTimeRole:
            return item.chat_time_;
        case ChatDataModel::ChatDataItemRoles::DiffSecondsRole:
            return item.diffSeconds_;
        case ChatDataModel::ChatDataItemRoles::UniqueIdRole:
            return item.unique_id_;      
        case FileTransSizeRole:
            return item.file_.trans_size_;
        case FileTotalSizeRole:
            return item.file_.total_size_;
        case FileTransferStatusRole:
            return item.file_.transfer_status_;
        case FileProgressRole:
            return item.file_.progress_;
        case FileLocalPathRole:
            return item.file_.local_path_;
        case FileTransFailedRole:
            return item.file_.error_;
//        case IsTurnToGrey:
//            return item.is_trun_grey_;
//        case IsPaused:
//            return item.is_paused_;
//        case IsTransfer:
//            return item.is_transfer_;
    default:
        return QVariant();
    }
}

ChatDataDelegate::ChatDataDelegate(ChatDataView *view, QObject *parent)
    : QStyledItemDelegate(parent), m_view(view)
{
}

ChatDataDelegate::~ChatDataDelegate()
{
}

QSize ChatDataDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    const ChatRole user = static_cast<ChatRole>(
        index.data(ChatDataModel::ChatDataItemRoles::ChatUserRole).toInt()
    );
    const int diffSeconds = index.data(ChatDataModel::ChatDataItemRoles::DiffSecondsRole).toInt();

    const CHAT_MSG_TYPE msg_type = static_cast<CHAT_MSG_TYPE>(
        index.data(ChatDataModel::ChatDataItemRoles::MsgTypeRole).toInt()
    );

    // 常量要和 paint 保持一致
    const int margin = 5;
    const int avatarSize = 40;
    const int nameHeight = 20;
    const int nameToBubbleGap = 5;

    // ⚠️ 与 paint 保持一致：你 paint 现在是 diffSeconds > 300 才画时间条
    const int timeSepH = (diffSeconds > 300) ? 25 : 0;

    // 系统消息
    if (user == ChatRole::System) {
        int h = 30 + timeSepH;
        return QSize(option.rect.width(), h);
    }

    // 气泡最大宽度（和 paint 一致）
    const int maxBubbleWidth = int(option.rect.width() * 0.7);

    // 内容字体（和 paint 一致）
    const QFont font("Microsoft YaHei", 14);

    // 每种类型不同 padding
    const QMargins pad = bubblePaddingForType(msg_type);

    // 内容区最大宽度（不含 padding）
    const int maxContentWidth = maxBubbleWidth - (pad.left() + pad.right());

    // 按类型测量内容区尺寸（不含 padding）
    QSize contentSize = measureContentSize(index, msg_type, font, maxContentWidth);

    // bubble 高度 = 内容高度 + padding
    int bubbleH = contentSize.height() + pad.top() + pad.bottom();
    bubbleH = std::max(bubbleH, 40); // 最小高度兜底

    // bubble 宽度（虽然 sizeHint 主要关心高度，但也顺便算对齐 paint）
    // 这里不强制使用，但保留让逻辑一致
    int bubbleW = contentSize.width() + pad.left() + pad.right();
    bubbleW = std::min(bubbleW, maxBubbleWidth);
    bubbleW = std::max(bubbleW, 40);
    Q_UNUSED(bubbleW);

    // 总高度
    int contentH = timeSepH + margin + nameHeight + nameToBubbleGap + bubbleH + margin;
    int avatarH  = timeSepH + margin + avatarSize + margin;

    return QSize(option.rect.width(), std::max(contentH, avatarH));
}

void ChatDataDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    // 绘制背景（选中/悬停效果）
    /*if (option.state & QStyle::State_Selected) {
        painter->fillRect(option.rect, option.palette.highlight().color().lighter(160));
    }else if (option.state & QStyle::State_MouseOver) {
        painter->fillRect(option.rect, QColor(245, 245, 245));
    }*/

    // 获取数据
    int userType = index.data(ChatDataModel::ChatDataItemRoles::ChatUserRole).toInt();
    ChatRole user = static_cast<ChatRole>(userType);
    QString name = index.data(ChatDataModel::ChatDataItemRoles::NameRole).toString();
    QString icon = index.data(ChatDataModel::ChatDataItemRoles::IconRole).toString();
    QString content = index.data(ChatDataModel::ChatDataItemRoles::ContentRole).toString();
    int status = index.data(ChatDataModel::ChatDataItemRoles::StatusRole).toInt();
    QDateTime time = index.data(ChatDataModel::ChatDataItemRoles::ChatTimeRole).toDateTime();
    int diffSeconds = index.data(ChatDataModel::ChatDataItemRoles::DiffSecondsRole).toInt();
    QString unique_id = index.data(ChatDataModel::ChatDataItemRoles::UniqueIdRole).toString();

    // 根据消息类型绘制Item
    const int margin = 5; // 每条消息的主体与边界的间隔
    const int avatarSize = 40; // 头像大小

    // 系统消息（居中显示）
    if (user == ChatRole::System) {
        painter->setPen(QColor(150, 150, 150));
        painter->setFont(QFont("Microsoft YaHei", 9));
        painter->drawText(option.rect, Qt::AlignCenter, content);
        painter->restore();
        return;
    }

    // 绘制时间分隔线（如果与上条消息间隔较大）
    if (diffSeconds > 300) {
        painter->setPen(QColor(150, 150, 150));
        painter->setFont(QFont("Microsoft YaHei", 8));
        QString timeStr = time.toString("MM-dd HH:mm");
        QRect timeRect(option.rect.left() + option.rect.width() / 2 - 50,
                      option.rect.top() + 5, 100, 20);
        painter->fillRect(timeRect, option.palette.base().color());
        painter->drawText(timeRect, Qt::AlignCenter, timeStr);
    }

    // 判断是自己还是对方
    bool isSelf = (user == ChatRole::self);
    QRect avatarRect;
    if (isSelf) {
        avatarRect = QRect(option.rect.right() - margin - avatarSize,
                          option.rect.top() + margin,
                          avatarSize, avatarSize);
    } else {
        avatarRect = QRect(option.rect.left() + margin,
                          option.rect.top() + margin,
                          avatarSize, avatarSize);
    }
    drawAvatar(painter, avatarRect, index, icon);

    // 绘制名字
    QRect nameRect;
    if (isSelf) {
        nameRect = QRect(option.rect.left() + margin,
                        option.rect.top() + margin,
                        option.rect.width() - 2*margin - avatarSize - 20, 20);
    } else {
        nameRect = QRect(avatarRect.right() + margin,
                        option.rect.top() + margin,
                        option.rect.width() - 2*margin - avatarSize - 20, 20);
    }

    painter->setPen(isSelf ? QColor(100, 100, 100) : QColor(33, 150, 243));
    painter->setFont(QFont("Microsoft YaHei", 7, isSelf ? QFont::Normal : QFont::Bold));
    painter->drawText(nameRect, isSelf ? Qt::AlignRight : Qt::AlignLeft, name);

    // 绘制消息气泡
    QRect bubbleRect;

    CHAT_MSG_TYPE msg_type = msgTypeOf(index);

    QFont font("Microsoft YaHei", 14);
    QMargins pad = bubblePaddingForType(msg_type);

    int maxBubbleWidth = int(option.rect.width() * 0.7);
    int maxContentWidth = maxBubbleWidth - (pad.left() + pad.right());

    // 测量内容区尺寸（不包含 padding）
    QSize contentSize = measureContentSize(index, msg_type, font, maxContentWidth);

    // 计算 bubble 的 W/H（包含 padding）
    int bubbleW = std::min(contentSize.width() + pad.left() + pad.right(), maxBubbleWidth);
    bubbleW = std::max(bubbleW, 40);

    int bubbleH = contentSize.height() + pad.top() + pad.bottom();
    bubbleH = std::max(bubbleH, 40);

    // 最小气泡尺寸兜底（按类型）
    auto minBubbleSizeForType = [](CHAT_MSG_TYPE t)->QSize {
        switch (t) {
        case PIC_MSG:  return QSize(100, 80);
        case FILE_MSG: return QSize(200, 56);
        case TEXT_MSG:
        default:       return QSize(40, 40);
        }
    };

    QSize minB = minBubbleSizeForType(msg_type);
    bubbleW = std::max(bubbleW, minB.width());
    bubbleH = std::max(bubbleH, minB.height());

    if (isSelf) {
        bubbleRect = QRect(avatarRect.left() - margin - bubbleW,
                           nameRect.bottom() + 5,
                           bubbleW, bubbleH);
    } else {
        bubbleRect = QRect(avatarRect.right() + margin,
                           nameRect.bottom() + 5,
                           bubbleW, bubbleH);
    }

    // 绘制气泡背景
    painter->setBrush(isSelf ? QColor(158,234,106) : QColor("#FFFFFF"));
    painter->setPen(QPen(QColor(200, 200, 200), 1));
    painter->drawRoundedRect(bubbleRect, 8, 8);

    // 绘制消息内容（使用 TextWordWrap 自动换行）
    painter->setPen(Qt::black);
    painter->setFont(font);
    QRect contentRect = bubbleRect.adjusted(pad.left(), pad.top(), -pad.right(), -pad.bottom());

    //drawContent(painter, index, msg_type, font, contentRect);
    painter->save();
    {
        QPainterPath clip;
        clip.addRoundedRect(bubbleRect, 8, 8);
        painter->setClipPath(clip);

        QRect contentRect = bubbleRect.adjusted(pad.left(), pad.top(), -pad.right(), -pad.bottom());
        if (contentRect.width() < 1 || contentRect.height() < 1) {
            painter->restore();
            return;
        }
        drawContent(painter, index, msg_type, font, contentRect);
    }
    painter->restore();

    // 绘制时间
    QString timeStr = time.toString("HH:mm");
    QRect timeRect;
    if (isSelf) {
        timeRect = QRect(bubbleRect.left() - 60, bubbleRect.bottom() - 20, 55, 20);
    } else {
        timeRect = QRect(bubbleRect.right() + 5, bubbleRect.bottom() - 20, 55, 20);
    }

    painter->setPen(QColor(150, 150, 150));
    painter->setFont(QFont("Microsoft YaHei", 8));
    painter->drawText(timeRect, isSelf ? Qt::AlignRight : Qt::AlignLeft, timeStr);

    // 绘制状态（仅自己消息）
    if (isSelf) {
        QString statusText;
        QColor statusColor;
        switch (static_cast<MsgStatus>(status)) {
        case MsgStatus::UN_READ: statusText = "●"; statusColor = QColor(255, 165, 0); break;
        case MsgStatus::READED: statusText = "✓"; statusColor = QColor(150, 150, 150); break;
        case MsgStatus::SEND_FAILED: statusText = "!"; statusColor = QColor(255, 0, 0); break;
        }
        qDebug() << "[Debug] statusText = " << statusText << ", context = " << content;
        QRect statusRect(timeRect.left() - 25, timeRect.top(), 20, 20);
        painter->setPen(statusColor);
        painter->setFont(QFont("Arial", 10, QFont::Bold));
        painter->drawText(statusRect, Qt::AlignCenter, statusText);
    }

    // === hover 气泡变灰逻辑 ===
    bool bubbleHover = false;

    if (m_view && m_view->hoverIndex() == index) {
        QRect itemRect = option.rect;
        QPoint mousePos = m_view->hoverPos();

        // 鼠标在 viewport 坐标，要换算成 item 内坐标
        QPoint mouseInItem = mousePos - itemRect.topLeft();
        // 将气泡区域转换为 item内的区域
        QRect bubbleInItem = bubbleRect.translated(-itemRect.topLeft());

        if (bubbleInItem.contains(mouseInItem)) {
            bubbleHover = true;
        }
    }

    if (bubbleHover) {
        painter->setBrush(QColor(0, 0, 0, 60));
        painter->setPen(Qt::NoPen);
        painter->drawRoundedRect(bubbleRect, 8, 8);
    }

    // ====== 改变文件传输的状态 =====
    bool trans_state = false;
    if (m_view && m_view->pressIndex() == index && msg_type == FILE_MSG) {
        QRect itemRect = option.rect;
        QPoint mousePos = m_view->hoverPos();
        // 鼠标在 viewport 坐标，要换算成 item 内坐标
        QPoint mouseInItem = mousePos - itemRect.topLeft();
        // 将气泡区域转换为 item内的区域
        QRect bubbleInItem = bubbleRect.translated(-itemRect.topLeft());
        if (bubbleInItem.contains(mouseInItem)) {
            trans_state = true;
        }
    }

    if (trans_state) {
        // 修改model去修改
        emit signalChangeTransStatus(content);
        m_view->setPressIndex(); // 设置为空，防止下次渲染出现错误
    }

    // ====== 查看图片 ======
    bool picture_view = false;
    if (m_view && m_view->pressIndex() == index && msg_type == PIC_MSG) {
        QRect itemRect = option.rect;
        QPoint mousePos = m_view->hoverPos();
        // 鼠标在 viewport 坐标，要换算成 item 内坐标
        QPoint mouseInItem = mousePos - itemRect.topLeft();
        // 将气泡区域转换为 item内的区域
        QRect bubbleInItem = bubbleRect.translated(-itemRect.topLeft());
        if (bubbleInItem.contains(mouseInItem)) {
            picture_view = true;
        }
    }

    if (picture_view) {
        // 将图片放大查看
        const QString local_path = index.data(ChatDataModel::ChatDataItemRoles::FileLocalPathRole).toString();
        // 交给 ChatDataList来显示，因为
        /*  在 paint() 里只能：
            ✔ 画
            ✔ 计算
            ✔ 读取 model 数据
            不能：
            ❌ 弹窗口
            ❌ 修改 model
            ❌ emit 改变 UI 的信号
            ❌ 创建 QWidget
            ❌ 创建 QPainter(this)
         */
        PictureView(local_path);
    }

    painter->restore();
}

void ChatDataDelegate::drawAvatar(QPainter *painter, const QRect &rect, const QModelIndex &index, const QString icon) const
{
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
        QString name = index.data(ChatDataModel::ChatDataItemRoles::NameRole).toString();
        QString firstLetter = name.isEmpty() ? "?" : name.left(1).toUpper();
        painter->setPen(Qt::white);
        painter->setFont(QFont("Arial", avatarSize / 2.5, QFont::Bold));
        painter->drawText(rect, Qt::AlignCenter, firstLetter);
    }
}

QSizeF ChatDataDelegate::measureTextLayout(const QString &text, const QFont &font, qreal maxWidth) const
{
    QTextOption opt;
    opt.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere); // 比 WordWrap 更稳，emoji/无空格文本也能换行

    QTextLayout layout(text, font);
    layout.setTextOption(opt);

    layout.beginLayout();
    qreal height = 0.0;
    qreal width  = 0.0;

    while (true) {
        QTextLine line = layout.createLine();
        if (!line.isValid()) break;
        line.setLineWidth(maxWidth);
        line.setPosition(QPointF(0, height));
        height += line.height();
        width = std::max(width, line.naturalTextWidth());
    }
    layout.endLayout();

    return QSizeF(width, height);
}

CHAT_MSG_TYPE ChatDataDelegate::msgTypeOf(const QModelIndex& index) const
{
    const int t = index.data(ChatDataModel::ChatDataItemRoles::MsgTypeRole).toInt();
    return static_cast<CHAT_MSG_TYPE>(t);
}

QMargins ChatDataDelegate::bubblePaddingForType(CHAT_MSG_TYPE type) const
{
    switch (type) {
    case TEXT_MSG:
        return QMargins(15, 8, 15, 8);
    case PIC_MSG:
        // 图片想更大：padding 小一些
        return QMargins(0, 0, 0, 0);
    case FILE_MSG:
        // 文件卡片通常更“卡片化”
        return QMargins(15, 8, 15, 8);
    default:
        return QMargins(10, 8, 10, 8);
    }
}

QSize ChatDataDelegate::measureContentSize(const QModelIndex& index,
                                          CHAT_MSG_TYPE type,
                                          const QFont& font,
                                          int maxContentWidth) const
{
    switch (type) {
    case TEXT_MSG:
        return measureTextContent(index, font, maxContentWidth);
    case PIC_MSG:
        return measurePicContent(index, maxContentWidth);
    case FILE_MSG:
        return measureFileContent(index, font, maxContentWidth);
    default:
        return measureTextContent(index, font, maxContentWidth);
    }
}

void ChatDataDelegate::drawContent(QPainter* p,
                                  const QModelIndex& index,
                                  CHAT_MSG_TYPE type,
                                  const QFont& font,
                                  const QRect& contentRect) const
{
    switch (type) {
    case TEXT_MSG:
        drawTextContent(p, index, font, contentRect);
        return;
    case PIC_MSG:
        drawPicContent(p, index, contentRect);
        return;
    case FILE_MSG:
        drawFileContent(p, index, font, contentRect);
        return;
    default:
        drawTextContent(p, index, font, contentRect);
        return;
    }
}

QSize ChatDataDelegate::measureTextContent(const QModelIndex& index,
                                          const QFont& font,
                                          int maxContentWidth) const
{
    const QString content = index.data(ChatDataModel::ChatDataItemRoles::ContentRole).toString();

    QSizeF sz = measureTextLayout(content, font, maxContentWidth);

    int w = int(std::ceil(sz.width()));
    int h = int(std::ceil(sz.height()));

    // 兜底：空文本也要有一行高度
    w = std::max(w, 10);
    h = std::max(h, QFontMetrics(font).height());

    return QSize(w, h);
}

//void ChatDataDelegate::drawTextContent(QPainter* p,
//                                      const QModelIndex& index,
//                                      const QFont& font,
//                                      const QRect& contentRect) const
//{
//    const QString content = index.data(ChatDataModel::ChatDataItemRoles::ContentRole).toString();

//    p->setPen(Qt::black);
//    p->setFont(font);
//    p->drawText(contentRect, Qt::TextWordWrap, content);
//}

void ChatDataDelegate::drawTextContent(QPainter* p,
                                      const QModelIndex& index,
                                      const QFont& font,
                                      const QRect& contentRect) const
{
    const QString content =
        index.data(ChatDataModel::ChatDataItemRoles::ContentRole).toString();

    QTextOption opt;
    opt.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);

    QTextLayout layout(content, font);
    layout.setTextOption(opt);

    p->setPen(Qt::black);

    layout.beginLayout();
    qreal y = contentRect.top();

    while (true) {
        QTextLine line = layout.createLine();
        if (!line.isValid())
            break;

        line.setLineWidth(contentRect.width());
        line.setPosition(QPointF(contentRect.left(), y));
        y += line.height();
    }

    layout.endLayout();

    layout.draw(p, QPointF(0, 0));
}


QSize ChatDataDelegate::measurePicContent(const QModelIndex& index,
                                         int maxContentWidth) const
{
    const QString local_path = index.data(ChatDataModel::ChatDataItemRoles::FileLocalPathRole).toString();
    const ChatRole role = static_cast<ChatRole>(index.data(ChatDataModel::ChatDataItemRoles::ChatUserRole).toInt());
    QPixmap pm = QPixmap(local_path);
    if (pm.isNull()) {
        // 图片加载失败：用一行文字的高度做兜底
        return QSize(std::min(160, maxContentWidth), 40);
    }

    const int maxW = 130;
    const int maxH = 130; // 可调：图片最大高度

    QSize scaled = pm.size();
    scaled.scale(maxW, maxH, Qt::KeepAspectRatio);

    // 最小兜底（避免极小图片导致难点）
    scaled.setWidth(std::max(scaled.width(), 80));
    scaled.setHeight(std::max(scaled.height(), 60));

    return scaled;
}

void ChatDataDelegate::drawPicContent(QPainter* p,
                                      const QModelIndex& index,
                                      const QRect& contentRect) const
{
    const QString local_path = index.data(ChatDataModel::ChatDataItemRoles::FileLocalPathRole).toString();
    const ChatRole role = static_cast<ChatRole>(index.data(ChatDataModel::ChatDataItemRoles::ChatUserRole).toInt());
    QPixmap pm = QPixmap(local_path);
    if (pm.isNull()) {
        qDebug() << "[Delegate] local_path = " << local_path;
        p->setPen(QColor(120,120,120));
        p->drawText(contentRect, Qt::AlignCenter, QString::fromUtf8("图片加载失败"));
        return;
    }
    QPixmap scaled = pm.scaled(contentRect.size(),
                               Qt::KeepAspectRatio,
                               Qt::SmoothTransformation);

    // 居中绘制
    QPoint topLeft = contentRect.center() - QPoint(scaled.width()/2, scaled.height()/2);
    QRect target(topLeft, scaled.size());
    p->drawPixmap(target, scaled);
}


QSize ChatDataDelegate::measureFileContent(const QModelIndex& index,
                                          const QFont& font,
                                          int maxContentWidth) const
{
    // 固定内容区尺寸（你已定义宏）
    int w = FILE_ICON_WIDTH;
    int h = FILE_ICON_HEIGHT;

    // 宽度不能超过 maxContentWidth（避免气泡超出）
    w = std::min(w, std::max(1, maxContentWidth));

    return QSize(w, h);
}

void ChatDataDelegate::drawFileContent(QPainter* p,
                                       const QModelIndex& index,
                                       const QFont&,
                                       const QRect& contentRect) const
{
    // ===== 数据 =====
    const QString local_path = index.data(ChatDataModel::ChatDataModel::FileLocalPathRole).toString();
    const qint64 trans_size  = index.data(ChatDataModel::FileTransSizeRole).toLongLong();
    const qint64 total_size  = index.data(ChatDataModel::FileTotalSizeRole).toLongLong();
    const TRANSFER_STATE state =
        static_cast<TRANSFER_STATE>(index.data(ChatDataModel::FileTransferStatusRole).toInt());
    const QString failedReason =
        index.data(ChatDataModel::FileTransFailedRole).toString();

//    qDebug() << "local_path = " << local_path;
//    qDebug() << "trans_size = " << trans_size;
//    qDebug() << "total_size = " << total_size;
//    qDebug() << "state = " << state;
//    qDebug() << "failedReasion = " << failedReason;

    QFileInfo info(local_path);
    QString fileName = info.fileName();
    if (fileName.isEmpty())
        fileName = QString::fromUtf8("文件");

    QString fileSizeText = getFileSize(
        info.exists() ? info.size() : total_size
    );

    // ===== 画笔设置 =====
    p->save();
    p->setRenderHint(QPainter::Antialiasing);
    p->setRenderHint(QPainter::TextAntialiasing);

    const int padding = 10;
    const int iconSize = 50;
    const int gap = 10;

    // ===== 区域划分 =====
    QRect iconRect(
        contentRect.right() - padding - iconSize,
        contentRect.top() + (contentRect.height() - iconSize) / 2,
        iconSize, iconSize
    );

    QRect textRect(
        contentRect.left() + padding,
        contentRect.top() + padding,
        iconRect.left() - contentRect.left() - padding - gap,
        contentRect.height() - 2 * padding
    );

    // ===== 文件图标 =====
    QFileIconProvider provider;
    QIcon icon = provider.icon(info);
    p->drawPixmap(iconRect, icon.pixmap(iconSize, iconSize));

    // ===== 字体 =====
    QFont titleFont("Microsoft YaHei", 10);
    titleFont.setBold(true);
    QFont subFont("Microsoft YaHei", 9);

    QFontMetrics fmTitle(titleFont);
    QFontMetrics fmSub(subFont);

    // ===== 第一行：文件名 =====
    QRect nameRect(
        textRect.left(),
        textRect.top(),
        textRect.width(),
        fmTitle.height()
    );

    p->setFont(titleFont);
    p->setPen(QColor(30, 30, 30));
    QString elidedName = fmTitle.elidedText(
        fileName, Qt::ElideRight, nameRect.width()
    );
    p->drawText(nameRect, Qt::AlignLeft | Qt::AlignVCenter, elidedName);

    // ===== 第二行：状态 =====
    QString statusText;
    switch (state) {
    case TRANSFER_STATE::Upload_Finish:
        statusText = QString::fromUtf8("文件上传成功");
        break;
    case TRANSFER_STATE::Upload_Failed:
        if (failedReason.isEmpty()) {
            statusText = QString::fromUtf8("文件上传失败");
        } else {
            statusText = QString::fromUtf8("文件上传失败：") + failedReason;
        }
        break;
    case TRANSFER_STATE::Download_Finish:
        statusText = QString::fromUtf8("文件下载完成");
        break;
    case TRANSFER_STATE::Download_Failed:
        if (failedReason.isEmpty()) {
            statusText = QString::fromUtf8("文件下载失败");
        } else {
            statusText = QString::fromUtf8("文件下载失败：") + failedReason;
        }
        break;
    case TRANSFER_STATE::Uploading:{
        int upload_percent = 0;
        upload_percent = int((double(trans_size) * 100.0) / double(total_size));
        upload_percent = std::clamp(upload_percent, 0, 100);
        statusText = QString::fromUtf8("文件已上传 %1%").arg(upload_percent);
        break;
    }

    case TRANSFER_STATE::Downloading:{
        int download_percent = 0;
        if(total_size != 0){
            download_percent = int((double(trans_size) * 100.0) / double(total_size));
            download_percent = std::clamp(download_percent, 0, 100);
        }
        statusText = QString::fromUtf8("文件已下载 %1%").arg(download_percent);
        break;
    }
    case TRANSFER_STATE::Upload_Paused:{
        statusText = QString::fromUtf8("文件已暂停上传");
    }
    case TRANSFER_STATE::Download_Paused:{
        statusText = QString::fromUtf8("文件已暂停下载");
    }
    }

    //qDebug() << "[DEBUG] statusText = " << statusText;

    QRect infoRect(
        textRect.left(),
        nameRect.bottom() + 4,
        textRect.width(),
        fmSub.height()
    );

    p->setFont(subFont);
    p->setPen(QColor(140, 140, 140));
    p->drawText(
        infoRect,
        Qt::AlignLeft | Qt::AlignVCenter,
        fmSub.elidedText(statusText, Qt::ElideRight, infoRect.width())
    );

    p->restore();
}

QString ChatDataDelegate::getFileSize(qint64 size) const
{
    QString Unit;
    double num;
    if(size < 1024){
        num = size;
        Unit = "B";
    }
    else if(size < 1024 * 1224){
        num = size / 1024.0;
        Unit = "KB";
    }
    else if(size <  1024 * 1024 * 1024){
        num = size / 1024.0 / 1024.0;
        Unit = "MB";
    }
    else{
        num = size / 1024.0 / 1024.0/ 1024.0;
        Unit = "GB";
    }
    return QString::number(num,'f',2) + " " + Unit;
}

void ChatDataDelegate::PictureView(QString local_path) const
{
    QPixmap pm(local_path);
    if (pm.isNull()) {
        // 你可以弹 toast / qDebug
        return;
    }

    // parent 建议用 view 的窗口，保证在正确屏幕居中/铺满
    QWidget* parent = m_view ? m_view->window() : nullptr;

    ImageViewerDialog dlg(pm, parent);
    dlg.exec();
}


ChatDataView::ChatDataView(QWidget *parent)
    : cur_thread_id_(-1), is_loading_(false)
{
    this->installEventFilter(this); // 安装事件过滤器
    this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // 关闭水平滚动条
    this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // 关闭垂直滚动条
    this->setMouseTracking(true); // 开启鼠标跟踪
}

ChatDataView::~ChatDataView()
{
}

void ChatDataView::mouseMoveEvent(QMouseEvent *event)
{
    //qDebug() << "mouse moving . . .";
    QModelIndex index = indexAt(event->pos());
    updateHover(index, event->pos());
    QListView::mouseMoveEvent(event);
}

void ChatDataView::leaveEvent(QEvent *event)
{
    // 鼠标离开 view，清空 hover 状态
    if (m_hoverIndex.isValid()) {
        QRect oldRect = visualRect(m_hoverIndex);
        m_hoverIndex = QModelIndex();
        viewport()->update(oldRect);
    }

    QListView::leaveEvent(event);
}

// 函数参数分别是 当前View选择项对应的QModelIndex，以及点击的位置
void ChatDataView::updateHover(const QModelIndex &index, const QPoint &pos)
{
    if (index == m_hoverIndex) {
        // 同一条 item，仅更新鼠标位置
        m_hoverPos = pos;
        viewport()->update(visualRect(m_hoverIndex));
        return;
    }

    // index 发生变化，需要刷新旧 item 和新 item
    QRect oldRect;
    if (m_hoverIndex.isValid())
        oldRect = visualRect(m_hoverIndex);

    m_hoverIndex = index;
    m_hoverPos = pos;

    // 重新绘制旧的Item
    if (!oldRect.isNull())
        viewport()->update(oldRect); // 请求视图在下一次事件循环在这个指定的矩形区域内进行重绘。
}

void ChatDataView::mousePressEvent(QMouseEvent *event)
{
    QModelIndex index = this->indexAt(event->pos());
//    CHAT_MSG_TYPE msg_type = static_cast<CHAT_MSG_TYPE>(index.data(ChatDataModel::ChatDataItemRoles::MsgTypeRole).toInt());
//    if(msg_type != CHAT_MSG_TYPE::FILE_MSG){
//        return;
//    }
//    QString content = index.data(ChatDataModel::ChatDataItemRoles::ContentRole).toString();
//    ChatDataModel* model = qobject_cast<ChatDataModel*>(this->model());
//    model->updateFileTransState(content);

    if (index.isValid() && index == m_pressIndex) {
        // 同一条 item，仅更新鼠标位置
        m_hoverPos = event->pos();
        viewport()->update(visualRect(m_pressIndex));
        QListView::mousePressEvent(event);
        qDebug() << "emit mouse press event.";
        return;
    }
    m_pressIndex = index;
    m_pressPos = event->pos();
    viewport()->update(visualRect(m_pressIndex));
    QListView::mousePressEvent(event);
    qDebug() << "emit mouse press event.";
}

bool ChatDataView::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == this && event->type() == QEvent::Wheel)
    {
        auto *wheelEvent = static_cast<QWheelEvent*>(event);
        QScrollBar *scrollBar = this->verticalScrollBar();
        if (!scrollBar) {
            return QListView::eventFilter(watched, event);
        }
        // 先让默认滚动生效
        bool handled = QListView::eventFilter(watched, event);
        const bool scrollingUp = wheelEvent->angleDelta().y() > 0;
        const int currentValue = scrollBar->value();
        const int minValue = scrollBar->minimum(); // 通常为0
        if (currentValue <= minValue && scrollingUp)
        {
            // 已经到顶部还在往上滚，开始累计
            m_overScrollAccumulated_ += wheelEvent->angleDelta().y();
            if (!is_loading_
                && is_load_more_chatMsg_[cur_thread_id_]
                && m_overScrollAccumulated_ > m_triggerThreshold_)
            {
                is_loading_ = true;
                m_overScrollAccumulated_ = 0;
                //qDebug() << "emit load more chatmsg (TOP trigger)";
                emit LoadLocalData::GetInstance()->signalLoadChatMessage(cur_thread_id_);
            }
        }
        else
        {
            // 只要不在顶部或不是向上滚，就清空累计
            m_overScrollAccumulated_ = 0;
        }
        return handled;
    }
    return QListView::eventFilter(watched, event);
}
