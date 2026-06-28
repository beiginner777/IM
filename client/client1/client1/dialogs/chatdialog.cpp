#include "chatdialog.h"
#include "ui_chatdialog.h"

#include <QFile>
#include "loadingdialog.h"
#include <QThread>
#include "friendlabel.h"
#include "network/tcpmsg.h"
#include "data/usermanager.h"
#include "offlinedialog.h"
#include "contactuserwidget.h"
#include "data/loadlocaldata.h"
#include "data/fileuploadmsg.h"

ChatDialog::ChatDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ChatDialog),
    mode_(ChatUIMode::ChatMode), // 默认展示聊天模式 和 聊天界面
    state_(ChatUIMode::ChatMode),
    curUid_(-1),
    cur_thread_id_(-1)
{
    ui->setupUi(this);
    // 设置qss样式
    QFile styleFile(":/style/chat.qss");
    if(!styleFile.open(QFile::ReadOnly)){
        qDebug("open chat.qss failed!");
    }
    QString style = QLatin1String(styleFile.readAll());
    this->setStyleSheet(style);

    // 安装事件过滤器
    this->installEventFilter(this);

    // 侧边栏
    ui->messageLabel->setProperty("state","normal");
    ui->messageLabel->setState("normal","hover","pressed","selected_normal","selected_hover","selected_pressed");
    ui->userLabel->setProperty("state","normal");
    ui->userLabel->setState("normal","hover","pressed","selected_normal","selected_hover","selected_pressed");
    ui->settingLabel->setProperty("state","normal");
    ui->settingLabel->setState("normal","hover","pressed","selected_normal","selected_hover","selected_pressed");
    addStateWidget(ui->messageLabel);
    addStateWidget(ui->userLabel);
    addStateWidget(ui->settingLabel);
    // 默认是messageLabel
    ui->messageLabel->setSelected(true);

    // 使得窗口右上角有 最大化 和 最小化 的按钮
    setWindowFlags(Qt::Window);

// 侧边栏显示
    // 头像显示
    // 图片自适应QLabel的大小
    ui->headLabel->setScaledContents(true);
    // 将图片设置到QLabel
    ui->headLabel->setPixmap(returnPixMapByUrl(UserManager::GetInstance()->getIcon()));
    // 清除测侧边栏的状态
    connect(ui->messageLabel,&StateWidget::clicked,this,&ChatDialog::slotMessageLabel);
    connect(ui->userLabel,&StateWidget::clicked,this,&ChatDialog::slotUserLabel);
    connect(ui->settingLabel,&StateWidget::clicked,this,&ChatDialog::slotSettigLabel);

// 搜索框
    // 搜索图标的动作
    QAction* searchAction = new QAction(ui->searchLineedit);
    searchAction->setIcon(QIcon(":/res/search.jpg"));
    // 将图标动作设置到最前端
    ui->searchLineedit->addAction(searchAction,QLineEdit::LeadingPosition);
    // 为输入框设置占位符文本，但实际不参与搜索主体
    ui->searchLineedit->setPlaceholderText(QStringLiteral("search"));

    // 透明图标的动作
    QAction* clearAction = new QAction(ui->searchLineedit);
    clearAction->setIcon(QIcon(":/res/transparent.png"));
    // 将图标动作设置到最末端
    ui->searchLineedit->addAction(clearAction,QLineEdit::TrailingPosition);

    // 当文本框的内容变换时，触发槽函数，来更改显示的图标（将透明图标 更换为 实际的cancel图标）
    connect(ui->searchLineedit,&QLineEdit::textChanged,[clearAction](const QString& text){
        if(!text.isEmpty()){
            // 文本框有内容时
            clearAction->setIcon(QIcon(":/res/cancel.jpg"));
        }else{
            // 文本框没有内容
            clearAction->setIcon(QIcon(":/res/transparent.png"));
        }
    });

    // 当文本框内容变化时，弹出SearchList
    connect(ui->searchLineedit,&QLineEdit::textChanged,[this](const QString& text){
        if(!text.isEmpty()){
            this->ShowSearch(true);
        }
        else{
            this->ShowSearch(false);
        }
    });

    // 当clearAction触发trigge信号时，就清除文本框的text
    // clearAction实际上就是清除图标
    // 只要点击清除图标就会触发triggered信号
    connect(clearAction,&QAction::triggered,[clearAction,this](){
        ui->searchLineedit->clear();
        clearAction->setIcon(QIcon(":/res/transparent.png"));
        // 失去焦点
        ui->searchLineedit->clearFocus();
    });

    // 跳转到好友申请界面
    connect(ui->connUserList,&ContactUserList::signalSwitchToFriendApplyInterface,this,[&](){
        ui->stackedWidget->setCurrentIndex(1);
    });
\
    // 当有有的好友申请的时候
    connect(TcpMsg::GetInstance().get(),&TcpMsg::signalReceiveFriendApply,this,&ChatDialog::slotRecvFriendApply);

// 给SearchList设置LineEdit
   ui->searchList->setSearchLineEdit(ui->searchLineedit);

// 聊天记录列表
    ShowSearch(false);

// test:
    // 聊天的Item
    // requestServerToAddChatUser();

// 接受到文本消息
    connect(TcpMsg::GetInstance().get(),&TcpMsg::signalReceiveNewTextMsg,this,&ChatDialog::slotRecvNewTextMsg);

// 切换当前的聊天用户
    connect(ui->chatUserList,&ChatUserList::signalChangeChatUser,this,&ChatDialog::slotChangeChatUser);

// 跳转到好友信息界面
    connect(ui->connUserList,&ContactUserList::signalSwitchToFriendChatInterface,this, &ChatDialog::slotChangeContactUser);

// 收到下线的消息
    connect(TcpMsg::GetInstance().get(),&TcpMsg::signalNotifyOffLine,[this](){
        qDebug() << "receive NotifyOffline messgae.";
        OfflineDialog* dialog = new OfflineDialog(this);
        dialog->show();
        connect(dialog,&OfflineDialog::click,[this](){
            this->close();
        });
    });

// 跳转到ChatUserList
    connect(ui->friendInfo, &friendInfoInterface::signalSwitchFromCotactToChat, this, &ChatDialog::slotSwitchFromCotactToChat);

// 创建私聊请求
    connect(TcpMsg::GetInstance().get(), &TcpMsg::signalRecvCreatePrivateChat, this, &ChatDialog::slotCreatePrivateChat);

// 通知消息是否发送成功
    connect(TcpMsg::GetInstance().get(),&TcpMsg::signalNotifyChatPageMsgStatus,this,&ChatDialog::slotNotifyChatPageMsgStatus);

// 收到更新自己头像的信息
    connect(ui->Setting,&SettingPage::updateHomeIcon,this,&ChatDialog::slotUpdateHomeIcon);

// 收到自己的头像下载完成的通知
    connect(FileUploadMsg::GetInstance().get(),&FileUploadMsg::signalSelfIconDownloadFinished,this,&ChatDialog::slotSelfIconDownloadFinished);
    connect(this,&ChatDialog::signalNotifySettingPage,ui->Setting,&SettingPage::slotRecvNotifyFromChatDialog);

// 向服务器加载更多的聊天用户
    connect(ui->chatUserList,&ChatUserList::signalRequestMoreFromServer,this,[=](){
        requestServerToAddChatUser();
    });

// 向服务器加载更多的好友申请
    connect(ui->AddFriend,&ApplyFriendPage::signalRequestMoreFromServer,this,[=](){
        requestServerToAddFriendApply();
    });

// 添加新的ChatUserWidget
    connect(TcpMsg::GetInstance().get(),&TcpMsg::signalAddNewChatUserWidget,this,&ChatDialog::slotAddChatUserWidget);

// 本地加载了新的ChatUserWidget
    connect(LoadLocalData::GetInstance().get(),&LoadLocalData::signalNotifyChatDialogSwitchFromConnToChat,this,&ChatDialog::slotNotifyChatDialogSwitchFromConnToChat);

// ChatServer第一次返回消息发送是否成功
    connect(TcpMsg::GetInstance().get(),&TcpMsg::signalNotifyChatPageImgStatus,this,&ChatDialog::slotNotifyChatPageImgStatus);

// 收到聊天图片
    connect(TcpMsg::GetInstance().get(),&TcpMsg::signalRecvNewImgMsg,this,&ChatDialog::slotRecvNewImgMsg);

// 加载特定的 ChatThreadInfo完成
    connect(LoadLocalData::GetInstance().get(),&LoadLocalData::singnalSpecifiedChatUserLoadFinished,this,&ChatDialog::slotSpecifiedChatUserLoadFinished);

// 收到好友消息加载消息的回包
    connect(TcpMsg::GetInstance().get(),&TcpMsg::signalLoadChatMsg,this,&ChatDialog::slotLoadChatMsgs);

// 本地加载聊天数据
    connect(LoadLocalData::GetInstance().get(),&LoadLocalData::signalNotifyLoadMoreMsgFromLocal,this,&ChatDialog::slotNotifyLoadMoreMsgFromLocal);
}

