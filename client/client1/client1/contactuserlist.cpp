#include "contactuserlist.h"
#include "grouptipitem.h"
#include "usermanager.h"
#include "chatuserwidget.h"
#include "loadlocaldata.h"
#include "tcpmsg.h"
#include "fileuploadmsg.h"

ContactUserList::ContactUserList(QWidget *parent)
    : QListWidget(parent)
{
    Q_UNUSED(parent);

    this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->viewport()->installEventFilter(this);

    // 初始化好友列表
    initContactUserList();

    connect(this,&QListWidget::itemClicked,this,&ContactUserList::slotItemClick);
    connect(TcpMsg::GetInstance().get(),&TcpMsg::signalLoadConnList,this,&ContactUserList::slotLoadConnList);
    connect(TcpMsg::GetInstance().get(),&TcpMsg::signalAddNewConnUserWidget,this,&ContactUserList::slotAddNewConnUserWidget);
    connect(FileUploadMsg::GetInstance().get(),&FileUploadMsg::signalOtherIconDownloadFinished,this,&ContactUserList::slotFriendIconChange);

    connect(FileUploadMsg::GetInstance().get(),&FileUploadMsg::signalOnlineFriendIconFinished,this,&ContactUserList::slotOnlineFriendIconFinished);
}

void ContactUserList::showRedPoint(bool bshow)
{
    if(bshow){
        addFriendItem_->showRedPoint(true);
    }
    else{
        addFriendItem_->showRedPoint(false);
    }
}

bool ContactUserList::eventFilter(QObject *watched, QEvent *event)
{
    if(watched == this->viewport())
    {
        if(event->type() == QEvent::Enter)
        {
            this->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        }
        else if(event->type() == QEvent::Leave)
        {
            this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        }
        else if(event->type() == QEvent::Wheel)
        {
            QWheelEvent* wheelEvent = static_cast<QWheelEvent*>(event);
            // 获取鼠标的滚动角度 （因为angleDelta().y()的单位是 1/8度，所以要 除以8）
            int numDegrees = wheelEvent->angleDelta().y() / 8;
            int numSteps = numDegrees / 15;
            // 设置滚动幅度
            this->verticalScrollBar()->setValue(this->verticalScrollBar()->value() - numSteps);
            // 检查是否滚顶到了底部
            QScrollBar* scrollBar = this->verticalScrollBar();
            int maxScrollValue = scrollBar->maximum();
            int currentValue = scrollBar->minimum();

            if(maxScrollValue - currentValue <= 0)
            {
                // 表明加载到了底部，那么就加载新的联系人
                qDebug() << "Loading more contact user.";
                // emit signalLoadingMoreMessage();
            }
            return true;
        }
    }
    return QListWidget::eventFilter(watched,event);
}

void ContactUserList::slotItemClick(QListWidgetItem *item)
{
    // 获取item的类型
    QWidget* widget = this->itemWidget(item);
    // 强制转换为基类
    ListItemBase* base = static_cast<ListItemBase*>(widget);
    ListItemType type = base->getItemType();

    //
    qDebug() << "item was clicked(" << type << ").";


    if(type == ListItemType::INVALID_ITEM || type == ListItemType::GROUP_TIP_ITEM)
    {
        qDebug() << "type " << type << " is unclicked.";
        return;
    }
    if(type == ListItemType::CONTACT_USER_ITEM)
    {
        // 创建对话框，提示用户
        qDebug()<< "contact user item clicked ";

        // stackWidget 跳转到对应的好友聊天界面
        auto t = dynamic_cast<ContactUserWidget*>(base); // 父类转换为基类
        if(t != nullptr){
            qDebug() << "----------" << t->getUserInfo()->uid_;
        }
        emit signalSwitchToFriendChatInterface(t);
        return;
    }
    if(type == ListItemType::APPLY_FRIEND_ITEM)
    {
        emit signalSwitchToFriendApplyInterface();
    }
}

void ContactUserList::DrawUserToList()
{
    auto friendList = UserManager::GetInstance()->getFriendList();
    for(int i = 0; i < friendList.size(); ++i){
        std::shared_ptr<FriendInfo> f = friendList[i];
        QListWidgetItem* item = new QListWidgetItem();
        ContactUserWidget* widget = new ContactUserWidget();
        icons_[f->icon_] = f->uid_;
        widget->SetInfo(f);
        item->setSizeHint(widget->sizeHint());
        addItem(item);
        setItemWidget(item,widget);
        ConnAddedItem_.insert(f->uid_,item);
    }
}

