#include "offlinedialog.h"
#include "ui_offlinedialog.h"

OfflineDialog::OfflineDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::offlineDialog)
{
    ui->setupUi(this);
   this->setModal(true);
}

OfflineDialog::~OfflineDialog()
{
    delete ui;
}

void OfflineDialog::on_pushButton_clicked()
{
    emit click();
}
