#include "applyfriendpage.h"
#include "ui_applyfriendpage.h"
#include "usermanager.h"
#include "authfriendapply.h"
#include "tcpmsg.h"
#include "loadlocaldata.h"
#include "fileuploadmsg.h"

ApplyFriendPage::ApplyFriendPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ApplyFriendPage),
    is_load_more_from_server_(true),
    is_load_more_from_local_(true)
{
    ui->setupUi(this);

    connect(TcpMsg::GetInstance().get(),&TcpMsg::signalLoadFriendApplyList,this,&ApplyFriendPage::slotLoadFriendApplyList);
    connect(LoadLocalData::GetInstance().get(),&LoadLocalData::signalDrawFriendApplyToList,this,&ApplyFriendPage::slotDrawFriendApplyToList);
    connect(FileUploadMsg::GetInstance().get(),&FileUploadMsg::signalOtherIconDownloadFinished,this,&ApplyFriendPage::slotUserIconChange);

    connect(FileUploadMsg::GetInstance().get(),&FileUploadMsg::signalOnlineFriendIconFinished,this,&ApplyFriendPage::slotOnlineFriendIconFinished);

    //ui->FriendApply->installEventFilter(this);
    ui->FriendApply->viewport()->installEventFilter(this);
}

ApplyFriendPage::~ApplyFriendPage()
{
    delete ui;
}

void ApplyFriendPage::addFriendApply(std::vector<std::shared_ptr<ApplyInfo> > apply_info)
{
    for(auto apply : apply_info){
        ApplyFriendItem* item = new ApplyFriendItem();
        auto info = std::make_shared<ApplyInfo>(apply->id_,apply->uid_,apply->name_,apply->email_,apply->desc_,apply->icon_,
                                                apply->sex_,apply->apply_time_,apply->status_);    
        icons_[info->icon_] = info->uid_;
        item->SetInfo(info);
        QListWidgetItem* widget = new QListWidgetItem();
        widget->setSizeHint(item->sizeHint());
        ui->FriendApply->insertItem(0, widget);
        ui->FriendApply->setItemWidget(widget,item);
        if(apply->status_ == 0){
            unAuthItems_[apply->uid_] = item;
        }
        ApplyAddedItem_[apply->uid_] = widget;
        connect(item,&ApplyFriendItem::signalAcceptFriendCommit,this,&ApplyFriendPage::acceptApply);
        connect(item,&ApplyFriendItem::signalRefuseApply,this,&ApplyFriendPage::refuseApply);
    }
}

void ApplyFriendPage::addNewFriendApply(std::shared_ptr<AddFriendApply> apply)
{
    auto info = std::make_shared<ApplyInfo>(apply->id_, apply->fromUid_,apply->name_,apply->email_,apply->desc_,apply->icon_,apply->sex_,apply->apply_time_,apply->status_);
    // 将好友申请保存到本地
    std::vector<std::shared_ptr<ApplyInfo>> apply_list;
    apply_list.push_back(info);
    emit LoadLocalData::GetInstance()->signalStoreFriendApply(apply_list);

    ApplyFriendItem* item = new ApplyFriendItem();
    icons_[info->icon_] = info->uid_;
    item->SetInfo(info);
    QListWidgetItem* widget = new QListWidgetItem();
    widget->setSizeHint(item->sizeHint());
    ui->FriendApply->addItem(widget);
    ui->FriendApply->setItemWidget(widget,item);
    unAuthItems_[apply->fromUid_] = item;
    ApplyAddedItem_[apply->fromUid_] = widget;
    connect(item,&ApplyFriendItem::signalAcceptFriendCommit,this,&ApplyFriendPage::acceptApply);
    connect(item,&ApplyFriendItem::signalRefuseApply,this,&ApplyFriendPage::refuseApply);
}

void ApplyFriendPage::slotAuthRsp(std::shared_ptr<AuthRsp> authRsp)
{
    int uid = authRsp->uid_;
    auto it = unAuthItems_.find(uid);
    if(it == unAuthItems_.end())
    {
        // 出现逻辑错误
        qDebug() << "error in " << __FILE__ << ":" << __FUNCTION__;
        return;
    }
    // 将状态置为 "已添加"
    it->second->ShowAddBtn(2);
    unAuthItems_.erase(it);
}

