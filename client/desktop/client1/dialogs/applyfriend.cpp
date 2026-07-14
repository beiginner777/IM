#include "applyfriend.h"
#include "ui_applyfriend.h"
#include "widgets/customizeedit.h"
#include "data/usermanager.h"
#include "friendlabel.h"
#include "network/tcpmsg.h"

ApplyFriend::ApplyFriend(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ApplyFriend),
    labelPoint_(2,6)
{
    ui->setupUi(this);

    // 设置qss样式
    QFile styleFile(":/style/applyfriend.qss");
    if(!styleFile.open(QFile::ReadOnly)){
        qDebug("open applyfriend.qss failed!");
    }
    QString style = QLatin1String(styleFile.readAll());
    this->setStyleSheet(style);

    // 隐藏对话框标题栏
    //setWindowFlag(windowFlags() | Qt::FramelessWindowHint);

    // 设置为模态对话框
    this->setModal(true);

    tipCurPoint_ = QPoint(0,0);
    ui->labelLineEdit->move(tipCurPoint_);
    //ui->tipLabelWidget->hide();

    tipDate_ = {"Deepseek","ChatGPT","ClaudeCode","Trac","MCP","Agent","CNN",};
    ui->labelLineEdit->setMinimumWidth(80);
    ui->labelLineEdit->setMaximumWidth(120);
    ui->labelLineEdit->setEchoMode(QLineEdit::Normal);

    ui->applyName->setPlaceholderText(UserManager::GetInstance()->getName());
    ui->bakName->setPlaceholderText(tr("燃烧的胸毛"));
    ui->labelLineEdit->setPlaceholderText(tr("标签"));

    // 初始化标签列表
    initTipLabel();
    // 点击 moreLabel 展示更多的标签
    connect(ui->moreLabel,&ClickOnceLabel::Clicked,this,&ApplyFriend::slotShowMoreTipLabel);
    // 按下回车之后的事件
    connect(ui->labelLineEdit,&CustomizeEdit::returnPressed,this,&ApplyFriend::slotLabelEnter);
    // 当输入框文本变化时
    connect(ui->labelLineEdit,&CustomizeEdit::textChanged,this,&ApplyFriend::slotLabelTextChange);
    // 当输入框完成输入时
    connect(ui->labelLineEdit,&CustomizeEdit::editingFinished,this,&ApplyFriend::slotLabelEditFinish);

    // why......
    // 当点击提示框时，将标签加入到 LabelLineEdit
    //connect(ui->tipLabel,&ClickOnceLabel::Clicked,this,&ApplyFriend::slotAddFriendLabelByClickTip);

    // 隐藏滚动条
    ui->scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    // 安装事件过滤器
    this->installEventFilter(this);

    ui->confirmButton->setState("normal","hover","press");
    ui->cancelButton->setState("normal","hover","press");

    // 处理点击 确认 按钮的信号
    connect(ui->confirmButton,&QPushButton::clicked,this,&ApplyFriend::slotHandleSure);
    // 处理点击 取消 按钮的信号
    connect(ui->cancelButton,&QPushButton::clicked,this,&ApplyFriend::slotHandleCancel);
}

ApplyFriend::~ApplyFriend()
{
    qDebug() << "ApplyFriend destrut.";
    delete ui;
}

void ApplyFriend::initTipLabel()
{
    //qDebug() << "init Tip Label.";

    //qDebug() << tipCurPoint_.x() << "." << tipCurPoint_.y();

    // 标签列表的 标签行数
    int lines = 1;
    for(int i = 0;i < tipDate_.size(); ++i)
    {
        // 因为标签列表中的每个元素是 Clicklabel
        ClickLabel* lb = new ClickLabel(ui->labelList);
        // 设置标签内容
        lb->setText(tipDate_[i]);
        // 初始化
        lb->setState("normal","hover","press","selected_normal",
                     "selected_hover","selected_press");
        lb->setObjectName("newLabel");

        // 一旦点击了 就将标签 添加到/ 删除在 标签列表
        connect(lb,&ClickLabel::Clicked,this,
                &ApplyFriend::slotChangeFriendLabelByTip);

        // 获取这个标签的长度和宽度
        QFontMetrics fontMEtrics(lb->font());
        int textWidth = fontMEtrics.width(lb->text());
        int textHtight = fontMEtrics.height();

        // 标签太长了的话 就需要换行展示
        /*if(tipCurPoint_.x() + textWidth + tipOffset > ui->labelList->width())
        {
            //qDebug() << "换行展示";

            // 修改下一个标签的起始位置
            tipCurPoint_.setX(tipOffset);

            tipCurPoint_.setY(tipCurPoint_.y() + textHtight + tipOffset);
        }*/

        QPoint nextPoint = tipCurPoint_;

        addTip(lb,tipCurPoint_,nextPoint,textWidth,textHtight);

        tipCurPoint_ = nextPoint;

        //qDebug() << "tipCurPoint_.x = " << tipCurPoint_.x();
        //qDebug() << "tipCurPoint_.y = " << tipCurPoint_.y();

    }
}

