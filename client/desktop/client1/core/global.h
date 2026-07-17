/******************************************************************************
 *
 * @file       global.h
 * @brief      XXXX Function
 *
 * @author     Jerry
 * @date       2025/07/27
 * @history
 *****************************************************************************/

#ifndef GLOBAL_H
#define GLOBAL_H

#include <QWidget>
#include <QLabel>
#include <functional>
#include <QDir>
#include <QSettings>
#include <QDebug>
#include <QTcpSocket>
#include <QString>
#include <QByteArray>
#include <QObject>
#include <QDebug>
#include <QDataStream>
#include <QIODevice>
#include <QAbstractSocket>  // 包含Socket错误类型的定义
#include <QLineEdit>
#include <memory>
#include <map>
#include <QAction>
#include <QListWidget>
#include <QEvent>
#include <QWheelEvent>
#include <QScrollBar>
#include <QScrollArea>
#include <QPixmap>
#include <QSize>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QTimer>
#include <QGridLayout>
#include <QPixmap>
#include <QFont>
#include <QSpacerItem>
#include <QFrame>
#include <QColor>
#include <QPoint>
#include <QTextEdit>
#include <QTextBlock>
#include <QLabel>
#include <QPoint>
#include <QDialog>
#include <QPushButton>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonValue>
#include <QRandomGenerator>
#include <unordered_map>
#include <QUuid>
#include <QDateTime>
#include <QDebug>
#include <QThread>
#include <QMetaType>
#include <QAbstractSocket>
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>
#include <QQueue>
#include <QCoreApplication>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QCryptographicHash>
#include <QPixmap>
#include <QProgressBar>
#include <QAbstractListModel>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QApplication>
#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <algorithm>
#include <deque>
#include <QHash>
#include <QFont>
#include <QTextLayout>
#include <QTextOption>
#include <cmath>
#include <QFileIconProvider>
#include <QSet>
#include <QGraphicsView>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QPointer>
#include <QWheelEvent>
#include <QWindow>
#include <QScreen>

#include "singleton.h"

// sqlite数据库文件的存储路径
#define DB_FILE_LOCATION "D:\\QtFile\\project\\Instant_Messaging_System\\LocalData\\LocalData.db"

// 图片大小
#define PIC_MAX_WIDTH 250 // 图片的最大宽度
#define PIC_MAX_HEIGHT 160 // 图片的最大高度

const int tipOffset = 5;
//申请好友标签输入框最低长度
const int MIN_APPLY_LABEL_ED_LEN = 80;
// closeLabel
const int CLOSE_LABEL_LENGTH = 15;
//
const int CHAT_COUNT_PER_PAGE = 13;
// ChatUserList一次向服务器加载的数量
const int CHATUSERLIST_LOAD_PAGESIZE = 10;
// ChatUserList一次性向本地数据库加载的数量
const int CHATUSERLIST_LOAD_PAGESIZE_FROM_LOCAL = 15;
// FriendApplyPage 一次向服务器加载的数量
const int FRIENDAPPLYLIST_LOAD_PAGESIZE = 10;
// FriendApplyPage 一次向本地数据库加载的数量
const int FRIENDAPPLYLIST_LOAD_PAGESIZE_FROM_LOCAL = 15;
// ChatInterface 一次向服务器加载的数量
const int CHATMESSAGE_LOAD_PAGESIZE = 20;
// ChatInterface 一次向本地数据库加载的数量
const int CHATMESSAGE_LOAD_PAGESIZE_FROM_LOCAL = 2;
// 上传文件的一个包的最大长度
const int MAX_FILE_LEN = 2048;
// Emoji面板的大小
const int EMOJI_BUTTON_SIZE = 50;
const int EMOJI_SIZE = 21;
const int EMOJI_INTERVAL = 10;
const int EMOJI_COUNT_EVERY_ROW = 8;
const int EMOJI_COUNT = 24;
// 文件ICON大小
const int FILE_ICON_WIDTH = 300;
const int FILE_ICON_HEIGHT = 100;
// 重连的最大次数
const int MAX_RECONNECT_TIMES = 10;

extern QString Gate_Url_Prefix;
extern QString GateServerHost;
extern QString GateServerPort;
extern QString getVerifyCodeAddr;
extern QString registerUserAddr;
extern QString loginAddr;

extern std::function<void(QWidget*)> repolish;

struct ServerInfo
{
    ServerInfo() = default;
    qint32 uid;
    QString host;
    QString port;
    QString token;
    QString res_host_;
    QString res_port_;
};
Q_DECLARE_METATYPE(ServerInfo)

