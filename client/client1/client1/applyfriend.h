#ifndef APPLYFRIEND_H
#define APPLYFRIEND_H

#include "global.h"
#include "userdata.h"
#include "clicklabel.h"

class FriendLabel;

namespace Ui {
class ApplyFriend;
}

class ApplyFriend : public QDialog
{
    Q_OBJECT

public:
    explicit ApplyFriend(QWidget *parent = nullptr);
    ~ApplyFriend();

    // 初始化 提示标签列表
    void initTipLabel();
    // 添加标签到存在列表
    void addTip(ClickLabel* lb,QPoint curPos,QPoint& nextPos,int textWidth,int textHeight);
    // 设置ApplyFriend的界面。在搜索用户的时候，将数据也保存在了这个 ApplyFriend 模块
    void setApplyInfo(std::shared_ptr<SearchInfo> si);

protected:
    bool eventFilter(QObject* obj,QEvent* event) override;

private:
    Ui::ApplyFriend *ui;

    // 重置存在列表
    void resetLabels();

    // 创建的标签
    QMap<QString,ClickLabel*> addLabels_;
    std::vector<QString> addLabelKeys_;
    //用来在输入框显示添加新好友的标签
    QMap<QString, FriendLabel*> friendLabel_;
    std::vector<QString> friendLabelKeys_;

    QPoint labelPoint_;

    // 添加标签到存在列表
    void addLabel(QString name);
    //
    std::vector<QString> tipDate_;
    // 下一个标签的添加位置
    QPoint tipCurPoint_;
    // 搜索到的用户信息
    std::shared_ptr<SearchInfo> si_;

public slots:
    // 在LabelLineEdit按下enter键之后
    void slotLabelEnter();
    // 展示更多标签
    void slotShowMoreTipLabel();
    // 从标签列表移除指定的 标签
    void slotRemoveTipLabel(QString tip);
    // 点击标签列表就可以将标签添加到 标签输入框
    void slotChangeFriendLabelByTip(QString,ClickLbState);
    // 输入框文本变化 显示 不同提示
    void slotLabelTextChange(QString text);
    // 输入框输入完成
    void slotLabelEditFinish();
    // 点击提示框中的标签后，添加标签到 LineEdit
    void slotAddFriendLabelByClickTip(QString text);
    // 处理 确认（confirmButton） 回调
    void slotHandleSure();
    // 处理 取消（cancalButton） 回调
    void slotHandleCancel();
};

#endif // APPLYFRIEND_H