void ApplyFriend::addTip(ClickLabel* lb, QPoint curPos, QPoint &nextPos, int textWidth, int textHeight)
{
    //qDebug() << "add lable(" << lb->text() << ") to LabelList";
    //qDebug() << "textWidth = " << textWidth;
    qDebug() << "textHeight = " << textHeight;

    //qDebug() << "add key(" << lb->text() << ") to LabelList.";

    //qDebug() << curPos.x() << ":" << curPos.y();

    lb->move(curPos);
    lb->show();

    //qDebug() << "located in pos(" << curPos.x() << "," << curPos.y();
    //qDebug() << "LabelList.width = " << ui->labelList->width();
    //qDebug() << "LabelList.height = " << ui->labelList->height();
    //qDebug() << "LabelListWidget.width = " << ui->LabelListWidget->width();
    //qDebug() << "LabelListWidget.height = " << ui->LabelListWidget->height();
    //qDebug() << "scrollContent.width = " << ui->scrollContent->width();
    //qDebug() << "scrollContent.height = " << ui->scrollContent->height();
    //qDebug() << lb->width() << "," << lb->height();

    addLabels_.insert(lb->text(),lb);
    addLabelKeys_.push_back(lb->text());

    curPos.setX(curPos.x() + textWidth + tipOffset);

    nextPos = curPos;

    if(curPos.x() + textWidth + tipOffset > ui->labelList->width())
    {
        //qDebug() << "换行展示";
        nextPos.setX(0);
        nextPos.setY(curPos.y() + textHeight + tipOffset);
        //qDebug() << nextPos.x() << ":" << nextPos.y();
    }

    if(tipCurPoint_.y() + textHeight > ui->labelList->height())
    {
        ui->labelList->setFixedHeight(tipCurPoint_.y() + textHeight * 2 + tipOffset);
    }

    /*if(tipCurPoint_.y() + ui->LabelListWidget->pos().y() + textHeight > ui->scrollContent->height())
    {
        ui->LabelListWidget->setFixedHeight(tipCurPoint_.y() + 2 * textHeight);
        ui->scrollContent->setFixedHeight(ui->LabelListWidget->pos().y() + tipCurPoint_.y() + textHeight * 2);
    }*/
}

void ApplyFriend::setApplyInfo(std::shared_ptr<SearchInfo> si)
{
    si_ = si;

    // 本人的名字
    QString selfName = UserManager::GetInstance()->getName();
    // 对方的名字
    QString otherName = si->name_;

    ui->applyName->setText(selfName);
    ui->bakName->setText(otherName);
}

bool ApplyFriend::eventFilter(QObject *obj, QEvent *event)
{
    if(obj == ui->scrollArea && event->type() == QEvent::Enter)
    {
        ui->scrollArea->verticalScrollBar()->setHidden(false);
    }
    else if(obj == ui->scrollArea && event->type() == QEvent::Leave)
    {
        ui->scrollArea->verticalScrollBar()->setHidden(true);
    }
    return QDialog::eventFilter(obj,event);
}