void ApplyFriendPage::acceptApply(ApplyFriendItem* item)
{
    item->ShowAddBtn(1);
    auto applyInfo = item->getApplyInfo();
    // 验证好友的对话框
    AuthFriendApply* auth = new AuthFriendApply();
    auth->setModal(true);
    auth->show();
    auth->setApplyInfo(item->getApplyInfo());

    QJsonObject obj;
    obj["fromuid"] = applyInfo->uid_;
    obj["touid"] = UserManager::GetInstance()->getUid();
    obj["status"] = FRIEND_APPLY::ACCEPTED;
    QJsonDocument doc(obj);
    QByteArray data = doc.toJson(QJsonDocument::Compact);
    emit TcpMsg::GetInstance()->signalSendData(ID_AUTH_FRIEND_REQ,data);
}

void ApplyFriendPage::refuseApply(ApplyFriendItem* item)
{
    item->ShowAddBtn(2);
    qDebug() << "refuse this apply.";

    auto applyInfo = item->getApplyInfo();
    QJsonObject obj;
    obj["fromuid"] = applyInfo->uid_;
    obj["touid"] = UserManager::GetInstance()->getUid();
    obj["status"] = FRIEND_APPLY::REFUSED;
    QJsonDocument doc(obj);
    QByteArray data = doc.toJson(QJsonDocument::Compact);
    emit TcpMsg::GetInstance()->signalSendData(ID_AUTH_FRIEND_REQ,data);
}

void ApplyFriendPage::slotLoadFriendApplyList(QJsonObject obj)
{
    std::vector<std::shared_ptr<ApplyInfo>> apply_list;
    // 解析数据
    bool is_load_more = obj["is_load_more"].toBool();
    int last_friend_apply_id = obj["last_friend_apply_id"].toInt();
    QJsonArray array = obj["apply_friend_list"].toArray();
    for(const QJsonValue& value : array){
        int id = value["id"].toInt();
        int uid = value["uid"].toInt();
        QString name = value["name"].toString();
        QString email = value["email"].toString();
        QString desc = value["desc"].toString();
        QString icon = value["icon"].toString();
        int sex = value["sex"].toInt();
        QString apply_time = value["apply_time"].toString();
        int status = value["status"].toInt();

        std::shared_ptr<ApplyInfo> apply = std::make_shared<ApplyInfo>(id,uid,name,email,desc,icon,sex,apply_time,status);
        apply_list.push_back(apply);
    }
    is_load_more_from_server_ = is_load_more;
    // 将数据加载到缓存
    UserManager::GetInstance()->set_last_friend_apply_id(last_friend_apply_id);
    UserManager::GetInstance()->appendApplyList(apply_list);
    // 将数据存储到本地的数据库，并且更新 last_friend_apply_id
    emit LoadLocalData::GetInstance()->signalStoreFriendApply(apply_list);
    // 将数据渲染到好友申请界面
    this->addFriendApply(apply_list);
    // 如果还可以加载，那么就继续发送请求
    if(is_load_more_from_server_){
        qDebug() << "Send LoadFriendApply again success.";
        emit signalRequestMoreFromServer();
    }else{
        //qDebug() << "Send LoadFriendApply again failed.";
    }
}

void ApplyFriendPage::slotDrawFriendApplyToList(std::vector<std::shared_ptr<ApplyInfo> > apply_list)
{
    //qDebug() << "Local FriendApply: ";
    for(auto apply : apply_list){
        ApplyFriendItem* item = new ApplyFriendItem();
        auto info = std::make_shared<ApplyInfo>(apply->id_,apply->uid_,apply->name_,apply->email_,apply->desc_,apply->icon_,
                                                apply->sex_,apply->apply_time_,apply->status_);
        icons_[info->icon_] = info->uid_;

        //qDebug() << "icon = " << info->icon_ << " uid = " << info->uid_;

        item->SetInfo(info);
        QListWidgetItem* widget = new QListWidgetItem();
        widget->setSizeHint(item->sizeHint());
        ui->FriendApply->addItem(widget);
        ui->FriendApply->setItemWidget(widget,item);
        if(apply->status_ == 0){
            unAuthItems_[apply->uid_] = item;
        }
        ApplyAddedItem_[apply->uid_] = widget;
        connect(item,&ApplyFriendItem::signalAcceptFriendCommit,this,&ApplyFriendPage::acceptApply);
        connect(item,&ApplyFriendItem::signalRefuseApply,this,&ApplyFriendPage::refuseApply);
    }
}

