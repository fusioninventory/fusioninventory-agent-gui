#ifndef CONSOLE_H
#define CONSOLE_H

#include <QDialog>
#include <QProcess>
#include "config.h"

namespace Ui {
    class Console;
}

class Console : public QDialog
{
    Q_OBJECT

public:
    explicit Console(QWidget *parent = 0);
    ~Console();
    bool startLocal(Config * config);
    bool startRemoteWin(Config * config);
    QProcess *myProcess;
    Config * config;

private slots:
    void updateConsole ();

    void on_pushButtonOK_clicked();

private:
    Ui::Console *ui;
};

#endif // CONSOLE_H
