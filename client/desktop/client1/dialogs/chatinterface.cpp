#include "chatinterface.h"
#include "ui_chatinterface.h"
#include "widgets/chatitembase.h"
#include "widgets/messagetextedit.h"
#include "data/usermanager.h"
#include "network/tcpmsg.h"
#include "data/fileuploadmsg.h"
#include "data/chatdatalist.h"
#include "data/loadlocaldata.h"

ChatInterface::ChatInterface(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ChatInterface),
    peerInfo_(nullptr)
{
    ui->setupUi(this);

    // 移除边框
    setWindowFlags(Qt::FramelessWindowHint);
    // 重写的QTextEdit连接chatinterface的发送信号
    connect(ui->chatEdit,&MessageTextEdit::send,this,&ChatInterface::on_sendButton_clicked);

    ui->emojiLabel->setState("normal","hover","pressed","selected_normal","selected_hover","selected_pressed");
    ui->fileLabel->setState("normal","hover","pressed","selected_normal","selected_hover","selected_pressed");

    // 创建Emoji面板
    createEmojiPlanel();

    connect(ui->emojiLabel,&ClickLabel::signalIsHide,this,[&](){
        if(ui->emojiLabel->getCurState() == ClickLbState::normal){
            emojiPanel->hide();
        }
        else{
            emojiPanel->show();
        }
    });

    connect(ui->fileLabel,&ClickLabel::signalIsHide,this,[&](){
        if(ui->fileLabel->getCurState() != ClickLbState::normal){
            onSelectAndInsertFiles();
            // 需要将 文件label 设置为 normal
            ui->fileLabel->setCurState(ClickLbState::normal);
        }
    });
}

ChatInterface::~ChatInterface()
{
    delete ui;
}

void ChatInterface::setUserInfo(std::shared_ptr<FriendInfo> userinfo)
{
    peerInfo_ = userinfo;
    ui->nameLabel->setText(peerInfo_->name_);
}

void ChatInterface::SetIsLoading(bool is_loading)
{
    ui->chatDateList->setIsLoading(is_loading);
}

void ChatInterface::appendChatMsg(std::shared_ptr<ChatDataBase> msg)
{
    ui->chatDateList->appendChatItem(msg);
}

void ChatInterface::insertChatMsg(std::shared_ptr<ChatDataBase> msg)
{
    ui->chatDateList->prependChatItem(msg);
}

void ChatInterface::insertChatMsgs(std::vector<std::shared_ptr<ChatDataBase> > msgs)
{
    ui->chatDateList->prependChatItems(msgs);
}

// 设置当前的ChatThreadData
void ChatInterface::setCurChatThreadData(std::shared_ptr<ChatThreadData> cur_thread_data)
{
    cur_thread_data_ = cur_thread_data;
    ui->chatDateList->SetCurThreadID(cur_thread_data_->GetThreadId());
    int other_id = cur_thread_data_->GetOtherId();
    if(other_id == 0){
        // 说明是群聊

    }else{
        // 说明是私聊
        auto friend_info = UserManager::GetInstance()->getFriendById(other_id);
        if(friend_info == nullptr){
            qDebug() << "[ERROR] Friend Not Exist in UserManager::FriendList.";
        }else{
            // 将聊天界面（ChatDataList）全部清空并且再次设置新的数据
            ui->chatDateList->resetAllItems(cur_thread_data);
            // 清空上次缓存的消息
            unrsp_items_.clear();
            // 设置新的聊天人名字
            ui->nameLabel->setText(friend_info->name_);
        }
    }
}

