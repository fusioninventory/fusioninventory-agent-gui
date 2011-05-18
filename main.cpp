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

    Dialog w;
    QString fusInvCfgPath;
    if (qApp->arguments().count()>1) {
        fusInvCfgPath = qApp->arguments().at(1);
    }
    Config c(fusInvCfgPath);
    if ( ! c.settings ) {
        return 1;
    }
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
