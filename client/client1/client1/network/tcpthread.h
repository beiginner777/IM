#ifndef TCPTHREAD_H
#define TCPTHREAD_H

#include "core/global.h"

class TcpThread
{
public:
    TcpThread();
    ~TcpThread();
private:
    QThread* tcp_thread_;
    QThread* file_upload_thread_;
    QThread* data_ware_thread_;
};

#endif // TCPTHREAD_H
