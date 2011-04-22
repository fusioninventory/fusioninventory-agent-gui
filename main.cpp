#include <QtGui/QApplication>
#include <iostream>



#include "dialog.h"
#include "config.h"
#include "console.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);


    if (argc<3) {
        std::cerr<<"Usage: "<<argv[0]<<" somewhere/agent.cfg shomewhere/usr/bin/fusioninventory"<<std::endl;
        exit(1);
    }

    Dialog w;
    QString fusInvBinPath = QString(argv[2]);
    w.setFusInvBinPath(fusInvBinPath);

    Config c(argv[1]);
    w.loadConfig(&c);
    w.show();

    return a.exec();
}
