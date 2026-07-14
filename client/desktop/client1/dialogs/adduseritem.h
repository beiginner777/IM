#ifndef ADDUSERITEM_H
#define ADDUSERITEM_H

#include "core/global.h"
#include "widgets/listitembase.h"

namespace Ui {
class AddUserItem;
}

class AddUserItem : public ListItemBase
{
    Q_OBJECT

public:
    explicit AddUserItem(QWidget *parent = nullptr);
    ~AddUserItem();

private:
    Ui::AddUserItem *ui;
};

#endif // ADDUSERITEM_H
