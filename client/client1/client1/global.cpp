#include "global.h"
#include <QStyle>
#include <QDebug>
#include <QLabel>
#include <QString>
#include <QLineEdit>
#include "userdata.h"
#include "fileuploadmsg.h"
#include "usermanager.h"
#include "loadlocaldata.h"

// 网管服务器的地址
QString Gate_Url_Prefix = "127.0.0.1";
QString GateServerHost;
QString GateServerPort;
// 获取验证码的地址
QString getVerifyCodeAddr = "/getVerifyCode";
// 注册用户的地址
QString registerUserAddr = "/registerUserAddr";
// 登录的地址
QString loginAddr = "/loginAddr";

// 刷新 QWidget w 的style(),强制刷新qss样式
std::function<void(QWidget*)> repolish = [](QWidget* w){
    w->style()->unpolish(w);
    w->style()->polish(w);
};

void showTip(QLabel* label,QString str,bool isTrue)
{
    if(isTrue){
        label->setProperty("state","correct");
    }else{
        label->setProperty("state","error");
    }
    label->setText(str);
    repolish(label);
}

QString convertDateTimeString(const QString &dateTimeStr)
{
    // 解析原始字符串
    QDateTime dateTime = QDateTime::fromString(dateTimeStr, "yyyy-MM-dd HH:mm:ss");
    if (!dateTime.isValid()) {
        //qWarning() << "Invalid datetime string:" << dateTimeStr;
        return dateTimeStr;
    }
    // 格式化为小时:分钟
    return dateTime.toString("HH:mm");
}

QString generateUniqueIconName(){
    QString uuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    return uuid + ".png";
}

void registerMetaType()
{
    qRegisterMetaType<ServerInfo>("ServerInfo");
    qRegisterMetaType<REQUEST_ID>("REQUEST_ID");
    qRegisterMetaType<MODULES>("MODULES");
    qRegisterMetaType<SearchInfo>("SearchInfo");
    qRegisterMetaType<std::shared_ptr<SearchInfo>>("std::shared_ptr<SearchInfo>");
    qRegisterMetaType<AddFriendApply>("AddFriendApply");
    qRegisterMetaType<std::shared_ptr<AddFriendApply>>("std::shared_ptr<AddFriendApply>");
    qRegisterMetaType<ApplyInfo>("ApplyInfo");
    qRegisterMetaType<std::shared_ptr<ApplyInfo>>("std::shared_ptr<ApplyInfo>");
    qRegisterMetaType<std::vector<std::shared_ptr<ApplyInfo>>>("std::vector<std::shared_ptr<ApplyInfo>>");
    qRegisterMetaType<AuthInfo>("AuthInfo");
    qRegisterMetaType<std::shared_ptr<AuthInfo>>("std::shared_ptr<AuthInfo>");
    qRegisterMetaType<AuthRsp>("AuthRsp");
    qRegisterMetaType<std::shared_ptr<AuthRsp>>("std::shared_ptr<AuthRsp>");
    qRegisterMetaType<FriendInfo>("FriendInfo");
    qRegisterMetaType<std::shared_ptr<FriendInfo>>("std::shared_ptr<FriendInfo>");
    qRegisterMetaType<std::vector<std::shared_ptr<FriendInfo> >>("std::vector<std::shared_ptr<FriendInfo>>");
    qRegisterMetaType<ChatDataBase>("ChatDataBase");
    qRegisterMetaType<std::shared_ptr<ChatDataBase>>("std::shared_ptr<ChatDataBase>");
    qRegisterMetaType<TextChatData>("TextChatData");
    qRegisterMetaType<std::shared_ptr<TextChatData>>("std::shared_ptr<TextChatData>");
    qRegisterMetaType<std::vector<std::shared_ptr<TextChatData> >>("std::vector<std::shared_ptr<TextChatData> >");
    qRegisterMetaType<UserInfo>("UserInfo");
    qRegisterMetaType<std::shared_ptr<UserInfo>>("std::shared_ptr<UserInfo>");
    qRegisterMetaType<std::vector<std::shared_ptr<UserInfo>>>("std::vector<std::shared_ptr<UserInfo>>");
    qRegisterMetaType<ChatThreadInfo>("ChatThreadInfo");
    qRegisterMetaType<std::vector<ChatThreadInfo>>("std::vector<ChatThreadInfo>");
    qRegisterMetaType<std::shared_ptr<ChatThreadInfo>>("std::shared_ptr<ChatThreadInfo>");
    qRegisterMetaType<ChatThreadInfo>("ChatThreadInfo");
    qRegisterMetaType<std::shared_ptr<ImageDataBase>>("std::shared_ptr<ImageDataBase>");
    qRegisterMetaType<std::vector<std::shared_ptr<ImageDataBase>>>("std::vector<std::shared_ptr<ImageDataBase>>");
    qRegisterMetaType<DownloadFileInfo>("DownloadFileInfo");
    qRegisterMetaType<std::shared_ptr<DownloadFileInfo>>("std::shared_ptr<DownloadFileInfo>");
    qRegisterMetaType<TRANSFER_TYPE>("TRANSFER_TYPE");
    qRegisterMetaType<TRANSFER_STATE>("TRANSFER_STATE");
    qRegisterMetaType<MsgInfo>("MsgInfo");
    qRegisterMetaType<std::shared_ptr<MsgInfo>>("std::shared_ptr<MsgInfo>");
    qRegisterMetaType<MsgInfo>("Message_Type");
    qRegisterMetaType<MsgStatus>("MsgStatus");
    qRegisterMetaType<ChatMessage>("ChatMessage");
    qRegisterMetaType<std::shared_ptr<ChatMessage>>("std::shared_ptr<ChatMessage>");
    qRegisterMetaType<std::vector<std::shared_ptr<ChatMessage>>>("std::vector<std::shared_ptr<ChatMessage>>");
    qRegisterMetaType<ChatDataBase>("ChatDataBase");
    qRegisterMetaType<std::shared_ptr<ChatDataBase>>("std::shared_ptr<ChatDataBase>");
    qRegisterMetaType<std::vector<std::shared_ptr<ChatDataBase> >>("std::vector<std::shared_ptr<ChatDataBase> >");
    qRegisterMetaType<FRIEND_APPLY>("FRIEND_APPLY");
}