void ApplyFriend::resetLabels()
{
    if(friendLabel_.isEmpty())
    {
        ui->labelLineEdit->move(labelPoint_);
        //ui->tipLabelWidget->move(labelPoint_.x(),labelPoint_.y() + 35);
        return;
    }

    int maxWidth = ui->gridWidet->width();
    int height = 0;

    for(auto it = friendLabel_.begin(); it != friendLabel_.end(); ++it)
    {
        // todo... 添加宽度统计
        if(labelPoint_.x() + it.value()->width() > maxWidth)
        {
            labelPoint_.setX(2);
            labelPoint_.setY(labelPoint_.y() + it.value()->height() + 6);
        }

        it.value()->move(labelPoint_);
        it.value()->show();

        labelPoint_.setX(labelPoint_.x() + it.value()->width() + 2);

        height = it.value()->height();
    }

    if(labelPoint_.x() + MIN_APPLY_LABEL_ED_LEN > ui->gridWidet->width())
    {
        ui->labelLineEdit->move(2,labelPoint_.y() + height + 6);
        //ui->tipLabelWidget->move(2,labelPoint_.y() + height + 6 + 35);
    }
    else
    {
        ui->labelLineEdit->move(labelPoint_);
        //ui->tipLabelWidget->move(labelPoint_.x(),labelPoint_.y() + 35);
    }
}

// 添加到 存在列表
void ApplyFriend::addLabel(QString name)
{
    // 已经添加过了
    if(friendLabel_.find(name) != friendLabel_.end())
    {
        qDebug() << "Label existed";
        ui->labelLineEdit->clear();
        // ui->labelLineEdit->setFocus();
        return;
    }

    //qDebug() << "add key(" << name << ")existLabelList.";

    FriendLabel* lb = new FriendLabel(ui->gridWidet);
    lb->setText(name);
    lb->setObjectName("FriendLabel");
    lb->setProperty("state","selected_normal");
    repolish(lb);
    update();

    int maxWidth = ui->gridWidet->width();
    // todo ... 添加宽度统计

    //qDebug() << "-------- labelPoint_.x() = " << labelPoint_.x();
    //qDebug() << "-------- lb_width() = " << lb->width();

    // 添加到存在标签列表
    if(labelPoint_.x() + tipOffset + lb->width() > maxWidth)
    {
        labelPoint_.setX(2);
        labelPoint_.setY(labelPoint_.y() + lb->height() + 6);
    }

    lb->move(labelPoint_);
    lb->show();

    // 记录在存在列表中
    friendLabel_[name] = lb;
    friendLabelKeys_.push_back(name);
    // 一旦点击了存在列表的元素，那么就直接将其在存在列表删除
    connect(lb,&FriendLabel::signalClose,this,&ApplyFriend::slotRemoveTipLabel);

    labelPoint_.setX(labelPoint_.x() + lb->width() + 2);

    if(labelPoint_.x() + MIN_APPLY_LABEL_ED_LEN > ui->gridWidet->width())
    {
        ui->labelLineEdit->move(2,labelPoint_.y() + lb->height() + 2);
         //ui->labelLineEdit->move(2,labelPoint_.y() + lb->height() + 2 + 35);
    }
    else
    {
        ui->labelLineEdit->move(labelPoint_);
        //ui->labelLineEdit->move(labelPoint_.x(),labelPoint_.y() + 35);
    }

    // why ....
    /*if(ui->gridWidet->height() < labelPoint_.y() + lb->height() + 2)
    {
        ui->gridWidet->setFixedHeight(labelPoint_.y() + lb->height() * 2 + 2);
    }*/

    // 如果当前存在列表加上 LabelLineEdit 的高度的话，那么就重新设置一下 gridWidget 的高度
    if(labelPoint_.y() + lb->height() + 2 + ui->labelLineEdit->height() > ui->gridWidet->height())
    {
        ui->gridWidet->setFixedHeight(labelPoint_.y() + lb->height() + 2 + ui->labelLineEdit->height());
    }

    //qDebug() << "LabelLineEdit.x = " << ui->labelLineEdit->pos().x();
    //qDebug() << "LabelLineEdit.y = " << ui->labelLineEdit->pos().y();

    //qDebug() << "labelPoint_.x = " << labelPoint_.x();
    //qDebug() << "labelPoint_.y = " << labelPoint_.y();
}

