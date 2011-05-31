#include <iostream>
#include <QFileDialog>
#include <QMessageBox>
#include <QTemporaryFile>
#include <QTextStream>

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
                 .arg(QDir::toNativeSeparators(config->get("agent-win")));
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
    connect ( myProcess, SIGNAL(readyReadStandardError()), this,
             SLOT(updateConsole()));
    connect ( myProcess, SIGNAL(readyReadStandardOutput()), this,
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
    //TODO
#ifdef Q_OS_WIN32
    QMessageBox::critical(this, tr("Not implemented"),
                          tr("Remote windows execution from a windows workstation is not implemented yet."),
                          QMessageBox::Ok);
    return false;
#endif
    this->show();
    ui->pushButtonOK->setText("Close");
    this->repaint();

    QString tempWinexe = createWinexeFile();
    if (tempWinexe .isEmpty()) {
        return false;
    }

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
    bool processExecStatus = processExec(tempWinexe, arguments);
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
        processExecStatus = false;
    }

    QFile(tempWinexe).remove();
    ui->pushButtonOK->setText("OK");
    return processExecStatus;
}

bool Console::instLocal(Config * config) {
    this->show();
    ui->pushButtonOK->setText("Close");
    this->repaint();

#ifdef Q_OS_WIN32
    QString installationFile = createInstWinFile();
    if (installationFile .isEmpty()) {
        return false;
    }

    QString program = QDir::toNativeSeparators(installationFile);
    QStringList arguments;
    arguments << "/S" << QString("/tag=%2").arg(config->get("tag"))
              <<  QString("/server=%3").arg(config->get("server"));
#else

    QString instUnixFile = QFileDialog::getOpenFileName(this,
                                                        tr("Unix installation file"),
                                                        config->get("inst-unix"),
                                                        "ZIP(*.zip);;Gzip compressed tar(*.tar.gz *.tgz);;Bzip2 compressed tar(*.tar.bz2);;tar(*.tar)");
    if (instUnixFile.isEmpty()) {
        return false;
    }
    QFileInfo instUnixPathInfo(instUnixFile);
    if (!instUnixPathInfo.isFile() || !instUnixPathInfo.isReadable()) {
        QMessageBox::critical(this, tr("installation file error"),
                              tr("Installation file is not readable."),
                              QMessageBox::Ok);
        return false;
    }


    QTemporaryFile tempFile(QString("%1/%2-XXXXXX.sh")
                            .arg(QDir::tempPath())
                            .arg(QFileInfo(qApp->applicationFilePath()).fileName()));

    if (! tempFile.open() || ! createInstScript(&tempFile, instUnixFile, config) ) {
        QMessageBox::critical(this, tr("installation script error"),
                              tr("Could not create a temporary installation script."),
                              QMessageBox::Ok);
        return false;
    }
    /* Making sure that AutoRemove is set and closing the file descriptor
      */
    tempFile.setAutoRemove(true);
    tempFile.close();

    if( ! tempFile.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner |
                                  QFile::ReadUser  | QFile::WriteUser | QFile::ExeUser |
                                  QFile::ReadGroup | QFile::ExeGroup |
                                  QFile::ReadOther | QFile::ExeOther) ) {
        QMessageBox::critical(this, tr("installation script error"),
                              tr("Could not set executable permission on the temporary installation script."),
                              QMessageBox::Ok);
        return false;
    }

    QString program = "sh";
    QStringList arguments;
    arguments << tempFile.fileName();
    //TODO
    /* Might be a good idea to run the shell by "errexit" option
    arguments << "-o" << "errexit";
    */
#endif

    myProcess = new QProcess();
    connect ( myProcess, SIGNAL(readyReadStandardError()), this,
             SLOT(updateConsole()));
    connect ( myProcess, SIGNAL(readyReadStandardOutput()), this,
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

    ui->pushButtonOK->setText("OK");
    /* Remove the installation script/program before returning
      */
#ifdef Q_OS_WIN32
    QFile::setPermissions(installationFile, QFile::ReadOwner | QFile::WriteOwner);
    QFile::remove(installationFile);
#else
    /* AutoRemove is set, .remove is not necessary.
      */
    // tempFile.remove();
#endif
    return successfulExec && ! exitCode;
}

