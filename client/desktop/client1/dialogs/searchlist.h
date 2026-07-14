#ifndef SEARCHLIST_H
#define SEARCHLIST_H

#include "core/global.h"
#include "loadingdialog.h"
#include "core/userdata.h"
#include "findsuccessdialog.h"
#include "widgets/customizeedit.h"

class SearchList : public QListWidget
{
public:
    SearchList(QWidget* parent = nullptr);
    void closeDialog();
    void setStateEdit(QWidget* w);
    void setSearchLineEdit(CustomizeEdit* edit);

protected:
    bool eventFilter(QObject* obj,QEvent* event) override;

private:
    void waitPending(bool pending = true);
    void addTipItem();

    bool send_pending_;
    std::shared_ptr<QDialog> findDialog_;
    // 因为需要需要searchLineEdit的text所以需要在ChatDialog给这个类设置
    CustomizeEdit* searchEdit_;
    LoadingDialog* loadingDialog_;

private slots:
    // TcpManager返回的搜索结果
    void slotSearchUser(std::shared_ptr<SearchInfo> si);
    void slotItemClick(QListWidgetItem* item);
};

#endif // SEARCHLIST_H
