#ifndef SETTINGPAGE_H
#define SETTINGPAGE_H

#include "core/global.h"

namespace Ui {
class SettingPage;
}

class SettingPage : public QWidget
{
    Q_OBJECT
public:
    explicit SettingPage(QWidget *parent = nullptr);
    ~SettingPage();

public slots:
    void slotRecvNotifyFromChatDialog();

private slots:
    void on_chooseButton_clicked();
    void slotUploadHeadIconIsSuccess(bool flag);
    void on_uploadButton_clicked();

signals:
    void updateHomeIcon(QString);

private:
    Ui::SettingPage *ui;
    QString file_path_;
    QString file_;
};

#endif // SETTINGPAGE_H
