#include "contactuserwidget.h"
#include "ui_contactuserwidget.h"

ContactUserWidget::ContactUserWidget(QWidget *parent) :
    ListItemBase(parent),
    ui(new Ui::ContactUserWidget)
{
    ui->setupUi(this);
    itemType_ = ListItemType::CONTACT_USER_ITEM;
}

ContactUserWidget::~ContactUserWidget()
{
    delete ui;
}
/*
void ContactUserWidget::SetInfo(std::shared_ptr<AuthInfo> authinfo_)
{
    info_ = std::make_shared<UserInfo>(authinfo_.get());
    // 加载图片
    QPixmap pixmap(info_->icon_);

    // 设置图片自动缩放
    ui->iconLabel->setPixmap(pixmap.scaled(ui->iconLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    ui->iconLabel->setScaledContents(true);

    ui->nameLabel->setText(info_->name_);
}*/

void ContactUserWidget::SetInfo(int uid, QString name, QString icon)
{
     info_ = std::make_shared<UserInfo>(uid,name, name, icon, 0);
     ui->iconLabel->setScaledContents(true);
     ui->iconLabel->setPixmap(returnPixMapByUrl(info_->icon_));
     ui->nameLabel->setText(info_->name_);
}

void ContactUserWidget::SetInfo(std::shared_ptr<FriendInfo> friendInfo)
{
    info_ = std::make_shared<UserInfo>(friendInfo);
    // 设置图片自动缩放
    ui->iconLabel->setScaledContents(true);
    // 加载图片
    QPixmap pixmap(returnPixMapByUrl(info_->icon_));
    ui->iconLabel->setPixmap(pixmap);
    ui->nameLabel->setText(info_->name_);
}

void ContactUserWidget::setIcon(QString icon)
{
    info_->icon_ = icon;
    ui->iconLabel->setPixmap(returnPixMapByUrl(info_->icon_));
}
/*
void ContactUserWidget::SetInfo(std::shared_ptr<AuthRsp> auth_rsp){
    info_ = std::make_shared<UserInfo>(auth_rsp);

    // 加载图片
    QPixmap pixmap(info_->icon_);

    // 设置图片自动缩放
    ui->iconLabel->setPixmap(pixmap.scaled(ui->iconLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    ui->iconLabel->setScaledContents(true);

    ui->nameLabel->setText(info_->name_);
}*/


void ContactUserWidget::showRedPoint(bool show)
{
    if(show){
        ui->redPointLabel->show();
    }else{
        ui->redPointLabel->hide();
    }
}
