#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QNetworkInterface>
#include <QAbstractSocket>
#include <QNetworkAddressEntry>
#include "dialog.h"
#include "ui_dialog.h"
#include "config.h"

Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    console(this),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);
#ifdef Q_OS_WIN32
    ui->toolButtonAgentWin->setEnabled(true);
    ui->checkBoxTryNet->setText(tr("Try the entire domain"));
#else
    ui->toolButtonAgentUnix->setEnabled(true);
    ui->checkBoxTryNet->setText(tr("Try the entire network subnet"));
#endif
}

Dialog::~Dialog()
{
    delete ui;
}

bool Dialog::loadConfig(Config * config) {
    this->config = config;

    ui->lineEditRemoteHost->setText(config->get("host"));
    ui->lineEditRemoteHostUser->setText(config->get("host-user"));
    ui->lineEditRemoteHostPassword->setText(config->get("host-password"));
    ui->lineEditServer->setText(config->get("server"));
    ui->lineEditServerUser->setText(config->get("server-user"));
    ui->lineEditServerPassword->setText(config->get("server-password"));
    ui->lineEditProxy->setText(config->get("proxy"));
    ui->lineEditTag->setText(config->get("tag"));
    if (config->get("no-ssl-check") == "1") {
        ui->checkBoxNoSSLCheck->setChecked(true);
    }
    ui->labelCaCertFile->setText(config->get("ca-cert-file"));
    ui->lineEditAgentWin->setText(config->get("agent-win"));
    ui->lineEditAgentUnix->setText(config->get("agent-unix"));

    if (config->isReadOnly() ) {
        ui->lineEditRemoteHost->setDisabled(true);
        ui->lineEditRemoteHostUser->setDisabled(true);
        ui->lineEditRemoteHostPassword->setDisabled(true);
        ui->lineEditServer->setDisabled(true);
        ui->lineEditServerUser->setDisabled(true);
        ui->lineEditServerPassword->setDisabled(true);
        ui->lineEditProxy->setDisabled(true);
        ui->lineEditTag->setDisabled(true);
        ui->checkBoxNoSSLCheck->setDisabled(true);
        ui->toolButtonSelectCert->setDisabled(true);

        ui->lineEditAgentWin->setDisabled(true);
        ui->lineEditAgentUnix->setDisabled(true);

        ui->pushButtonCancel->setEnabled(true);
        ui->pushButtonTest->setDisabled(true);
        ui->pushButton->setDisabled(true);

        QMessageBox msgBox;
        msgBox.setText(tr("The configuration changes won't be saved. Do you have the required privilege?"));
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.exec();
    }

    return true;
}

bool Dialog::setConfig() {

    config->set("host", ui->lineEditRemoteHost->text());
    config->set("host-user", ui->lineEditRemoteHostUser->text());
    config->set("host-password", ui->lineEditRemoteHostPassword->text());
    config->set("server", ui->lineEditServer->text());
    config->set("server-user", ui->lineEditServerUser->text());
    config->set("server-password", ui->lineEditServerPassword->text());
    config->set("proxy", ui->lineEditProxy->text());
    config->set("tag", ui->lineEditTag->text());
    config->set("no-ssl-check", ui->checkBoxNoSSLCheck->isChecked()?"1":"0");
    config->set("agent-win", ui->lineEditAgentWin->text());
    config->set("agent-unix", ui->lineEditAgentUnix->text());
    config->save();

    console.setConfig(config);

    return true;
}

void Dialog::on_pushButton_clicked()
{
    setConfig();
    QApplication::exit(0);
}

void Dialog::on_pushButtonCancel_clicked()
{
    QApplication::exit(0);
}

void Dialog::on_toolButtonSelectCert_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"),
                                                    "/home");

    ui->labelCaCertFile->setText(fileName);
    config->set("ca-cert-file", fileName);

}

