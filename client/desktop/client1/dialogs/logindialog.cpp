#include "logindialog.h"
#include "ui_logindialog.h"
#include "network/httpmanager.h"
#include "network/tcpmsg.h"
#include "data/fileuploadmsg.h"
#include <QFile>
#include <QDebug>
#include <QLineEdit>
#include "data/loadlocaldata.h"
#include "logineduserlist.h"
#include "logineduserwidget.h"

LoginDialog::LoginDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LoginDialog),
    is_show_(false)
{
    ui->setupUi(this);

    // 设置qss样式
    QFile styleFile(":/style/login.qss");
    if(!styleFile.open(QFile::ReadOnly)){
        qDebug("open login.qss failed!");
    }
    QString style = QLatin1String(styleFile.readAll());
    this->setStyleSheet(style);

    ui->headLabel->setScaledContents(true);
    ui->headLabel->setPixmap(QPixmap(":/res/default.jpeg"));

    ui->user_lineedit->setPlaceholderText("请输入用户名");
    ui->password_lineedit->setPlaceholderText("请输入密码");

    // 注册回调函数
    registerHanles();

    // 处理跳转到登录界面
    connect(ui->register_button,&QPushButton::clicked,this,&LoginDialog::signalSwitckToRegister);

    // 处理登录的逻辑
    connect(HttpManager::GetInstance().get(),&HttpManager::signal_send_to_login,this,&LoginDialog::handleSignals);

    // 发起一个长连接到服务器
    connect(this,&LoginDialog::signalConnTcp,TcpMsg::GetInstance().get(),&TcpMsg::slotTcpConnect);

    // 由管理者告知登录界面，长连接chatServer成功
    connect(TcpMsg::GetInstance().get(),&TcpMsg::signalConnectSuccess,this,&LoginDialog::slotTcpConnFinish); 

    connect(LoadLocalData::GetInstance().get(),&LoadLocalData::signalloadHistortUserFinish,this,&LoginDialog::slotLoadHistortUserFinish);

    // 在本地数据库加载历史用户
    emit LoadLocalData::GetInstance()->signalGetLoginedUser();
}

LoginDialog::~LoginDialog()
{
    delete ui;
}

void LoginDialog::slotLoadHistortUserFinish(std::vector<std::shared_ptr<UserInfo>> historyUsers)
{
    // 历史用户列表
    list_ = new LoginedUserList(ui->widget_3);
    list_->move(30,45);
    list_->hide();
    if(historyUsers.size() == 0){
        list_->resize(0,0);
    }
    else if(historyUsers.size() == 1){
        list_->resize(380,35);
    }
    if(historyUsers.size() == 2){
        list_->resize(380,70);
    }else{
        list_->resize(380,87);
    }

//    std::vector<std::shared_ptr<UserInfo>> infos;
//    infos.push_back(std::make_shared<UserInfo>(1,"jerry","ovoovo9966@qq.com","123","jerry","jerry.jpg",1,"Hello,World!"));
//    infos.push_back(std::make_shared<UserInfo>(2,"tom","ovoovo9966@qq.com","123","jerry","jerry.jpg",1,"Hello,World!"));
//    infos.push_back(std::make_shared<UserInfo>(3,"Sara","ovoovo9966@qq.com","123","jerry","jerry.jpg",1,"Hello,World!"));
//    infos.push_back(std::make_shared<UserInfo>(4,"John","ovoovo9966@qq.com","123","jerry","jerry.jpg",1,"Hello,World!"));
//    infos.push_back(std::make_shared<UserInfo>(5,"Lisa","ovoovo9966@qq.com","123","jerry","jerry.jpg",1,"Hello,World!"));

    for(std::shared_ptr<UserInfo> info : historyUsers){
        LoginedUserWidget* widget = new LoginedUserWidget();
        widget->SetUserInfo(info);
        QListWidgetItem* item = new QListWidgetItem();
        item->setSizeHint(widget->sizeHint());
        list_->addItem(item);
        list_->setItemWidget(item,widget);
    }

    // 邪修
    {// =======================================
        // 透明图标的动作
        QAction* tmp1 = new QAction(ui->user_lineedit);
        tmp1->setIcon(QIcon(":/res/transparent.png"));
        // 将图标动作设置到最末端
        ui->user_lineedit->addAction(tmp1,QLineEdit::LeadingPosition);

        QAction* tmp2 = new QAction(ui->user_lineedit);
        tmp2->setIcon(QIcon(":/res/transparent.png"));
        // 将图标动作设置到最末端
        ui->user_lineedit->addAction(tmp2,QLineEdit::LeadingPosition);

        QAction* tmp3 = new QAction(ui->password_lineedit);
        tmp3->setIcon(QIcon(":/res/transparent.png"));
        // 将图标动作设置到最末端
        ui->password_lineedit->addAction(tmp3,QLineEdit::LeadingPosition);}

    // =======================================

    // 透明图标的动作
    QAction* listLoginedUserAction = new QAction(ui->user_lineedit);
    listLoginedUserAction->setIcon(QIcon(":/res/down.png"));
    // 将图标动作设置到最末端
    ui->user_lineedit->addAction(listLoginedUserAction,QLineEdit::TrailingPosition);
    connect(listLoginedUserAction,&QAction::triggered,[listLoginedUserAction,this](){
        if(is_show_){
            is_show_ = false;
            listLoginedUserAction->setIcon(QIcon(":/res/down.png"));
            list_->hide();
        }else{
            is_show_ = true;
            listLoginedUserAction->setIcon(QIcon(":/res/up.png"));
            list_->show();
        }
    });

    // =======================================

    // 透明图标的动作
    QAction* clearAction = new QAction(ui->user_lineedit);
    clearAction->setIcon(QIcon(":/res/transparent.png"));
    // 将图标动作设置到最末端
    ui->user_lineedit->addAction(clearAction,QLineEdit::TrailingPosition);
    // 当文本框的内容变换时，触发槽函数，来更改显示的图标（将透明图标 更换为 实际的cancel图标）
    connect(ui->user_lineedit,&QLineEdit::textChanged,[clearAction](const QString& text){
        if(!text.isEmpty()){
            // 文本框有内容时
            clearAction->setIcon(QIcon(":/res/close.png"));
        }else{
            // 文本框没有内容
            clearAction->setIcon(QIcon(":/res/transparent.png"));
        }
    });
    // 当clearAction触发trigge信号时，就清除文本框的text
    connect(clearAction,&QAction::triggered,[clearAction,this](){
        ui->user_lineedit->clear();
        clearAction->setIcon(QIcon(":/res/transparent.png"));
    });

    // =======================================

    // 透明图标的动作
    QAction* clearAction1 = new QAction(ui->password_lineedit);
    clearAction1->setIcon(QIcon(":/res/transparent.png"));
    // 将图标动作设置到最末端
    ui->password_lineedit->addAction(clearAction1,QLineEdit::TrailingPosition);
    // 当文本框的内容变换时，触发槽函数，来更改显示的图标（将透明图标 更换为 实际的cancel图标）
    connect(ui->password_lineedit,&QLineEdit::textChanged,[clearAction1](const QString& text){
        if(!text.isEmpty()){
            // 文本框有内容时
            clearAction1->setIcon(QIcon(":/res/close.png"));
        }else{
            // 文本框没有内容
            clearAction1->setIcon(QIcon(":/res/transparent.png"));
        }
    });
    // 当clearAction触发trigge信号时，就清除文本框的text
    connect(clearAction1,&QAction::triggered,[clearAction1,this](){
        ui->password_lineedit->clear();
        clearAction1->setIcon(QIcon(":/res/transparent.png"));
    });

    // =======================================

    // 选择登录的用户
    connect(list_,&LoginedUserList::signalFullLineEdit,this,[this,listLoginedUserAction](std::shared_ptr<UserInfo> userinfo){
        // 图片自适应QLabel的大小
        ui->headLabel->setScaledContents(true);
        // 将图片设置到QLabel
        ui->headLabel->setPixmap(returnPixMapByUrl(userinfo->icon_));

        // 填写账号和密码
        ui->user_lineedit->setText(userinfo->name_);
        ui->password_lineedit->setText(userinfo->pwd_);

        listLoginedUserAction->trigger();
    });

}

