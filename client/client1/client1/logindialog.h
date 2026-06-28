#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include "global.h"
#include "userdata.h"

namespace Ui {
class LoginDialog;
}

class LoginedUserList;

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    using handleFunction = std::function<void(QJsonObject)>;
    explicit LoginDialog(QWidget *parent = nullptr);
    ~LoginDialog();

private:
    ServerInfo si_;
    qint32 uid_;
    QString token_;
    Ui::LoginDialog *ui;

    void registerHanles();
    void login(QJsonObject obj);

private:
    std::unordered_map<int,handleFunction> handles_;
    bool is_show_;
    LoginedUserList* list_;
signals:
    void signalSwitckToRegister();
    void signalConnTcp(ServerInfo si);

public slots:
    void on_login_button_clicked();
    void handleSignals(QString res, ERRORCODE err, REQUEST_ID req_id);
    void slotTcpConnFinish(bool isSuccess); // tcp长连接成功
    void slotLoadHistortUserFinish(std::vector<std::shared_ptr<UserInfo>> historyUsers);
};

#endif // LOGINDIALOG_H
