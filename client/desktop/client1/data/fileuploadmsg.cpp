#include "fileuploadmsg.h"
#include <QJsonObject>
#include <QJsonDocument>
#include <QHostAddress>
#include "dialogs/contactuserwidget.h"
#include "usermanager.h"
#include "loadlocaldata.h"

FileUploadMsg::FileUploadMsg()
    : is_conneted_(false)
    , socket_(nullptr)
    , host_("")
    , port_(0)
    , b_recv_pedding_(false)
    , msg_id_(0)
    , msg_len_(0)
    , timer_(nullptr)
    , current_block_(QByteArray())
    , bytes_sent_(0)
    , is_pedding_(false)
{
    // 注册回调函数
    registerFunctionCallbacks();

    // 滑动窗口定时器（500ms 扫描窗口内缺失分片）
    timer_ = new QTimer(this);
    connect(timer_, &QTimer::timeout, this, &FileUploadMsg::scanWindow);
    timer_->start(500);
    // 续传逻辑
    connect(this,&FileUploadMsg::signalContinueUploadFile,this,&FileUploadMsg::slotContinueUploadFile);
    // 继续下载逻辑
    connect(this,&FileUploadMsg::signalContinueDownloadFile,this,&FileUploadMsg::slotContinueDownloadFile);
}

FileUploadMsg::~FileUploadMsg()
{
    delete timer_;
}

void FileUploadMsg::upload_head_icon(REQUEST_ID req_id, int msg_length, QByteArray data)
{
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if(doc.isNull()){
        qDebug() << "Failed to create QJsonDocument";
        return;
    }
    QJsonObject obj = doc.object();

    // 出现错误
    if(obj["code"].toInt() != ERRORCODE::SUCCESS)
    {
        qDebug() << "error code: " << obj["code"].toInt();
        qDebug() << "error message: " << obj["message"].toString();
        emit signalUploadHeadIconIsSuccess(false);
        return;
    }

    int uid = obj["uid"].toInt();
    int seq = obj["seq"].toInt();
    int last_seq = obj["lastseq"].toInt();
    QString md5 = obj["md5"].toString();
    QString file_name = obj["file"].toString();
    int total_size = obj["totol_size"].toInt();
    int trans_size = obj["trans_size"].toInt();

    if(seq == last_seq){
        qDebug() << "md5 = " << md5 << " file_name = " << file_name << " icon upload success.\n";
        // 保存头像信息到本地数据库之后，再触发这个信号 to do ...
        emit LoadLocalData::GetInstance()->signalupdateSelfIcon(file_name);
        // emit signalUploadHeadIconIsSuccess(true);
        return;
    }

    // 继续上传下一个包
    QString file_path = UserManager::GetInstance()->get_file_path(file_name);
    if(file_path == ""){
        qDebug() << "根据 文件名字 无法找到文件信息";
        emit signalUploadHeadIconIsSuccess(false);
        return;
    }

    QFile file(file_path);
    if(!file.open(QIODevice::ReadOnly)){
        qDebug() << "filepath = " << file_path;
        qDebug() << "文件 " << file_path << " 打开失败";
         emit signalUploadHeadIconIsSuccess(false);
        return;
    }

    QFileInfo file_info(file);
    QString fileName = file_info.fileName();

    // 读取文件内容并且发送
    QByteArray buffer;
    // 将文件偏移到指定的位置
    file.seek(trans_size);
    buffer = file.read(MAX_FILE_LEN);

    seq++;

    // 将文件内容转换为 Base64编码（实际上一次传输的文件大小不止是2048，因为base64编码之后，会将数据变大）
    QString base64Data = buffer.toBase64();
    //qDebug() << "Send Seq = " << seq << " Data is " << base64Data;


    QJsonObject send_msg;
    send_msg["uid"] = uid;
    send_msg["filename"] = fileName;
    send_msg["seq"] = seq;
    send_msg["lastseq"] = last_seq;
    send_msg["transferredsize"] = buffer.size() + (seq - 1) * MAX_FILE_LEN;
    send_msg["totolsize"] = total_size;
    send_msg["data"] = base64Data;
    send_msg["md5"] = md5;

    QJsonDocument send_doc(send_msg);
    QByteArray sendData = send_doc.toJson();

    emit signalSendData(ID_UPLOAD_HEAD_ICON_REQ,sendData);

    file.close();
}

