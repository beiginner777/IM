#ifndef OFFLINEDIALOG_H
#define OFFLINEDIALOG_H

#include <QDialog>

namespace Ui {
class offlineDialog;
}

class OfflineDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OfflineDialog(QWidget *parent = nullptr);
    ~OfflineDialog();
private:
    Ui::offlineDialog *ui;
signals:
    void click();
private slots:
    void on_pushButton_clicked();
};

#endif // OFFLINEDIALOG_H
