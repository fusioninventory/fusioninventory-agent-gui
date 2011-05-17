#include <iostream>

#include "console.h"
#include "ui_console.h"

Console::Console(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Console)
{
    ui->setupUi(this);
}

Console::~Console()
{
    delete ui;
}

bool Console::startLocal(Config * config) {
    //FIXME totally draft
    //use the tag from config
    //is this->config needed?
    this->config = config;

    this->show();
    this->repaint();

    QString program;
    QStringList arguments;

#ifdef Q_OS_WIN32
    program = QString("%1\\perl\\bin\\perl.exe")
            .arg(config->get("agent-win"));
    arguments << QString( "\"%1\\perl\\bin\\fusioninventory-agent\")
                         .arg(config->get("agent-win"));
#else
    program = QString("%1/fusioninventory-agent")
            .arg(config->get("agent-unix"));
#endif

    arguments << "--debug";
    arguments << QString("--tag=%1")
                 .arg(config->get("tag"));
    arguments << QString("--server=%1")
                 .arg(config->get("server"));

    myProcess = new QProcess();
    connect ( myProcess, SIGNAL(readyReadStandardError ()), this,
             SLOT(updateConsole()));

    ui->plainTextConsole->clear();
    ui->plainTextConsole->appendPlainText(program);

    /*foreach(QString tmpStr, arguments) {
        ui->plainTextConsole->appendPlainText(tmpStr);
    }
    */

    myProcess->start(program, arguments);
    while(myProcess->state() != QProcess::NotRunning) {
        qApp->processEvents(QEventLoop::AllEvents, 700);
    }

    ui->plainTextConsole->appendPlainText("---------- Process Completed ----------");
    ui->plainTextConsole->appendPlainText(QString("Exit code: %1").arg(myProcess->exitCode()));

    return true;
}


bool Console::startRemoteWin(Config * config) {
    //FIXME totally draft
    //use the tag from config
    //is this->config needed?
    this->config = config;

    this->show();
    this->repaint();

    QString program = config->get("winexe");
    QStringList arguments;
    arguments << "--user"
              << QString("%1%%2")
                 .arg(config->get("host-user"))
                 .arg(config->get("host-password"));
    arguments << "-d" << "11";
    arguments << QString("//%1").arg(config->get("host"));
    arguments << QString("\"%1\\perl\\bin\\perl.exe\" \"%1\\perl\\bin\\fusioninventory-agent\" --debug  --tag=%2 --server %3")
                 .arg(config->get("agent-win"))
                 .arg(config->get("tag"))
                 .arg(config->get("server"));

    myProcess = new QProcess();
    connect ( myProcess, SIGNAL(readyReadStandardError ()), this,
             SLOT(updateConsole()));

    ui->plainTextConsole->clear();
    ui->plainTextConsole->appendPlainText(program);

    /*foreach(QString tmpStr, arguments) {
        ui->plainTextConsole->appendPlainText(tmpStr);
    }
    */

    myProcess->start(program, arguments);
    while(myProcess->state() != QProcess::NotRunning) {
        qApp->processEvents(QEventLoop::AllEvents, 700);
    }

    ui->plainTextConsole->appendPlainText("---------- Process Completed ----------");
    ui->plainTextConsole->appendPlainText(QString("Exit code: %1").arg(myProcess->exitCode()));

    return true;
}

void Console::updateConsole () {
    QByteArray stdOutText = myProcess->readAllStandardOutput();
    QByteArray stdErrText = myProcess->readAllStandardError();
    if(stdOutText.size()>0) {
        ui->plainTextConsole->appendPlainText("---------- StdOut ----------");
        ui->plainTextConsole->appendPlainText(stdOutText.data());
    }
    if(stdErrText.size()>0) {
        ui->plainTextConsole->appendPlainText("---------- StdErr ----------");
        ui->plainTextConsole->appendPlainText(stdErrText.data());
    }
}

void Console::on_pushButtonOK_clicked()
{
    myProcess->kill();
    this->hide();
}
