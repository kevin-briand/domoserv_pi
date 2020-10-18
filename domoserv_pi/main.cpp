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
    for(int i=0;i<qApp->arguments().count();i++)
        if(qApp->arguments().at(i) == "-server")
            srv = true;

    bool exit = false;
    if(srv)
        Interface interface(exit);
    else
        Configure conf;

    return a.exec();
}
