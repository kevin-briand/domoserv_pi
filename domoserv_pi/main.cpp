#include <QCoreApplication>
#include <interface.h>
#include <iostream>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    printf("--------------------------------------------\n");
    printf("|                                          |\n");
    printf("|                DOMOSERV_PI               |\n");
    printf("|                Version 1.1               |\n");
    printf("|                                          |\n");
    printf("--------------------------------------------\n");

    bool exit = false;
    Interface interface(exit);

    if(!exit)
        return a.exec();
}
