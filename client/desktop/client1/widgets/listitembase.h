#ifndef LISTITEMBASE_H
#define LISTITEMBASE_H

#include "core/global.h"

// 相当于是在聊天列表中的每一条记录的抽象，让 聊天记录 / 聊天联系人 / 搜索联系人的时候显示不同的 类

// 因为需要将子类设置给QListWidgetItem需要的是QWidget对象，因此需要继承QWidget类

class ListItemBase : public QWidget
{
    Q_OBJECT
public:
    explicit ListItemBase(QWidget* parent = nullptr);
    void setItemType(ListItemType itemType);
    ListItemType getItemType();
    ListItemType itemType_;
};

#endif // LISTITEMBASE_H