void LoginDialog::registerHanles()
{
    handles_[ID_LOGIN] = std::bind(&LoginDialog::login,this,std::placeholders::_1);
}

void LoginDialog::login(QJsonObject obj)
{
   if(obj["code"] != ERRORCODE::SUCCESS)
   {
       qDebug() << "login failed";
       qDebug() << "error code: " << obj["code"];
       return;
   }

   ServerInfo si;
   si.uid = obj["uid"].toInt();
   si.host = obj["host"].toString();
   si.port = obj["port"].toString();
   si.token = obj["token"].toString();
   si.res_host_ = obj["res_host"].toString();
   si.res_port_ = obj["res_port"].toString();

   uid_ = si.uid;
   token_ = si.token;
   si_ = si;

   emit signalConnTcp(si);
}

// 登录对应的槽函数
void LoginDialog::on_login_button_clicked()
{
    ServerInfo si;
    si.uid = 1;
    si.host = "127.0.0.1";
    si.port = "8090";
    si.token = "dev_token";  // 绕过 GateServer，直接给一个开发用 token
    si.res_host_ = "0.0.0.0";
    si.res_port_ = "9090";
    uid_ = si.uid;
    token_ = si.token;
    si_ = si;
    emit signalConnTcp(si);
    /*QJsonObject obj;
    obj["username"] = ui->user_lineedit->text();
    obj["password"] = ui->password_lineedit->text();
    HttpManager::GetInstance()->sendPostRequest(QUrl(Gate_Url_Prefix + loginAddr),obj,ID_LOGIN,LOGINMOD);*/
}

void LoginDialog::handleSignals(QString res, ERRORCODE err, REQUEST_ID req_id)
{
    // 解析 JSON 字符串,res需转化为QByteArray
    QJsonDocument jsonDoc = QJsonDocument::fromJson(res.toUtf8());

    //json解析错误
    if(jsonDoc.isNull()){
        qDebug("data is null");
        return;
    }

    //json解析错误
    if(!jsonDoc.isObject()){
        qDebug("json解析错误");
        return;
    }

    QJsonObject jsonObj = jsonDoc.object();

    // 调用对应的回调函数
    if(handles_.count(req_id)){
        handles_[req_id](jsonObj);
    }else{
        qDebug() << "can't find handleFunction for reqId = " << req_id;
        return;
    }
}

void LoginDialog::slotTcpConnFinish(bool isSuccess)
{
    if(isSuccess)
    {
        qDebug() << "connect to chatServer success,logining . . . ";
        QJsonObject obj;

        obj["uid"] = uid_;
        obj["token"] = token_;

        QJsonDocument doc(obj);
        QByteArray data = doc.toJson(QJsonDocument::Compact);

        // 向chatServer发送消息
        emit TcpMsg::GetInstance()->signalSendData(REQUEST_ID::ID_CHAT_LOGIN,data);

        // 同时向ResourceServer发送建立socket请求
        si_.res_host_ = "127.0.0.1";
        si_.res_port_ = "9090";
        emit FileUploadMsg::GetInstance()->signalConnToResServer(si_);
    }
    else
    {
       qDebug() << "error network!";
    }
}