void ChatDialog::slotCreatePrivateChat(QJsonObject obj)
{
//    curUid_ = friendUid_;
//    mode_ = ChatUIMode::ChatMode;
//    // state_ = ChatUIMode::ChatMode;
//    ShowSearch(false);

//    //如果没找到，则创建新的插入listwidget
//    auto* chat_user_wid = new ChatUserWidget();
//    //查询好友信息
//    auto fi_ptr = UserManager::GetInstance()->getFriendById(curUid_);
//    chat_user_wid->setUserInfo(fi_ptr);
//    QListWidgetItem* item = new QListWidgetItem;
//    //qDebug()<<"chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
//    item->setSizeHint(chat_user_wid->sizeHint());
//    ui->chatUserList->insertItem(0, item);
//    ui->chatUserList->setItemWidget(item, chat_user_wid);
//    chatAddedItem_[curUid_]= item;
//    // 侧边栏 / UserList / StackWidget 跳转
//    slotMessageLabel();
//    // 设置选中条目
//    ui->chatUserList->setCurrentItem(item);
//    // 设置界面信息
//    slotChangeChatUser(chat_user_wid);

//    int thread_id = obj["thread_id"].toInt();
//    int user1_id = obj["create_uid"].toInt();
//    int user2_id = obj["peer_uid"].toInt() ;
//    qDebug() << "Create Private Chat(Thread_id = " << thread_id << " user1_id = " << user1_id << " user2_id = " << user2_id << " success.";

//    // 更新信息
//    UserManager::GetInstance()->setLastChatThreadID(thread_id);
//    // 缓存创建新ChatThreadData和thread_id的映射
//    std::shared_ptr<ChatThreadData> thread_data = std::make_shared<ChatThreadData>(user2_id,thread_id);
//    UserManager::GetInstance()->addChatThreadData(thread_data);
}

