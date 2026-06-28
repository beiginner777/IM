#include "tcpthread.h"

#include "tcpmsg.h"
#include "fileuploadmsg.h"
#include "loadlocaldata.h"

TcpThread::TcpThread()
{
    // 聊天服务器线程
    tcp_thread_ = new QThread();
    TcpMsg::GetInstance()->moveToThread(tcp_thread_);
    QObject::connect(tcp_thread_,&QThread::finished,&QThread::deleteLater);
    tcp_thread_->start();
    QObject::connect(tcp_thread_, &QThread::started,
              TcpMsg::GetInstance().get(), &TcpMsg::onThreadStarted);

    // 资源服务器线程
    file_upload_thread_ = new QThread();
    FileUploadMsg::GetInstance()->moveToThread(file_upload_thread_);
    QObject::connect(file_upload_thread_,&QThread::finished,&QThread::deleteLater);
    file_upload_thread_->start();
    QObject::connect(file_upload_thread_, &QThread::started,
              FileUploadMsg::GetInstance().get(), &FileUploadMsg::onThreadStarted);


    // 本地数据库线程
    data_ware_thread_ = new QThread();
    LoadLocalData::GetInstance()->moveToThread(data_ware_thread_);
    QObject::connect(data_ware_thread_,&QThread::finished,&QThread::deleteLater);
    data_ware_thread_->start();
    QObject::connect(data_ware_thread_, &QThread::started,
              LoadLocalData::GetInstance().get(), &LoadLocalData::onThreadStarted);

}

TcpThread::~TcpThread()
{
    tcp_thread_->quit();
}
