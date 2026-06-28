#include "grouptipitem.h"
#include "ui_grouptipitem.h"

GroupTipItem::GroupTipItem(QWidget *parent) :
    ListItemBase(parent),
    ui(new Ui::GroupTipItem)
{
    ui->setupUi(this);
    this->setItemType(ListItemType::GROUP_TIP_ITEM);
}

GroupTipItem::~GroupTipItem()
{
    delete ui;
}

void GroupTipItem::setTip(QString text)
{
    ui->label->setText(text);
}