void FileUploadMsg::upload_file(REQUEST_ID req_id, int msg_length, QByteArray data)
{
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if(doc.isNull()){
        qDebug() << "Failed to create QJsonDocument";
        return;
    }
    QJsonObject obj = doc.object();
    // 出现错误
    if(obj["code"].toInt() != ERRORCODE::SUCCESS)
    {
        qDebug() << "error code: " << obj["code"].toInt();
        qDebug() << "error message: " << obj["message"].toString();
        return;
    }
    // 解析数据
    int seq = obj["seq"].toInt();
    int last_seq = obj["lastseq"].toInt();
    QString md5 = obj["md5"].toString();
    QString file_name = obj["file"].toString();
    int total_size = obj["totol_size"].toInt();
    int trans_size = obj["trans_size"].toInt();
    int type = obj["type"].toInt();

    // 获取文件信息
    std::shared_ptr<MsgInfo> file_info = UserManager::GetInstance()->get_trans_file(file_name);
    // 继续上传下一个包
    if(file_info == nullptr) {
        qDebug() << "根据 文件名字 无法找到文件信息";
        return;
    }
    //更新已经传输的文件大小
    // ── 滑动窗口 ACK 处理 ──
    int last_acked = obj["last_acked"].toInt();

    // 服务端 last_acked 落后于当前 ack → 中间有分片丢包（服务端检测到空洞）
    if (last_acked < seq && last_acked > 0) {
        qDebug() << "[Client] SERVER detected GAP: file=" << file_name
                 << " last_acked=" << last_acked << " but received seq=" << seq
                 << " window_base=" << file_info->window_base_;
    }

    qDebug() << "[" << file_info->unique_name_ << "] receive seq = " << seq;
    file_info->acked_set_.insert(seq);
    file_info->in_flight_.remove(seq);
    file_info->current_size_ = qMax(file_info->current_size_, (qint64)seq * MAX_FILE_LEN);

    // 累积滑动
    int slide = 0;
    while (file_info->acked_set_.contains(file_info->window_base_ + slide))
        slide++;
    if (slide > 0) {
        qDebug() << "[Client] window slide: file=" << file_name
                 << " base " << file_info->window_base_ << " -> " << (file_info->window_base_ + slide)
                 << " (acked " << slide << " chunks)";
        file_info->window_base_ += slide;
        for (int s = file_info->window_base_ - slide; s < file_info->window_base_; s++) {
            file_info->acked_set_.remove(s);
            file_info->chunk_cache_.remove(s);
        }
    }

    if (file_info->last_seq_ == last_acked) {
        qDebug() << "file_name = " << file_name << " chat image upload success.";
        file_info->_state = TRANSFER_STATE::Upload_Finish;
        emit signalUpdateUploadProgress(file_name);
        return;
    }

    emit signalUpdateUploadProgress(file_name);
    if (file_info->_state != Uploading) return;

    // 发送窗口内新分片
    sendWindow(file_info);
}

