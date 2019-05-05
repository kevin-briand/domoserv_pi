#include <QCoreApplication>
#include <interface.h>
#include <iostream>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    printf("--------------------------------------------\n");
    printf("|                                          |\n");
    printf("|                DOMOSERV_PI               |\n");
    printf("|                Version 1.02              |\n");
    printf("|                                          |\n");
    printf("--------------------------------------------\n");

    Interface interface;

    return a.exec();
}
