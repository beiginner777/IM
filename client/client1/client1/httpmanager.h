#ifndef HTTPMANAGER_H
#define HTTPMANAGER_H

#include <QObject>
#include <QString>
#include <QUrl>
#include <QNetworkAccessManager>
#include "global.h"
#include <QJsonObject>
#include <QJsonDocument>

class HttpManager : public QObject, public SingleTon<HttpManager> ,public std::enable_shared_from_this<HttpManager>
{
    Q_OBJECT
public:
    ~HttpManager() override;

    // post                 目的地址      发送内容          post请求的ID     发送请求的模块
    void sendPostRequest(QUrl url, QJsonObject json, REQUEST_ID req_id, MODULES mod);
private:
    friend class SingleTon<HttpManager>;
    HttpManager();
    // QNetworkAccessManager Qt程序的"浏览器引擎",可以发起请求,接收服务器响应
    QNetworkAccessManager manager_;

public slots:
    void handleSignals(QString res,ERRORCODE err,REQUEST_ID req_id, MODULES mod);
signals:
    void signal_receive_post_reply(QString res,ERRORCODE err,REQUEST_ID req_id, MODULES mod);
    void signal_send_to_register(QString res,ERRORCODE err,REQUEST_ID req_id);
    void signal_send_to_login(QString res,ERRORCODE err,REQUEST_ID req_id);
};

#endif // HTTPMANAGER_H
