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
bool Console::start() {

    this->show();
    this->repaint();

    QString program = fusInvBinPath;
    QStringList arguments;
    arguments <<"--debug";

    myProcess = new QProcess();
    connect ( myProcess, SIGNAL(readyReadStandardError ()), this,
                          SLOT(updateConsole()));

    ui->plainTextConsole->clear();
    myProcess->start(program, arguments);
     int result = myProcess->exitCode();
  /*  do {
       std::cout<<"aa"<<std::endl;
       QByteArray stdOutText = myProcess->readAllStandardOutput();
       QByteArray stdErrText = myProcess->readAllStandardError();
       std::cout<<stdOutText.data()<<std::endl;
       std::cout<<stdErrText.data()<<std::endl;
       std::cout<<"bb"<<myProcess->state()<<std::endl;
       this->repaint();

       QThread::wait(250);
       myProcess->waitForFinished(1000);
    } while(myProcess->state() != QProcess::NotRunning || myProcess->waitForReadyRead(500));*/


    return true;
}

void Console::updateConsole () {
    QByteArray stdOutText = myProcess->readAllStandardOutput();
    QByteArray stdErrText = myProcess->readAllStandardError();
    ui->plainTextConsole->insertPlainText(stdErrText.data());
    ui->plainTextConsole->insertPlainText(stdOutText.data());
}

void Console::on_pushButtonOK_clicked()
{
    myProcess->kill();
    this->hide();
}