void ChatDialog::slotChangeChatUser(int friend_id)
{ 
    // 切换当前的聊天用户的uid,thread_id
    curUid_  = friend_id;
    cur_thread_id_ = UserManager::GetInstance()->GetThreadIdByUid(curUid_);
    if(cur_thread_id_ < 0){
        qDebug() << "Failed to Get ThreadID By Friend Uid: friend_uid = " << friend_id;
        return;
    }

    std::shared_ptr<FriendInfo> friend_info = UserManager::GetInstance()->getFriendById(friend_id);
    ui->Chat->setUserInfo(friend_info);
    auto chat_thread_data = UserManager::GetInstance()->GetChatThreadDataBuThreadID(cur_thread_id_);
    assert(chat_thread_data != nullptr);
    ui->Chat->setCurChatThreadData(chat_thread_data);
}

void ChatDialog::slotChangeContactUser(ContactUserWidget* item)
{
    auto info = item->getUserInfo();
    qDebug() << "change ContactUser to " << info->name_;
    // 切换当前聊天用户
    friendUid_  = item->getUserInfo()->uid_;
    ui->friendInfo->setUserInfo(item->getUserInfo());
    ui->stackedWidget->setCurrentIndex(3);
}

void ChatDialog::slotSwitchFromCotactToChat(int uid)
{
    qDebug() << "slotSwitchFromCotactToChat::uid = " << uid;
    if(ui->chatUserList->isChatUserExistByUid(uid)){
        qDebug() << "set chat item msg, uid is " << uid;
        curUid_ = friendUid_;
        mode_ = ChatUIMode::ChatMode;
        // state_ = ChatUIMode::ChatMode;
        ShowSearch(false);
        // 侧边栏 / UserList / StackWidget 跳转
        slotMessageLabel();
        // 设置选中条目
        ui->chatUserList->setCurrentChatUserItem(uid);
        // 设置界面信息
        slotChangeChatUser(uid);
    }
    else{
        // 需要在本地数据库加载这一条记录
        qDebug() << "[ERROR]: " << __FILE__ << ":" << __LINE__;
    }
}

void ChatDialog::slotRecvNewTextMsg(std::vector<std::shared_ptr<ChatDataBase>> chat_datas)
{
    qDebug() << "Receice New TextMsg.";
    // 将联系人的StateWidget显示红点状态
    ui->messageLabel->showRedPoint(true);
    // 将数据同步到数据库当中
    int thread_id = -1;
    for (auto& data : chat_datas) {
        thread_id = data->GetThreadId();
        // 修改数据库
        emit LoadLocalData::GetInstance()->signalmodifyMsgId(thread_id,data->GetMsgId(),INT_MAX);
        // 修改缓存
        UserManager::GetInstance()->set_threadId_min_message_id(thread_id,data->GetMsgId());
        // 添加消息到聊天记录当中
        std::shared_ptr<ChatThreadData> thread_data = UserManager::GetInstance()->GetChatThreadDataBuThreadID(thread_id);
        thread_data->AddMsg(data);
        // 渲染到 ChatUserlist 中
        int friend_id = UserManager::GetInstance()->GetUidByThreadId(thread_id);
        if (cur_thread_id_ != thread_id) {
            ui->chatUserList->updateItemUnreadMsgCount(friend_id, data->GetContent(), data->GetChatTime(),1);
            continue;
        }else{
            ui->chatUserList->updateItemLastMsg(friend_id,data->GetContent(),data->GetChatTime());
        }
        // 渲染到聊天界面中
        ui->Chat->appendChatMsg(data);
    }
    // 将数据同步到数据库当中
    emit LoadLocalData::GetInstance()->signalstoreMessages(chat_datas);
}

void ChatDialog::slotRecvFriendApply(std::shared_ptr<AddFriendApply> apply)
{
    qDebug() << "receive new friend apply.";
    // 将联系人的StateWidget显示红点状态
    ui->userLabel->showRedPoint(true);
    // 修改ApplyFriendPage的 map 和 界面
    ui->AddFriend->addNewFriendApply(apply);
    // 将好友申请保存到本地
    std::vector<std::shared_ptr<ApplyInfo>> applys;
    std::shared_ptr<ApplyInfo> apply_info = std::make_shared<ApplyInfo>(apply);
    // 修改缓存
    UserManager::GetInstance()->set_last_friend_apply_id(apply->id_);
    applys.push_back(apply_info);
    emit LoadLocalData::GetInstance()->signalStoreFriendApply(applys);
}

ChatDialog::~ChatDialog()
{
    delete ui;
}

void ChatDialog::addStateWidget(StateWidget *w)
{
    widgets_.push_back(w);
}

void ChatDialog::initConnUserList()
{
    requestServerToAddConnUser();
}

void ChatDialog::initChatUserList()
{
    requestServerToAddChatUser();
    emit LoadLocalData::GetInstance()->signalLoadChatUserList();
}