//
void showTip(QLabel* label,QString str,bool isTrue);
// 将 "YYYY-MM-DD HH:MM:SS" 转换为 "HH:MM"
QString convertDateTimeString(const QString &dateTimeStr);
// 注册元对象
void registerMetaType();
// 生成头像的随机文件名
QString generateUniqueIconName();
// 根据头像的url来返回QPixmap
QPixmap returnPixMapByUrl(QString url);
// 根据文件名字来生成唯一的标识符
QString generateUniqueFileName(const QString& originalName);
// 根据文件的路径来计算Hash值
QString calculateFileHash(const QString& filePath);
// 计算时间
QString formatTime(const QDateTime &time);
// 将字符串转换为 QDateTime
QDateTime returnQDateTimeByQString(QString mysqlStr);

enum REQUEST_ID
{
    ID_GET_VERIFY_CODE = 1001, //获取验证码

    ID_REG_USER = 1002, //注册用户

    ID_LOGIN, // 登录

    ID_CHAT_LOGIN,// 发送长连接

    ID_CHAT_LOGIN_RSP, // tcpMsg接收到服务器的消息

    ID_SEARCH_USER_REQ, //用户搜索请求
    ID_SEARCH_USER_RSP, //搜索用户回包

    ID_APPLY_FRIEND_REQ, // 申请好友
    ID_APPLY_FRIEND_RSP, // 申请好友的回包

    ID_NOTIFY_ADD_FRIEND_REQ, // 服务器通知有好友请求

    ID_AUTH_FRIEND_REQ, // 验证好友申请的请求
    ID_AUTH_FRIEND_RSP, // 验证好友申请的回包

    ID_NOTIFY_ACCESS_VERIFY, // 服务器通知好友申请被对方通过了

    ID_TEXT_CHAT_MSG_REQ, // 发送聊天消息
    ID_TEXT_CHAT_MSG_RSP, // 回包

    ID_NOTIFY_TEXT_CHAT_MSG_REQ, // 有新的聊天消息

    ID_NOTIFY_OFFLINE, // 通知下线

    ID_HEADT_CHECK_REQ, // 心跳检测的请求
    ID_HEADT_CHECK_RSP, // 心跳检测的回包

    ID_LOAD_CHAT_THREAD_REQ,// 加载聊天列表请求
    ID_LOAD_CHAT_THREAD_RSP, // 加载聊天列表回包

    ID_CREATE_PRIVATE_CHAT_THREAD_REQ, // 创建私聊会话请求
    ID_CREATE_PRIVATE_CHAT_THREAD_RSP, // 创建私聊会话回包

    ID_LOAD_MORE_FRIEND_REQ, // 加载好友列表的请求
    ID_LOAD_MORE_FRIEND_RSP,  // 加载好友列的回包

    ID_UPLOAD_HEAD_ICON_REQ, // 上传头像请求
    ID_UPLOAD_HEAD_ICON_RSP, // 上传头像回包

    ID_UPLOAD_FILE_REQ, // 上传文件请求
    ID_UPLOAD_FILE_RSP, // 上传文件回包

    ID_SYNC_FILE_REQ, // 同步文件请求（续传）
    ID_SYNC_FILE_RSP, // 同步文件回包（续传）

    ID_GET_NOTIFY_MESSAGE_REQ, // 获取通知消息的请求
    ID_GET_NOTIFY_MESSAGE_RSP, // 获取通知消息的回包

    ID_IMAGE_CHAT_MSG_REQ, // 发送图片的请求
    ID_IMAGE_CHAT_MSG_RSP, // 发送图片的回包

    ID_DOWN_LOAD_FILE_REQ, //下载文件请求
    ID_DOWN_LOAD_FILE_RSP, //下载文件回复

    ID_LOAD_FRIEND_APPLY_REQ, // 加载好友申请请求
    ID_LOAD_FRIEND_APPLY_RSP, // 加载好友申请回包

    ID_IMG_CHAT_CONTINUE_UPLOAD_REQ, // 聊天图片续传请求
    ID_IMG_CHAT_CONTINUE_UPLOAD_RSP, // 聊天图片续传回包

    ID_NOTIFY_FRIEND_ICON_CHANGE, // 通知有好友的头像变更

    ID_NOTIFY_CHAT_IMAGE_MSG, // 收到聊天图片消息

    ID_LOAD_CHAT_MSG_REQ, // 加载聊天消息的请求
    ID_LOAD_CHAT_MSG_RSP, // 加载聊天消息的回包

    ID_FILE_CONTINUE_DOWNLOAD_REQ, // 文件续传下载请求
    ID_FILE_CONTINUE_DOWNLOAD_RSP, // 文件续传下载回包
    ID_RESUME_UPLOAD_REQ, // 断点续传 - 查询上传进度
    ID_RESUME_UPLOAD_RSP, // 断点续传 - 返回进度
};
Q_DECLARE_METATYPE(REQUEST_ID)