void Dialog::on_pushButtonTest_clicked()
{
    this->setConfig();
    if(ui->radioButtonLocal->isChecked()) {
        if(! console.startLocal()) {
            if(QMessageBox::Yes == QMessageBox::question(this,
                                                         tr("Agent execution failed"),
                                                         tr("Agent execution failed. Would you like to try to install/reinstall the agent?"),
                                                         QMessageBox::Yes, QMessageBox::Ignore)) {
                if(console.instLocal()) {
                    QMessageBox::information(this, tr("Installation completed"),
                                             tr("Installation completed successfuly. Retrying agent execution..."),
                                             QMessageBox::Ok);
                    console.startLocal();
                } else {
                    QMessageBox::critical(this, tr("Installation failed"),
                                          tr("Installation failed"),
                                          QMessageBox::Ok);
                }
            }
        }
    } else if (ui->radioButtonRemoteWin->isChecked()) {
        if(ui->checkBoxTryNet->isChecked()) {
            QNetworkInterface tempInterface;
            QNetworkAddressEntry tempAddress;
            foreach (tempInterface, QNetworkInterface::allInterfaces()) {
                foreach (tempAddress, tempInterface.addressEntries()) {
                    if ( tempAddress.ip().protocol() == QAbstractSocket::IPv4Protocol &&
                            tempAddress.ip() != QHostAddress::LocalHost &&
                            tempAddress.ip() != QHostAddress::LocalHostIPv6) {
                        quint32 networkIPv4Address = tempAddress.ip().toIPv4Address() & tempAddress.netmask().toIPv4Address();
                        console.performRemoteWindowsInventoryOnIPv4Range(networkIPv4Address+1, tempAddress.broadcast().toIPv4Address()-1);
                    }
                }
            }
        } else {
            QString remoteHosts = ui->lineEditRemoteHost->text();
            remoteHosts.remove(QRegExp("\\s+"));
            foreach (QString IPv4RangeString, remoteHosts.split(',')) {
                QRegExp rx;
                if (IPv4RangeString.isEmpty()) {
                    continue;
                }
                rx.setPattern("^([1-9][0-9]*\\.[0-9]+\\.[0-9]+\\.[0-9]+)([-/])(?:([1-9][0-9]*\\.[0-9]+\\.[0-9]+\\.[0-9]+)|([0-9]+))$");
                if (IPv4RangeString.contains(rx)) {
                    quint32 startIP;
                    quint32 endIP;
                    if(rx.cap(2) == "-") {
                        quint32 IP1 = QHostAddress(rx.cap(1)).toIPv4Address();
                        quint32 IP2 = QHostAddress(rx.cap(3)).toIPv4Address();
                        if (IP1<=IP2) {
                            startIP = IP1;
                            endIP = IP2;
                        } else {
                            startIP = IP2;
                            endIP = IP1;
                        }
                    } else {
                        QPair<QHostAddress, int> networkIPv4Address = QHostAddress::parseSubnet(IPv4RangeString);
                        startIP = ((QHostAddress) networkIPv4Address.first).toIPv4Address();
                        endIP = startIP | ( ~((quint32) 0) >> (  (int) networkIPv4Address.second) );
                    }
                    console.performRemoteWindowsInventoryOnIPv4Range(startIP, endIP);
                } else {
                    console.performRemoteWindowsInventory(IPv4RangeString);
                }
            }
        }

    } else if(ui->radioButtonRemoteUnix->isChecked()) {
        QMessageBox msgBox;
        msgBox.setText(tr("Remote Unix inventory is not implemented yet!"));
        msgBox.setIcon(QMessageBox::Information);
        msgBox.exec();
    } else {
        QMessageBox msgBox;
        msgBox.setText(tr("No server selected."));
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.exec();
    }
}


void Dialog::on_radioButtonLocal_toggled(bool checked)
{
    if(checked) {
        ui->groupBoxRemoteHost->setDisabled(true);
    } else {
        ui->groupBoxRemoteHost->setEnabled(true);
    }

}

void Dialog::on_toolButtonAgentWin_clicked()
{

    QString agentPath = QFileDialog::getExistingDirectory(0,
                                                          tr("Installation folder of the FusionInventory Agent"),
                                                          QDir::homePath());
    if (!agentPath.isEmpty() ) {
        QFileInfo agentPathInfo(agentPath);
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Critical);
        if (!agentPathInfo.exists()) {
            msgBox.setText(tr("Folder does not exist!"));
            msgBox.exec();
            return;
        }
        if (!agentPathInfo.isDir()) {
            msgBox.setText(tr("Not a folder!"));
            msgBox.exec();
            return;
        }
        agentPathInfo = QFileInfo(QString("%1/perl/bin/perl.exe").arg(agentPath));
        if (!agentPathInfo.exists()) {
            msgBox.setText(tr("Folder does not contain perl/bin/perl.exe!"));
            msgBox.exec();
            return;
        }
        if (!agentPathInfo.isFile() || !agentPathInfo.isExecutable()) {
            msgBox.setText(tr("/perl/bin/perl.exe either is not a file or is not executable!"));
            msgBox.exec();
            return;
        }
        agentPathInfo = QFileInfo(QString("%1/perl/bin/fusioninventory-agent").arg(agentPath));
        if (!agentPathInfo.exists()) {
            msgBox.setText(tr("Folder does not contain /perl/bin/fusioninventory-agent!"));
            msgBox.exec();
            return;
        }
        if (!agentPathInfo.isFile() || !agentPathInfo.isReadable()) {
            msgBox.setText(tr("/perl/bin/fusioninventory-agent either is not a file or is not readable!"));
            msgBox.exec();
            return;
        }

        ui->lineEditAgentWin->setText(agentPath);
    }
}

void Dialog::on_toolButtonAgentUnix_clicked()
{
    QString agentPath = QFileDialog::getExistingDirectory(0,
                                                          tr("Installation folder of the FusionInventory Agent"),
                                                          QDir::homePath());
    if (!agentPath.isEmpty() ) {
        QFileInfo agentWinPathInfo(agentPath);
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Critical);
        if (!agentWinPathInfo.exists()) {
            msgBox.setText(tr("Folder does not exist!"));
            msgBox.exec();
            return;
        }
        if (!agentWinPathInfo.isDir()) {
            msgBox.setText(tr("Not a folder!"));
            msgBox.exec();
            return;
        }
        agentWinPathInfo = QFileInfo(QString("%1/fusioninventory-agent").arg(agentPath));
        if (!agentWinPathInfo.exists()) {
            msgBox.setText(tr("Folder does not contain fusioninventory-agent!"));
            msgBox.exec();
            return;
        }
        if (!agentWinPathInfo.isFile() || !agentWinPathInfo.isExecutable()) {
            msgBox.setText(tr("fusioninventory-agent either is not a file or is not executable!"));
            msgBox.exec();
            return;
        }

        ui->lineEditAgentUnix->setText(agentPath);
    }
}

void Dialog::on_checkBoxTryNet_toggled(bool checked)
{
    if(checked) {
        ui->lineEditRemoteHost->setDisabled(true);
    } else {
        ui->lineEditRemoteHost->setEnabled(true);
    }
}