void ChatDialog::initFriendApply()
{
    requestServerToAddFriendApply();
    emit LoadLocalData::GetInstance()->signalloadFriendApplyList();
}

void ChatDialog::slotMessageLabel()
{
    clearStateWidget(ui->messageLabel);
    ui->stackedWidget->setCurrentIndex(0);
    mode_ = ChatUIMode::ChatMode;
    //state_ = ChatUIMode::ChatMode;
    ShowSearch(false);
}

void ChatDialog::slotUserLabel()
{
    clearStateWidget(ui->userLabel);
    ui->stackedWidget->setCurrentIndex(1);
    mode_ = ChatUIMode::ContactMode;
    //state_ = ChatUIMode::ContactMode;
    ShowSearch(false);
}

void ChatDialog::slotSettigLabel()
{
    clearStateWidget(ui->settingLabel);
    ui->stackedWidget->setCurrentIndex(2);
    mode_ = ChatUIMode::SettingMode;
    // 这个就保持不变
    //state_ = ChatUIMode::SettingMode;
    ShowSearch(false);
    qDebug() << "Switch to SettingPage.";
}

bool ChatDialog::eventFilter(QObject *obj, QEvent *event)
{
    if(event->type() == QEvent::MouseButtonPress)
    {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        handleMousePressEvent(mouseEvent);
    }
    QDialog::eventFilter(obj,event);
}

// 如果鼠标超出了特定区域，就将SearchLineEdit清空
void ChatDialog::handleMousePressEvent(QMouseEvent *event)
{
    if(state_ != ChatUIMode::SearchMode)
    {
        // 如果不处于搜索的状态那么就直接返回
        return;
    }
    // 将鼠标的位置转换为搜索列表坐标系中的位置
    QPoint point = ui->searchList->mapFromGlobal(event->globalPos());
    // 判断点击位置point是否在列表坐标系当中
    if(!ui->searchList->rect().contains(point))
    {
        // 不在聊天列表当中，那么就退出搜索状态
        ui->searchLineedit->clear();
        //ShowSearch(false);
        qDebug() << "quit SearchMode.";
    }
}

void ChatDialog::ShowSearch(bool bsearch)
{
    if(bsearch){
        ui->chatUserList->hide(); // 隐藏用户聊天记录
        ui->connUserList->hide(); // 隐藏用户列表
        ui->searchList->show(); // 展示搜索结果
        state_ = ChatUIMode::SearchMode;
    }else if(mode_ == ChatUIMode::ChatMode){
        ui->connUserList->hide();
        ui->searchList->hide();
        ui->chatUserList->show();
        state_ = ChatUIMode::ChatMode;
    }else if(mode_ == ChatUIMode::ContactMode){
        ui->chatUserList->hide();
        ui->searchList->hide();
        ui->connUserList->show();
        state_ = ChatUIMode::ContactMode;
    }else if(mode_ == ChatUIMode::SettingMode){
        // 不做处理
        return;
    }
}

void ChatDialog::requestServerToAddConnUser()
{
    QJsonObject jsonobj;
    jsonobj["uid"] = UserManager::GetInstance()->getUid();
    jsonobj["last_friend_id"] = UserManager::GetInstance()->get_last_friend_id();
    QJsonDocument doc(jsonobj);
    QByteArray jsondata = doc.toJson(QJsonDocument::Compact);
    emit TcpMsg::GetInstance()->signalSendData(ID_LOAD_MORE_FRIEND_REQ,jsondata);
}

void ChatDialog::requestServerToAddChatUser()
{
    QJsonObject obj;
    int uid = UserManager::GetInstance()->getUid();
    obj["uid"] = uid;
    // 获取上一次请求的ThreadID
    int last_chat_thread_id = UserManager::GetInstance()->GetLastChatThreadID();
    obj["last_thread_id"] = last_chat_thread_id;
    obj["page_size"] = CHATUSERLIST_LOAD_PAGESIZE;
    QJsonDocument doc(obj);
    QByteArray data = doc.toJson(QJsonDocument::Compact);
    emit TcpMsg::GetInstance()->signalSendData(ID_LOAD_CHAT_THREAD_REQ, data);
}

void ChatDialog::requestServerToAddFriendApply()
{
    // 向服务器发送加载ChatUserWidget请求
    QJsonObject obj;
    int uid = UserManager::GetInstance()->getUid();
    obj["uid"] = uid;
    // 获取上一次请求的ThreadID
    int last_friend_apply_id = UserManager::GetInstance()->get_last_friend_apply_id();
    obj["last_friend_apply_id"] = last_friend_apply_id;
    // 加载的数量
    obj["page_size"] = FRIENDAPPLYLIST_LOAD_PAGESIZE;

    QJsonDocument doc(obj);
    QByteArray data = doc.toJson(QJsonDocument::Compact);

    emit TcpMsg::GetInstance()->signalSendData(ID_LOAD_FRIEND_APPLY_REQ,data);
}

void ChatDialog::clearStateWidget(StateWidget *w)
{
    for(StateWidget* sw : widgets_)
    {
        if(sw == w){
            continue;
        }
        sw->clearState();
    }
}

