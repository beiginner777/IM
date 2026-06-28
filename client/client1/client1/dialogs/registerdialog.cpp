#include "registerdialog.h"
#include "ui_registerdialog.h"

#include <QFile>
#include <QFile>
#include <QStyle>
#include <QRegExp>
#include <QRegExpValidator>
#include <QRegularExpression>
#include <QDebug>
#include <QTimer>
#include <thread>
#include <chrono>
#include "network/httpmanager.h"

RegisterDialog::RegisterDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RegisterDialog),
    timer_(new QTimer(this)),
    counter_(5)
{
    ui->setupUi(this);

    // 设置qss样式
    QFile styleFlie(":/style/register.qss");
    if(!styleFlie.open(QFile::ReadOnly)){
        qDebug("open register.qss failed!");
    }
    QString style = QLatin1String(styleFlie.readAll());
    this->setStyleSheet(style);

    ui->passVisible->setState("normal","hover","pressed","selected_normal","selected_hover","selected_pressed");
    ui->confirmVisible->setState("normal","hover","pressed","selected_normal","selected_hover","selected_pressed");


    // 注册回调函数
    registerHandles();

    // 在注册时，验证输入
    connect(ui->user_lineedit,&QLineEdit::editingFinished,this,&RegisterDialog::checkUserValid);
    connect(ui->password_lineedit,&QLineEdit::editingFinished,this,&RegisterDialog::checkPasswordValid);
    connect(ui->confirm_lineedit,&QLineEdit::editingFinished,this,&RegisterDialog::checkConfirmValid);
    connect(ui->email_lineedit,&QLineEdit::editingFinished,this,&RegisterDialog::checkEmailValid);

    // 收到HttpManager发回的对于请求验证码的回复
    connect(HttpManager::GetInstance().get(),&HttpManager::signal_send_to_register,this,&RegisterDialog::handleSignals);

    // 接收到来自定时器的注册成功信号
    connect(timer_,&QTimer::timeout,this,[&](){
        counter_--;
        ui->returnLabel->setText(QString("there are %1 seconds to return to LoginWindow.").arg(counter_));
        if(counter_ == 0)
        {
            emit signalSwitchToLogin();
        }
    });

    // 是否显示密码
    connect(ui->passVisible,&ClickLabel::signalIsHide,[this](){
        if(ui->passVisible->getCurState() == ClickLbState::normal)
        {
            ui->password_lineedit->setEchoMode(QLineEdit::Password);
        }else
        {
            ui->password_lineedit->setEchoMode(QLineEdit::Normal);
        }
    });
    connect(ui->confirmVisible,&ClickLabel::signalIsHide,[this](){
        if(ui->confirmVisible->getCurState() == ClickLbState::normal)
        {
            ui->confirm_lineedit->setEchoMode(QLineEdit::Password);
        }else
        {
            ui->confirm_lineedit->setEchoMode(QLineEdit::Normal);
        }
    });
}

// 转换到提示界面
void RegisterDialog::switchToTip()
{  
    ui->page1->hide();
    ui->page_2->show();
    qDebug() << "timer starts.";
    timer_->start(1000);
}

RegisterDialog::~RegisterDialog()
{
    delete ui;
}

void RegisterDialog::clear()
{
    ui->page_2->hide();
    ui->page1->show();

    ui->tip->setText("Tip");
    ui->tip->setProperty("state","normal");
    repolish(ui->tip);
    ui->user_lineedit->clear();
    ui->password_lineedit->clear();
    ui->confirm_lineedit->clear();
    ui->email_lineedit->clear();
    ui->code_lineedit->clear();

    timer_->stop();
}

// 注册按钮对应的槽函数
void RegisterDialog::on_confirm_button_clicked()
{
    if(ui->code_lineedit->text() == ""){
        showTip(ui->tip,"please input VerifyCode.",false);
        return;
    }
    QJsonObject jsonObj;
    jsonObj["code"] = ui->code_lineedit->text();
    jsonObj["email"] = ui->email_lineedit->text();
    jsonObj["password"] = ui->password_lineedit->text();
    jsonObj["name"] = ui->user_lineedit->text();
    HttpManager::GetInstance()->sendPostRequest(QUrl(Gate_Url_Prefix  + registerUserAddr),jsonObj,ID_REG_USER,MODULES::REGISTERMOD);
}

