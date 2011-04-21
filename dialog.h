#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include "config.h"

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

private slots:
    void on_pushButton_clicked();

    void on_pushButtonCancel_clicked();

private:
    Ui::Dialog *ui;
};

#endif // DIALOG_H
