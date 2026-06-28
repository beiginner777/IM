#ifndef CHATINTERFACE_H
#define CHATINTERFACE_H

#include <QWidget>
#include "userdata.h"
#include "chatthreaddata.h"
#include "chatitembase.h"

namespace Ui {
class ChatInterface;
}

class ChatInterface : public QWidget
{
    Q_OBJECT

public:
    explicit ChatInterface(QWidget *parent = nullptr);
    ~ChatInterface();

    void setUserInfo(std::shared_ptr<FriendInfo> userinfo);
    void SetIsLoading(bool is_loading);

    void appendChatMsg(std::shared_ptr<ChatDataBase> msg); // 追加到后面
    void insertChatMsg(std::shared_ptr<ChatDataBase> msg); // 插入到前面
    void insertChatMsgs(std::vector<std::shared_ptr<ChatDataBase>> msgs);

    void setCurChatThreadData(std::shared_ptr<ChatThreadData> cur_thread_data);
    std::shared_ptr<ChatThreadData> GetCurChatThreadData() { return cur_thread_data_; }
    void UpdateChatInterface(std::shared_ptr<ChatDataBase> data);

public slots:
    void onSelectAndInsertFiles();

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void on_sendButton_clicked();
    void click_pause_btn(QString unique_name, TRANSFER_STATE transfer_state);
    void click_resume_btn(QString unique_name, TRANSFER_STATE transfer_state);

private:
    void createEmojiPlanel(); // 创建emoji面板
    void repositionEmojiPanel(); // 调整emoji面板的位置

    QWidget *emojiPanel;

    Ui::ChatInterface *ui;
    // 对方的信息
    std::shared_ptr<FriendInfo> peerInfo_;
    // 当前选择的ChatThreadData
    std::shared_ptr<ChatThreadData> cur_thread_data_;
    // 当前聊天用户未回复的item(unique_id:item)
    QMap<QString,ChatItemBase*> unrsp_items_; // to do ... delete
    // 当前聊天用户已经回复的item(MsgId:Item)
    QMap<int,ChatItemBase*> base_items_;
};


#endif // CHATINTERFACE_H
