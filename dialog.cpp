#include <QFileDialog>
#include "dialog.h"
#include "ui_dialog.h"
#include "config.h"

Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);
}

Dialog::~Dialog()
{
    delete ui;
}

bool Dialog::loadConfig(Config * config) {
    this->config = config;

    ui->lineEditServer->setText(config->get("server"));
    ui->lineEditUser->setText(config->get("user"));
    ui->lineEditPassword->setText(config->get("password"));
    ui->lineEditProxy->setText(config->get("proxy"));
    ui->lineEditTag->setText(config->get("tag"));
    if (config->get("no-ssl-check") == "1") {
        ui->checkBoxNoSSLCheck->setChecked(true);
    }
    ui->labelCaCertFile->setText(config->get("ca-cert-file"));
    return true;
}

bool Dialog::setConfig() {

    config->set("server", ui->lineEditServer->text());
    config->set("user", ui->lineEditUser->text());
    config->set("password", ui->lineEditPassword->text());
    config->set("proxy", ui->lineEditProxy->text());
    config->set("tag", ui->lineEditTag->text());
    config->set("no-ssl-check", ui->checkBoxNoSSLCheck->isChecked()?"1":"0");
    config->save();
    return true;
}

void Dialog::on_pushButton_clicked()
{
    setConfig();
    QApplication::exit(0);
}

void Dialog::on_pushButtonCancel_clicked()
{
    QApplication::exit(0);
}

void Dialog::on_toolButtonSelectCert_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"),
                                                     "/home");

    ui->labelCaCertFile->setText(fileName);
    config->set("ca-cert-file", fileName);

}

void Dialog::on_pushButton_2_clicked()
{

}

void Dialog::on_pushButtonTest_clicked()
{

    //console.show();
    this->setConfig();
    console.start();
    //terminal.show();
    //terminal.show();
    //terminal.start();
}

void Dialog::setFusInvBinPath(QString & path) {
    console.fusInvBinPath = path;

}