void ChatInterface::on_sendButton_clicked()
{
    // 没有设置当前的聊天用户
    if(peerInfo_ == nullptr)
    {
        qDebug() << "peer is nullptr.";
        return;
    }
    // Client本人信息
    auto selfInfo = UserManager::GetInstance()->getUserInfo();

    auto pTextEdit = ui->chatEdit;
    ChatRole role = ChatRole::self;
    QString userName = selfInfo->name_;
    QString userIcon = selfInfo->icon_;
    //qDebug() << "username = " << userName << " " << "userIcon = " << userIcon;

    // 获取发送的消息
    QVector<std::shared_ptr<MsgInfo>> msgList = pTextEdit->getMsgList();

    QJsonObject textObj;
    QJsonArray textArray;

    QString uuidString;
    QUuid uuid;

    int txt_size = 0;

    if(cur_thread_data_ == nullptr){
        qDebug() << "cur_thread_data_ == nullptr.";
    }

    // 当前对应的 thread_id
    int thread_id = cur_thread_data_->GetThreadId();

    for(int i=0; i<msgList.size(); ++i)
    {
        //消息内容长度不合规就跳过
        if(msgList[i]->text_or_url_.length() > 1024){
            continue;
        }
        // 消息类型
        CHAT_MSG_TYPE type = msgList[i]->msg_type_;
        if(type == CHAT_MSG_TYPE::TEXT_MSG)
        {
            //生成不带大括号的唯一id
            uuid = QUuid::createUuid();
            //转为字符串
            uuidString = uuid.toString(QUuid::WithoutBraces);
            if(txt_size + msgList[i]->text_or_url_.length()> 1024){
                //qDebug() << "txt_size + msgList[i].content.length() = " << txt_size + msgList[i].content.length();

                textObj["fromuid"] = selfInfo->uid_;
                textObj["touid"] = peerInfo_->uid_;
                textObj["text_array"] = textArray;
                textObj["thread_id"] = thread_id;

                QJsonDocument doc(textObj);
                QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
                //发送并清空之前累计的文本列表
                txt_size = 0;
                textArray = QJsonArray();
                textObj = QJsonObject();
                //发送tcp请求给chat server
                emit TcpMsg::GetInstance()->signalSendData(REQUEST_ID::ID_TEXT_CHAT_MSG_REQ, jsonData);
            }

            //将bubble和uid绑定，以后可以等网络返回消息后设置是否送达
            //_bubble_map[uuidString] = pBubble;
            txt_size += msgList[i]->text_or_url_.length();
            QJsonObject obj;
            QByteArray utf8Message = msgList[i]->text_or_url_.toUtf8();

            QString content = QString::fromUtf8(utf8Message);
            obj["content"] = content;
            obj["unique_id"] = uuidString;
            textArray.append(obj);
            // to do ... 这里可能也可能是群聊
            auto txt_msg = std::make_shared<TextChatData>(-1,uuidString, thread_id, CHAT_THREAD_TYPE::CHAT_THREAD_TYPE_PRIVATE,
                CHAT_MSG_TYPE::TEXT_MSG, content, UserManager::GetInstance()->getUid(),peerInfo_->uid_, 0, QDateTime());
            // 添加进入未回复的map
            cur_thread_data_->AppenUnrspMsg(uuidString,txt_msg);  
            // 添加到界面
            ui->chatDateList->appendChatItem(txt_msg);
        }
        else if(type == CHAT_MSG_TYPE::PIC_MSG || type == CHAT_MSG_TYPE::FILE_MSG)
        {
            // 如果在图片/文件之前有消息，那么就需要先将文本消息发送过去
             if(txt_size){
                 //发送给服务器
                 textObj["text_array"] = textArray;
                 textObj["fromuid"] = selfInfo->uid_;
                 textObj["touid"] = peerInfo_->uid_;
                 textObj["thread_id"] = thread_id;
                 QJsonDocument doc(textObj);
                 QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
                 //发送并清空之前累计的文本列表
                 txt_size = 0;
                 textArray = QJsonArray();
                 textObj = QJsonObject();
                 //发送tcp请求给chat server
                 emit TcpMsg::GetInstance()->signalSendData(REQUEST_ID::ID_TEXT_CHAT_MSG_REQ, jsonData);
             } 
             //生成唯一id
             uuid = QUuid::createUuid();
             //转为字符串
             uuidString = uuid.toString(QUuid::WithoutBraces);
             // 将图片消息封装成ImageMsg
             std::shared_ptr<ImageDataBase> image = std::make_shared<ImageDataBase>(msgList[i],-1,uuidString,thread_id,CHAT_THREAD_TYPE::CHAT_THREAD_TYPE_PRIVATE,
                                                                                    type, selfInfo->uid_,peerInfo_->uid_, 0,QDateTime());
             // 添加进入未回复的map
             cur_thread_data_->AppenUnrspMsg(uuidString,image);
             // 添加到界面
             ui->chatDateList->appendChatItem(image);

             // 发送消息
             textObj["fromuid"] = selfInfo->uid_;
             textObj["touid"] = peerInfo_->uid_;
             textObj["thread_id"] = thread_id;
             textObj["md5"] = msgList[i]->md5_;
             textObj["name"] = msgList[i]->unique_name_;
             textObj["token"] = UserManager::GetInstance()->GetToken();
             textObj["unique_id"] = uuidString;
             textObj["type"] = type;

             // 将文件加入文件管理的容器
             qDebug() << "unique_name = " << msgList[i]->unique_name_;
             UserManager::GetInstance()->add_trans_file(msgList[i]->unique_name_,msgList[i]);
             // 发送消息
             QJsonDocument doc(textObj);
             QByteArray data = doc.toJson(QJsonDocument::Compact);
             emit TcpMsg::GetInstance()->signalSendData(ID_IMAGE_CHAT_MSG_REQ,data); 
             // 将传输文件的信息保存到数据库
             QString local_path = msgList[i]->text_or_url_;
             emit LoadLocalData::GetInstance()->signalAddTransferToDb(msgList[i]->unique_name_,0, msgList[i]->total_size_,local_path
                                                                      , TRANSFER_STATE::Uploading, TRANSFER_TYPE::Upload,"");
        }
        else
        {

        }
    }
    if(txt_size == 0){
        qDebug() << "Nothing leave.";
        return;
    }
    qDebug() << "textArray is " << textArray  << ", txt_size = " << txt_size;
    //发送给服务器
    textObj["text_array"] = textArray;
    textObj["fromuid"] = selfInfo->uid_;
    textObj["touid"] = peerInfo_->uid_;
    textObj["thread_id"] = thread_id;
    QJsonDocument doc(textObj);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
    //发送并清空之前累计的文本列表
    txt_size = 0;
    textArray = QJsonArray();
    textObj = QJsonObject();
    //发送tcp请求给chat server
    emit TcpMsg::GetInstance()->signalSendData(REQUEST_ID::ID_TEXT_CHAT_MSG_REQ, jsonData);
}