void ContactUserList::slotLoadConnList(QJsonObject obj)
{
    // 将数据保存在UserManager，并且通知 ConnUserList 去渲染这些数据
    std::vector<std::shared_ptr<FriendInfo>> friends;

    int last_friend_id = obj["max_friend_id"].toInt();
    UserManager::GetInstance()->set_last_friend_id(last_friend_id);

    auto friendArrary = obj["friends"].toArray();
    for(const QJsonValue& fr : friendArrary){
        int id = fr["id"].toInt();
        int uid = fr["uid"].toInt();
        QString name = fr["name"].toString();
        QString nick = fr["nick"].toString();
        QString desc = fr["desc"].toString();
        int sex = fr["sex"].toInt();
        QString email = fr["email"].toString();
        QString icon = fr["icon"].toString();

        std::shared_ptr<FriendInfo> friend_info = std::make_shared<FriendInfo>(
                    id,uid,name,nick,icon,sex,desc,email);
        friends.push_back(friend_info);
    }
    // 添加到本地缓存，统一管理
    UserManager::GetInstance()->appendFriendList(friends);  
    // 将数据渲染到列表
    DrawUserToList();
    if(friends.size() != 0){
        // 将数据存储在本地的数据库中，并且需要更新last_friend_id
        emit LoadLocalData::GetInstance()->signalStoreFriends(friends);
    }
}

void ContactUserList::slotAddNewConnUserWidget(std::shared_ptr<FriendInfo> friend_info)
{
    QListWidgetItem* item = new QListWidgetItem();
    ContactUserWidget* widget = new ContactUserWidget();
    icons_[friend_info->icon_] = friend_info->uid_;
    widget->SetInfo(friend_info);
    item->setSizeHint(widget->sizeHint());
    addItem(item);
    setItemWidget(item,widget);
    ConnAddedItem_.insert(friend_info->uid_,item);
}

void ContactUserList::slotFriendIconChange(QString icon)
{
    auto iter = icons_.find(icon);
    if(iter == icons_.end()){
        qDebug() << "[ERROR]: " << __FILE__ << ":" << __LINE__;
        return;
    }
    int friend_id = icons_[icon];
    UserManager::GetInstance()->changeFriendIconByuid(friend_id,icon);
    QListWidgetItem* item = ConnAddedItem_[friend_id];
    QWidget* wid = this->itemWidget(item);
    ListItemBase* base = static_cast<ListItemBase*>(wid);
    ContactUserWidget* contactWidget = dynamic_cast<ContactUserWidget*>(base);
    contactWidget->setIcon(icon);
}

void ContactUserList::slotOnlineFriendIconFinished(int icon_uid, QString icon)
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
    int friend_id = icons_[icon];
    QListWidgetItem* item = ConnAddedItem_[friend_id];
    QWidget* wid = this->itemWidget(item);
    ListItemBase* base = static_cast<ListItemBase*>(wid);
    ContactUserWidget* contactWidget = dynamic_cast<ContactUserWidget*>(base);
    contactWidget->setIcon(icon);
}

void ContactUserList::initContactUserList()
{
    QListWidgetItem* groupItemWidget1 = new QListWidgetItem();
    GroupTipItem* groupItem1 = new GroupTipItem(this);
    groupItem1->setItemType(ListItemType::INVALID_ITEM);
    groupItemWidget1->setSizeHint(groupItem1->sizeHint());
    this->addItem(groupItemWidget1);
    this->setItemWidget(groupItemWidget1,groupItem1);

    QListWidgetItem* addFriendWidget = new QListWidgetItem();
    addFriendItem_ = new ContactUserWidget();
    addFriendItem_->setObjectName("ADD_USER_TIP_ITEM");
    addFriendItem_->SetInfo(0,tr("新的朋友"),":/res/add_friend.png");
    addFriendItem_->setItemType(ListItemType::APPLY_FRIEND_ITEM);
    addFriendWidget->setSizeHint(addFriendItem_->sizeHint());
    this->addItem(addFriendWidget);
    this->setItemWidget(addFriendWidget,addFriendItem_);

    // 设置默认的选中条目
    this->setCurrentItem(addFriendWidget);

    QListWidgetItem* groupItemWidget2 = new QListWidgetItem();
    GroupTipItem* groupItem2 = new GroupTipItem();
    groupItem2->setItemType(ListItemType::INVALID_ITEM);
    groupItem2->setTip(tr("联系人"));
    groupItemWidget2->setSizeHint(groupItem2->sizeHint());
    this->addItem(groupItemWidget2);
    this->setItemWidget(groupItemWidget2,groupItem2);
}


































