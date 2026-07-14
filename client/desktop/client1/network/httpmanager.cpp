#include "httpmanager.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QDebug>

HttpManager::HttpManager()
{
    connect(this,&HttpManager::signal_receive_post_reply,this,&HttpManager::handleSignals);
}

//发送信号通知指定模块http响应结束
void HttpManager::handleSignals(QString res, ERRORCODE err,REQUEST_ID req_id, MODULES mod)
{
    if(mod == MODULES::REGISTERMOD){
        emit signal_send_to_register(res,err,req_id);
    }
    if(mod == MODULES::LOGINMOD){
        emit signal_send_to_login(res,err,req_id);
    }
}

HttpManager::~HttpManager()
{
    qDebug() << "HttpManager destructed.";
}

void HttpManager::sendPostRequest(QUrl url, QJsonObject json, REQUEST_ID req_id, MODULES mod)
{
    // 将json对象 转化为 二进制数据
    QByteArray data = QJsonDocument(json).toJson();
    // 通过url构造post请求
    QNetworkRequest request(url);
    // 设置请求头
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");
    request.setHeader(QNetworkRequest::ContentLengthHeader,QByteArray::number(data.length()));
    // 发送请求
    auto self = shared_from_this();
    QNetworkReply* reply = manager_.post(request,data);
    // 当网络操作完成（成功、超时、错误等）并且所有数据都可用时，finished() 信号会被发送。
    // 当接收到服务器的回复触发这个信号之后，再次发出一个信号，让槽函数接收（这个函数的作用是：对不同的消息ID，需要发送到不同的模块）
    QObject::connect(reply, &QNetworkReply::finished,[self,reply,req_id,mod](){
        // 说明出现错误了
        if(reply->error() != QNetworkReply::NoError)
        {
            emit self->signal_receive_post_reply("",ERRORCODE::ERROR_NET,req_id,mod);
            // 在控制台打印错误
            qDebug() << reply->errorString();
            return;
        }
        // 处理没有出现错误的逻辑
        // 读取服务器回复的数据
        QString res = reply->readAll();
        //qDebug() << "res = " << res ;
        // 触发信号
        emit self->signal_receive_post_reply(res,ERRORCODE::SUCCESS,req_id,mod);
    });
}