void FileUploadMsg::download_file(REQUEST_ID req_id, int msg_length, QByteArray data)
{
    //qDebug() << "收到文件下载的回包";
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if(doc.isNull()){
        qDebug() << "Failed to create QJsonDocument";
        return;
    }
    QJsonObject obj = doc.object();

    // 出现错误
    if(obj["code"].toInt() != ERRORCODE::SUCCESS)
    {
        //qDebug() << "error code: " << obj["code"].toInt();
        //qDebug() << "error message: " << obj["message"].toString();
        //emit signalUploadHeadIconIsSuccess(false);
        return;
    }

    // 解析返回结果
    QString download_file = obj["download_file"].toString();
    QString recv_data = obj["data"].toString();
    int seq = obj["seq"].toInt();
    int last_seq = obj["last_seq"].toInt();
    int total_size = obj["total_size"].toInt();
    int trans_size = obj["trans_size"].toInt();
    QString client_save_path = obj["client_save_path"].toString();
    int download_file_type = obj["download_file_type"].toInt();
    int icon_uid = obj["icon_uid"].toInt();

    // 将data写入文件当中
    QFile file(client_save_path);

    // seq == 1：清空并重新写
    QIODevice::OpenMode mode = QIODevice::WriteOnly;
    if (seq > 1) {
        mode |= QIODevice::Append;
    } else {
        mode |= QIODevice::Truncate;
    }

    if (!file.open(mode)) {
        qWarning() << "Failed to open file:" << client_save_path;
        return;
    }

    // recv_data 是 Base64 字符串 → 先解码
    QByteArray base64_decode_data = QByteArray::fromBase64(recv_data.toUtf8());
    if (base64_decode_data.isEmpty() && !recv_data.isEmpty()) {
        qWarning() << "Base64 decode failed";
        return;
    }
    file.write(base64_decode_data);
    file.flush();
    file.close();

    std::shared_ptr<MsgInfo> msg = nullptr;
    if(download_file_type == Download_File_Type::CHAT_IMAGE || download_file_type == Download_File_Type::CHAT_FILE){
        // 更新下载文件的信息
        msg = UserManager::GetInstance()->get_trans_file(download_file);
        msg->current_size_ = trans_size;
        msg->total_size_ = total_size;
    }

//    std::shared_ptr<DownloadFileInfo> file_info = std::make_shared<DownloadFileInfo>(download_file,seq,last_seq,trans_size,
//                                                                                     total_size,client_save_path,(Download_File_Type)download_file_type, TRANSFER_STATE::Downloading);
//    UserManager::GetInstance()->add_download_file(download_file,file_info);

    if(seq == last_seq){
        qDebug() << "文件下载完成: " << download_file;
        // 文件下载完成,那么就删除缓冲
//        bool ret = UserManager::GetInstance()->remove_download_file(download_file);
//        if(!ret){
//            qDebug() << "删除下载文件的信息失败";
//        }else{
//            qDebug() << "删除下载文件的信息成功";
//        }
        if(download_file_type == Download_File_Type::SELF_HEAD_ICON){
            UserManager::GetInstance()->setSelfIcon(download_file);
            // 通知自己的头像下载完成（主界面）
            emit signalSelfIconDownloadFinished(true);
            return;
        }else if(download_file_type == Download_File_Type::FRIEND_HEAD_ICON){
            // 通知已经加载的条目 或者 别的头像下载完成
            emit signalOtherIconDownloadFinished(download_file);
            return;
        }else if(download_file_type == Download_File_Type::FRIEND_HEAD_ICON_ONLINE){
            int icon_uid = obj["icon_uid"].toInt();
            UserManager::GetInstance()->changeFriendIconByuid(icon_uid,download_file);
            // 通知在线好友的头像下载完成
            emit signalOnlineFriendIconFinished(icon_uid,download_file);
            return;
        }/*else if(download_file_type == Download_File_Type::CHAT_IMAGE){
            //
            auto image = UserManager::GetInstance()->get_trans_file(download_file);
            if(image == nullptr){
                qDebug() << "[ERROR]: cant get chat image info.";
                return;
            }else{
                qDebug() << "[DEBUG]: get chat image info success.";
                qDebug() << "CHAT_MSG_TYPE = " << image->GetMsgType();
            }
        }else if(download_file_type == Download_File_Type::CHAT_FILE){
            //
            auto image = UserManager::GetInstance()->get_chat_image(download_file);
            if(image == nullptr){
                qDebug() << "[ERROR]: cant get chat image info.";
                return;
            }else{
                qDebug() << "[DEBUG]: get chat image info success.";
                qDebug() << "CHAT_MSG_TYPE = " << image->GetMsgType();
            }
        }*/else{
            qDebug() << "Unkown Download_File_Type: " << download_file_type;
        }
        // 通知界面去更新数据
        msg->_state = TRANSFER_STATE::Download_Finish;
        emit signalUpdateDownloadProgress(download_file);
        return;
    }
    // 通知去更新图标
    if(download_file_type == Download_File_Type::CHAT_IMAGE || download_file_type == Download_File_Type::CHAT_FILE){
        emit signalUpdateDownloadProgress(download_file);
    }

    if((download_file_type == Download_File_Type::CHAT_IMAGE ||
        download_file_type == Download_File_Type::CHAT_FILE) &&
            msg->_state != TRANSFER_STATE::Downloading){
        qDebug() << "[DEBUG]: file: " << download_file << " is not downloading.";
        return;
    }

    // 再次请求下载文件
    QJsonObject send_obj;
    send_obj["download_file"] = download_file;
    send_obj["seq"] = seq + 1;
    send_obj["last_seq"] = last_seq;
    send_obj["trans_size"] = trans_size;
    send_obj["total_size"] = total_size;
    send_obj["client_save_path"] = client_save_path;
    send_obj["download_file_type"] = download_file_type;
    send_obj["icon_uid"] = icon_uid;
    QJsonDocument send_doc(send_obj);
    QByteArray send_data = send_doc.toJson(QJsonDocument::Compact);
    emit signalSendData(ID_DOWN_LOAD_FILE_REQ,send_data);
}