QPixmap returnPixMapByUrl(QString url)
{
    // 检查是否是自己的头像
    QRegularExpression regex("^:/res/head(\\d+)\\.png$");
    QRegularExpressionMatch match = regex.match(url);
    if(match.hasMatch()){
        //是默认头像
        return QPixmap(url);
    }else{
        // 用户自己上传的头像
        QString storgeDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir dir(storgeDir + "/avatars");
        // 检查头像所在的目录是否存在
        if(dir.exists()){
            QString path = dir.filePath(url);
            QPixmap pic(path);
            // 检查头像这个存在
            if(!pic.isNull()){
                // 存在，那么就返回
                return pic;
            }else{
                // 不存在
                //qDebug() << "加载 path = " << path << " 的头像失败,正在向资源服务器请求下载文件: "<< url;
                // 判断是否与资源服务器建立连接了
                if(!FileUploadMsg::GetInstance()->is_connected()){
                    qDebug() << "还未与资源服务器建立连接,无法请求下载";
                    return QPixmap(":/res/default.jpeg");
                }
                // 并且需要判断这个头像是否已经在下载了
                std::shared_ptr<DownloadFileInfo> is_download = UserManager::GetInstance()->get_download_file(url);
                if(is_download != nullptr){
                    // 正在下载,那么就直接返回
                    //qDebug() << "url = " << url << " 的文件正在下载.";
                    return QPixmap(":/res/default.jpeg");
                }
                // 不在下载,那么就请求下载
                std::shared_ptr<DownloadFileInfo> file_info = std::make_shared<DownloadFileInfo>();
                file_info->download_file_ = url;
                file_info->seq_ = 1;
                file_info->client_save_path_ = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/avatars/" + url;
                file_info->state_ = TRANSFER_STATE::Downloading;
                if(url == UserManager::GetInstance()->getIcon()){
                    file_info->type_ = Download_File_Type::SELF_HEAD_ICON;
                }else{
                    file_info->type_ = Download_File_Type::FRIEND_HEAD_ICON;
                }
                //qDebug() << "向UserManager添加下载文件的信息: " << url;
                UserManager::GetInstance()->add_download_file(url,file_info);
                // 那么就去向资源服务器请求文件下载
                QString local_path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/avatars/" + url;
                QJsonObject obj;
                obj["download_file"] = url;
                obj["seq"] = 1;
                obj["last_seq"] = 0;
                obj["trans_size"] = 0;
                obj["total_size"] = 0;
                obj["uid"] = UserManager::GetInstance()->getUid();
                obj["token"] = UserManager::GetInstance()->GetToken();
                obj["client_save_path"] = local_path;
                obj["download_file_type"] = file_info->type_;
                QJsonDocument doc(obj);
                QByteArray data = doc.toJson(QJsonDocument::Compact);
                emit FileUploadMsg::GetInstance()->signalSendData(ID_DOWN_LOAD_FILE_REQ,data);
                // 将这个文件的下载信息添加到数据库
                emit LoadLocalData::GetInstance()->signalAddTransferToDb(url, 0, 0, local_path,TRANSFER_STATE::Downloading
                                                                         ,TRANSFER_TYPE::Download,"");

                // 先返回默认头像，等头像加载完成之后，再去更新
                return QPixmap(":/res/default.jpeg");
            }
        }else{
            qWarning() << "头像存储目录不存在.";
            return QPixmap(":/res/default.jpeg");
        }
    }
}

QString generateUniqueFileName(const QString& originalName)
{
     QString uuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
     QFileInfo fileInfo(originalName);
     QString extension = fileInfo.suffix();
     return uuid + (extension.isEmpty() ? "" : "." + extension);
}

QString calculateFileHash(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
        return QString();

    QCryptographicHash hash(QCryptographicHash::Md5);

    // 分块计算哈希，避免大文件占用过多内存
    const qint64 chunkSize = 1024 * 1024; // 1MB
    while (!file.atEnd())
    {
        hash.addData(file.read(chunkSize));
    }
    file.close();

    return hash.result().toHex();
}

QString formatTime(const QDateTime &time)
{
   QDateTime now = QDateTime::currentDateTime();

   if (time.date() == now.date()) {
       // 今天：显示时间
       return time.toString("HH:mm");
   } else if (time.daysTo(now) == 1) {
       // 昨天
       return "昨天";
   } else if (time.daysTo(now) < 7) {
       // 一周内：显示星期
       QString weekDay;
       switch (time.date().dayOfWeek()) {
       case 1: weekDay = "周一"; break;
       case 2: weekDay = "周二"; break;
       case 3: weekDay = "周三"; break;
       case 4: weekDay = "周四"; break;
       case 5: weekDay = "周五"; break;
       case 6: weekDay = "周六"; break;
       case 7: weekDay = "周日"; break;
       }
       return weekDay;
   } else {
       // 更早：显示月日
       return time.toString("M/d");
   }
}

QDateTime returnQDateTimeByQString(QString mysqlStr)
{
   return QDateTime::fromString(mysqlStr, "yyyy-MM-dd HH:mm:ss");
}
