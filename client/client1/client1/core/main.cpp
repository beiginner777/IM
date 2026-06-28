#include "mainwindow.h"
#include "global.h"
#include <QApplication>
#include <QIcon>
#include "network/tcpthread.h"
#include <thread>
#include <chrono>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    // 设置全局的QIcon: 这个icon应该放在 /build/bin 目录中
    a.setWindowIcon(QIcon("favicon.ico"));
    // 加载config文件
    // 当前项目的路径
    QString app_path = QCoreApplication::applicationDirPath();
    qDebug() << "app_path = " << app_path;
    // 配置文件的相对路径
    QString config_name = "config.ini";
    // 配置文件的绝对路径
    QString config_path = QDir::toNativeSeparators(app_path + QDir::separator() + config_name);
    // 将配置加载到环境当中
    QSettings settings(config_path,QSettings::IniFormat);
    GateServerHost = settings.value("GateServer/Host").toString();
    GateServerPort = settings.value("GateServer/Port").toString();
    // 拼接网关服务器地址
    Gate_Url_Prefix = "http://" + GateServerHost + ":" + GateServerPort;
    qDebug() << "GateServerAdrdr = " << Gate_Url_Prefix;

    // 注册元对象
    registerMetaType();
    // 启动TcpThread
    TcpThread tcp_thread;

    MainWindow w;
    qDebug() << "Main Thread: " << QThread::currentThread();

    return a.exec();
}