void ApplyFriend::slotLabelEnter()
{
    //qDebug() << "click Key Enter.";

    if(ui->labelLineEdit->text().isEmpty()){
        //qDebug() << "labelLineEdit is empty.";
        return;
    }

    auto text = ui->labelLineEdit->text();

    ui->labelLineEdit->clear();
    //ui->tipLabelWidget->hide();

    auto find_it = std::find(friendLabelKeys_.begin(), friendLabelKeys_.end(), text);
    // 已经存在
    if (find_it != friendLabelKeys_.end()) {
        qDebug() << "key(" << text << ") is existed.";
        return;
    }
    // 不存在那么就直接加入到存在列表中
    addLabel(text);

    //判断标签列表是否有该标签
    auto find_add = addLabels_.find(text);
    // 标签列表存在，那么就直接设置为选中状态
    if (find_add != addLabels_.end()) {
        //qDebug() << "key(" << text << " exists in LabelList.";
        find_add.value()->setCurState(ClickLbState::select);
        return;
    }
    // 不存在就在标签列表中添加, 并设置绿色选中
    auto* lb = new ClickLabel(ui->labelList);
    lb->setText(text);
    lb->setState("normal", "hover", "press", "selected_normal",
        "selected_hover", "selected_pressed");
    lb->setObjectName("newLabel");
    lb->setCurState(ClickLbState::select);

    connect(lb, &ClickLabel::Clicked, this, &ApplyFriend::slotChangeFriendLabelByTip);
    //qDebug() << "ui->lb_list->width() is " << ui->labelLineEdit->width();
    //qDebug() << "_tip_cur_point.x() is " << tipCurPoint_.x();

    QFontMetrics fontMetrics(lb->font()); // 获取QLabel控件的字体信息
    int textWidth = fontMetrics.width(lb->text()); // 获取文本的宽度
    int textHeight = fontMetrics.height(); // 获取文本的高度
    //qDebug() << "textWidth is " << textWidth;

    //qDebug() << " ------------------- ";
   //qDebug() << tipCurPoint_.x() << "+" << textWidth << "+" << tipOffset << "  " << ui->labelList->width();
    if (tipCurPoint_.x() + textWidth + tipOffset > ui->labelList->width()) {

        tipCurPoint_.setX(5);
        tipCurPoint_.setY(tipCurPoint_.y() + textHeight + tipOffset);
        qDebug() << "换行展示";

        qDebug() << tipCurPoint_.x() << ":" << tipCurPoint_.y();

    }
    qDebug() << "-------------------";

    auto next_point = tipCurPoint_;

    addTip(lb, tipCurPoint_, next_point, textWidth, textHeight);

    tipCurPoint_ = next_point;

    qDebug() << tipCurPoint_.x() << ":" << tipCurPoint_.y();

    ui->labelLineEdit->clear();
    // why ...
    //int diff_height = tipCurPoint_.y() + textHeight + tipOffset - ui->labelLineEdit->height();
    //ui->labelLineEdit->setFixedHeight(tipCurPoint_.y() + textHeight + tipOffset);
    // ui->scrollContent->setFixedHeight(ui->scrollContent->height() + diff_height);
}

