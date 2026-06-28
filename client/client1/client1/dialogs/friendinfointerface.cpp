#include "friendinfointerface.h"
#include "ui_friendinfointerface.h"

friendInfoInterface::friendInfoInterface(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::friendInfoInterface)
{
    ui->setupUi(this);

    // 设置qss样式
    QFile styleFile(":/style/FriendInfoInterface.qss");
    if(!styleFile.open(QFile::ReadOnly)){
        qDebug("open chat.qss failed!");
    }
    QString style = QLatin1String(styleFile.readAll());
    this->setStyleSheet(style);

    /*
    // 加载图片
    QPixmap pixmap1(":/res/msg_chat_normal.png");
    // 设置图片自动缩放
    ui->chatLabel->setPixmap(pixmap1.scaled(ui->chatLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    ui->chatLabel->setScaledContents(true);

    // 加载图片
    QPixmap pixmap2(":/res/voice_chat_normal.png");
    // 设置图片自动缩放
    ui->audioLabel->setPixmap(pixmap2.scaled(ui->audioLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    ui->audioLabel->setScaledContents(true);

    // 加载图片
    QPixmap pixmap3(":/res/video_chat_normal.png");
    // 设置图片自动缩放
    ui->videoLabel->setPixmap(pixmap3.scaled(ui->videoLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    ui->videoLabel->setScaledContents(true);
*/
    // Click Button
    ui->chatLabel->setState("normal","hover","press");

    ui->audioLabel->setState("normal","hover","press");

    ui->videoLabel->setState("normal","hover","press");

}

friendInfoInterface::~friendInfoInterface()
{
    delete ui;
}

void friendInfoInterface::setUserInfo(std::shared_ptr<UserInfo> userInfo)
{
    userInfo_ = userInfo;
    selected_uid_ = userInfo_->uid_;
    // 设置图片自动缩放
    ui->headLabel->setScaledContents(true);
    ui->headLabel->setPixmap(returnPixMapByUrl(userInfo_->icon_));
}

void friendInfoInterface::on_chatLabel_clicked()
{
    emit signalSwitchFromCotactToChat(selected_uid_);
}
