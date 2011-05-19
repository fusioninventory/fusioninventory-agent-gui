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
    this->show();
    this->repaint();

    QString program;
    QStringList arguments;

#ifdef Q_OS_WIN32
    program = QString("%1\\perl\\bin\\perl.exe")
            .arg(config->get("agent-win"));
    arguments << QString( "\"%1\\perl\\bin\\fusioninventory-agent\"")
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

    ui->plainTextConsoleOut->clear();
    ui->plainTextConsoleErr->clear();
    ui->plainTextConsoleOut->appendPlainText(program);

    /* foreach(QString tmpStr, arguments) {
        ui->plainTextConsoleOut->appendPlainText(tmpStr);
    }
    */

    myProcess->start(program, arguments);
    while(myProcess->state() != QProcess::NotRunning) {
        qApp->processEvents(QEventLoop::AllEvents, 700);
    }

    updateConsole();
    //FIXME Detect Error Code 3 (ERROR_PATH_NOT_FOUND) and suggest installation
    ui->plainTextConsoleOut->appendPlainText("---------- Process Completed ----------");
    ui->plainTextConsoleOut->appendPlainText(QString("Exit code: %1").arg(myProcess->exitCode()));

    ui->pushButtonOK->setText("OK");
    return true;
}


bool Console::startRemoteWin(Config * config) {
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

    ui->plainTextConsoleOut->clear();
    ui->plainTextConsoleErr->clear();
    ui->plainTextConsoleOut->appendPlainText(program);

    /* foreach(QString tmpStr, arguments) {
        ui->plainTextConsoleOut->appendPlainText(tmpStr);
    }
    */

    myProcess->start(program, arguments);
    while(myProcess->state() != QProcess::NotRunning) {
        qApp->processEvents(QEventLoop::AllEvents, 700);
    }

    updateConsole();
    //FIXME Detect following and suggest installation:
    //Error: error Creating process("C:\Program Files\FusionInventory-Agent\perl\bin\perl.exe" "C:\Program Files\FusionInventory-Agent\perl\bin\fusioninventory-agent" --debug  --tag=Testing-by_GUI --server http://ocs/ocsinventory) 3
    ui->plainTextConsoleOut->appendPlainText("---------- Process Completed ----------");
    ui->plainTextConsoleOut->appendPlainText(QString("Exit code: %1").arg(myProcess->exitCode()));

    ui->pushButtonOK->setText("OK");
    return true;
}

bool Console::instRemoteWin(Config * config) {
    this->show();
    this->repaint();

    QString program = config->get("winexe");
    QStringList arguments;
    arguments << "--user"
              << QString("%1%%2")
                 .arg(config->get("host-user"))
                 .arg(config->get("host-password"));
    arguments << QString("--runas=%1%%2")
                 .arg(config->get("host-user"))
                 .arg(config->get("host-password"));
    arguments << "-d" << "11";
    arguments << QString("//%1").arg(config->get("host"));
    arguments << QString("CMD.EXE /C \"%1\" /S /tag=%2 /server=%3")
                 .arg(config->get("inst-win"))
                 .arg(config->get("tag"))
                 .arg(config->get("server"));

    myProcess = new QProcess();
    connect ( myProcess, SIGNAL(readyReadStandardError ()), this,
             SLOT(updateConsole()));

    ui->plainTextConsoleOut->clear();
    ui->plainTextConsoleErr->clear();
    ui->plainTextConsoleOut->appendPlainText(program);

    /* foreach(QString tmpStr, arguments) {
        ui->plainTextConsoleOut->appendPlainText(tmpStr);
    }
    */

    myProcess->start(program, arguments);
    while(myProcess->state() != QProcess::NotRunning) {
        qApp->processEvents(QEventLoop::AllEvents, 700);
    }

    updateConsole();
    ui->plainTextConsoleOut->appendPlainText("---------- Process Completed ----------");
    ui->plainTextConsoleOut->appendPlainText(QString("Exit code: %1").arg(myProcess->exitCode()));

    ui->pushButtonOK->setText("OK");
    return true;
}

void Console::updateConsole () {
    QByteArray stdOutText = myProcess->readAllStandardOutput();
    QByteArray stdErrText = myProcess->readAllStandardError();
    if(stdOutText.size()>0) {
        ui->plainTextConsoleOut->appendPlainText(stdOutText.data());
    }
    if(stdErrText.size()>0) {
        ui->plainTextConsoleErr->appendPlainText(stdErrText.data());
    }
}

void Console::on_pushButtonOK_clicked()
{
    myProcess->kill();
    this->hide();
}
