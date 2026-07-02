#include "settingpage.h"
#include "ui_settingpage.h"
#include "data/usermanager.h"
#include "data/fileuploadmsg.h"

SettingPage::SettingPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SettingPage)
{
    ui->setupUi(this);

    auto info = UserManager::GetInstance()->getUserInfo();

    ui->headLabel->setScaledContents(true);
    ui->headLabel->setPixmap(returnPixMapByUrl(info->icon_));
    ui->nameLineEdit->setPlaceholderText(info->name_);
    ui->nickLineEdit->setPlaceholderText(info->nick_);
    ui->descLineEdit->setPlaceholderText(info->desc_);

    QObject::connect(FileUploadMsg::GetInstance().get(),&FileUploadMsg::signalUploadHeadIconIsSuccess,this,&SettingPage::slotUploadHeadIconIsSuccess);
}

SettingPage::~SettingPage()
{
    delete ui;
}

void SettingPage::on_chooseButton_clicked()
{
    // 选择图片
    file_path_ = QFileDialog::getOpenFileName(
        this,tr("选择图片"),QString(),tr("图片文件(*.jpg *.png *.jpeg *.webp)")
    );
    if(file_path_.isEmpty()){
        return;
    }
    // 将选中的文件加载成图片
    QPixmap img;
    if(!img.load(file_path_)){
        QMessageBox::critical(
          this,tr("错误"),tr("图片加载失败"),QMessageBox::Ok
        );
        return;
    }
    // 修改图片
    ui->headLabel->setScaledContents(true);
    ui->headLabel->setPixmap(img);
}

void SettingPage::slotUploadHeadIconIsSuccess(bool flag)
{
    if(flag){
        qDebug() << "头像上传成功.";
        // 修改UserManager
        auto self_info = UserManager::GetInstance()->getUserInfo();
        self_info->icon_ = file_;
        UserManager::GetInstance()->setUserInfo(self_info);
        // 通知ChatDialog去修改主页的信息
        emit updateHomeIcon(file_);
    }else{
        qDebug() << "头像上传失败.";
    }
}

void SettingPage::on_uploadButton_clicked()
{
    // 保存头像的目录（C:/Users/guoru/AppData/Roaming/test）
    QString storageDir = QStandardPaths::writableLocation(
                                 QStandardPaths::AppDataLocation);
    // 在其下再建一个 avatars 子目录
    QDir dir(storageDir);
    if (!dir.exists("avatars")) {
        if (!dir.mkpath("avatars")) {
            qWarning() << "无法创建 avatars 目录：" << dir.filePath("avatars");
            QMessageBox::warning(
                this,
                tr("错误"),
                tr("无法创建存储目录，请检查权限或磁盘空间。")
            );
            return;
        }
    }

    QPixmap img;
    if(!img.load(file_path_)){
        QMessageBox::critical(
          this,tr("错误"),tr("图片加载失败"),QMessageBox::Ok
        );
        return;
    }
    // 拼接最终的文件名
    QString filePath = dir.filePath("avatars/" + generateUniqueIconName());
    // 保存 scaledPixmap 为 PNG（无损、最高质量）
    if (!img.save(filePath, "PNG")) {
        QMessageBox::warning(
            this,
            tr("保存失败"),
            tr("头像保存失败，请检查权限或磁盘空间。")
        );
    } else {
        qDebug() << "头像已保存到本地路径：" << filePath;
    }

    // 打开文件之后，才可以根据QFile来获取md5
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open file:" << filePath;
        return;
    }

    qint64 originalPos = file.pos();

    // 获取文件所对应的 md5
    QCryptographicHash hash(QCryptographicHash::Md5);
    if(!hash.addData(&file)){
        qWarning() << "Could not read from file: " << filePath;
        return;
    }

    QString md5 = hash.result().toHex();

    // 获取文件信息
    std::shared_ptr<QFileInfo> fileInfo = std::make_shared<QFileInfo>(file);
    // 获取文件的名字
    QString fileName = fileInfo->fileName();
    file_ = fileName;
    // 获取文件的总大小
    int totolSize = fileInfo->size();
    // 获取最后一个序号
    int lastSeq = 0;
    if(totolSize % MAX_FILE_LEN){
        lastSeq = totolSize / MAX_FILE_LEN + 1;
    }else{
        lastSeq = totolSize / MAX_FILE_LEN;
    }
    // 恢复文件指针原来的位置
    file.seek(originalPos);

    // 读取文件内容并且发送
    QByteArray buffer;
    // 序号(发送的时候是从1开始)
    int seq = 0;
    buffer = file.read(MAX_FILE_LEN);

    seq++;

    // 将文件内容转换为 Base64编码（实际上一次传输的文件大小不止是2048，因为base64编码之后，会将数据变大）
    QString base64Data = buffer.toBase64();
    //qDebug() << "Send Seq = " << seq << " Data is " << base64Data;

    UserManager::GetInstance()->add_file_path(fileName,filePath);

    QJsonObject obj;
    obj["uid"] = UserManager::GetInstance()->getUid();
    obj["token"] = UserManager::GetInstance()->GetToken();
    obj["filename"] = fileName;
    obj["seq"] = seq;
    obj["lastseq"] = lastSeq;
    obj["transferredsize"] = buffer.size() + (seq - 1) * MAX_FILE_LEN;
    obj["totolsize"] = totolSize;
    obj["data"] = base64Data;
    obj["md5"] = md5;

    QJsonDocument doc(obj);
    QByteArray sendData = doc.toJson();

    emit FileUploadMsg::GetInstance()->signalSendData(ID_UPLOAD_HEAD_ICON_REQ,sendData);

    file.close();
}

void SettingPage::slotRecvNotifyFromChatDialog()
{
    auto info = UserManager::GetInstance()->getUserInfo();
    ui->headLabel->setPixmap(returnPixMapByUrl(info->icon_));
}
