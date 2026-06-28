#include "listitembase.h"

ListItemBase::ListItemBase(QWidget *parent) : QWidget(nullptr)
{

}

void ListItemBase::setItemType(ListItemType itemType)
{
    itemType_ = itemType;
}

ListItemType ListItemBase::getItemType()
{
    return itemType_;
}