void ChatInterface::click_pause_btn(QString unique_name, TRANSFER_STATE transfer_state)
{
    UserManager::GetInstance()->PauseTransFileByName(unique_name, transfer_state);
}

void ChatInterface::click_resume_btn(QString unique_name, TRANSFER_STATE trans_state)
{
   auto msg_info = UserManager::GetInstance()->get_trans_file(unique_name);
   UserManager::GetInstance()->ResumeTransFileByName(unique_name); // 这一行不用也行，因为 所有的 MsgInfo结构体，共用一块内存。（在PictureBubble已经修改）
   // 继续发送
   if (msg_info->_state == TRANSFER_STATE::Uploading) {
       //qDebug() << "发送继续上传聊天图片的请求.";
       emit FileUploadMsg::GetInstance()->signalContinueUploadFile(unique_name);
   }else{
       qDebug() << "[ERROR]: Unknown Error.";
   }
}

void ChatInterface::createEmojiPlanel()
{
    const int EMOJI_PLANEL_WIDTH = (EMOJI_COUNT_EVERY_ROW + 1) * EMOJI_INTERVAL + EMOJI_COUNT_EVERY_ROW * EMOJI_BUTTON_SIZE;
    const int EMOJI_PLANEL_HEIGHT = (EMOJI_COUNT / EMOJI_COUNT_EVERY_ROW) * (EMOJI_BUTTON_SIZE + EMOJI_INTERVAL) + EMOJI_INTERVAL;

    emojiPanel = new QWidget(this);
    emojiPanel->resize(EMOJI_PLANEL_WIDTH,EMOJI_PLANEL_HEIGHT);
    emojiPanel->setWindowFlags(Qt::FramelessWindowHint); // 移除边框
    QGridLayout *emojiLayout = new QGridLayout(emojiPanel);
    emojiLayout->setSpacing(EMOJI_INTERVAL);
    // 定义一组Emoji
    QStringList emojis = {
        QString::fromUtf8("😀"), QString::fromUtf8("😂"), QString::fromUtf8("😍"),
        QString::fromUtf8("😊"), QString::fromUtf8("👍"), QString::fromUtf8("❤️"),
        QString::fromUtf8("🎉"), QString::fromUtf8("🚀"), QString::fromUtf8("🌟"),
        QString::fromUtf8("🐱"), QString::fromUtf8("🐶"), QString::fromUtf8("🍎"),
        QString::fromUtf8("🍕"), QString::fromUtf8("⚽"), QString::fromUtf8("🎸"),
        QString::fromUtf8("📱"), QString::fromUtf8("💻"), QString::fromUtf8("🎨"),
        QString::fromUtf8("🌈"), QString::fromUtf8("🔥"), QString::fromUtf8("💧"),
        QString::fromUtf8("🍦"), QString::fromUtf8("🎁"), QString::fromUtf8("💎")
    };
    // 创建Emoji按钮
    int row = 0, col = 0;
    const int colsPerRow = EMOJI_COUNT_EVERY_ROW;
    for (const QString &emoji : emojis) {
        QPushButton *btn = new QPushButton(emoji, emojiPanel);
        btn->setFixedSize(EMOJI_BUTTON_SIZE, EMOJI_BUTTON_SIZE);
        btn->setFont(QFont("Segoe UI Emoji", EMOJI_SIZE));
        emojiLayout->addWidget(btn, row, col);
        col++;
        if (col >= colsPerRow) {
            col = 0;
            row++;
        }
    }
    // 连接信号槽
    for (QPushButton *btn : emojiPanel->findChildren<QPushButton*>()) {
        QObject::connect(btn, &QPushButton::clicked, [=]() { // 注意，不能使用引用来捕获
            assert(btn != nullptr);
            ui->chatEdit->insertPlainText(btn->text());
            ui->chatEdit->setFocus();
        });
    }
    emojiPanel->hide();
}

