#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include "config.h"
#include "console.h"

namespace Ui {
    class Dialog;
}

class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = 0);
    ~Dialog();
    bool loadConfig(Config * config);
    bool setConfig();
    Config * config;
    Console console;
    void setFusInvBinPath(QString & path);

private slots:
    void on_pushButton_clicked();

    void on_pushButtonCancel_clicked();

    void on_toolButtonSelectCert_clicked();

    void on_pushButton_2_clicked();

    void on_pushButtonTest_clicked();

private:
    Ui::Dialog *ui;
};

#endif // DIALOG_H
