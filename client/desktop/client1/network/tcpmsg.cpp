#include "tcpmsg.h"
#include <QJsonObject>
#include <QJsonDocument>
#include <QHostAddress>
#include "dialogs/contactuserwidget.h"
#include "data/usermanager.h"
#include "data/fileuploadmsg.h"
#include "data/loadlocaldata.h"

TcpMsg::TcpMsg()
    : socket_(nullptr)
    , host_("")
    , port_(0)
    , b_recv_pedding_(false)
    , msg_id_(0)
    , msg_len_(0)
    , timer_(nullptr)
    , current_block_(QByteArray())
    , bytes_sent_(0)
    , is_pedding_(false)
    , reconnectAttempts_(0)
    , reconnectEnabled_(false)
    , retryTimer_(nullptr)
{
    // 注册回调函数
    registerFunctionCallbacks();
}

TcpMsg::~TcpMsg()
{
    delete timer_;
    delete retryTimer_;
}

void TcpMsg::registerFunctionCallbacks()
{
    // 收到ChatServer发送的效验结果
    handlers_[ID_CHAT_LOGIN_RSP] = std::bind(&TcpMsg::verify,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3);
    // 收到ChatServer返回的搜索结果
    handlers_[ID_SEARCH_USER_RSP] = std::bind(&TcpMsg::search,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3);
    // 收到好友申请结果的回包（申请成功 还是 申请失败）
    handlers_[ID_APPLY_FRIEND_RSP] = std::bind(&TcpMsg::applyResult,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3);
    // 收到ChatServer的好友申请 （收到好友申请）
    handlers_[ID_NOTIFY_ADD_FRIEND_REQ] = std::bind(&TcpMsg::receiveFriendApply,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3);
    // 收到认证好友申请的回复 （收到对方通过好友申请的消息）
    handlers_[ID_NOTIFY_ACCESS_VERIFY] = std::bind(&TcpMsg::authApply,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3);
    // 通过好友申请 成功（通过对方的申请成功）
    handlers_[ID_AUTH_FRIEND_RSP] = std::bind(&TcpMsg::accessApply,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3);
    // 通知发送消息成功还是失败
    handlers_[ID_TEXT_CHAT_MSG_RSP] = std::bind(&TcpMsg::sendTextMsg,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3);
    // 有新的文本消息
    handlers_[ID_NOTIFY_TEXT_CHAT_MSG_REQ] = std::bind(&TcpMsg::receiveNewTextMsg,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3);
    // 收到下线消息
    handlers_[ID_NOTIFY_OFFLINE] = std::bind(&TcpMsg::notifyOffLine,this,std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    // 收到心跳检测信息的回包
    handlers_[ID_HEADT_CHECK_RSP] = std::bind(&TcpMsg::heartCheck,this,std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    // 收到加载聊天列表的回包
    handlers_[ID_LOAD_CHAT_THREAD_RSP] = std::bind(&TcpMsg::loadChatList,this,std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    // 收到创建私聊线程的回包
    handlers_[ID_CREATE_PRIVATE_CHAT_THREAD_RSP] = std::bind(&TcpMsg::createPrivateChatThread,this,std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    // 收到加载好友列表的回包
    handlers_[ID_LOAD_MORE_FRIEND_RSP] = std::bind(&TcpMsg::loadConnlist,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3);
    // 收到发送图片消息的回包
    handlers_[ID_IMAGE_CHAT_MSG_RSP] = std::bind(&TcpMsg::sendImgMsg,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3);
    // 收到加载好友请求列表的回包
    handlers_[ID_LOAD_FRIEND_APPLY_RSP] = std::bind(&TcpMsg::loadFriendApplyList,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3);
    // 收到好友头像变更的通知
    handlers_[ID_NOTIFY_FRIEND_ICON_CHANGE] = std::bind(&TcpMsg::friendIconChange,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3);
    // 收到聊天图片
    handlers_[ID_NOTIFY_CHAT_IMAGE_MSG] = std::bind(&TcpMsg::RecvNewImgMsg,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3);
    // 收到加载聊天消息的回包
    handlers_[ID_LOAD_CHAT_MSG_RSP] = std::bind(&TcpMsg::LoadChatMsg,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3);
}

void TcpMsg::RecvNewImgMsg(REQUEST_ID reqId, int len, QByteArray data)
{
    // 将QByteArray转换为QJsonDocument
    QJsonDocument doc = QJsonDocument::fromJson(data);

    // 检查转换是否成功
    if (doc.isNull()) {
        //qDebug() << "Failed to create QJsonDocument.";
        return;
    }

    QJsonObject jsonObj = doc.object();
    if(jsonObj["code"] != ERRORCODE::SUCCESS)
    {
        //qDebug() << "error code: " << jsonObj["code"].toInt();
        //qDebug() << "error message: " << jsonObj["message"].toString();
        return;
    }

    // 添加到UserManager缓存
    int message_id = jsonObj["message_id"].toInt();
    int send_id = jsonObj["send_id"].toInt();
    int recv_id = jsonObj["recv_id"].toInt();
    int thread_id = jsonObj["thread_id"].toInt();
    int status = jsonObj["status"].toInt();
    QString unique_id = jsonObj["unique_id"].toString();
    QDateTime chat_time = returnQDateTimeByQString(jsonObj["chat_time"].toString());
    QString unique_name = jsonObj["unique_name"].toString();
    int type = jsonObj["type"].toInt();

    QString local_path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/avatars/" + unique_name;

    std::shared_ptr<MsgInfo> msg = std::make_shared<MsgInfo>();
    msg->text_or_url_ = local_path;
    msg->unique_name_ = unique_name;
    msg->total_size_ = 0;
    msg->current_size_ = 0;
    msg->seq_ = 0;
    msg->_type = TRANSFER_TYPE::Download;
    msg->_state = TRANSFER_STATE::Downloading;

    std::shared_ptr<ImageDataBase> image = std::make_shared<ImageDataBase>(msg,message_id,unique_id,thread_id,CHAT_THREAD_TYPE::CHAT_THREAD_TYPE_PRIVATE,
                                                                           (CHAT_MSG_TYPE)type,send_id,recv_id,status,chat_time);
    //UserManager::GetInstance()->add_chat_image(unique_name, image);

    // 添加到聊天界面
    emit signalRecvNewImgMsg(image);

    // 并且需要判断是否已经在下载了
    std::shared_ptr<MsgInfo> is_download = UserManager::GetInstance()->get_trans_file(unique_name);
    if(is_download != nullptr){
        // 正在下载,那么就直接返回
        qDebug() << "url = " << unique_name << " 的文件正在下载.";
        return;
    }
    //
    UserManager::GetInstance()->add_trans_file(unique_name, msg);
    // 请求下载
    QJsonObject send_obj;
    send_obj["download_file"] = unique_name;
    send_obj["seq"] = 1;
    send_obj["last_seq"] = 0;
    send_obj["trans_size"] = 0;
    send_obj["total_size"] = 0;
    send_obj["uid"] = UserManager::GetInstance()->getUid();
    send_obj["token"] = UserManager::GetInstance()->GetToken();
    send_obj["client_save_path"] = local_path;
    send_obj["download_file_type"] = Download_File_Type::CHAT_IMAGE;
    QJsonDocument send_doc(send_obj);
    QByteArray send_data = send_doc.toJson(QJsonDocument::Compact);
    emit FileUploadMsg::GetInstance()->signalSendData(ID_DOWN_LOAD_FILE_REQ,send_data);

    // 将这个文件的下载信息添加到数据库
    emit LoadLocalData::GetInstance()->signalAddTransferToDb(unique_name, 0, 0, local_path,TRANSFER_STATE::Downloading
                                                             ,TRANSFER_TYPE::Download,"");
}

void TcpMsg::LoadChatMsg(REQUEST_ID reqId, int len, QByteArray data)
{
    QJsonDocument doc = QJsonDocument::fromJson(data);
    // 检查转换是否成功
    if (doc.isNull()) {
        //qDebug() << "Failed to create QJsonDocument.";
        return;
    }
    QJsonObject jsonObj = doc.object();
    if(jsonObj["code"] != ERRORCODE::SUCCESS)
    {
        qDebug() << "error code: " << jsonObj["code"].toInt();
        qDebug() << "error message: " << jsonObj["message"].toString();
        return;
    }
    //qDebug() << "收到加载聊天记录的回包.";
    emit signalLoadChatMsg(jsonObj);
}

void TcpMsg::loadFriendApplyList(REQUEST_ID reqId, int len, QByteArray data)
{
    QJsonDocument doc = QJsonDocument::fromJson(data);
    // 检查转换是否成功
    if (doc.isNull()) {
        //qDebug() << "Failed to create QJsonDocument.";
        return;
    }
    QJsonObject jsonObj = doc.object();
    if(jsonObj["code"] != ERRORCODE::SUCCESS)
    {
        qDebug() << "error code: " << jsonObj["code"].toInt();
        qDebug() << "error message: " << jsonObj["message"].toString();
        return;
    }
    //qDebug() << "收到加载好友申请列表的回包.";
    // 将数据放在本地数据库和UserManager缓存当中
    emit signalLoadFriendApplyList(jsonObj);
}

void TcpMsg::friendIconChange(REQUEST_ID reqId, int len, QByteArray data)
{
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull()) {
        qDebug() << "Failed to create QJsonDocument.";
        return;
    }
    QJsonObject jsonObj = doc.object();
    if(jsonObj["code"] != ERRORCODE::SUCCESS)
    {
        qDebug() << "error code: " << jsonObj["code"].toInt();
        qDebug() << "error message: " << jsonObj["message"].toString();
        return;
    }
    qDebug() << "收到好友头像变更的通知";

    int self_uid = jsonObj["uid"].toInt();
    int redis_id = jsonObj["redis_id"].toInt();
    int friend_id = jsonObj["friend_id"].toInt();
    QString friend_icon = jsonObj["friend_icon"].toString();
    QString message = jsonObj["message"].toString();

    emit LoadLocalData::GetInstance()->signalupdateFriendIcon(friend_id, friend_icon);

    // 检查是否在下载
    if(UserManager::GetInstance()->get_download_file(friend_icon) != nullptr){
        qDebug() << "friend icon = " << friend_icon << " is downloading.";
        return;
    }

    // 不在下载,那么就请求下载
    std::shared_ptr<DownloadFileInfo> file_info = std::make_shared<DownloadFileInfo>();
    file_info->download_file_ = friend_icon;
    file_info->type_ = Download_File_Type::FRIEND_HEAD_ICON_ONLINE;
    file_info->seq_ = 1;
    file_info->client_save_path_ = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/avatars/" + friend_icon;
    file_info->state_ = TRANSFER_STATE::Downloading;
    UserManager::GetInstance()->add_download_file(friend_icon, file_info);

    // 那么就去向资源服务器请求文件下载
    QString local_path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/avatars/" + friend_icon;
    QJsonObject send_obj;
    send_obj["download_file"] = friend_icon;
    send_obj["seq"] = 1;
    send_obj["last_seq"] = 0;
    send_obj["trans_size"] = 0;
    send_obj["total_size"] = 0;
    send_obj["uid"] = UserManager::GetInstance()->getUid();
    send_obj["token"] = UserManager::GetInstance()->GetToken();
    send_obj["client_save_path"] = local_path ;
    send_obj["download_file_type"] = file_info->type_;
    send_obj["icon_uid"] = friend_id;

    QJsonDocument send_doc(send_obj);
    QByteArray send_data = send_doc.toJson(QJsonDocument::Compact);

    emit FileUploadMsg::GetInstance()->signalSendData(ID_DOWN_LOAD_FILE_REQ,send_data);

    // 将这个文件的下载信息添加到数据库
    emit LoadLocalData::GetInstance()->signalAddTransferToDb(friend_icon, 0, 0, local_path,TRANSFER_STATE::Downloading
                                                             ,TRANSFER_TYPE::Download,"");
}

void TcpMsg::createPrivateChatThread(REQUEST_ID reqId, int len, QByteArray data)
{
    // 将QByteArray转换为QJsonDocument
    QJsonDocument doc = QJsonDocument::fromJson(data);

    // 检查转换是否成功
    if (doc.isNull()) {
        //qDebug() << "Failed to create QJsonDocument.";
        return;
    }

    QJsonObject jsonObj = doc.object();
    if(jsonObj["code"] != ERRORCODE::SUCCESS)
    {
        //qDebug() << "error code: " << jsonObj["code"].toInt();
        //qDebug() << "error message: " << jsonObj["message"].toString();
        return;
    }
    //qDebug() << "收到创建私聊线程的回包";
    emit signalRecvCreatePrivateChat(jsonObj);
}

void TcpMsg::loadConnlist(REQUEST_ID reqId, int len, QByteArray data)
{
    // 将QByteArray转换为QJsonDocument
    QJsonDocument doc = QJsonDocument::fromJson(data);
    // 检查转换是否成功
    if (doc.isNull()) {
        //qDebug() << "Failed to create QJsonDocument.";
        return;
    }
    QJsonObject jsonObj = doc.object();
    if(jsonObj["code"] != ERRORCODE::SUCCESS)
    {
        qDebug() << "error code: " << jsonObj["code"].toInt();
        qDebug() << "error message: " << jsonObj["message"].toString();
        return;
    }
    emit signalLoadConnList(jsonObj);
}

void TcpMsg::sendImgMsg(REQUEST_ID reqId, int len, QByteArray data)
{
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if(doc.isNull()){
        qDebug() << "Failed to create QJsonDocument.";
        return;
    }

    QJsonObject obj = doc.object();
    int code = obj["code"].toInt();
    if(code != SUCCESS){
        qDebug() << "error code: " << code;
        qDebug() << "error message: " << obj["message"].toString();
        return;
    }

    // 解析消息
    int messgae_id = obj["message_id"].toInt();
    int thread_id = obj["thread_id"].toInt();
    int send_id = obj["send_id"].toInt();
    int recv_id = obj["recv_id"].toInt();
    QString unique_id = obj["unique_id"].toString();
    QString unique_name = obj["unique_name"].toString();
    QDateTime chat_time = returnQDateTimeByQString(obj["chat_time"].toString());
    int status = obj["status"].toInt();
    int type = obj["type"].toInt();

    std::shared_ptr<MsgInfo> file_info = UserManager::GetInstance()->get_trans_file(unique_name);
    if(file_info == nullptr){
        qDebug() << "cant find msg_info by unique_name = " << unique_name;
        return;
    }

    std::shared_ptr<ImageDataBase> image_info = std::make_shared<ImageDataBase>(file_info,messgae_id,unique_id,thread_id,CHAT_THREAD_TYPE::CHAT_THREAD_TYPE_PRIVATE,
                                                                                (CHAT_MSG_TYPE)type,send_id,recv_id,status,chat_time);
    emit signalNotifyChatPageImgStatus(thread_id,image_info);

    if((MsgStatus)status == MsgStatus::SEND_FAILED){
        qDebug() << "ChatImageData: " << unique_name << " send failed.";
        return;
    }
    QFile file(file_info->text_or_url_);
    if(!file.open(QIODevice::ReadOnly)){
        qDebug() << "Cant open file: " << file.errorString();
        return;
    }

    int last_seq = 0;
    if(file.size() % MAX_FILE_LEN == 0){
        last_seq = file.size() / MAX_FILE_LEN;
    }else{
        last_seq = file.size() / MAX_FILE_LEN + 1;
    }

    // 将文件指针回复到 初始位置
    file.seek(file_info->current_size_);

    auto buffer = file.read(MAX_FILE_LEN);
    // 将文件内容转化为 base64 编码
    QString baseData64 = buffer.toBase64();
    QJsonObject send_data;
    send_data["filename"] = file_info->unique_name_;
    send_data["seq"] = file_info->seq_;
    send_data["lastseq"] = last_seq;
    file_info->current_size_ += buffer.size();
    send_data["uid"] = UserManager::GetInstance()->getUid();
    send_data["data"] = baseData64;
    send_data["totolsize"] = file.size();
    send_data["transferredsize"] = file_info->current_size_;
    send_data["type"] = type;

    QJsonDocument send_doc(send_data);
    QByteArray file_data = send_doc.toJson(QJsonDocument::Compact);

    // 发送消息给 ResourceServer
    emit FileUploadMsg::GetInstance()->signalSendData(ID_IMAGE_CHAT_MSG_REQ,file_data);
}

void TcpMsg::registerSignal()
{
    // 与服务器建立连接
    connect(socket_,&QTcpSocket::connected,[&](){
        qDebug() << "connect to ChatServer(host:"<< host_ << ":" << port_ << ").";
        // 如果是断线重连的，那么就不需要重新打开一个界面
        if(reconnectEnabled_) {
            reconnectEnabled_ = false;
            return;
        }
        emit signalConnectSuccess(true);

        reconnectTimer_->stop();
        reconnectAttempts_ = 0;
    });
    // 当缓冲区接收到服务器的数据之后，触发对应的槽函数
    connect(socket_,&QTcpSocket::readyRead,[&](){
        // 将服务器发送的消息存储到缓冲区
        buffer_.append(socket_->readAll());

        forever
        {
            // 检查消息id 和 消息长度 字段是否发送完成
            if(!b_recv_pedding_)
            {
                // Protocol: uuid(36B) + msg_id(2B) + msg_len(2B) = 40B header
                const int HEADER_SIZE = sizeof(quint16) * 2 + 36;
                // 没有读取到完整的消息头，那么就先返回，等下一次读取凑够40bits
                if(buffer_.size() < HEADER_SIZE){
                    return;
                }
                // 读取uuid
                char uuidBuf[37] = {0};
                QDataStream stream(&buffer_, QIODevice::ReadOnly);
                stream.setVersion(QDataStream::Qt_5_0);
                stream.readRawData(uuidBuf, 36);
                stream.setByteOrder(QDataStream::BigEndian);
                QString recvUuid = QString::fromUtf8(uuidBuf, 36).replace(QChar('\0'), QString());
                // 读取msg_id + msg_len
                stream >> msg_id_ >> msg_len_;
                // 移动缓冲区的读取位置
                buffer_ = buffer_.mid(HEADER_SIZE);
                // 移除服务端回复的消息
                assert(!recvUuid.isEmpty());
                if (!recvUuid.isEmpty()) {
                    pending_ack_.remove(recvUuid);
                }
                b_recv_pedding_ = true;
            }
            // 剩余数据不足
            if(buffer_.size() < msg_len_){
                return;
            }

            // 一条消息读取完成，那么就调用回调函数来处理
            // 消息主体
            QByteArray messageBody = buffer_.mid(0, msg_len_);
            buffer_ = buffer_.mid(msg_len_);
            b_recv_pedding_ = false;

            // 添加对应的参数
            handlers_[msg_id_]((REQUEST_ID)msg_id_,msg_len_,messageBody);

            }
        });

    // 处理出现错误的逻辑
    connect(socket_,static_cast<void(QTcpSocket::*)(QTcpSocket::SocketError)>(&QTcpSocket::error),[&](QTcpSocket::SocketError socketError){
        qDebug() << "Connect to ChatServer(host:"<< host_ << ":" << port_ << ") failed!";
        qDebug() << "Error: " << socket_->errorString();

        /*
        // 获取更详细的网络状态信息
        //qDebug() << "Socket state:" << socket_.state();
        //qDebug() << "Peer address:" << socket_.peerAddress().toString();
        //qDebug() << "Peer port:" << socket_.peerPort();
        */

        switch (socketError) {
            case QTcpSocket::ConnectionRefusedError:
                //qDebug() << "Connection refused!";
                emit signalConnectSuccess(false);
                break;
            case QTcpSocket::RemoteHostClosedError:
                //qDebug() << "Remote host closed Connection!";
                break;
            case QTcpSocket::SocketTimeoutError:
               //qDebug() << "Connection Timeout!";
               emit signalConnectSuccess(false);
               break;
            case QTcpSocket::HostNotFoundError:
                //qDebug() << "Host not found!";
                emit signalConnectSuccess(false);
                break;
            case QTcpSocket::NetworkError:
                //qDebug() << "Network error!";
                break;
            default:
                //qDebug() << "Unknow error in Socket!";
                break;
        }
    });

    // 处理连接断开
    QObject::connect(socket_, &QTcpSocket::disconnected, [&]() {
       //qDebug() << "DisConn Thread: " << QThread::currentThread();
        qDebug() << "Disconnected from server.";
        // 正在断线重连
        reconnectEnabled_ = true;
        // 停止可能正在运行的重连定时器
        reconnectTimer_->stop();
        // 重置重试次数（根据需求，每次断开都重新计数）
        reconnectAttempts_ = 0;
        // 立即尝试第一次重连，或延迟一段时间再尝试
        attemptReconnect();
    });

    // 统一发送数据
    QObject::connect(this, &TcpMsg::signalSendData,this,&TcpMsg::slotSendData);

    // 处理异步发送逻辑
    QObject::connect(socket_,&QTcpSocket::bytesWritten,[&](qint64 bytes_written){
       bytes_sent_ += bytes_written;
       if(bytes_sent_ < current_block_.size()){
           // 继续发送
           auto data = current_block_.mid(0,bytes_written);
           socket_->write(data);
           return;
       }
       // 那么剩下的就是发送完成
       if(send_queue_.empty()){
           is_pedding_ = false;
           bytes_sent_ = 0;
           current_block_.clear();
           return;
       }
       current_block_ = send_queue_.dequeue();
       bytes_sent_ = 0;
       is_pedding_ = true;
       socket_->write(current_block_);
    });
}

void TcpMsg::attemptReconnect()
{
    if (reconnectAttempts_ >= MAX_RECONNECT_TIMES) {
        qDebug() << "Max reconnect attempts reached. Giving up.";
        emit signalNotifyOffLine();
        return;
    }

    // 已连接
    if (socket_->state() == QAbstractSocket::ConnectedState) {
        qDebug() << "stop reconnect, socket is connected.";
        return;
    }
    // 正在连接
    if(socket_->state() == QAbstractSocket::ConnectingState) {
        qDebug() << "trying to connect to server, continue";
    } else {
        qDebug() << "Reconnect attempt" << (reconnectAttempts_ + 1);
        socket_->connectToHost(host_,port_);
        reconnectAttempts_++;
    }
    // 如果还没达到最大次数，启动定时器等待下次重连
    if (reconnectAttempts_ < MAX_RECONNECT_TIMES) {
        reconnectTimer_->start(3000);
    }
}

void TcpMsg::loadChatList(REQUEST_ID reqId, int len, QByteArray data)
{
    //qDebug() << "TcpMsg::handle_id = " << reqId;
    //qDebug() << "data is " << QString::fromUtf8(data);
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull()) {
        //qDebug() << "Failed to create QJsonDocument.";
        return;
    }
    QJsonObject jsonObj = doc.object();
    if(jsonObj["code"] != ERRORCODE::SUCCESS)
    {
        //qDebug() << "error code: " << jsonObj["code"].toInt();
        //qDebug() << "error message: " << jsonObj["message"].toString();
        return;
    }
   //qDebug() << "收到加载聊天列表的回包";
    emit signalLoadChatList(jsonObj);
}

void TcpMsg::heartCheck(REQUEST_ID reqId, int len, QByteArray data)
{
    // 将QByteArray转换为QJsonDocument
    QJsonDocument doc = QJsonDocument::fromJson(data);
    // 检查转换是否成功
    if (doc.isNull()) {
        //qDebug() << "Failed to create QJsonDocument.";
        return;
    }
    QJsonObject jsonObj = doc.object();
    if(jsonObj["code"] != ERRORCODE::SUCCESS)
    {
        //qDebug() << "error code: " << jsonObj["code"].toInt();
        //qDebug() << "error message: " << jsonObj["message"].toString();
        return;
    }
    //qDebug() << "收到心跳检测的回包";
}

void TcpMsg::notifyOffLine(REQUEST_ID reqId, int len, QByteArray data)
{
    qDebug() << "receive offline message.";
    emit signalNotifyOffLine();
}

void TcpMsg::receiveNewTextMsg(REQUEST_ID reqId, int len, QByteArray data)
{
    // 将QByteArray转换为QJsonDocument
    QJsonDocument doc = QJsonDocument::fromJson(data);

    // 检查转换是否成功
    if (doc.isNull()) {
        //qDebug() << "Failed to create QJsonDocument.";
        return;
    }

    QJsonObject jsonObj = doc.object();
    if(jsonObj["code"] != ERRORCODE::SUCCESS)
    {
        //qDebug() << "error code: " << jsonObj["code"].toInt();
        //qDebug() << "error message: " << jsonObj["message"].toString();
        return;
    }
    //qDebug() << "Receive New TextChatMsg." ;
    int thread_id = jsonObj["thread_id"].toInt();
    int sender_id = jsonObj["fromuid"].toInt();
    int recver_id = UserManager::GetInstance()->getUid();

    std::vector<std::shared_ptr<ChatDataBase>> chat_datas;
    for (const QJsonValue& data : jsonObj["chat_datas"].toArray()) {
        auto msg_id = data["message_id"].toInt();
        auto unique_id = data["unique_id"].toString();
        auto msg_content = data["content"].toString();
        QDateTime chat_time = returnQDateTimeByQString(data["chat_time"].toString());
        int status = data["status"].toInt();
        auto chat_data = std::make_shared<TextChatData>(msg_id, unique_id, thread_id, CHAT_THREAD_TYPE::CHAT_THREAD_TYPE_PRIVATE,
            CHAT_MSG_TYPE::TEXT_MSG, msg_content, sender_id, recver_id, status, chat_time);
        chat_datas.push_back(chat_data);
    }
    emit signalReceiveNewTextMsg(chat_datas);

    /*auto msg_ptr = std::make_shared<TextChatMsg>(jsonObj["fromuid"].toInt(),
            jsonObj["touid"].toInt(),jsonObj["text_array"].toArray());

    emit signalReceiveNewTextMsg(msg_ptr);*/
}

void TcpMsg::sendTextMsg(REQUEST_ID reqId, int len, QByteArray data)
{
    //qDebug() << "TcpMsg::handle_id = " << reqId;
    //qDebug() << "data is " << QString::fromUtf8(data);

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if(doc.isNull()){
        // 转换失败
        //qDebug() << "Failed to create QJsonDocument";
        return;
    }

    QJsonObject obj = doc.object();

    // 出现错误
    if(obj["code"].toInt() != ERRORCODE::SUCCESS)
    {
        //qDebug() << "error code: " << obj["code"].toInt();
        //qDebug() << "error message: " << obj["message"].toString();
        return;
    }

    //qDebug() << "发送消息 " << obj["text_array"] << " 成功.";

    // 将消息发送成功或者失败的消息通知 ChatInterface / ChatThradData
    int thread_id  = obj["thread_id"].toInt();
    int sender_id = obj["fromuid"].toInt(); // 实际上就是自己的uid
    int recver_id = obj["touid"].toInt();
    auto arrays = obj["chat_datas"].toArray();

    std::vector<std::shared_ptr<TextChatData>> chat_datas;
    for(const QJsonValue& data : arrays){
        int msg_id = data["message_id"].toInt();
        QString unique_id = data["unique_id"].toString();
        QString content = data["content"].toString();
        QDateTime chat_time = returnQDateTimeByQString(data["chat_time"].toString());
        int status = data["status"].toInt();
        auto chat_data = std::make_shared<TextChatData>(
                    msg_id,unique_id,thread_id,CHAT_THREAD_TYPE::CHAT_THREAD_TYPE_PRIVATE,
                    CHAT_MSG_TYPE::TEXT_MSG,content,sender_id,recver_id,status,chat_time);
        chat_datas.push_back(chat_data);
    }
    //qDebug() << "notify " << thread_id << "msg send status.";
    emit signalNotifyChatPageMsgStatus(thread_id,chat_datas);
}

void TcpMsg::accessApply(REQUEST_ID reqId, int len, QByteArray data)
{
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if(doc.isNull()){
        // 转换失败
        //qDebug() << "Failed to create QJsonDocument";
        return;
    }

    QJsonObject obj = doc.object();

    // 出现错误
    if(obj["code"].toInt() != ERRORCODE::SUCCESS)
    {
        //qDebug() << "error code: " << obj["code"].toInt();
        //qDebug() << "error message: " << obj["message"].toString();
        return;
    }

    // 解析数据
    int peeruid = obj["peeruid"].toInt();
    int status = obj["status"].toInt();
    if(status == FRIEND_APPLY::REFUSED){
        //qDebug() << "拒绝好友申请成功。";
        // 将好友申请的信息设置为 拒绝
        emit LoadLocalData::GetInstance()->signalSetFriendApplyStatus(peeruid,FRIEND_APPLY::REFUSED);
        return;
    }
    // 将好友申请设置为 通过
    emit LoadLocalData::GetInstance()->signalSetFriendApplyStatus(peeruid,FRIEND_APPLY::ACCEPTED);
    if(UserManager::GetInstance()->isFriendByUid(peeruid)){
        qDebug() << "you are already friends.";
        return;
    }
    int id = obj["friend_id"].toInt();
    int uid = obj["uid"].toInt();
    QString name = obj["name"].toString();
    QString nick = obj["nick"].toString();
    QString desc = obj["desc"].toString();
    QString icon = obj["icon"].toString();
    QString email = obj["email"].toString();
    int sex = obj["sex"].toInt();
    int thread_id = obj["thread_id"].toInt();

    // 将好友信息保存到本地
    std::vector<std::shared_ptr<FriendInfo>> friend_info;
    std::shared_ptr<FriendInfo> info = std::make_shared<FriendInfo>(
                id,uid,name,nick,icon,sex,desc,email);
    friend_info.push_back(info);
    emit LoadLocalData::GetInstance()->signalStoreFriends(friend_info);
    // 将好友信息保存到缓冲
    UserManager::GetInstance()->appendFriendList(friend_info);
    // 缓存设置最大的 friend_id
    UserManager::GetInstance()->set_last_friend_id(id);

    // 将聊天线程信息保存到本地
    int self_uid = UserManager::GetInstance()->getUid();
    std::shared_ptr<ChatThreadInfo> thread_info = std::make_shared<ChatThreadInfo>(thread_id,
                                                              CHAT_THREAD_TYPE::CHAT_THREAD_TYPE_PRIVATE,self_uid,uid);
    emit LoadLocalData::GetInstance()->signalStoreThreadInfo(thread_info);

    // 缓存创建新ChatThreadData和thread_id的映射
    std::shared_ptr<ChatThreadData> thread_data = std::make_shared<ChatThreadData>(uid,thread_id);
    UserManager::GetInstance()->addChatThreadData(thread_data);
    // 缓存设置最大的thread_id
    UserManager::GetInstance()->setLastChatThreadID(thread_id);

    // 通知好友列表去加载新的条目
    emit signalAddNewConnUserWidget(info);

    // 通知聊天列表去加载新的条目
    std::shared_ptr<UserInfo> userInfo = std::make_shared<UserInfo>(uid,name,nick,icon,sex);
    emit signalAddNewChatUserWidget(userInfo);
}

void TcpMsg::authApply(REQUEST_ID reqId, int len, QByteArray data)
{
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if(doc.isNull()){
        // 转换失败
        //qDebug() << "Failed to create QJsonDocument";
        return;
    }

    QJsonObject obj = doc.object();

    // 出现错误
    if(obj["code"].toInt() != ERRORCODE::SUCCESS)
    {
        //qDebug() << "error code: " << obj["code"].toInt();
        //qDebug() << "error message: " << obj["message"].toString();
        return;
    }
    // 解析数据
    int id = obj["friend_id"].toInt();
    int uid = obj["uid"].toInt();
    QString name = obj["name"].toString();
    QString nick = obj["nick"].toString();
    QString desc = obj["desc"].toString();
    QString icon = obj["icon"].toString();
    QString email = obj["email"].toString();
    int sex = obj["sex"].toInt();
    int thread_id = obj["thread_id"].toInt();

    // 将好友信息保存到本地
    std::vector<std::shared_ptr<FriendInfo>> friend_info;
    std::shared_ptr<FriendInfo> info = std::make_shared<FriendInfo>(
                id,uid,name,nick,icon,sex,desc,email);
    friend_info.push_back(info);
    emit LoadLocalData::GetInstance()->signalStoreFriends(friend_info);
    // 将好友信息保存到缓冲
    UserManager::GetInstance()->appendFriendList(friend_info);
    // 缓存设置最大的 friend_id
    UserManager::GetInstance()->set_last_friend_id(id);

    // 将聊天线程信息保存到本地
    int self_uid = UserManager::GetInstance()->getUid();
    std::shared_ptr<ChatThreadInfo> thread_info = std::make_shared<ChatThreadInfo>(thread_id,
                                                              CHAT_THREAD_TYPE::CHAT_THREAD_TYPE_PRIVATE,self_uid,uid);
    emit LoadLocalData::GetInstance()->signalStoreThreadInfo(thread_info);

    // 缓存创建新ChatThreadData和thread_id的映射
    std::shared_ptr<ChatThreadData> thread_data = std::make_shared<ChatThreadData>(uid,thread_id);
    UserManager::GetInstance()->addChatThreadData(thread_data);
    // 缓存设置最大的thread_id
    UserManager::GetInstance()->setLastChatThreadID(thread_id);

    // 通知好友列表去加载新的条目
    emit signalAddNewConnUserWidget(info);

    // 通知聊天列表去加载新的条目
    std::shared_ptr<UserInfo> userInfo = std::make_shared<UserInfo>(uid,name,nick,icon,sex);
    emit signalAddNewChatUserWidget(userInfo);
}

void TcpMsg::applyResult(REQUEST_ID reqId, int len, QByteArray data)
{
    //qDebug() << "TcpMsg::handle_id = " << reqId;
    //qDebug() << "data is " << QString::fromUtf8(data);

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if(doc.isNull()){
        // 转换失败
        //qDebug() << "Failed to create QJsonDocument";
        return;
    }

    QJsonObject obj = doc.object();

    // 出现错误
    if(obj["code"].toInt() != ERRORCODE::SUCCESS)
    {
        //qDebug() << "申请失败";
        //qDebug() << "error code: " << obj["code"].toInt();
        //qDebug() << "error message: " << obj["message"].toString();
        return;
    }
    //qDebug() << "申请成功,等待对方验证";
}

void TcpMsg::receiveFriendApply(REQUEST_ID reqId, int len, QByteArray data)
{
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if(doc.isNull()){
        // 转换失败
        //qDebug() << "Failed to create QJsonDocument";
        return;
    }

    QJsonObject obj = doc.object();

    // 出现错误
    if(obj["code"].toInt() != ERRORCODE::SUCCESS)
    {
        //qDebug() << "error code: " << obj["code"].toInt();
        //qDebug() << "error message: " << obj["message"].toString();
        return;
    }

    //qDebug() << "收到了好友申请.";

    int id = obj["id"].toInt();
    int fromuid = obj["fromuid"].toInt();
    QString name = obj["name"].toString();
    QString email = obj["email"].toString();
    QString desc = obj["desc"].toString();
    QString icon = obj["icon"].toString();
    int sex = obj["sex"].toInt();
    QString apply_time = obj["apply_time"].toString();
    int status = obj["status"].toInt();

    std::shared_ptr<AddFriendApply> applyInfo = std::make_shared<AddFriendApply>(id,fromuid,name,email,desc,icon,sex,apply_time,status);
    emit signalReceiveFriendApply(applyInfo);
}

void TcpMsg::search(REQUEST_ID reqId, int len, QByteArray data)
{
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if(doc.isNull()){
        // 转换失败
        //qDebug() << "Failed to create QJsonDocument";
        return;
    }

    QJsonObject obj = doc.object();

    // 出现错误,查看失败
    if(obj["code"].toInt() != ERRORCODE::SUCCESS)
    {
        //qDebug() << "error code: " << obj["code"].toInt();
        emit signalSearchUser(nullptr);
        return;
    }

    //int uid, QString name, QString nick, QString icon, int sex, QString last_msg = "", QString desc=""
    // 校验成功，将搜索的结果设置为 item 并且展示在 SearchList
    int uid = obj["uid"].toInt();
    QString name = obj["name"].toString();
    QString nick = obj["nick"].toString();
    QString desc = obj["desc"].toString();
    int sex = obj["sex"].toInt();
    QString icon = obj["icon"].toString();

    std::shared_ptr<SearchInfo> searchInfo = std::make_shared<SearchInfo>(
                    uid,name,nick,desc,sex,icon
                );
    emit signalSearchUser(searchInfo);
}

void TcpMsg::verify(REQUEST_ID reqId, int len, QByteArray data)
{
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if(doc.isNull()){
        // 转换失败
        qDebug() << "Failed to create QJsonDocument";
        return;
    }

    QJsonObject obj = doc.object();

    // 出现错误 , 登陆失败
    if(obj["code"].toInt() != ERRORCODE::SUCCESS)
    {
        //qDebug() << "error code: " << obj["code"].toInt();
        //qDebug() << "error message: " << obj["message"].toString();
        return;
    }

    //qDebug() << "Token verify success.";
    connect(timer_,&QTimer::timeout,[&](){
       QJsonObject obj;
       obj["uid"] = UserManager::GetInstance()->getUid();
       QJsonDocument doc(obj);
       QByteArray data = doc.toJson(QJsonDocument::Compact);
       emit TcpMsg::GetInstance()->signalSendData(ID_HEADT_CHECK_REQ,data);
    });
    timer_->start(60000);

    // 获取Client本人的信息
    int uid = obj["uid"].toInt();
    QString name = obj["name"].toString();
    QString email = obj["email"].toString();
    QString pwd = obj["password"].toString();
    QString nick = obj["nick"].toString();
    QString desc = obj["desc"].toString();
    int sex = obj["sex"].toInt();
    QString icon = obj["icon"].toString();

    QString token = obj["token"].toString();
    UserManager::GetInstance()->setToken(token);

    // QString icon = ":/res/jerry.jpg";
    // UserInfo(int uid, QString name, QString nick, QString icon, int sex, QString last_msg = "", QString desc=""):
    std::shared_ptr<UserInfo> info = std::make_shared<UserInfo>(uid,name,email,pwd,nick,icon,sex,desc);
    UserManager::GetInstance()->setUserInfo(info);

    // 将登录信息保存在数据库当中
    emit LoadLocalData::GetInstance()->signalAddUser(info);

    // 获取Client本人的好友列表
    /*if(obj.contains("friendList")){
        UserManager::GetInstance()->appendFriendList(obj["friendList"].toArray());
    }*/

//    // 获取Client本人的好友申请列表
//    if(obj.contains("applyList")){
//        UserManager::GetInstance()->appendApplyList(obj["applyList"].toArray());
//    }

    // 同时处理离线消息。再去切换到登录界面
    if(obj.contains("notify_messages")){
        QJsonArray array = obj["notify_messages"].toArray();
        for (const QJsonValue& value : array) {
            int redis_id = value["redis_id"].toInt();
            int friend_id = value["friend_id"].toInt();
            QString messgae = value["message"].toString();
            QString friend_icon = value["friend_icon"].toString();
            if(redis_id == REDIS_ID_FRIEND_ICON_CHANGE){
                emit LoadLocalData::GetInstance()->signalupdateFriendIcon(friend_id,friend_icon);
            }
        }
    }

    // 校验成功，转换到登录界面
    emit signalSwitchToChat();
}

void TcpMsg::slotTcpConnect(ServerInfo si)
{
    if(socket_ == nullptr){
        //qDebug() << "Socket is nullptr.";
        return;
    }
    si_ = si;
    // 尝试连接到聊天服务器
    qDebug() << "Connecting to ChatServer...";
    host_ = si.host;
    port_ = static_cast<uint16_t>(si.port.toUInt());
    socket_->connectToHost(host_, port_);
}

void TcpMsg::slotSendData(REQUEST_ID reqId,QByteArray data)
{
    //qDebug() << "slotSendData current:" << QThread::currentThread() << "socket thread:" << socket_->thread();

    // 每次发送都生成新的 protocol-layer UUID，拼在包头前 36 字节
    QString uuidStr = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QByteArray uuidBytes = uuidStr.toUtf8();
    QByteArray uuidPadded = uuidBytes.leftJustified(36, '\0', true);

    uint16_t id = reqId;
    // 计算数据长度
    quint16 len = static_cast<quint16>(data.size());
    // 创建一个QByteArray用于存储要发送的数据
    QByteArray block;
    QDataStream out(&block,QIODevice::WriteOnly);
    // 写入 UUID (36B)
    out.writeRawData(uuidPadded.constData(), 36);
    // 设置数据流使用网络字节序
    out.setByteOrder(QDataStream::BigEndian);
    // 写入 消息id (2B) + 消息长度 (2B)
    out << id << len;
    // 添加字符数据
    block.append(data);

    // 存入 pending_ack_，等待服务端 ACK
    {
        PendingAck pending;
        pending.block = block;
        pending.reqId = id;
        pending.sendTime = QDateTime::currentMSecsSinceEpoch();
        pending.retryCount = 0;
        pending_ack_[uuidStr] = pending;
    }

    // 确保重试定时器在运行（首次发送启动，后续无操作）
    if (!retryTimer_->isActive()) {
        retryTimer_->start(BASE_RETRY_MS);
    }

    // 检查是否在发送
    if(is_pedding_){
        // 那么就投递到队列
        send_queue_.enqueue(block);
        return;
    }
    // 那么就发送
    is_pedding_ = true;
    current_block_ = block;
    bytes_sent_ = 0;
    // 发送数据
    socket_->write(block);
}

void TcpMsg::onThreadStarted()
{
    socket_ = new QTcpSocket(this);
    timer_  = new QTimer(this);
    reconnectTimer_ = new QTimer(this);
    reconnectTimer_->setSingleShot(true); // 单次定时器，每次触发后需重新 start

    // 重试定时器：固定 3s 间隔扫描 pending_ack_，触发超时重发
    retryTimer_ = new QTimer(this);
    connect(retryTimer_, &QTimer::timeout, this, &TcpMsg::scanRetry);

    connect(reconnectTimer_, &QTimer::timeout, this, [this]() {
        attemptReconnect();
    });

    qDebug() << "Tcp Thread: " << QThread::currentThread();

    // 注册信号
    registerSignal();
}

void TcpMsg::scanRetry()
{
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    bool hasPending = false;

    for (auto it = pending_ack_.begin(); it != pending_ack_.end(); ) {
        const QString& uuid = it.key();
        PendingAck& pending = it.value();

        // 计算当前轮次的超时阈值: 3s * 2^retryCount = 3s, 6s, 12s
        qint64 timeout = BASE_RETRY_MS * (1 << pending.retryCount);
        if (now - pending.sendTime < timeout) {
            hasPending = true;
            ++it;
            continue;
        }

        // 超过最大重试次数 → 放弃，移除
        if (pending.retryCount >= MAX_RETRIES) {
            qDebug() << "Message" << uuid << "retry exhausted ("
                     << pending.retryCount << "attempts), giving up.";
            it = pending_ack_.erase(it);
            continue;
        }

        // 重发（同一 block，同一 UUID → 服务端去重自然生效）
        qDebug() << "Retry" << (pending.retryCount + 1)
                 << "for message" << uuid;
        socket_->write(pending.block);
        pending.sendTime = now;
        pending.retryCount++;
        hasPending = true;
        ++it;
    }

    // 所有条目的 ACK 都回来了 → 停掉定时器，等下次发送时再启动
    if (!hasPending) {
        retryTimer_->stop();
    }
}

