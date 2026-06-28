#ifndef APPLYFRIENDITEM_H
#define APPLYFRIENDITEM_H

#include "global.h"
#include "listitembase.h"
#include "userdata.h"

namespace Ui {
class ApplyFriendItem;
}

class ApplyFriendItem : public ListItemBase
{
    Q_OBJECT
public:
    explicit ApplyFriendItem(QWidget *parent = nullptr);
    ~ApplyFriendItem();

    void setIcon(QString icon);
    void SetInfo(std::shared_ptr<ApplyInfo> apply_info);
    // 是否显示 “AcceptButton”(这个应该是在外部调用这个方法，从而来初始化显示列表)
    void ShowAddBtn(int bshow);
    // 返回自定义的尺寸
    QSize sizeHint() const override {
        return QSize(800, 120);
    }
    int GetUid();
    std::shared_ptr<ApplyInfo> getApplyInfo(){
        return applyInfo_;
    }

private:
    Ui::ApplyFriendItem *ui;
    std::shared_ptr<ApplyInfo> applyInfo_;
    bool added_;

signals:
    void signalAcceptFriendCommit(ApplyFriendItem*);
    void signalRefuseApply(ApplyFriendItem*);

};

#endif // APPLYFRIENDITEM_H
