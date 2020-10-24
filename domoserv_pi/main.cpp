#include <QCoreApplication>
#include <interface.h>
#include <configure.h>
#include <iostream>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    printf("--------------------------------------------\n");
    printf("|                                          |\n");
    printf("|                DOMOSERV_PI               |\n");
    printf("|                Version 1.2               |\n");
    printf("|                                          |\n");
    printf("--------------------------------------------\n");

    //ARG
    bool srv = false;
    int confArg = -1;
    for(int i=0;i<qApp->arguments().count();i++)
        if(qApp->arguments().at(i) == "-server")
            srv = true;
        else if(qApp->arguments().at(i) == "-import")
            confArg = 1;
        else if(qApp->arguments().at(i) == "-export")
            confArg = 0;

    bool exit = false;
    if(srv) {
        Interface interface(exit);
    }
    else {
        Configure *conf = new Configure(confArg);
        //return 0;
    }
    return a.exec();
}
