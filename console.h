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
    enum remoteExecError {NoError, NotInstalled, LogonFailure, HostUnreachable, IOTimeout, Killed ,UnknownError};

    explicit Console(QWidget *parent = 0);
    ~Console();
    void setConfig(Config *config);
    bool startLocal();
    remoteExecError startRemoteWin(QString curHost);
    bool instLocal();
    remoteExecError instRemoteWin(QString curHost);

    void performRemoteWindowsInventory(QString curHost);
    void performRemoteWindowsInventoryOnIPv4Range(quint32 startIP, quint32 endIP);


    QProcess *myProcess;

private slots:
    void updateConsole ();
    void execError(QProcess::ProcessError error);

    void on_pushButtonOK_clicked();

private:
    remoteExecError execStatus;
    Ui::Console *ui;
    Config *config;

    void checkConsoleOut();
    void processExec(QString program, QStringList arguments);
    bool createInstScript(QFile * scriptFile, QString fileName);
    QString createWinexeFile();
    QString createInstWinFile();
};

#endif // CONSOLE_H
