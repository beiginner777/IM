#ifndef TCPMSG_H
#define TCPMSG_H

#include "core/global.h"
#include "core/userdata.h"
#include <QMap>
#include <QDateTime>

struct PendingAck {
	QByteArray block;
	quint16    reqId;
	qint64     sendTime;
	int        retryCount;
};

class TcpMsg : public QObject,
               public SingleTon<TcpMsg>,
               public std::enable_shared_from_this<TcpMsg>
{
    Q_OBJECT
public:
    friend class SingleTon<TcpMsg>;
    using functionCallback = std::function<void(REQUEST_ID,int,QByteArray)>;

    TcpMsg();
    ~TcpMsg();

    void verify(REQUEST_ID reqId,int len,QByteArray bytes);
    void search(REQUEST_ID reqId, int len, QByteArray data);
    void applyResult(REQUEST_ID reqId, int len, QByteArray data);
    void receiveFriendApply(REQUEST_ID reqId, int len, QByteArray data);
    void authApply(REQUEST_ID reqId, int len, QByteArray data);
    void accessApply(REQUEST_ID reqId, int len, QByteArray data);
    void sendTextMsg(REQUEST_ID reqId, int len, QByteArray data);
    void receiveNewTextMsg(REQUEST_ID reqId, int len, QByteArray data);
    void notifyOffLine(REQUEST_ID reqId, int len, QByteArray data);
    void heartCheck(REQUEST_ID reqId, int len, QByteArray data);
    void loadChatList(REQUEST_ID reqId, int len, QByteArray data);
    void createPrivateChatThread(REQUEST_ID reqId, int len, QByteArray data);
    void loadConnlist(REQUEST_ID reqId, int len, QByteArray data);
    void sendImgMsg(REQUEST_ID reqId, int len, QByteArray data);
    void loadFriendApplyList(REQUEST_ID reqId, int len, QByteArray data);
    void friendIconChange(REQUEST_ID reqId, int len, QByteArray data);
    void RecvNewImgMsg(REQUEST_ID reqId,int len, QByteArray data);
    void LoadChatMsg(REQUEST_ID reqId,int len, QByteArray data);

private:
    void registerFunctionCallbacks();
    void registerSignal();
    void attemptReconnect();

private:
    QTcpSocket* socket_; // 与 ChatServer 通信的 socket
    QString host_; // 对应的 ChatServer 主机
    uint16_t port_; // 对应的 ChatServer 端口
    QByteArray buffer_; // 接受缓冲区
    bool b_recv_pedding_; // 一条信息是否接收完成
    quint16 msg_id_; // 接收消息id
    quint16 msg_len_; // 接收消息长度
    std::map<int,functionCallback> handlers_; // 回调函数
    QTimer* timer_; // 心跳检测定时器（定时向服务器发送心跳检测的请求）
    QTimer* reconnectTimer_; // 重连的定时器
    //发送队列
    QQueue<QByteArray> send_queue_;
    //正在发送的包
    QByteArray current_block_;
    //当前已发送的字节数
    qint64 bytes_sent_;
    //是否正在发送
    std::atomic_bool is_pedding_;
    // 服务器信息
    ServerInfo si_;
    // 重连的次数
    int reconnectAttempts_;
    // 是否处于断线重连的状态
    bool reconnectEnabled_;
    // pending ACK: key=UUID, value=待确认的消息包
    QMap<QString, PendingAck> pending_ack_;
    // 重试定时器：固定 3s 间隔扫描 pending_ack_
    QTimer* retryTimer_;
    void scanRetry();
    static constexpr int MAX_RETRIES = 3;
    static constexpr int BASE_RETRY_MS = 3000;

public slots:
    void slotTcpConnect(ServerInfo si);
    void slotSendData(REQUEST_ID reqId,QByteArray data);
    void onThreadStarted();

signals:
    void signalConnectSuccess(bool);
    void signalSendData(REQUEST_ID reqId,QByteArray data);
    void signalReceiveFriendApply(std::shared_ptr<AddFriendApply>);
    void signalSwitchToChat();
    void signalReceiveNewTextMsg(std::vector<std::shared_ptr<ChatDataBase>>);
    void signalRecvNewImgMsg(std::shared_ptr<ImageDataBase>);
    void signalSearchUser(std::shared_ptr<SearchInfo> si);
    void signalAddNewChatUserWidget(std::shared_ptr<UserInfo>);
    void signalAddNewConnUserWidget(std::shared_ptr<FriendInfo>);
    void signalNotifyOffLine();
    void signalLoadChatList(QJsonObject obj);
    void signalRecvCreatePrivateChat(QJsonObject obj);
    void signalNotifyChatPageMsgStatus(int thread_id,std::vector<std::shared_ptr<TextChatData>> chat_datas);
    void signalLoadConnList(QJsonObject obj);
    void signalLoadFriendApplyList(QJsonObject obj);
    void signalNotifyChatPageImgStatus(int, std::shared_ptr<ImageDataBase>);
    void signalLoadChatMsg(QJsonObject obj);
};

#endif // TCPMSG_H