void ApplyFriend::slotShowMoreTipLabel()
{
    qDebug() << "click MoreLabel.";

    ui->moreLabel->hide();

    tipCurPoint_ = QPoint(5,5);

    auto nextPoint = tipCurPoint_;

    int textWidth = 0;
    int textHeight = 0;

    // 重新排列已有的标签
    for (auto addLabel : addLabelKeys_)
    {
        // 获取ClickLabel
        ClickLabel* lb = addLabels_[addLabel];

        QFontMetrics fontMetrics(lb->font());

        textWidth = fontMetrics.width(lb->text());
        textHeight = fontMetrics.height();

        if(tipCurPoint_.x() + textWidth + tipOffset > ui->labelList->width())
        {
            tipCurPoint_.setX(tipOffset);
            tipCurPoint_.setY(tipCurPoint_.y() + textHeight + 15);
            //qDebug() << "换行展示";
        }
        lb->move(tipCurPoint_);

        nextPoint.setX(lb->pos().x() + textWidth + 15);
        nextPoint.setY(tipCurPoint_.y());

        tipCurPoint_ = nextPoint;
    }

    /*
    // 添加未添加的标签
    for(int i = 0;i < tipDate_.size(); ++i)
    {
        auto it = addLabels_.find(tipDate_[i]);
        if(it != addLabels_.end()){
            continue;
        }
        ClickLabel* lb = new ClickLabel(ui->labelList);
        lb->setText(tipDate_[i]);
        lb->setState("normal","hover","press","selected_normal","selected_hover","selected_press");
        lb->setObjectName("newLabel");


        connect(lb,&ClickLabel::Clicked,this,&ApplyFriend::slotChangeFriendLabelByTip);

        QFontMetrics fontMetrics(lb->font()); // 获取QLabel控件的字体信息
        textWidth = fontMetrics.width(lb->text()); // 获取文本的宽度
        textHeight = fontMetrics.height(); // 获取文本的高度

        if (tipCurPoint_.x() + textWidth + tipOffset > ui->labelList->width()) {

            tipCurPoint_.setX(tipOffset);
            tipCurPoint_.setY(tipCurPoint_.y() + textHeight + 15);

        }

        nextPoint = tipCurPoint_;
        addTip(lb, tipCurPoint_, nextPoint, textWidth, textHeight);
        tipCurPoint_ = nextPoint;
    }*/

    // why ...

    // 不可见区间的高度
    //int diffHeight = nextPoint.y() + textHeight + tipOffset - ui->labelList->height();
    // 标签列表的高度
    //ui->labelList->setFixedHeight(nextPoint.y() + textHeight + 5);
    // 总高度
    //ui->scrollContent->setFixedHeight(ui->scrollContent->height() + diffHeight);
}

// 点击 存在列表 删除该标签
void ApplyFriend::slotRemoveTipLabel(QString tip)
{
    //qDebug() << "receive closeSignal to close Label(" << tip << ").";

    //
    labelPoint_.setX(2);
    labelPoint_.setY(6);

    // 找到要删除的标签在存在列表的 迭代器
    auto it = friendLabel_.find(tip);
    if(it == friendLabel_.end())
    {
        qDebug() << "error in " << __FILE__ << ":" << __FUNCTION__ << ": failed to delete label from ExistList.";
        return;
    }
    else
    {
        // 析构 FriendLabel 对象，防止内存泄漏
        delete it.value();
        friendLabel_.erase(it);
        // 同时要删除FriendLabelKeys中的记录
        auto it1 = std::find(friendLabelKeys_.begin(),friendLabelKeys_.end(),tip);
        if(it1 == friendLabelKeys_.end())
        {
            qDebug() << "error in " << __FILE__ << ":" << __FUNCTION__;
            return;
        }
        else {
            friendLabelKeys_.erase(it1);
            auto it2 = addLabels_.find(tip);
            if(it2 == addLabels_.end())
            {
                qDebug() << "error in " << __FILE__ << ":" << __FUNCTION__;
                return;
            }
            else
            {
                // 将标签列表的状态置为 normal
                it2.value()->resetNormalState();
                // 重新排列存在列表
                resetLabels();
            }
        }
    }
    //qDebug() << "Remove In ExistList::LabelLineEdit.x = " << ui->labelLineEdit->pos().x();
    //qDebug() << "Remove In ExistList::LabelLineEdit.y = " << ui->labelLineEdit->pos().y();
}

void ApplyFriend::slotChangeFriendLabelByTip(QString text, ClickLbState state)
{
    auto it = addLabels_.find(text);
    if(it == addLabels_.end())
    {
        return;
    }
    if(state == ClickLbState::select)
    {
        // 添加到 存在列表中
        addLabel(text);
        //
        return;
    }
    if(state == ClickLbState::normal)
    {
        // 如果是已经选择的的话,就需要在存在列表删除这个标签
        slotRemoveTipLabel(text);
        qDebug() << "remove Label(" << text;
    }
}

void ApplyFriend::slotLabelTextChange(QString text)
{
    if(text.isEmpty())
    {
        return;
    }

    auto it = addLabels_.find(text);
    if(it != addLabels_.end())
    {
        //ui->tipLabel->setText(text);
    }
    else
    {
        QString tip = "添加标签";
        QString t = tip + text;
        //ui->tipLabel->setText(t);
    }

    // ui->tipLabel->adjustSize();
    //ui->tipLabelWidget->show();
}

void ApplyFriend::slotLabelEditFinish()
{
    //ui->tipLabelWidget->hide();
}

