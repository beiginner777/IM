#ifndef GROUPTIPITEM_H
#define GROUPTIPITEM_H

#include "core/global.h"
#include "listitembase.h"

namespace Ui {
class GroupTipItem;
}

class GroupTipItem : public ListItemBase
{
    Q_OBJECT

public:
    explicit GroupTipItem(QWidget *parent = nullptr);
    ~GroupTipItem();
    void setTip(QString text);
    QSize sizeHint() const override{
        return QSize(350,30);
    }

private:
    Ui::GroupTipItem *ui;
};

#endif // GROUPTIPITEM_H
