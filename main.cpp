#include <QtGui/QApplication>
#include <iostream>
#include <QDir>
#include <QFileInfo>


#include "dialog.h"
#include "config.h"
#include "console.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);


/*    if (argc<3) {
        std::cerr<<"Usage: "<<argv[0]<<" somewhere/agent.cfg shomewhere/usr/bin/fusioninventory"<<std::endl;
        exit(1);
    } */

    Dialog w;

    /* QString fusInvCfgPath = QString("/etc/fusioninventory/agent.cfg"); */
    /* QString fusInvCfgPath = QString("/home/goneri/tmp/agent.cfg"); */
    QString fusInvCfgPath;
    if (qApp->arguments().count()>1) {
        fusInvCfgPath = qApp->arguments().at(1);
    }
    Config c(fusInvCfgPath);
    if (qApp->arguments().count()>2) {
        QString fusInvPath = qApp->arguments().at(2);
#ifdef Q_OS_WIN32
        c.set("agent-win", fusInvPath);
#else
        c.set("agent-unix", fusInvPath);
#endif
    }
    w.loadConfig(&c);
    w.show();


    return a.exec();
}