void FileUploadMsg::imgChatContinueUpload(REQUEST_ID req_id, int msg_length, QByteArray data)
{
    //qDebug() << "收到聊天的图片文件续传的回包";
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if(doc.isNull()){
        qDebug() << "Failed to create QJsonDocument";
        return;
    }
    QJsonObject obj = doc.object();

    // 出现错误
    if(obj["code"].toInt() != ERRORCODE::SUCCESS)
    {
        qDebug() << "error code: " << obj["code"].toInt();
        qDebug() << "error message: " << obj["message"].toString();
        return;
    }

    // 解析返回结果
    QString unique_name = obj["unique_name"].toString();
    QString md5 = obj["md5"].toString();
    int last_ack_seq = obj["last_ack_seq"].toInt();
    int last_seq = obj["last_seq"].toInt();
    int total_size = obj["total_size"].toInt();
    int trans_size = obj["trans_size"].toInt();
    int uid = obj["uid"].toInt();

    // 查看文件是否存在
    auto msg_info = UserManager::GetInstance()->get_trans_file(unique_name);
    if(msg_info == nullptr){
        qDebug() << "Cant find unique_name = " << unique_name;
        return;
    }

    // last_ack_seq 是服务端连续确认值
    if(last_ack_seq >= last_seq){
        qDebug() << "unique_name = " << unique_name << " upload already complete, last_ack_seq=" << last_ack_seq;
        msg_info->_state = TRANSFER_STATE::Upload_Finish;
        emit signalUpdateUploadProgress(unique_name);
        return;
    }

    // 设置窗口位置：从 last_acked + 1 开始，走滑动窗口
    msg_info->window_base_ = last_ack_seq + 1;
    msg_info->last_seq_    = last_seq;
    msg_info->total_size_  = total_size;
    msg_info->current_size_= last_ack_seq * MAX_FILE_LEN;
    msg_info->_state       = TRANSFER_STATE::Uploading;
    qDebug() << "[Client] resume: unique_name=" << unique_name
             << " start from seq=" << msg_info->window_base_
             << " last_seq=" << last_seq;
    sendWindow(msg_info);
}

