#include "chatuserwidget.h"
#include "ui_chatuserwidget.h"

ChatUserWidget::ChatUserWidget(QWidget *parent) :
    ListItemBase(parent),
    ui(new Ui::ChatUserWidget),
    userInfo_(nullptr)
{
    ui->setupUi(this);

    // 默认是聊天界面
    setItemType(ListItemType::CHAT_USER_ITEM);

    // qss样式
    this->setStyleSheet("QTextEdit{background:#e1e1e1;border:none}");
}

ChatUserWidget::~ChatUserWidget()
{
    delete ui;
}

void ChatUserWidget::setInfo(QString head, QString name, QString msg)
{
    head_ = head;
    name_ = name;
    msg_ = msg;

    // 加载图片
    QPixmap pixmap(head_);
    // 将图片根据QLabel的大小，填充到QLabel当中
    ui->iconLabel->setPixmap(pixmap.scaled(ui->iconLabel->size(),Qt::KeepAspectRatio,Qt::SmoothTransformation));
    // 设置头像自动缩放
    ui->iconLabel->setScaledContents(true);

    ui->nameLabel->setText(name_);
    ui->msglabel->setText(msg_);
}

void ChatUserWidget::setUserInfo(std::shared_ptr<UserInfo> user_info)
{
    userInfo_ = user_info;
    // 设置图片自动缩放
    ui->iconLabel->setScaledContents(true);
    // 加载图片
    QPixmap pixmap(returnPixMapByUrl(userInfo_->icon_));
    ui->iconLabel->setPixmap(pixmap);
    ui->nameLabel->setText(userInfo_->name_);
    ui->msglabel->setText(userInfo_->lastMsg_);
}

void ChatUserWidget::setUserInfo(std::shared_ptr<FriendInfo> friend_info)
{
    userInfo_ = std::make_shared<UserInfo>(friend_info);
    // 设置图片自动缩放
    ui->iconLabel->setScaledContents(true);
    // 加载图片
    QPixmap pixmap(returnPixMapByUrl(userInfo_->icon_));
    ui->iconLabel->setPixmap(pixmap);
    ui->nameLabel->setText(userInfo_->name_);
    ui->msglabel->setText(userInfo_->lastMsg_);
}

void ChatUserWidget::setIcon(QString icon)
{
    userInfo_->icon_ = icon;
    QPixmap pixmap(returnPixMapByUrl(userInfo_->icon_));
    ui->iconLabel->setPixmap(pixmap);
}

void ChatUserWidget::updateLastMsg(QString lastMsg,QString chat_time)
{
    if(chat_time_ == "" || chat_time > chat_time_){
        msg_ = lastMsg;
        chat_time_ = chat_time;
        ui->msglabel->setText(msg_);
        qDebug() << " ============== " << convertDateTimeString(chat_time) << " ================= ";
        ui->timeLabel->setText(convertDateTimeString(chat_time));
    }
}
