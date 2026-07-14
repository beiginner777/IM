#include "logineduserwidget.h"
#include "ui_logineduserwidget.h"

LoginedUserWidget::LoginedUserWidget(QWidget *parent) :
    ListItemBase(parent),
    ui(new Ui::LoginedUserWidget)
{
    ui->setupUi(this);

    itemType_ = ListItemType::LOGINED_USER_ITEM;
}

LoginedUserWidget::~LoginedUserWidget()
{
    delete ui;
}

void LoginedUserWidget::SetUserInfo(std::shared_ptr<UserInfo> userInfo)
{
    userInfo_ = userInfo;

    ui->headLabel->setScaledContents(true);

    ui->headLabel->setScaledContents(true);
    ui->headLabel->setPixmap(returnPixMapByUrl(userInfo->icon_));

    ui->nameLabel->setText(userInfo->name_);
}