// 消息是否发送成功
void ChatDialog::slotNotifyChatPageMsgStatus(int thread_id, std::vector<std::shared_ptr<TextChatData>> chat_datas)
{
    // 调整至顶部
    //ui->chatUserList->recreateItemAtTop(UserManager::GetInstance()->GetUidByThreadId(thread_id));

    // 找到thread_id对应的ChatThradData
    auto chat_thread_data = UserManager::GetInstance()->GetChatThreadDataBuThreadID(thread_id);
    assert(chat_thread_data != nullptr);
    // 将记录保存到数据库
    std::vector<std::shared_ptr<ChatDataBase>> datas;
    for(auto data : chat_datas){
        datas.push_back(std::static_pointer_cast<ChatDataBase>(data));
    }
    emit LoadLocalData::GetInstance()->signalstoreMessages(datas);

    // 更新消息
    for(auto& msg : chat_datas){
        // 更新数据库
        emit LoadLocalData::GetInstance()->signalmodifyMsgId(thread_id,msg->GetMsgId(),INT_MAX);
        // 更新缓存
        emit UserManager::GetInstance()->set_threadId_min_message_id(thread_id,msg->GetMsgId());
        // 更新聊天记录
        chat_thread_data->UpdateMsgStatus(msg);
        // 更新ChatUserList最后一条消息
        int friend_id = msg->_recv_uid_;
        QString content = msg->GetContent();
        if(msg->GetMsgType() == CHAT_MSG_TYPE::PIC_MSG){
            content = "[图片]";
        }else if(msg->GetMsgType() == CHAT_MSG_TYPE::FILE_MSG){
            content = "[文件]";
        }
        if (cur_thread_id_ != thread_id) {
            ui->chatUserList->updateItemUnreadMsgCount(friend_id, content, msg->GetChatTime(),1);
            continue;
        }else{
            ui->chatUserList->updateItemLastMsg(friend_id,content,msg->GetChatTime());
        }
        ui->Chat->UpdateChatInterface(msg);
    }
}

void ChatDialog::slotUpdateHomeIcon(QString icon)
{
    ui->headLabel->setScaledContents(true);

    QPixmap pic = returnPixMapByUrl(icon);
    ui->headLabel->setPixmap(pic);

    qDebug() << "主页头像更新成功.";
}

void ChatDialog::slotSelfIconDownloadFinished(bool flag)
{
    if(flag){
        ui->headLabel->setPixmap(returnPixMapByUrl(UserManager::GetInstance()->getIcon())); // 主界面的头像
        emit signalNotifySettingPage(); // SettingPage 界面的头像（可做可不做）
    }else{
        qDebug() << "Self head_icon download failed.";
    }
}

void ChatDialog::slotAddChatUserWidget(std::shared_ptr<UserInfo> user_info)
{
    qDebug() << "add new chatuser.";
    // 将联系人的StateWidget显示红点状态
    ui->messageLabel->showRedPoint(true);
    // 修改ChatUserList的 map 和 界面
    ui->chatUserList->addChatUserWidget(user_info);
}

void ChatDialog::slotNotifyChatDialogSwitchFromConnToChat(int uid)
{
    slotSwitchFromCotactToChat(uid);
}

void ChatDialog::slotNotifyChatPageImgStatus(int thread_id, std::shared_ptr<ImageDataBase> msg)
{
    // 更新数据库
    emit LoadLocalData::GetInstance()->signalmodifyMsgId(thread_id,msg->GetMsgId(),INT_MAX);
    // 更新缓存
    emit UserManager::GetInstance()->set_threadId_min_message_id(thread_id,msg->GetMsgId());

    // 调整至顶部
    //ui->chatUserList->recreateItemAtTop(UserManager::GetInstance()->GetUidByThreadId(thread_id));

    // 找到thread_id对应的ChatThradData
    auto chat_thread_data = UserManager::GetInstance()->GetChatThreadDataBuThreadID(thread_id);
    assert(chat_thread_data != nullptr);
    // 更新 ChatThreadData缓存 中的信息
    chat_thread_data->UpdateMsgStatus(msg);
    // 将记录保存到数据库
    std::vector<std::shared_ptr<ChatDataBase>> msgs;
    msgs.push_back(msg);
    emit LoadLocalData::GetInstance()->signalstoreMessages(msgs);
    // 将最后一条消息渲染到 ChatUserlist 中
    int friend_id = msg->_recv_uid_;
    // 渲染到 ChatUserlist 中
    QString content = msg->GetContent();
    if(msg->GetMsgType() == CHAT_MSG_TYPE::PIC_MSG){
        content = "[图片]";
    }else if(msg->GetMsgType() == CHAT_MSG_TYPE::FILE_MSG){
        content = "[文件]";
    }
    if (cur_thread_id_ != thread_id) {
        ui->chatUserList->updateItemUnreadMsgCount(friend_id, content, msg->GetChatTime(),1);
    }else{
        ui->chatUserList->updateItemLastMsg(friend_id, content, msg->GetChatTime());
    }
    ui->Chat->UpdateChatInterface(msg);
}

