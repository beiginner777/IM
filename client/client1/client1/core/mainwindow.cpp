#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "network/tcpmsg.h"
#include "dialogs/offlinedialog.h"
#include "data/loadlocaldata.h"
#include "data/usermanager.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    loginDialog_ = new LoginDialog();
    registerDialog_ = new RegisterDialog();

    loginDialog_->show();

    // 登录 和 注册 界面的跳转
    connect(loginDialog_,&LoginDialog::signalSwitckToRegister,this,&MainWindow::slotSwitchToRegister);
    // 注册成功时，从注册界面跳转回登录界面
    connect(registerDialog_,&RegisterDialog::signalSwitchToLogin,this,&MainWindow::slotSwitchToLogin);
    // 登录校验成功之后，转换到聊天界面
    connect(TcpMsg::GetInstance().get(),&TcpMsg::signalSwitchToChat,this,&MainWindow::slotSwitchToChat);
    // 通知本地的信息加载完成
    connect(LoadLocalData::GetInstance().get(),&LoadLocalData::signalLoadLocalInfoFinish,this,&MainWindow::slotLoadLocalInfoFinish);
}

MainWindow::~MainWindow()
{
    if(loginDialog_ != nullptr){
        delete loginDialog_;
        loginDialog_ = nullptr;
    }
    if(registerDialog_ != nullptr){
        delete registerDialog_;
        registerDialog_ = nullptr;
    }
    if(chatDialog_ != nullptr){
        delete chatDialog_;
        chatDialog_ = nullptr;
    }

    delete ui;
}

void MainWindow::slotSwitchToRegister()
{
    // 将注册界面的消息给清空
    registerDialog_->clear();
    registerDialog_->show();
}

// 转回登录界面
void MainWindow::slotSwitchToLogin()
{
    registerDialog_->hide();
    loginDialog_->show();
}

void MainWindow::slotSwitchToChat()
{
    chatDialog_ = new ChatDialog();
    loginDialog_->hide();
    registerDialog_->hide();
    chatDialog_->show();
    // 先加载本地的全部好友信息（防止在渲染数据到ChatUserList的时候，出现需要再次去服务器加载的情况）
    emit LoadLocalData::GetInstance()->signalloadFriendList();
    // 应该先确保load_info的消息被正确加载之后，再开始向服务器发送请求
    emit LoadLocalData::GetInstance()->signalLoadLocalInfo();
}

void MainWindow::slotLoadLocalInfoFinish()
{
    chatDialog_->initConnUserList();
    chatDialog_->initChatUserList();
    chatDialog_->initFriendApply();
}
