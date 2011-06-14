#include <iostream>
#include <QFileDialog>
#include <QMessageBox>
#include <QTemporaryFile>
#include <QTextStream>
#include <QNetworkInterface>
#include <QAbstractSocket>
#include <QNetworkAddressEntry>
#include "stdlib.h"
#include "console.h"
#include "ui_console.h"

Console::Console(QWidget *parent) :
    QDialog(parent),
    execStatus(Console::NoError),
    ui(new Ui::Console)
{
    ui->setupUi(this);
}

Console::~Console()
{
    delete ui;
}

void Console::setConfig(Config *config) {
    this->config = config;
}

bool Console::startLocal() {
    this->show();
    ui->pushButtonOK->setText("Close");
    this->setWindowTitle(tr("Localhost Inventory"));
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

    /* foreach(QString tempStr, arguments) {
        ui->plainTextConsoleOut->appendPlainText(tempStr);
    }
    */

    execStatus = NoError;
    myProcess->start(program, arguments);
    ui->pushButtonOK->setText("Kill / Close");
    while(myProcess->state() != QProcess::NotRunning) {
        qApp->processEvents(QEventLoop::AllEvents, 700);
    }

    updateConsole();
    //FIXME Detect Error Code 3 (ERROR_PATH_NOT_FOUND) and suggest installation
    ui->plainTextConsoleOut->appendPlainText("---------- Process Completed ----------");
    int exitCode = myProcess->exitCode();
    ui->plainTextConsoleOut->appendPlainText(QString("Exit code: %1").arg(exitCode));

    ui->pushButtonOK->setText("OK");
    return (execStatus ==NoError) && ! exitCode;
}


Console::remoteExecError Console::startRemoteWin(QString curHost="") {
    //TODO
#ifdef Q_OS_WIN32
    QMessageBox::critical(this, tr("Not implemented"),
                          tr("Remote windows execution from a windows workstation is not implemented yet."),
                          QMessageBox::Ok);
    return UnknownError;
#endif
    this->show();
    ui->pushButtonOK->setText("Close");
    this->repaint();

    QString tempWinexe = createWinexeFile();
    if (tempWinexe .isEmpty()) {
        return UnknownError;
    }

    QString targetHost = curHost.isEmpty()? config->get("host") : curHost;
    this->setWindowTitle(tr("Remote Windows Host Inventory of ") + targetHost);
    QStringList arguments;
    arguments << "--user"
              << QString("%1%%2")
                 .arg(config->get("host-user"))
                 .arg(config->get("host-password"));
    arguments << "-d" << "11";
    arguments << QString("//%1").arg(targetHost);
    arguments << QString("\"%1\\perl\\bin\\perl.exe\" \"%1\\perl\\bin\\fusioninventory-agent\" --debug  --tag=%2 --server %3")
                 .arg(config->get("agent-win"))
                 .arg(config->get("tag"))
                 .arg(config->get("server"));
    processExec(tempWinexe, arguments);

    checkConsoleOut();

    QFile(tempWinexe).remove();
    ui->pushButtonOK->setText("OK");
    return execStatus;
}

