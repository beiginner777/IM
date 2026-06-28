#include "findsuccessdialog.h"
#include "ui_findsuccessdialog.h"
#include "applyfriend.h"
#include "usermanager.h"
#include "fileuploadmsg.h"

FindSuccessDialog::FindSuccessDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FindSuccessDialog)
{
    ui->setupUi(this);

    // 设置对话框的title
    setWindowTitle("添加");
    // 隐藏右上角样式
    connect(FileUploadMsg::GetInstance().get(),&FileUploadMsg::signalOtherIconDownloadFinished,this,[this](QString icon){
        if(this->isVisible()){
            ui->iconLabel->setPixmap(returnPixMapByUrl(icon));
        }
    });
    ui->friendApplyLabel->setState("normal","hover","press");
}

FindSuccessDialog::~FindSuccessDialog()
{
    qDebug() << "FindSuccessDialog destrute.";
    delete ui;
}

void FindSuccessDialog::setSearchInfo(std::shared_ptr<SearchInfo> si)
{
    si_ = si;
    ui->iconLabel->setScaledContents(true);
    ui->iconLabel->setPixmap(returnPixMapByUrl(si->icon_));
    ui->nameLabel->setText(si->name_);
    ui->descLabel->setText(si->desc_);

    // 设置为模态对话框
    this->setModal(true);
}

void FindSuccessDialog::on_friendApplyLabel_clicked()
{
    qDebug() << "click addFriendButton.";
    if(UserManager::GetInstance()->isFriendByUid(si_->uid_)){
        qDebug() << "you are friends already.";
        return;
    }
    this->hide();
    ApplyFriend* addFriendDialog = new ApplyFriend();
    addFriendDialog->show();
    addFriendDialog->setApplyInfo(si_);
}