void FileUploadMsg::fileContinueDownload(REQUEST_ID req_id, int msg_length, QByteArray data)
{
    //qDebug() << "收到聊天的图片文件续传的回包";
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if(doc.isNull()){
        qDebug() << "Failed to create QJsonDocument";
        return;
    }
    QJsonObject obj = doc.object();

    // 出现错误
    if(obj["code"].toInt() != ERRORCODE::SUCCESS)
    {
        qDebug() << "error code: " << obj["code"].toInt();
        qDebug() << "error message: " << obj["message"].toString();
        return;
    }

    // 解析返回结果
    QString download_file = obj["download_file"].toString();
    int total_size = obj["total_size"].toInt();
    int trans_size = obj["trans_size"].toInt();
    int last_seq = obj["last_seq"].toInt();
    int seq = obj["seq"].toInt();
    int uid = obj["uid"].toInt();
    QString client_save_path = obj["client_save_path"].toString();
    int download_file_type = obj["download_file_type"].toInt();

    // 查看文件是否存在
    auto msg_info = UserManager::GetInstance()->get_trans_file(download_file);
    if(msg_info == nullptr){
        qDebug() << "Cant find unique_name = " << msg_info->unique_name_;
        return;
    }

    // 下载完成
    if(trans_size >= total_size){
        qDebug() << "unique_name = " << download_file << " file download success.\n";
        return;
    }

    // 再次请求下载文件
    QJsonObject send_obj;
    send_obj["download_file"] = download_file;
    send_obj["seq"] = seq + 1;
    send_obj["last_seq"] = last_seq;
    send_obj["trans_size"] = trans_size;
    send_obj["total_size"] = total_size;
    send_obj["client_save_path"] = client_save_path;
    send_obj["uid"] = uid;
    send_obj["toktn"] = UserManager::GetInstance()->GetToken();
    send_obj["download_file_type"] = download_file_type;
    QJsonDocument send_doc(send_obj);
    QByteArray send_data = send_doc.toJson(QJsonDocument::Compact);
    emit signalSendData(ID_DOWN_LOAD_FILE_REQ,send_data);
}

void FileUploadMsg::registerFunctionCallbacks()
{
    handlers_[ID_UPLOAD_HEAD_ICON_RSP] = std::bind(&FileUploadMsg::upload_head_icon,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3);
    handlers_[ID_IMAGE_CHAT_MSG_RSP] = std::bind(&FileUploadMsg::upload_file,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3);
    handlers_[ID_DOWN_LOAD_FILE_RSP] = std::bind(&FileUploadMsg::download_file,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3);
    handlers_[ID_IMG_CHAT_CONTINUE_UPLOAD_RSP] = std::bind(&FileUploadMsg::imgChatContinueUpload,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3);
    handlers_[ID_FILE_CONTINUE_DOWNLOAD_RSP] = std::bind(&FileUploadMsg::fileContinueDownload,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3);
}

void FileUploadMsg::registerSignal()
{
    // 与服务器建立连接
    connect(socket_,&QTcpSocket::connected,[&](){
        qDebug() << "connect to ResourceServer(host:"<< host_ << ":" << port_ << ") success.";
        is_conneted_ = true;
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
                const int HEADER_SIZE = static_cast<int>(sizeof(qint16) + sizeof(qint32));
                if(buffer_.size() < HEADER_SIZE){
                    return;
                }
                // 先拷贝头部出来解析，避免 buffer_ = mid() 时 stream 内部指针失效
                QByteArray headerData = buffer_.left(HEADER_SIZE);
                QDataStream stream(headerData);
                stream.setVersion(QDataStream::Qt_5_0);
                stream >> msg_id_ >> msg_len_;
                buffer_ = buffer_.mid(HEADER_SIZE);
                b_recv_pedding_ = true;
            }
            // 剩余数据不足
            if(buffer_.size() < msg_len_){
                // b_recv_pedding_ = true;
                return;
            }

            // 一条消息读取完成，那么就调用回调函数来处理
            // 消息主体
            QByteArray messageBody = buffer_.mid(0, msg_len_);
            //qDebug() << "receive from ChatServer: " << messageBody ;
            buffer_ = buffer_.mid(msg_len_);
            b_recv_pedding_ = false;

            //qDebug() << "[ResourceServer] msg_id = " << msg_id_;

            // 添加对应的参数
            handlers_[msg_id_]((REQUEST_ID)msg_id_,msg_len_,messageBody);

            }
        });

    // 处理出现错误的逻辑
    connect(socket_,static_cast<void(QTcpSocket::*)(QTcpSocket::SocketError)>(&QTcpSocket::error),[&](QTcpSocket::SocketError socketError){
        qDebug() << "Connect to ResourceServer(host:"<< host_ << ":" << port_ << ") failed!";
        qDebug() << "Error: " << socket_->errorString();

        /*
        // 获取更详细的网络状态信息
        qDebug() << "Socket state:" << socket_.state();
        qDebug() << "Peer address:" << socket_.peerAddress().toString();
        qDebug() << "Peer port:" << socket_.peerPort();
        */

        switch (socketError) {
            case QTcpSocket::ConnectionRefusedError:
                qDebug() << "Connection refused!";
                break;
            case QTcpSocket::RemoteHostClosedError:
                qDebug() << "Remote host closed Connection!";
                break;
            case QTcpSocket::SocketTimeoutError:
               qDebug() << "Connection Timeout!";
               break;
            case QTcpSocket::HostNotFoundError:
                qDebug() << "Host not found!";
                break;
            case QTcpSocket::NetworkError:
                qDebug() << "Network error!";
                break;
            default:
                qDebug() << "Unknow error in Socket!";
                break;
        }
    });

    // 处理连接断开
    QObject::connect(socket_, &QTcpSocket::disconnected, [&]() {
       qDebug() << "Disconnected from ResourceServer.";
    });

    // 统一发送数据
    QObject::connect(this, &FileUploadMsg::signalSendData,this,&FileUploadMsg::slotSendData);

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

    // 建立与ResourceServer的socket连接
    connect(this,&FileUploadMsg::signalConnToResServer,this,&FileUploadMsg::slotConnToResServer);
}