void ChatDialog::slotRecvNewImgMsg(std::shared_ptr<ImageDataBase> image)
{
    qDebug() << "Receive Image Msg.";
    // 将联系人的StateWidget显示红点状态
    ui->messageLabel->showRedPoint(true);
    // 添加消息到聊天记录当中
    auto thread_id = image->GetThreadId();
    // 获取ChatThreadData
    auto thread_data = UserManager::GetInstance()->GetChatThreadDataBuThreadID(thread_id);
    thread_data->AddMsg(image);
    // 将记录保存到数据库
    std::vector<std::shared_ptr<ChatDataBase>> msgs;
    msgs.push_back(image);
    emit LoadLocalData::GetInstance()->signalstoreMessages(msgs);
    emit LoadLocalData::GetInstance()->signalmodifyMsgId(thread_id,image->GetMsgId(),INT_MAX);
    // 修改缓存
    UserManager::GetInstance()->set_threadId_min_message_id(thread_id,image->GetMsgId());
    int friend_id = UserManager::GetInstance()->GetUidByThreadId(thread_id);
    // 渲染到 ChatUserlist 中
    QString content = image->GetContent();
    if(image->GetMsgType() == CHAT_MSG_TYPE::PIC_MSG){
        content = "[图片]";
    }else if(image->GetMsgType() == CHAT_MSG_TYPE::FILE_MSG){
        content = "[文件]";
    }
    if (cur_thread_id_ != thread_id) {
        ui->chatUserList->updateItemUnreadMsgCount(friend_id, content, image->GetChatTime(),1);
    }else{
        ui->chatUserList->updateItemLastMsg(friend_id, content, image->GetChatTime());
    }
    // 渲染到界面中
    ui->Chat->appendChatMsg(image);
}

void ChatDialog::slotSpecifiedChatUserLoadFinished(std::shared_ptr<ChatThreadInfo> info)
{
    int peerUid = 0;
    if(info->user1_id_ == UserManager::GetInstance()->getUid()){
        peerUid = info->user2_id_;
    }else{
        peerUid = info->user1_id_;
    }
    int thread_id = info->threadId_;
    // 将聊天记录条目渲染到ChatUserList
    std::shared_ptr<FriendInfo> friend_info = UserManager::GetInstance()->getFriendById(peerUid);
    std::shared_ptr<UserInfo> user_info = std::make_shared<UserInfo>();
    user_info->uid_ = peerUid;
    user_info->icon_ = friend_info->icon_;
    user_info->sex_ = friend_info->sex_;
    user_info->desc_ = friend_info->desc_;
    user_info->name_ = friend_info->name_;
    user_info->nick_ = friend_info->nick_;
    user_info->email_ = friend_info->email_;
    ui->chatUserList->addChatUserWidget(user_info);
    // 获取ChatThreadData
    auto thread_data = UserManager::GetInstance()->GetChatThreadDataBuThreadID(thread_id);
    assert(thread_data != nullptr);
    // 更新界面
    for(std::shared_ptr<ChatDataBase> data : UserManager::GetInstance()->GetCacheMsg(thread_id)){
         thread_data->AddMsg(data);
         // 渲染到 ChatUserlist 中
         ui->chatUserList->updateItemLastMsg(peerUid, data->GetContent(), data->GetChatTime());
         if(cur_thread_id_ != thread_id){
             return;
         }
         // 渲染到界面中
         ui->Chat->appendChatMsg(data);
    }
    UserManager::GetInstance()->rmCacheMsg(thread_id);
}