// 处理发送过来的信号 并且 执行逻辑
void RegisterDialog::handleSignals(QString res, ERRORCODE err, REQUEST_ID req_id)
{
    if(err != ERRORCODE::SUCCESS){
        showTip(ui->tip,tr("网络请求错误"),false);
        return;
    }

    // 解析 JSON 字符串,res需转化为QByteArray
    QJsonDocument jsonDoc = QJsonDocument::fromJson(res.toUtf8());

    //json解析错误
    if(jsonDoc.isNull()){
        showTip(ui->tip,tr("json解析错误"),false);
        return;
    }

    //json解析错误
    if(!jsonDoc.isObject()){
        showTip(ui->tip,tr("json解析错误"),false);
        return;
    }

    QJsonObject jsonObj = jsonDoc.object();

    // 调用相应的逻辑
    handles_[req_id](jsonObj);
}
// 注册回调函数
void RegisterDialog::registerHandles()
{
    handles_[ID_GET_VERIFY_CODE] = std::bind(&RegisterDialog::getVerifyCode,this,std::placeholders::_1);
    handles_[ID_REG_USER] = std::bind(&RegisterDialog::registerAccount,this,std::placeholders::_1);
}

// 发送验证码的回调函数
void RegisterDialog::getVerifyCode(QJsonObject jsonobj)
{
    if(jsonobj["code"] != ERRORCODE::SUCCESS)
    {
        showTip(ui->tip,"register failed !",false);
        qDebug() << "error code: " << jsonobj["code"].toInt();
        qDebug() << "errro message: " << jsonobj["message"].toString();
        return;
    }
    qDebug() << jsonobj["code"].toString();
    showTip(ui->tip,"验证码已发送至邮箱，请注意查收！",true);
}
// 注册用户的回调函数
void RegisterDialog::registerAccount(QJsonObject jsonobj)
{
    if(jsonobj["code"] != ERRORCODE::SUCCESS)
    {
        showTip(ui->tip,"register failed !",false);
        qDebug() << "error code: " << jsonobj["code"].toInt();
        qDebug() << "errro message: " << jsonobj["message"].toString();
        return;
    }
    qDebug() << jsonobj["code"].toString();
    showTip(ui->tip,"Register success!",true);

    switchToTip();
}

// 用户名验证：3-20位字母、数字或下划线
bool RegisterDialog::checkUserValid()
{
    QString user_text = ui->user_lineedit->text();
    QRegularExpression r1(R"([a-zA-Z0-9_]{3,20})");  // 修正：包含0-9
    bool userIsValid = r1.match(user_text).hasMatch();
    if(!userIsValid){
        showTip(ui->tip,"用户名格式错误！(3-20位字母、数字或下划线)", false);
        return false;  // 验证失败立即返回
    }
    return true;
}

// 密码验证：3-20位字母、数字或指定特殊字符
bool RegisterDialog::checkPasswordValid()
{
    QString password_text = ui->password_lineedit->text();
    QRegularExpression r2(R"([a-zA-Z0-9.,!?/\\]{3,20})");  // 修正：包含0-9
    bool passwordIsValid = r2.match(password_text).hasMatch();
    if(!passwordIsValid){
        showTip(ui->tip,"密码格式错误！(3-20位字母、数字或.,!?/\\)", false);
        return false;
    }
    return true;
}

// 确认密码验证
bool RegisterDialog::checkConfirmValid()
{
    QString password_text = ui->password_lineedit->text();
    QString confirm_text = ui->confirm_lineedit->text();
    if(password_text != confirm_text){  // 直接比较，不需要再验证格式
        showTip(ui->tip,"两次输入的密码不一致！", false);
        return false;
    }
    return true;
}

// 邮箱验证
bool RegisterDialog::checkEmailValid()
{
    QString email_text = ui->email_lineedit->text();
    QRegularExpression r4(R"(\b[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}\b)");
    bool emailIsValid = r4.match(email_text).hasMatch();
    if(!emailIsValid){
        showTip(ui->tip,"邮箱格式错误！", false);
        return false;
    }
    return true;
}

// 触发这个槽函数的前提是：需要在GET按钮的鼠标左键点击事件中，需要手动发射信号（因为之前的事件被覆盖了）
void RegisterDialog::on_get_button_clicked()
{
    qDebug() << "Button clicked";
    bool valid = checkUserValid();
    if(!valid){
       return;
    }
    valid = checkPasswordValid();
    if(!valid){
       return;
    }
    valid = checkConfirmValid();
    if(!valid){
       return;
    }
    valid = checkEmailValid();
    if(!valid){
       return;
    }
    // 所有验证通过
    showTip(ui->tip,"验证通过,等待发送验证码!", true);

    // 发送验证码
    QJsonObject jsonObj;
    jsonObj["email"] = ui->email_lineedit->text();
    HttpManager::GetInstance()->sendPostRequest(QUrl(Gate_Url_Prefix + getVerifyCodeAddr),jsonObj,REQUEST_ID::ID_GET_VERIFY_CODE,MODULES::REGISTERMOD);
    qDebug() << "sendPostRequest at " << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << endl;
}

void RegisterDialog::on_returnButton_clicked()
{
    emit signalSwitchToLogin();
}
