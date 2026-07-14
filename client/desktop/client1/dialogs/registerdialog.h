#ifndef REGISTERDIALOG_H
#define REGISTERDIALOG_H

#include <QDialog>
#include "core/global.h"
#include <map>
#include <functional>
#include <unordered_map>
#include "widgets/timerbtn.h"

namespace Ui {
class RegisterDialog;
}

class RegisterDialog : public QDialog
{
    Q_OBJECT

public:

    using handleFunction = std::function<void(QJsonObject)>;

    explicit RegisterDialog(QWidget *parent = nullptr);
    ~RegisterDialog();
    // 清空注册界面的信息
    void clear();

public slots:
    void handleSignals(QString res,ERRORCODE err,REQUEST_ID req_id);
    void on_confirm_button_clicked();
    void on_get_button_clicked();

signals:
    void signalSwitchToLogin();
    void signalIsHide();

private slots:
    void on_returnButton_clicked();

private:
    Ui::RegisterDialog *ui;

    // 注册相应的处理函数
    void registerHandles();
    std::map<int,handleFunction> handles_;

    // 获取验证码
    void getVerifyCode(QJsonObject jsonobj);
    // 注册
    void registerAccount(QJsonObject jsonobj);

    // 注册验证
    bool checkUserValid();
    bool checkPasswordValid();
    bool checkConfirmValid();
    bool checkEmailValid();

    // 注册成功时转回提示界面
    void switchToTip();

private:
    QTimer* timer_;
    int counter_;
};

#endif // REGISTERDIALOG_H