bool Console::instLocal() {
    this->show();
    ui->pushButtonOK->setText("Close");
    this->setWindowTitle(tr("Local Agent Installation"));
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
                                                        QDir::currentPath(),
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

    if (! tempFile.open() || ! createInstScript(&tempFile, instUnixFile) ) {
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

    /* foreach(QString tempStr, arguments) {
        ui->plainTextConsoleOut->appendPlainText(tempStr);
    }
    */
    execStatus = NoError;
    myProcess->start(program, arguments);
    ui->pushButtonOK->setText("Kill / Close");
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
    return (execStatus == NoError) && ! exitCode;
}

Console::remoteExecError Console::instRemoteWin(QString curHost="") {
    //TODO
#ifdef Q_OS_WIN32
    QMessageBox::critical(this, tr("Not implemented"),
                          tr("Remote windows execution from a windows workstation is not implemented yet."),
                          QMessageBox::Ok);
    return UnknownError;
#endif
    this->show();
    ui->pushButtonOK->setText("Close");
    this->repaint();

    QString tempWinexe = createWinexeFile();
    if (tempWinexe .isEmpty()) {
        return UnknownError;
    }

    QString targetHost = curHost.isEmpty()? config->get("host") : curHost;
    this->setWindowTitle(tr("Remote Windows Agent Installation on ") + targetHost);
    QStringList arguments;
    arguments << "--user"
              << QString("%1%%2")
                 .arg(config->get("host-user"))
                 .arg(config->get("host-password"));
    arguments << QString("--runas=%1%%2")
                 .arg(config->get("host-user"))
                 .arg(config->get("host-password"));
    arguments << "-d" << "11";
    arguments << QString("//%1").arg(targetHost);

    QStringList tempArguments;
    tempArguments = QStringList(arguments);
    tempArguments << "CMD.EXE /C \"MKDIR %TEMP%\\FusInv\"";
    processExec(tempWinexe, tempArguments);
    /* This directory might already be there. Ignoring the return value */

    tempArguments = QStringList(arguments);
    tempArguments << "CMD.EXE /C \"NET SHARE FusInv=%TEMP%\\FusInv /UNLIMITED\"";
    processExec(tempWinexe, tempArguments);
    //TODO check the return value


    QString installationFile = createInstWinFile();
    if (installationFile .isEmpty()) {
        return UnknownError;
    }
    tempArguments = QStringList();
    tempArguments << QString("//%1/FusInv").arg(targetHost);
    tempArguments << config->get("host-password");
    tempArguments << "-U" << config->get("host-user");
    tempArguments << "-c" << QString("lcd %1 ; put %2")
                    .arg(QFileInfo(installationFile).absolutePath())
                    .arg(QFileInfo(installationFile).fileName());
    processExec("smbclient", tempArguments);
    QFile(installationFile).remove();
    //TODO check the return value

    tempArguments = QStringList(arguments);
    tempArguments << "CMD.EXE /C \"NET SHARE FusInv /d\"";
    processExec(tempWinexe, tempArguments);

    tempArguments = QStringList(arguments);
    tempArguments << QString("CMD.EXE /C \"%TEMP%\\FusInv\\%1\" /S /tag=%2 /server=%3")
                    .arg(QFileInfo(installationFile).fileName())
                    .arg(config->get("tag"))
                    .arg(config->get("server"));
    ui->plainTextConsoleOut->appendPlainText("---------- Installation in progress ----------");
    ui->plainTextConsoleErr->appendPlainText("---------- Installation in progress ----------");
    processExec(tempWinexe, tempArguments);
    //TODO check the return value

    tempArguments = QStringList(arguments);
    tempArguments << "CMD.EXE /C \"RMDIR /S /Q %TEMP%\\FusInv\"";
    processExec(tempWinexe, tempArguments);

    QFile(tempWinexe).remove();
    ui->pushButtonOK->setText("OK");
    return execStatus;
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
    if (execStatus == NoError) { execStatus = UnknownError; }
    ui->plainTextConsoleErr->appendPlainText(QString("Execution failed due to error no. %1").arg(error));
    if(error == QProcess::FailedToStart) {
        ui->plainTextConsoleErr->appendPlainText(tr("The process failed to start. Either the invoked program is missing, or you may have insufficient permissions to invoke the program."));
    }
}

void Console::on_pushButtonOK_clicked()
{
    if (myProcess->state() == QProcess::Running) {
        myProcess->kill();
        execStatus = Killed;
    }
    this->hide();
}

bool Console::createInstScript(QFile * scriptFile, QString fileName) {
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

void Console::performRemoteWindowsInventory(QString curHost) {
    if(startRemoteWin(curHost) == NotInstalled) {
        if(QMessageBox::Yes == QMessageBox::question(this,
                                                     tr("Agent execution failed"),
                                                     tr("Agent execution failed. Would you like to try to install/reinstall the agent?"),
                                                     QMessageBox::Yes, QMessageBox::Ignore)) {
            if(instRemoteWin(curHost) == NoError) {
                QMessageBox::information(this, tr("Installation completed"),
                                         tr("Installation completed successfuly. Retrying agent execution..."),
                                         QMessageBox::Ok);
                startRemoteWin(curHost);
            } else {
                QMessageBox::critical(this, tr("Installation failed"),
                                      tr("Installation failed"),
                                      QMessageBox::Ok);
            }
        }
    }
}

void Console::performRemoteWindowsInventoryOnIPv4Range(quint32 startIP, quint32 endIP) {
    quint32 curIPv4Address;
    QString tempIPv4RangeName = QString("\n%1 - %2\n").arg(QHostAddress(startIP).toString()).arg(QHostAddress(endIP).toString());
    bool askEachIPv4Inventory = true;
    for (curIPv4Address = startIP; curIPv4Address <= endIP; curIPv4Address++) {
        if (askEachIPv4Inventory || execStatus == Killed) {
            QMessageBox::StandardButton tempAnswer = QMessageBox::question(this,
                                                                           tr("IP Address"),
                                                                           tr("Should we continue inventory of")
                                                                           + tempIPv4RangeName
                                                                           + tr("with the following IP address?")
                                                                           + "\n" + QHostAddress(curIPv4Address).toString(),
                                                                           QMessageBox::Yes | QMessageBox::YesToAll | QMessageBox::No);
            if (tempAnswer == QMessageBox::YesToAll) {
                askEachIPv4Inventory = false;
            } else if (tempAnswer == QMessageBox::No) {
                return;
            }
        }
        performRemoteWindowsInventory(QHostAddress(curIPv4Address).toString());
    }
}

void Console::checkConsoleOut() {
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
        execStatus = NotInstalled;
    } else if(ui->plainTextConsoleOut->find("ERROR: Failed to open connection - NT_STATUS_LOGON_FAILURE",
                                            QTextDocument::FindBackward)) {
        ui->plainTextConsoleErr->appendPlainText(tr("Could not log onto the remote win host."));
        execStatus = LogonFailure;
    } else if(ui->plainTextConsoleOut->find("ERROR: Failed to open connection - NT_STATUS_HOST_UNREACHABLE",
                                            QTextDocument::FindBackward)) {
        ui->plainTextConsoleErr->appendPlainText(tr("No such a host exists or host unreachable."));
        execStatus = HostUnreachable;
    } else if(ui->plainTextConsoleOut->find("ERROR: Failed to open connection - NT_STATUS_IO_TIMEOUT",
                                            QTextDocument::FindBackward)) {
        ui->plainTextConsoleErr->appendPlainText(tr("Remote host could not be contacted (timed out)."));
        execStatus = IOTimeout;
    }
}

void Console::processExec(QString program, QStringList arguments) {
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

    /* foreach(QString tempStr, arguments) {
        ui->plainTextConsoleOut->appendPlainText(tempStr);
    }
    */

    execStatus = NoError;
    myProcess->start(program, arguments);
    ui->pushButtonOK->setText("Kill / Close");
    while(myProcess->state() != QProcess::NotRunning) {
        qApp->processEvents(QEventLoop::AllEvents, 700);
    }

    updateConsole();
    ui->plainTextConsoleOut->appendPlainText("---------- Process Completed ----------");
    int exitCode = myProcess->exitCode();
    ui->plainTextConsoleOut->appendPlainText(QString("Exit code: %1").arg(exitCode));

    if ( execStatus == NoError && exitCode) {
        execStatus = UnknownError;
    }
}