bool Console::instRemoteWin(Config * config) {
    //TODO
#ifdef Q_OS_WIN32
    QMessageBox::critical(this, tr("Not implemented"),
                          tr("Remote windows execution from a windows workstation is not implemented yet."),
                          QMessageBox::Ok);
    return false;
#endif
    this->show();
    ui->pushButtonOK->setText("Close");
    this->repaint();

    QString tempWinexe = createWinexeFile();
    if (tempWinexe .isEmpty()) {
        return false;
    }

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

    QStringList tmpArguments;
    bool processExecStatus;
    tmpArguments = QStringList(arguments);
    tmpArguments << "CMD.EXE /C \"MKDIR %TEMP%\\FusInv\"";
    processExecStatus = processExec(tempWinexe, tmpArguments);
    /* This directory might already be there. Ignoring the return value */

    tmpArguments = QStringList(arguments);
    tmpArguments << "CMD.EXE /C \"NET SHARE FusInv=%TEMP%\\FusInv /UNLIMITED\"";
    processExecStatus = processExec(tempWinexe, tmpArguments);
    //TODO check the return value


    QString installationFile = createInstWinFile();
    if (installationFile .isEmpty()) {
        return false;
    }
    tmpArguments = QStringList();
    tmpArguments << QString("//%1/FusInv").arg(config->get("host"));
    tmpArguments << config->get("host-password");
    tmpArguments << "-U" << config->get("host-user");
    tmpArguments << "-c" << QString("lcd %1 ; put %2")
                    .arg(QFileInfo(installationFile).absolutePath())
                    .arg(QFileInfo(installationFile).fileName());
    processExecStatus = processExec("smbclient", tmpArguments);
    QFile(installationFile).remove();
    //TODO check the return value

    tmpArguments = QStringList(arguments);
    tmpArguments << "CMD.EXE /C \"NET SHARE FusInv /d\"";
    processExecStatus = processExec(tempWinexe, tmpArguments);

    tmpArguments = QStringList(arguments);
    tmpArguments << QString("CMD.EXE /C \"%TEMP%\\FusInv\\%1\" /S /tag=%2 /server=%3")
                    .arg(QFileInfo(installationFile).fileName())
                    .arg(config->get("tag"))
                    .arg(config->get("server"));
    ui->plainTextConsoleOut->appendPlainText("---------- Installation in progress ----------");
    ui->plainTextConsoleErr->appendPlainText("---------- Installation in progress ----------");
    processExecStatus = processExec(tempWinexe, tmpArguments);
    //TODO check the return value

    tmpArguments = QStringList(arguments);
    tmpArguments << "CMD.EXE /C \"RMDIR /S /Q %TEMP%\\FusInv\"";
    processExecStatus = processExec(tempWinexe, tmpArguments);

    QFile(tempWinexe).remove();
    ui->pushButtonOK->setText("OK");
    return processExecStatus;
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
    if (myProcess->state() == QProcess::Running) {
        myProcess->kill();
    }
    this->hide();
}

