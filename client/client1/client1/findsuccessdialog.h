#ifndef FINDSUCCESSDIALOG_H
#define FINDSUCCESSDIALOG_H

#include "global.h"
#include "userdata.h"

namespace Ui {
class FindSuccessDialog;
}

class FindSuccessDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FindSuccessDialog(QWidget *parent = nullptr);
    ~FindSuccessDialog();

    void setSearchInfo(std::shared_ptr<SearchInfo> si);

private slots:
    // 点击好友申请的按钮
    void on_friendApplyLabel_clicked();

private:
    Ui::FindSuccessDialog *ui;

    QWidget* parent_;
    std::shared_ptr<SearchInfo> si_;
};

#endif // FINDSUCCESSDIALOG_H