void ApplyFriendPage::slotUserIconChange(QString icon)
{
    qDebug() << "icon change in FriendApplyPage.";

    for (auto iter = icons_.begin(); iter != icons_.end(); iter++) {
        qDebug() << "icon = " << iter.key() << " uid = " << iter.value();
    }

    auto iter = icons_.find(icon);
    if(iter == icons_.end()){
        qDebug() << "[ERROR]: " << __FILE__ << ":" << __LINE__;
        return;
    }
    int friend_id = icons_[icon];
    QListWidgetItem* item = ApplyAddedItem_[friend_id];
    QWidget* wid = ui->FriendApply->itemWidget(item);
    ListItemBase* base = static_cast<ListItemBase*>(wid);
    ApplyFriendItem* applyWidget = dynamic_cast<ApplyFriendItem*>(base);
    applyWidget->setIcon(icon);
}

void ApplyFriendPage::slotOnlineFriendIconFinished(int icon_uid, QString icon)
{
    qDebug() << "icon_uid = " << icon_uid << " icon = " << icon ;
    auto iter = icons_.begin();
    for(iter = icons_.begin(); iter != icons_.end(); iter++){
        qDebug() << "icon = " << iter.key() << " uid = " << iter.value();
        if(iter.value() == icon_uid){
            icons_.erase(iter);
            break;
        }
    }
    if(iter == icons_.end()){
        qDebug() << "ChatUserWidget not load yet.";
        return;
    }
    icons_[icon] = icon_uid;
    qDebug() << "after, icon_uid = "  << icon_uid << " icon = " << icon;
    int friend_id = icons_[icon];
    QListWidgetItem* item = ApplyAddedItem_[friend_id];
    QWidget* wid = ui->FriendApply->itemWidget(item);
    ListItemBase* base = static_cast<ListItemBase*>(wid);
    ApplyFriendItem* applyWidget = dynamic_cast<ApplyFriendItem*>(base);
    applyWidget->setIcon(icon);
}


bool ApplyFriendPage::eventFilter(QObject *watched, QEvent *event)
{
   // qDebug() << "watched object: " << watched->metaObject()->className();  // 打印 watched 对象的类型
    if (watched == ui->FriendApply->viewport()) {
        if (event->type() == QEvent::Enter) {
            ui->FriendApply->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        } else if (event->type() == QEvent::Leave) {
            ui->FriendApply->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        }
    }

    // 检查是否是滚轮事件
    if (watched == ui->FriendApply->viewport() && event->type() == QEvent::Wheel) {
        QWheelEvent *wheelEvent = static_cast<QWheelEvent *>(event);
        int numDegrees = wheelEvent->angleDelta().y() / 8;
        int numSteps = numDegrees / 15;
        ui->FriendApply->verticalScrollBar()->setValue(ui->FriendApply->verticalScrollBar()->value() - numSteps);
        if (ui->FriendApply->verticalScrollBar()) {
            QScrollBar *scrollBar = ui->FriendApply->verticalScrollBar();
            int maxScrollValue = scrollBar->maximum();
            int currentValue = scrollBar->value();
            if (maxScrollValue - currentValue <= 0) {
                if (is_load_more_from_local_) {
                    qDebug() << "loading more FriendApply";
                    emit LoadLocalData::GetInstance()->signalloadFriendApplyList();
                }
            }
        }
        // 截断事件传递
        return true;
    }
    return QWidget::eventFilter(watched, event);
}

void ApplyFriendPage::paintEvent(QPaintEvent *ev)
{
    QStyleOption opt;
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget,&opt,&p,this);
}
