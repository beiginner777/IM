#ifndef CHATDIALOG_H
#define CHATDIALOG_H

#include <QDialog>
#include <vector>
#include "core/global.h"
#include "chatuserwidget.h"
#include "widgets/statewidget.h"
#include "core/userdata.h"

class ContactUserWidget;

namespace Ui {
class ChatDialog;
}

class ChatDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ChatDialog(QWidget *parent = nullptr);
    ~ChatDialog();
    void addStateWidget(StateWidget* w); // 添加侧边栏按钮
    void initConnUserList(); // 登陆时加载好友列表
    void initChatUserList(); // 登陆时加载聊天用户列表
    void initFriendApply(); // 登陆时加载好友申请列表

public slots:
    void slotMessageLabel();
    void slotUserLabel();
    void slotSettigLabel();
    void slotRecvFriendApply(std::shared_ptr<AddFriendApply> apply);
    void slotRecvNewTextMsg(std::vector<std::shared_ptr<ChatDataBase>> chat_datas);
    void slotChangeChatUser(int friend_id);
    void slotChangeContactUser(ContactUserWidget* item);
    void slotSwitchFromCotactToChat(int uid);
    void slotCreatePrivateChat(QJsonObject obj);
    void slotNotifyChatPageMsgStatus(int thread_id, std::vector<std::shared_ptr<TextChatData>> chat_datas);
    void slotUpdateHomeIcon(QString icon);
    void slotSelfIconDownloadFinished(bool flag);
    void slotAddChatUserWidget(std::shared_ptr<UserInfo> user_info);
    void slotNotifyChatDialogSwitchFromConnToChat(int uid);
    void slotNotifyChatPageImgStatus(int thread_id, std::shared_ptr<ImageDataBase> msg);
    void slotRecvNewImgMsg(std::shared_ptr<ImageDataBase> image);
    void slotSpecifiedChatUserLoadFinished(std::shared_ptr<ChatThreadInfo> info);
    void slotLoadChatMsgs(QJsonObject obj);
    void slotNotifyLoadMoreMsgFromLocal(int thread_id,std::vector<std::shared_ptr<ChatDataBase>> datas);

signals:
    void signalNotifyConnList();
    void signalNotifySettingPage();

protected:
    bool eventFilter(QObject* obj,QEvent* event) override;

private:
    void ShowSearch(bool bsearch);
    void requestServerToAddConnUser(); // 向服务器请求加载数据到好友列表
    void requestServerToAddChatUser(); // 向服务器请求加载数据到聊天联系人列表
    void requestServerToAddFriendApply(); // 向服务器请求加载数据到好友申请列表
    // 清除非当前statewidget的状态
    void clearStateWidget(StateWidget* w);
    // 判断鼠标点击的位置是否是搜索框内
    void handleMousePressEvent(QMouseEvent* event);
    //
    // void updateChatMsg(std::vector<std::shared_ptr<TextChatData>> msgdata);
    Ui::ChatDialog *ui;
    // 控制侧边栏
    ChatUIMode mode_;
    // 控制用户列表
    ChatUIMode state_;
    // 侧边栏的各个状态(方便更改状态)
    std::vector<StateWidget*> widgets_;
    // 当前正在聊天的用户
    int curUid_;
    // 当前显示在FriendInfoInterface界面显示的好友uid
    int friendUid_;
    // 当前的ChatThreadID
    int cur_thread_id_;
};

#endif // CHATDIALOG_H