void ApplyFriend::slotAddFriendLabelByClickTip(QString text)
{
    qDebug() << "SlotAddfriendLabel was emited  by click Tip.";

    QString t = "添加标签";
    int index = text.indexOf("添加标签");
    if(index != -1)
    {
        // why ...
        text = text.mid(index + t.length());
    }

    // 在标签列表找
    auto it = addLabels_.find(text);
    if(it == addLabels_.end())
    {
        // qDebug() << "initList not exist Label(" << text <<").";

        // 没有的话，就把它加入初始列表，方便下次使用
        // tipDate_.push_back(text);

        // 添加到 存在列表
        addLabel(text);
        // 同时添加到标签列表
        ClickLabel* lb = new ClickLabel(ui->labelList);
        lb->setText(text);
        lb->setState("normal","hover","press","selected_normal","selected_hover",
                     "selected_press");
        lb->setObjectName("newLabel");
        lb->setCurState(ClickLbState::select);
        // qDebug() << "set text: " << text  << " fot NewLabel";


        // 在标签列表点击之后，可以在存在列表显示
        connect(lb,&ClickLabel::Clicked,this,&ApplyFriend::slotChangeFriendLabelByTip);

        QFontMetrics fontMetrics(lb->font()); // 获取QLabel控件的字体信息
        int textWidth = fontMetrics.width(lb->text()); // 获取文本的宽度
        int textHeight = fontMetrics.height(); // 获取文本的高度

        if (tipCurPoint_.x() + textWidth + tipOffset > ui->labelList->width()) {

            tipCurPoint_.setX(tipOffset);
            tipCurPoint_.setY(tipCurPoint_.y() + textHeight + 15);
            //qDebug() << "换行展示";
        }

        auto nextPoint = tipCurPoint_;

        addTip(lb, tipCurPoint_, nextPoint, textWidth, textHeight);

        tipCurPoint_ = nextPoint;

        // 不可见区间的高度
        //int diffHeight = nextPoint.y() + textHeight + tipOffset - ui->labelList->height();
        // 标签列表的高度
        //ui->labelList->setFixedHeight(nextPoint.y() + textHeight + 5);
        // 总高度
        //ui->scrollContent->setFixedHeight(ui->scrollContent->height() + diffHeight);
    }
    else
    {
        // 存在的话，还需要在 已有的列表里面，判断是否存在。存在就返回。不存在再添加到存在列表
        // 判断存在的标签栏是否有该标签
        auto it1 = friendLabel_.find(text);
        if(it1 != friendLabel_.end())
        {
            qDebug() << "Label " << it1.value()->getText() << "existed.";
        }
        else
        {
            // 添加到 存在列表
            addLabel(text);
        }
    }
    //qDebug() << "add label to existList::LabelLineEdit.x = " << ui->labelLineEdit->pos().x();
    //qDebug() << "add label to existList::LabelLineEdit.y = " << ui->labelLineEdit->pos().y();
}

void ApplyFriend::slotHandleSure()
{
    // todo.... 发送好友申请
    qDebug() << "send friend apply requeset.";

    QJsonObject obj;
    obj["fromuid"] = UserManager::GetInstance()->getUid();
    obj["touid"] = si_->uid_;

    QString applyname = ui->applyName->text();
    if(applyname == ""){
        applyname = ui->applyName->placeholderText();
    }
    obj["applyname"] = applyname;

    QString bakname = ui->bakName->text();
    if(bakname == ""){
        bakname = ui->applyName->placeholderText();
    }
    obj["bakname"] = bakname;
    obj["status"] = 0;

    // 因为其余的信息在redis或者mysql中可以查到，因此不需要发送
    QJsonDocument doc(obj);
    QByteArray data = doc.toJson(QJsonDocument::Compact);

    // TcpMgr发送数据给ChatServer
    emit TcpMsg::GetInstance()->signalSendData(ID_APPLY_FRIEND_REQ,data);

    this->hide();
    deleteLater();
}

void ApplyFriend::slotHandleCancel()
{
    // 意味着 ApplyFriend这个类在每次使用的时候，都是new出来的。
    qDebug() << "click confirmButton.";
    this->hide();
    // 删除自身对象
    deleteLater();
}