bool Console::createInstScript(QFile * scriptFile, QString fileName, Config * config) {
    // Generating a decompression command for each file type
    // ZIP(*.zip);;Gzip compressed tar(*.tar.gz *.tgz);;Bzip2 compressed tar(*.tar.bz2);;tar(*.tar)
    QString decompressCmd;
    if(fileName.right(4).toLower() == ".zip") {
        decompressCmd = QString("unzip '%1' -d \"$TEMP_DIR\"").arg(fileName);
    } else if(fileName.right(4).toLower() == ".tar") {
        decompressCmd = QString("tar xvf '%1' -C \"$TEMP_DIR\"").arg(fileName);
    } else if(fileName.right(4).toLower() == ".tgz" || fileName.right(7).toLower() == ".tar.gz") {
        decompressCmd = QString("gzip -dc '%1' | tar xvf - -C \"$TEMP_DIR\"").arg(fileName);
    } else if(fileName.right(8).toLower() == ".tar.bz2") {
        decompressCmd = QString("bzip2 -dc '%1' | tar xvf - -C \"$TEMP_DIR\"").arg(fileName);
    } else {
        QMessageBox::information(this, tr("not implemented yet"),
                                 tr("Installation using this type of file is not supported yet."),
                                 QMessageBox::Ok);
        return false;
    }

    //TODO
    //List exit codes
    QTextStream scriptTextStream(scriptFile);
    scriptTextStream << "#!/bin/sh";
    scriptTextStream << "\n";

    QString instParentPath = QDir(QFileInfo(config->get("agent-unix").replace(QRegExp("/$"),"")).absolutePath()).absolutePath();
    /* Checking the parent directory of the installation directory to be writable
      */
    scriptTextStream << "\n" << QString("if [ -d '%1' ]").arg(instParentPath);
    scriptTextStream << "\n" <<         "then";
    scriptTextStream << "\n" << QString("  if [ ! -w '%1' ]").arg(instParentPath);
    scriptTextStream << "\n" <<         "  then";
    scriptTextStream << "\n" << QString("    echo '%1'").arg(tr("Parent directory of the installation path is not writable."));
    scriptTextStream << "\n" <<         "    exit 1";
    scriptTextStream << "\n" <<         "  fi";
    scriptTextStream << "\n" <<         "else";
    scriptTextStream << "\n" << QString("  mkdir -p '%1'").arg(instParentPath);
    scriptTextStream << "\n" << QString("  if [ ! -d '%1' -o !-w '%1' ]; then echo '%2'; exit 2; fi").arg(instParentPath).arg(tr("Could not create the parent directory of the installation path."));
    scriptTextStream << "\n" <<         "fi";
    scriptTextStream << "\n";

    scriptTextStream << "\n" << QString("TEMP_DIR='%1/%2.%3.%4'")
                        .arg(instParentPath)
                        .arg(QFileInfo(qApp->applicationFilePath()).fileName())
                        .arg(qApp->applicationPid())
                        .arg(rand() % 100);
    scriptTextStream << "\n" <<         "mkdir -p \"$TEMP_DIR\"";
    scriptTextStream << "\n" << QString("if [ ! -d \"$TEMP_DIR\" -o ! -w \"$TEMP_DIR\" ]; then echo '%1'; exit 3; fi").arg(tr("Could not create the temp directory."));

    scriptTextStream << "\n" << decompressCmd;

    scriptTextStream << "\n" << QString("rm -rf '%1'").arg(config->get("agent-unix"));
    scriptTextStream << "\n" << QString("mv  \"$TEMP_DIR\"/* '%2'").arg(config->get("agent-unix"));
    scriptTextStream << "\n" <<         "rm -rf \"$TEMP_DIR\"";
    scriptTextStream << "\n\n";

    return true;
}

QString Console::createWinexeFile() {
    QString fileName = QString("%1/winexe.%2.%3")
            .arg(QDir::tempPath())
            .arg(qApp->applicationPid())
            .arg(rand() % 100);

    if ( QFile::copy(":/fusinv/winexe", fileName) &&
            QFile(fileName).setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner |
                                           QFile::ReadUser  | QFile::WriteUser | QFile::ExeUser |
                                           QFile::ReadGroup | QFile::ExeGroup |
                                           QFile::ReadOther | QFile::ExeOther) ) {
        return fileName;
    } else {
        QMessageBox::critical(this, tr("Embedded winexe file error"),
                              tr("Could not copy the embedded winexe file into a regular file."),
                              QMessageBox::Ok);
        return "";
    }
}


QString Console::createInstWinFile() {
    QString fileName = QDir::toNativeSeparators(
                QString("%1/FusInvAgentInst.%2.%3.exe")
            .arg(QDir::tempPath())
            .arg(qApp->applicationPid())
            .arg(rand() % 100) );

    if ( QFile::copy(":/fusinv/inst-win.exe", fileName) &&
            QFile(fileName).setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner |
                                           QFile::ReadUser  | QFile::WriteUser | QFile::ExeUser |
                                           QFile::ReadGroup | QFile::ExeGroup |
                                           QFile::ReadOther | QFile::ExeOther) ) {
        return fileName;
    } else {
        QMessageBox::critical(this, tr("Embedded Installation File error"),
                              tr("Could not copy the embedded installation file into a regular file."),
                              QMessageBox::Ok);
        return "";
    }
}

bool Console::processExec(QString program, QStringList arguments) {
    myProcess = new QProcess();
    connect ( myProcess, SIGNAL(readyReadStandardError()), this,
             SLOT(updateConsole()));
    connect ( myProcess, SIGNAL(readyReadStandardOutput()), this,
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

    return successfulExec && ! exitCode;
}
