#ifndef CONSOLE_H
#define CONSOLE_H

#include <QDialog>
#include <QProcess>
#include <QFile>
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
    bool instLocal(Config * config);
    bool instRemoteWin(Config * config);
    QProcess *myProcess;

private slots:
    void updateConsole ();
    void execError(QProcess::ProcessError error);

    void on_pushButtonOK_clicked();

private:
    bool successfulExec;
    Ui::Console *ui;

    bool processExec(QString program, QStringList arguments);
    bool createInstScript(QFile * scriptFile, QString fileName, Config * config);
    QString createWinexeFile();
    QString createInstWinFile();
};

#endif // CONSOLE_H