void ChatInterface::repositionEmojiPanel()
{
    int old_x = emojiPanel->x();
    int oly_y = emojiPanel->y();
    // 获取functionWidget的位置
    int x = ui->functionWidget->x();
    int y = ui->functionWidget->y();
    // 获取emojiWidget的大小
    int size_x = emojiPanel->size().width();
    int size_y = emojiPanel->size().height();
    // 计算获取emojiWidget的大小新的位置
    int new_x = x;
    int new_y = y - size_y;
    // 移动位置
    emojiPanel->move(new_x,new_y);
}

void ChatInterface::UpdateChatInterface(std::shared_ptr<ChatDataBase> data)
{
    // 根据 unique_id 去更新状态
    QString unique_id = data->GetUniqueId();
    int status = data->GetStatus();
    QDateTime chat_time = data->GetChatTime();
    ui->chatDateList->updateMsgStatus(unique_id, (MsgStatus)status);
    ui->chatDateList->updateMsgChatTime(unique_id, chat_time);
}

void ChatInterface::onSelectAndInsertFiles()
{
    // 常见图片 + 所有文件（可按需扩展）
    const QString filter =
        "All Supported Files (*.png *.jpg *.jpeg *.bmp *.gif *.webp *.tif *.tiff *.svg "
        "*.txt *.pdf *.doc *.docx *.xls *.xlsx *.ppt *.pptx *.zip *.rar *.7z);;"
        "Images (*.png *.jpg *.jpeg *.bmp *.gif *.webp *.tif *.tiff *.svg);;"
        "All Files (*.*)";

    // 选择多个文件（返回的是绝对路径的 QStringList）
    QStringList files = QFileDialog::getOpenFileNames(
        this,
        tr("选择要发送的图片/文件"),
        QDir::homePath(),
        filter
    );

    if (files.isEmpty())
        return;

    // 依次插入：复用你已有的逻辑（会自动区分图片/文件并生成缩略/卡片）
    ui->chatEdit->insertFileFromUrl(files);

    // 可选：插入后让光标保持在末尾
    ui->chatEdit->moveCursor(QTextCursor::End);
    this->setFocus();
}

void ChatInterface::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    // 获取新大小
    QSize newSize = event->size();
    QSize oldSize = event->oldSize();

    qDebug() << "窗口大小改变:" << oldSize << "->" << newSize;

    // 调整子控件位置/大小
    if (emojiPanel) {
       repositionEmojiPanel();  // 重新定位 Emoji 面板
    }
}
