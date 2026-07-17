#ifndef FILEUPLOADMSG_H
#define FILEUPLOADMSG_H

#include "core/global.h"

class ImageDataBase;

class FileUploadMsg: public QObject,
                     public SingleTon<FileUploadMsg>,
                     public std::enable_shared_from_this<FileUploadMsg>
{
    Q_OBJECT
public:
    friend class SingleTon<FileUploadMsg>;
    using functionCallback = std::function<void(REQUEST_ID,int,QByteArray)>;

    FileUploadMsg();
    ~FileUploadMsg();

    void upload_head_icon(REQUEST_ID req_id,int msg_length, QByteArray data);
    void upload_file(REQUEST_ID req_id, int msg_length, QByteArray data);
    void download_file(REQUEST_ID req_id, int msg_length, QByteArray data);
    void imgChatContinueUpload(REQUEST_ID req_id, int msg_length, QByteArray data);
    void fileContinueDownload(REQUEST_ID req_id, int msg_length, QByteArray data);
    bool is_connected() { return is_conneted_; }

private:
    void registerFunctionCallbacks();
    void registerSignal();

private:
    bool is_conneted_;

    QTcpSocket* socket_; // 与 ResourceServer 通信的 socket
    QString host_; // 对应的 ResourceServer 主机
    uint16_t port_; // 对应的 ResourceServer 端口

    // 接收消息需要的变量（切包）
    QByteArray buffer_; // 接受缓冲区
    bool b_recv_pedding_; // 一条信息是否接收完成
    qint16 msg_id_; // 接收消息id
    qint32 msg_len_; // 接收消息长度
    std::map<int,functionCallback> handlers_; // 回调函数

    QTimer* timer_;

    // 发送消息需要的变量（异步发送队列，保证数据有序发送）
    QQueue<QByteArray> send_queue_; // 异步发送队列
    QByteArray current_block_; //正在发送的包
    qint32 bytes_sent_; //当前已发送的字节数
    std::atomic_bool is_pedding_; //是否正在发送
    ServerInfo si_;

public slots:
    void slotConnToResServer(ServerInfo si);
    void slotTcpConnect(ServerInfo si);
    void slotSendData(REQUEST_ID reqId,QByteArray data);
    void slotContinueUploadFile(QString unique_name);
    void slotContinueDownloadFile(QString unique_name);
    void onThreadStarted();
    // 滑动窗口
    void sendWindow(std::shared_ptr<MsgInfo> info);
    void scanWindow();
    void onResumeUploadRsp(REQUEST_ID reqId, int msgLen, QByteArray data);
    void queryResumeProgress(std::shared_ptr<MsgInfo> info);

signals:
    void signalConnToResServer(ServerInfo si);
    void signalSendData(REQUEST_ID reqId,QByteArray data);
    void signalUploadHeadIconIsSuccess(bool);
    void signalSelfIconDownloadFinished(bool);
    void signalOtherIconDownloadFinished(QString);
    void signalOnlineFriendIconFinished(int,QString);
    void signalContinueUploadFile(QString unique_name);
    void signalContinueDownloadFile(QString unique_name);
    void sig_update_upload_progress(std::shared_ptr<MsgInfo>);
    void signalRecvNewImgMsg(std::shared_ptr<ImageDataBase>);
    void signalUpdateUploadProgress(QString);
    void signalUpdateDownloadProgress(QString);
};

#endif // FILEUPLOADMSG_H
