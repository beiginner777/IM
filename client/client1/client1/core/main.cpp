#include "mainwindow.h"
#include "global.h"
#include <QApplication>
#include <QIcon>
#include <QNetworkRequest>
#include <QSslConfiguration>
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
    QString GateServerScheme = settings.value("GateServer/Scheme", "http").toString();
    // 拼接网关服务器地址（支持 http/https 配置）
    Gate_Url_Prefix = GateServerScheme + "://" + GateServerHost + ":" + GateServerPort;
    qDebug() << "GateServerAdrdr = " << Gate_Url_Prefix;

    // dev 环境忽略自签证书错误（仅 HTTPS 时生效，生产环境必须移除）
    if (GateServerScheme == "https") {
        // 获取当前的全局默认 SSL 配置
        QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
        // 将对端证书验证模式设置为“不验证”
        sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
        // 将修改后的配置设回全局默认
        QSslConfiguration::setDefaultConfiguration(sslConfig);
    }

    // 注册元对象
    registerMetaType();
    // 启动TcpThread
    TcpThread tcp_thread;

    MainWindow w;
    qDebug() << "Main Thread: " << QThread::currentThread();

    return a.exec();
}