void FileUploadMsg::slotConnToResServer(ServerInfo si)
{
    si_ = si;
    // 尝试连接到聊天服务器
    qDebug() << "Connecting to ResourceServer...";
    host_ = si.res_host_;
    port_ = static_cast<uint16_t>(si.res_port_.toUInt());
    socket_->connectToHost(host_, port_);
}

void FileUploadMsg::slotTcpConnect(ServerInfo si)
{
    qDebug() << "slotTcpConnect current:" << QThread::currentThread()
             << "socket thread:" << socket_->thread();

    qDebug()<< "receive tcp connect signal";
    // 尝试连接到服务器
    qDebug() << "Connecting to server...";
    host_ = si.host;
    port_ = static_cast<uint16_t>(si.port.toUInt());
    socket_->connectToHost(host_, port_);
}

void FileUploadMsg::slotSendData(REQUEST_ID reqId,QByteArray data)
{
//    qDebug() << "slotSendData current:" << QThread::currentThread()
//             << "socket thread:" << socket_->thread();

    qint16 id = reqId;
    // 计算数据长度
    qint32 len = static_cast<quint16>(data.size());
    // 创建一个QByteArray用于存储要发送的数据
    QByteArray block;
    QDataStream out(&block,QIODevice::WriteOnly);
    // 设置数据流使用网络字节序
    out.setByteOrder(QDataStream::BigEndian);
    // 写入消息id 和 消息长度
    out << id << len;
    // 添加字符数据
    block.append(data);

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

void FileUploadMsg::slotContinueUploadFile(QString unique_name)
{
    auto msg_info = UserManager::GetInstance()->get_trans_file(unique_name);
    if (msg_info == nullptr) {
        return;
    }
    if ((msg_info->seq_) * MAX_FILE_LEN >= msg_info->total_size_) {
        qDebug() << "file has sent finished";
        return;
    }
    QJsonObject sendObj;
    sendObj["md5"] = msg_info->md5_;
    sendObj["unique_name"] = msg_info->unique_name_;
    sendObj["uid"] = UserManager::GetInstance()->getUid();
    sendObj["token"] = UserManager::GetInstance()->GetToken();

    QJsonDocument doc(sendObj);
    auto send_data = doc.toJson();

    emit signalSendData(ID_IMG_CHAT_CONTINUE_UPLOAD_REQ, send_data);
}

void FileUploadMsg::slotContinueDownloadFile(QString unique_name)
{
    // to do ...
    auto msg_info = UserManager::GetInstance()->get_trans_file(unique_name);
    if (msg_info == nullptr) {
        return;
    }
    if ((msg_info->seq_) * MAX_FILE_LEN >= msg_info->total_size_) {
        qDebug() << "file has sent finished";
        return;
    }
    QJsonObject sendObj;
    sendObj["unique_name"] = msg_info->unique_name_;
    sendObj["uid"] = UserManager::GetInstance()->getUid();
    sendObj["token"] = UserManager::GetInstance()->GetToken();

    QJsonDocument doc(sendObj);
    auto send_data = doc.toJson();

    emit signalSendData(ID_FILE_CONTINUE_DOWNLOAD_REQ, send_data);
}

void FileUploadMsg::onThreadStarted()
{
    socket_ = new QTcpSocket(this);
    timer_  = new QTimer(this);
    qDebug() << "File Thread: " << QThread::currentThread();
    // 连接信号与槽函数
    registerSignal();
}


// 滑动窗口发送：一次性发送窗口内（最多 8 个）未发出的分片
void FileUploadMsg::sendWindow(std::shared_ptr<MsgInfo> info)
{
    static const int WINDOW_SIZE = 8;
    int lastSeq = (info->total_size_ + MAX_FILE_LEN - 1) / MAX_FILE_LEN;
    int end = qMin(info->window_base_ + WINDOW_SIZE, info->last_seq_ + 1);

    for (int seq = info->window_base_; seq < end; seq++) {
        if (info->acked_set_.contains(seq)) continue;
        if (info->in_flight_.contains(seq)) continue;

        // 读磁盘分片
        QFile file(info->text_or_url_);
        if (!file.open(QIODevice::ReadOnly)) continue;
        file.seek((seq - 1) * MAX_FILE_LEN);
        QByteArray buffer = file.read(MAX_FILE_LEN);
        file.close();
        if (buffer.isEmpty()) continue;

        // 缓存（重传用）
        info->chunk_cache_[seq] = buffer;

        // 构造消息并发送
        QJsonObject msg;
        msg["filename"] = info->unique_name_;
        msg["seq"] = seq;
        msg["lastseq"] = info->last_seq_;
        msg["transferredsize"] = qMin((qint64)(seq) * MAX_FILE_LEN, info->total_size_);
        msg["totolsize"] = info->total_size_;
        msg["data"] = QString(buffer.toBase64());
        msg["md5"] = info->md5_;
        msg["type"] = CHAT_FILE;
        QJsonDocument doc(msg);
        QByteArray sendData = doc.toJson();
        // 发送数据
        emit signalSendData(ID_IMAGE_CHAT_MSG_REQ, sendData);
        info->in_flight_.insert(seq);
    }
}

// 500ms 定时扫描：窗口内未确认的分片从缓存重传
void FileUploadMsg::scanWindow()
{
    static const int WINDOW_SIZE = 8;
    auto& uploads = UserManager::GetInstance()->getTransFiles();

    for (auto it = uploads.begin(); it != uploads.end(); ++it) {
        auto info = it->second;
        if (info->_state != Uploading) continue;

        int end = qMin(info->window_base_ + WINDOW_SIZE, info->last_seq_ + 1);

        bool firstRetrans = true;
        for (int seq = info->window_base_; seq < end; seq++) {
            qDebug() << "["  << info->unique_name_ << "]" << " Reupdate seq = " << seq;
            if (info->acked_set_.contains(seq)) continue;
            info->in_flight_.remove(seq);
            if (info->in_flight_.contains(seq)) continue;
            if (!info->chunk_cache_.contains(seq)) continue;
            if (firstRetrans) {
                qDebug() << "[Client] PACKET LOSS detected: file=" << info->unique_name_
                         << " window=[" << info->window_base_ << ".." << (end-1) << "]"
                         << " in_flight=" << info->in_flight_.size();
                firstRetrans = false;
            }
            // 重传
            qDebug() << "[Client] RETRANSMIT seq=" << seq << " file=" << info->unique_name_;
            QByteArray buffer = info->chunk_cache_[seq];
            QJsonObject msg;
            msg["filename"] = info->unique_name_;
            msg["seq"] = seq;
            msg["lastseq"] = info->last_seq_;
            msg["transferredsize"] = qMin((qint64)(seq) * MAX_FILE_LEN, info->total_size_);
            msg["totolsize"] = info->total_size_;
            msg["data"] = QString(buffer.toBase64());
            msg["md5"] = info->md5_;
            msg["type"] = CHAT_FILE;
            QJsonDocument doc(msg);
            emit signalSendData(ID_IMAGE_CHAT_MSG_REQ, doc.toJson());
            info->in_flight_.insert(seq);   // 标记为重传中，避免 sendWindow 重复发送
        }
    }
}
