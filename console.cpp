#include <iostream>
#include <QFileDialog>
#include <QMessageBox>

#include "stdlib.h"
#include "console.h"
#include "ui_console.h"

Console::Console(QWidget *parent) :
    QDialog(parent),
    successfulExec(false),
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
    ui->pushButtonOK->setText("Close");
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
    connect ( myProcess, SIGNAL(error(QProcess::ProcessError)), this,
             SLOT(execError(QProcess::ProcessError)));

    ui->plainTextConsoleOut->clear();
    ui->plainTextConsoleErr->clear();
    ui->plainTextConsoleOut->appendPlainText(program);

    /* foreach(QString tmpStr, arguments) {
        ui->plainTextConsoleOut->appendPlainText(tmpStr);
    }
    */

    successfulExec = true;
    myProcess->start(program, arguments);
    while(myProcess->state() != QProcess::NotRunning) {
        qApp->processEvents(QEventLoop::AllEvents, 700);
    }

    updateConsole();
    //FIXME Detect Error Code 3 (ERROR_PATH_NOT_FOUND) and suggest installation
    ui->plainTextConsoleOut->appendPlainText("---------- Process Completed ----------");
    int exitCode = myProcess->exitCode();
    ui->plainTextConsoleOut->appendPlainText(QString("Exit code: %1").arg(exitCode));

    ui->pushButtonOK->setText("OK");
    return successfulExec && ! exitCode;
}


bool Console::startRemoteWin(Config * config) {
    this->show();
    ui->pushButtonOK->setText("Close");
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
    connect ( myProcess, SIGNAL(error(QProcess::ProcessError)), this,
                 SLOT(execError(QProcess::ProcessError)));

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
    int exitCode = myProcess->exitCode();
    ui->plainTextConsoleOut->appendPlainText(QString("Exit code: %1").arg(exitCode));
    /* Followong output means that the agent is not installed on the remote win host
       Error: error Creating process("C:\Program Files\FusionInventory-Agent\perl\bin\perl.exe" "C:\Program Files\FusionInventory-Agent\perl\bin\fusioninventory-agent" --debug  --tag=Testing-by_GUI --server http://ocs/ocsinventory) 3
      */
    if(ui->plainTextConsoleOut->find(
                QString("Error: error Creating process(%1) 3").arg(QString("\"%1\\perl\\bin\\perl.exe\" \"%1\\perl\\bin\\fusioninventory-agent\" --debug  --tag=%2 --server %3")
                                                                   .arg(config->get("agent-win"))
                                                                   .arg(config->get("tag"))
                                                                   .arg(config->get("server"))),
                QTextDocument::FindBackward)) {
        ui->plainTextConsoleErr->appendPlainText(tr("Agent is not installed on the remote win host"));
        /* Just making sure that successfulExec is set to false
          */
        successfulExec = false;
    }

    ui->pushButtonOK->setText("OK");
    return successfulExec && ! exitCode;
}

bool Console::instLocal(Config * config) {
    this->show();
    ui->pushButtonOK->setText("Close");
    this->repaint();

#ifdef Q_OS_WIN32
    QString program = config->get("inst-win");
    QStringList arguments;
    arguments << "/S" << Qstring("/tag=%2").arg(config->get("tag"))
              <<  QString("/server=%3").arg(config->get("server"));
#else

    QString instUnixPath = QFileDialog::getOpenFileName(this,
                                                        tr("Unix installation file"),
                                                        config->get("inst-unix"),
                                                        "ZIP (*.zip);;Gzip compressed tar (*.tar.gz *.tgz);;Bzip2 compressed tar (*.tar.bz2);;tar (*.tar)");
    if (instUnixPath.isEmpty()) {
        return false;
    }
    QFileInfo instUnixPathInfo(instUnixPath);
    if (!instUnixPathInfo.isFile() || !instUnixPathInfo.isReadable()) {
        QMessageBox::critical(this, tr("installation file error"),
                              tr("Installation file is not readable."),
                              QMessageBox::Ok);
        return false;
    }
    //FIXME
    if(instUnixPath.right(4).toLower() != ".zip") {
        QMessageBox::information(this, tr("not implemented yet"),
                                 tr("Installation using this type of file is not implemented yet"),
                                 QMessageBox::Ok);
        return false;
    }
    /* Checking the parent directory of the installation directory to be writable
      */
    QFileInfo instParentPath(QFileInfo(config->get("agent-unix").replace(QRegExp("/$"),"")).absolutePath());

    if(instParentPath.exists()) {
        if(!instParentPath.isWritable()) {
            QMessageBox::critical(this, tr("installation path error"),
                                  tr("Parent directory of the installation path is not writable."),
                                  QMessageBox::Ok);
            return false;
        }
    } else if (! QDir("/").mkpath(instParentPath.absoluteFilePath())) {
        QMessageBox::critical(this, tr("installation path error"),
                              tr("Could not create the parent directory of the installation path."),
                              QMessageBox::Ok);
        return false;
    }

    srand ( time(NULL) );
    QDir tmpDir(QString("%1/%2.%3.%4")
                .arg(instParentPath.absoluteFilePath())
                .arg(QFileInfo(qApp->applicationFilePath()).fileName())
                .arg(qApp->applicationPid())
                .arg(rand() % 100));
    QString program = "unzip";
    QStringList arguments;
    arguments << instUnixPath <<"-d" << tmpDir.absolutePath();
#endif

    myProcess = new QProcess();
    connect ( myProcess, SIGNAL(readyReadStandardError ()), this,
             SLOT(updateConsole()));
    connect ( myProcess, SIGNAL(error(QProcess::ProcessError)), this,
             SLOT(execError(QProcess::ProcessError)));

    ui->plainTextConsoleOut->clear();
    ui->plainTextConsoleErr->clear();
    ui->plainTextConsoleOut->appendPlainText(program);

    /* foreach(QString tmpStr, arguments) {
        ui->plainTextConsoleOut->appendPlainText(tmpStr);
    }
    */
    successfulExec = true;
    myProcess->start(program, arguments);
    while(myProcess->state() != QProcess::NotRunning) {
        qApp->processEvents(QEventLoop::AllEvents, 700);
    }
    updateConsole();
    ui->plainTextConsoleOut->appendPlainText("---------- Process Completed ----------");
    int exitCode = myProcess->exitCode();
    ui->plainTextConsoleOut->appendPlainText(QString("Exit code: %1").arg(exitCode));

#ifndef Q_OS_WIN32
    if(successfulExec && ! exitCode) {
        system(QString("rm -rf '%1'").arg(config->get("agent-unix")).toAscii());
        tmpDir.rename(tmpDir.entryList().last(), config->get("agent-unix"));
        tmpDir.rmdir(tmpDir.absolutePath());
    }
#endif

    ui->pushButtonOK->setText("OK");
    return successfulExec && ! exitCode;
}

bool Console::instRemoteWin(Config * config) {
    this->show();
    ui->pushButtonOK->setText("Close");
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
    if(stdErrText.size()>0) { /* This condition would be always true */
        ui->plainTextConsoleErr->appendPlainText(stdErrText.data());
    }
}

void Console::execError(QProcess::ProcessError error) {
    successfulExec = false;
    ui->plainTextConsoleErr->appendPlainText(QString("Execution failed due to error no. %1").arg(error));
    if(error == QProcess::FailedToStart) {
        ui->plainTextConsoleErr->appendPlainText(tr("The process failed to start. Either the invoked program is missing, or you may have insufficient permissions to invoke the program."));
    }
}

void Console::on_pushButtonOK_clicked()
{
    myProcess->kill();
    this->hide();
}
