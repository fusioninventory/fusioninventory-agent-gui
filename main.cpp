#include <QtGui/QApplication>
#include <iostream>
#include "dialog.h"
#include "config.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    if (argc<2) {
        std::cerr<<"Usage: "<<argv[0]<<" somewhere/agent.cfg"<<std::endl;
        exit(1);
    }

    Dialog w;
    Config c(argv[1]);
    w.loadConfig(&c);
    w.show();

    return a.exec();
}
