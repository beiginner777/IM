/******************************************************************************
 *
 * @file       mainwindow.h
 * @brief      XXXX Function
 *
 * @author     Jerry
 * @date       2025/07/26
 * @history
 *****************************************************************************/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "dialogs/logindialog.h"
#include "dialogs/registerdialog.h"
#include "dialogs/chatdialog.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void slotSwitchToRegister();
    void slotSwitchToLogin();
    void slotSwitchToChat();
    void slotLoadLocalInfoFinish();

private:

    LoginDialog* loginDialog_;
    RegisterDialog* registerDialog_;
    ChatDialog* chatDialog_;

    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