void ChatDialog::slotLoadChatMsgs(QJsonObject jsonObj)
{
    int selfuid = jsonObj["uid"].toInt();
    int peerUid = jsonObj["peerUid"].toInt();
    bool is_load_more =  jsonObj["is_load_more"].toBool();
    int min_message_id = jsonObj["min_message_id"].toInt();
    int max_message_id = jsonObj["max_message_id"].toInt();
    int thread_id = jsonObj["thread_id"].toInt();

    assert(UserManager::GetInstance()->isCreateThreadByThreadID(thread_id) == true);

    // 更新缓存
    UserManager::GetInstance()->set_threadId_min_message_id(thread_id,min_message_id);
    UserManager::GetInstance()->set_threadId_max_message_id(thread_id,max_message_id);

    std::shared_ptr<ChatThreadData> thread_data = UserManager::GetInstance()->GetChatThreadDataBuThreadID(thread_id);
    assert(thread_data != nullptr);

    std::vector<std::shared_ptr<ChatDataBase>> chat_data_bases;
    for(const QJsonValue& value : jsonObj["chat_messages"].toArray()){
        int message_id = value["message_id"].toInt();
        int thread_id = value["thread_id"].toInt();
        int sender_id = value["sender_id"].toInt();
        int recv_id = value["recv_id"].toInt();
        QString content = value["content"].toString();
        QDateTime chat_time = returnQDateTimeByQString(value["chat_time"].toString());
        int status = value["status"].toInt();
        int message_type = value["message_type"].toInt();

        QUuid uuid = QUuid::createUuid();
        QString uuidString = uuid.toString();

        std::shared_ptr<ChatDataBase> base = nullptr;
        if(message_type == CHAT_MSG_TYPE::TEXT_MSG){
            base = std::make_shared<TextChatData>(message_id,"",thread_id,CHAT_THREAD_TYPE::CHAT_THREAD_TYPE_PRIVATE,
                                                                                CHAT_MSG_TYPE::TEXT_MSG,content,sender_id,recv_id,status,chat_time);
        }else if(message_type == CHAT_MSG_TYPE::PIC_MSG){
            // 添加到缓存
            QString local_path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/avatars/" + content;
            std::shared_ptr<MsgInfo> info = std::make_shared<MsgInfo>();
            info->text_or_url_ = local_path;
            info->unique_name_ = content;

            if(sender_id == selfuid){
                info->_type = TRANSFER_TYPE::Upload;
                // 上传
                if(status == MsgStatus::UN_READ){
                    info->_state = TRANSFER_STATE::Upload_Failed;
                    // 将这个文件的上传信息添加到数据库
                    emit LoadLocalData::GetInstance()->signalAddTransferToDb(content, 0, 0, local_path,TRANSFER_STATE::Upload_Failed
                                                                             ,TRANSFER_TYPE::Upload,"");
                }else if(status == MsgStatus::READED) {
                    info->_state = TRANSFER_STATE::Upload_Finish;
                    // 将这个文件的上传信息添加到数据库
                    emit LoadLocalData::GetInstance()->signalAddTransferToDb(content, 0, 0, local_path,TRANSFER_STATE::Upload_Finish
                                                                             ,TRANSFER_TYPE::Upload,"");
                }else if(status == MsgStatus::SEND_FAILED){
                    info->_state = TRANSFER_STATE::Upload_Failed;
                    // 将这个文件的上传信息添加到数据库
                    emit LoadLocalData::GetInstance()->signalAddTransferToDb(content, 0, 0, local_path,TRANSFER_STATE::Upload_Failed
                                                                             ,TRANSFER_TYPE::Upload,"");
                }
                UserManager::GetInstance()->add_trans_file(content, info);
                base = std::make_shared<ImageDataBase>(info,message_id,uuidString,thread_id,CHAT_THREAD_TYPE::CHAT_THREAD_TYPE_PRIVATE,
                                                                                    CHAT_MSG_TYPE::PIC_MSG,sender_id,recv_id,status,chat_time);

            }

            if(sender_id != selfuid){
                info->_type = TRANSFER_TYPE::Download;
                // 下载
                if(status == MsgStatus::UN_READ){
                    info->_state = TRANSFER_STATE::Downloading;
                }else if(status == MsgStatus::READED) {
                    info->_state = TRANSFER_STATE::Download_Finish;
                }else if(status == MsgStatus::SEND_FAILED){
                    info->_state = TRANSFER_STATE::Download_Failed;
                }

//                std::shared_ptr<DownloadFileInfo> file_info = std::make_shared<DownloadFileInfo>(content,0,0,0,
//                                                                                                 0,local_path,Download_File_Type::CHAT_IMAGE, TRANSFER_STATE::Downloading);
//                UserManager::GetInstance()->add_download_file(content,file_info);

                UserManager::GetInstance()->add_trans_file(content, info);

                // 那么就去向资源服务器请求文件下载
                QJsonObject send_obj;
                send_obj["download_file"] = content;
                send_obj["seq"] = 1;
                send_obj["last_seq"] = 0;
                send_obj["trans_size"] = 0;
                send_obj["total_size"] = 0;
                send_obj["uid"] = UserManager::GetInstance()->getUid();
                send_obj["token"] = UserManager::GetInstance()->GetToken();
                send_obj["client_save_path"] = local_path ;
                send_obj["download_file_type"] = Download_File_Type::CHAT_IMAGE;
                QJsonDocument send_doc(send_obj);
                QByteArray send_data = send_doc.toJson(QJsonDocument::Compact);

                emit FileUploadMsg::GetInstance()->signalSendData(ID_DOWN_LOAD_FILE_REQ,send_data);

                base = std::make_shared<ImageDataBase>(info,message_id,uuidString,thread_id,CHAT_THREAD_TYPE::CHAT_THREAD_TYPE_PRIVATE,
                                                                                    CHAT_MSG_TYPE::PIC_MSG,sender_id,recv_id,status,chat_time);

                // 将这个文件的下载信息添加到数据库
                emit LoadLocalData::GetInstance()->signalAddTransferToDb(content, 0, 0, local_path,TRANSFER_STATE::Downloading
                                                                         ,TRANSFER_TYPE::Download,"");
                }

        }else if(message_type == CHAT_MSG_TYPE::FILE_MSG){

            // 添加到缓存
            QString local_path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/avatars/" + content;
            std::shared_ptr<MsgInfo> info = std::make_shared<MsgInfo>();
            info->text_or_url_ = local_path;
            info->unique_name_ = content;
            if(sender_id == selfuid){
                info->_type = TRANSFER_TYPE::Upload;
                // 上传
                if(status == MsgStatus::UN_READ){
                    info->_state = TRANSFER_STATE::Upload_Failed;
                    // 将这个文件的上传信息添加到数据库
                    emit LoadLocalData::GetInstance()->signalAddTransferToDb(content, 0, 0, local_path,TRANSFER_STATE::Upload_Failed
                                                                             ,TRANSFER_TYPE::Upload,"");
                }else if(status == MsgStatus::READED) {
                    info->_state = TRANSFER_STATE::Upload_Finish;
                    // 将这个文件的上传信息添加到数据库
                    emit LoadLocalData::GetInstance()->signalAddTransferToDb(content, 0, 0, local_path,TRANSFER_STATE::Upload_Finish
                                                                             ,TRANSFER_TYPE::Upload,"");
                }else if(status == MsgStatus::SEND_FAILED){
                    info->_state = TRANSFER_STATE::Upload_Failed;
                    // 将这个文件的上传信息添加到数据库
                    emit LoadLocalData::GetInstance()->signalAddTransferToDb(content, 0, 0, local_path,TRANSFER_STATE::Upload_Failed
                                                                             ,TRANSFER_TYPE::Upload,"");
                }
                UserManager::GetInstance()->add_trans_file(content, info);
                base = std::make_shared<ImageDataBase>(info,message_id,uuidString,thread_id,CHAT_THREAD_TYPE::CHAT_THREAD_TYPE_PRIVATE,
                                                                                    CHAT_MSG_TYPE::FILE_MSG,sender_id,recv_id,status,chat_time);

            }

            if(sender_id != selfuid){
                info->_type = TRANSFER_TYPE::Download;
                // 下载
                if(status == MsgStatus::UN_READ){
                    info->_state = TRANSFER_STATE::Downloading;
                }else if(status == MsgStatus::READED) {
                    info->_state = TRANSFER_STATE::Download_Finish;
                }else if(status == MsgStatus::SEND_FAILED){
                    info->_state = TRANSFER_STATE::Download_Failed;
                }
//                std::shared_ptr<DownloadFileInfo> file_info = std::make_shared<DownloadFileInfo>(content,0,0,0,
//                                                                                                 0,local_path,Download_File_Type::CHAT_IMAGE, TRANSFER_STATE::Downloading);
//                UserManager::GetInstance()->add_download_file(content,file_info);

                UserManager::GetInstance()->add_trans_file(content, info);
                // 那么就去向资源服务器请求文件下载
                QJsonObject send_obj;
                send_obj["download_file"] = content;
                send_obj["seq"] = 1;
                send_obj["last_seq"] = 0;
                send_obj["trans_size"] = 0;
                send_obj["total_size"] = 0;
                send_obj["uid"] = UserManager::GetInstance()->getUid();
                send_obj["token"] = UserManager::GetInstance()->GetToken();
                send_obj["client_save_path"] = local_path ;
                send_obj["download_file_type"] = Download_File_Type::CHAT_FILE;
                QJsonDocument send_doc(send_obj);
                QByteArray send_data = send_doc.toJson(QJsonDocument::Compact);

                emit FileUploadMsg::GetInstance()->signalSendData(ID_DOWN_LOAD_FILE_REQ,send_data);

                base = std::make_shared<ImageDataBase>(info,message_id,uuidString,thread_id,CHAT_THREAD_TYPE::CHAT_THREAD_TYPE_PRIVATE,
                                                                                    CHAT_MSG_TYPE::FILE_MSG,sender_id,recv_id,status,chat_time);

                // 将这个文件的下载信息添加到数据库
                emit LoadLocalData::GetInstance()->signalAddTransferToDb(content, 0, 0, local_path,TRANSFER_STATE::Downloading
                                                                         ,TRANSFER_TYPE::Download,"");
            }

        }

        thread_data->AddMsg(base);
        chat_data_bases.push_back(base);

        // 将最后一条消息更新到 对应的 chatuserwidget
        if(message_type == CHAT_MSG_TYPE::PIC_MSG){
            content = "[图片]";
        }else if(message_type == CHAT_MSG_TYPE::FILE_MSG){
            content = "[文件]";
        }
        if(cur_thread_id_ != thread_id){
            ui->chatUserList->updateItemUnreadMsgCount(peerUid,content,chat_time,1);
            continue;
        }else{
            ui->chatUserList->updateItemLastMsg(peerUid,content,chat_time);
        }
        // 渲染到界面中
        ui->Chat->appendChatMsg(base);

    }
    // 更新数据库
    emit LoadLocalData::GetInstance()->signalmodifyMsgId(thread_id,min_message_id,max_message_id);
    emit LoadLocalData::GetInstance()->signalstoreMessages(chat_data_bases);

    if(is_load_more){
        ui->chatUserList->requestChatMessageToServer(thread_id,peerUid,min_message_id,max_message_id);
    }else{
        emit LoadLocalData::GetInstance()->signalLoadChatMessage(thread_id);
    }
}

void ChatDialog::slotNotifyLoadMoreMsgFromLocal(int thread_id, std::vector<std::shared_ptr<ChatDataBase> > datas)
{
    //qDebug() << "going to draw local data to chatview，thread_id = " << thread_id;
    // 将数据渲染到当前的UI界面
    for(auto data : datas){
        int friend_id = UserManager::GetInstance()->GetUidByThreadId(thread_id);
        // 将最后一条消息更新到 对应的 chatuserwidget
        QString content = data->GetContent();
        if(data->GetMsgType() == CHAT_MSG_TYPE::PIC_MSG){
            content = "[图片]";
        }else if(data->GetContent() == CHAT_MSG_TYPE::FILE_MSG){
            content = "[文件]";
        }
        ui->chatUserList->updateItemLastMsg(friend_id,content,data->GetChatTime());
    }
    if(cur_thread_id_ != thread_id){
        return;
    }
    // 渲染到界面中
    ui->Chat->insertChatMsgs(datas);
    // 设置为 不是加载状态
    ui->Chat->SetIsLoading(false);
}