enum MODULES
{
    REGISTERMOD = 0,
    LOGINMOD,
};
Q_DECLARE_METATYPE(MODULES)

enum ERRORCODE
{
    SUCCESS = 0,
    ERROR_NET,
    ERROR_JSON
};

enum ChatUIMode
{
    SearchMode, // 搜索模式
    ChatMode, // 聊天模式
    ContactMode, // 联系模式
    SettingMode, // 设置模式
};

enum ListItemType{
    CHAT_USER_ITEM, //聊天用户
    CONTACT_USER_ITEM, //联系人用户
    SEARCH_USER_ITEM, //搜索到的用户
    ADD_USER_TIP_ITEM, //提示添加用户
    INVALID_ITEM,  //不可点击条目
    GROUP_TIP_ITEM, //分组提示条目
    LINE_ITEM,  //分割线
    APPLY_FRIEND_ITEM, //好友申请
    LOGINED_USER_ITEM // 历史登录用户
};

enum ChatRole
{
    self,
    Other,
    System
};

enum ClickLbState
{
    normal,
    //hover,
    //press,
    select,
    //select_hover,
    //select_press
};

// 聊天列表中每一台聊天记录的类型
enum CHAT_THREAD_TYPE
{
    CHAT_THREAD_TYPE_PRIVATE = 1, // 私聊
    CHAT_THREAD_TYPE_GROUP = 2, // 群聊
};

// 聊天消息的类型
enum CHAT_MSG_TYPE
{
    TEXT_MSG, // 文本消息
    PIC_MSG, // 图片信息
    FILE_MSG // 文件信息
};

//
enum MsgStatus{
    UN_READ = 0,  // 正在发送
    SEND_FAILED = 1,  //发送失败
    READED = 2,  // 上传成功
    //UN_UPLOAD = 3 // 未上传完成
};
Q_DECLARE_METATYPE(MsgStatus)

// 传输图片的类型
enum TRANSFER_TYPE
{
    Upload,
    Download
};
Q_DECLARE_METATYPE(TRANSFER_TYPE)

// 传输图片时的状态
enum TRANSFER_STATE
{
    None,
    Downloading,
    Download_Finish,
    Uploading,
    Upload_Finish,
    Paused,
    Completed,
    Upload_Failed,
    Download_Failed,
    Upload_Paused,
    Download_Paused
};
Q_DECLARE_METATYPE(TRANSFER_STATE)

struct MsgInfo
{
    MsgInfo() = default;
    MsgInfo(CHAT_MSG_TYPE msgtype, QString text_or_url, QPixmap pixmap, QString unique_name, qint64 total_size, QString md5)
    :msg_type_(msgtype), text_or_url_(text_or_url), priview_pix_(pixmap),unique_name_(unique_name),total_size_(total_size),
        current_size_(0),seq_(1),md5_(md5),window_base_(1),last_seq_(0)
    {
	    last_seq_ = (total_size_ + MAX_FILE_LEN - 1) / MAX_FILE_LEN;
	}

    CHAT_MSG_TYPE msg_type_;// 消息的类型
    QString text_or_url_;// 表示文件和图像的本地路径
    QPixmap priview_pix_;// 文件和图片的缩略图
    QString unique_name_; // 文件唯一名字
    qint64 total_size_; // 文件总大小
    qint64 current_size_; // 传输大小
    qint64 seq_;          // 传输序号
    QString md5_;         // 文件md5
    TRANSFER_TYPE _type; // 上传 or 下载
    TRANSFER_STATE _state; // 传输状态
	// 滑动窗口字段

	int window_base_;

	QSet<int> acked_set_;

	QSet<int> in_flight_;

	QHash<int, QByteArray> chunk_cache_;
	int last_seq_;
};
Q_DECLARE_METATYPE(MsgInfo);

enum Download_File_Type
{
    SELF_HEAD_ICON, // 自己的头像
    FRIEND_HEAD_ICON, // 刚上线去加载别人的头像
    FRIEND_HEAD_ICON_ONLINE,// 在线去加载别人的头像
    CHAT_IMAGE, // 聊天图片
    CHAT_FILE, // 文件
    NONE //
};

enum REDIS_ID
{
    REDIS_ID_FRIEND_ICON_CHANGE, // 用户信息变更
};

// 好友申请状态
enum  FRIEND_APPLY
{
    UN_HANDLE,
    ACCEPTED,
    REFUSED
};
Q_DECLARE_METATYPE(FRIEND_APPLY);

#endif // GLOBAL_H
