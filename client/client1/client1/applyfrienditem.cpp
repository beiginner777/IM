#include "applyfrienditem.h"
#include "ui_applyfrienditem.h"
#include "usermanager.h"

ApplyFriendItem::ApplyFriendItem(QWidget *parent) :
    ListItemBase(parent),
    ui(new Ui::ApplyFriendItem)
{
    ui->setupUi(this);
    connect(ui->acceptButton,&QPushButton::clicked,[&](){  
           qDebug() << "click accept button.";
           emit signalAcceptFriendCommit(this);
    });
    connect(ui->refuseButton,&QPushButton::clicked,[&](){
        qDebug() << "Click refuse button.";
        emit signalRefuseApply(this);
    });
}

ApplyFriendItem::~ApplyFriendItem()
{
    delete ui;
}

void ApplyFriendItem::setIcon(QString icon)
{
    applyInfo_->icon_ = icon;
    ui->iconLabel->setPixmap(returnPixMapByUrl(icon));
}

void ApplyFriendItem::SetInfo(std::shared_ptr<ApplyInfo> apply_info)
{
    applyInfo_ = apply_info;
    ui->iconLabel->setScaledContents(true);
    ui->iconLabel->setPixmap(returnPixMapByUrl(apply_info->icon_));
    ui->nameLabel->setText(applyInfo_->name_);
    ui->remarkLabel->setText(applyInfo_->desc_);

    ShowAddBtn(apply_info->status_);
}

void ApplyFriendItem::ShowAddBtn(int bshow)
{
    // 未添加
    if(bshow == 0)
    {
        ui->acceptButton->show();
        ui->addedLabel->hide();
        ui->refuseButton->show();
        //qDebug() << "bshow == 0";
    }
    // 1 -- 已添加
    else if(bshow == 1)
    {
        ui->addedLabel->setText("已添加");
        ui->addedLabel->show();
        ui->acceptButton->hide();
        ui->refuseButton->hide();
        //qDebug() << "bshow == 1";
    }
    // 2 -- 已忽略
    else if(bshow == 2)
    {
        ui->addedLabel->setText("已忽略");
        ui->addedLabel->show();
        ui->acceptButton->hide();
        ui->refuseButton->hide();
        //qDebug() << "bshow == 2";
    }
}

int ApplyFriendItem::GetUid()
{
    return applyInfo_->uid_;
}
