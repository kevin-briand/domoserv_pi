#include <QCoreApplication>
#include <interface.h>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    printf("--------------------------------------------\n");
    printf("|                                          |\n");
    printf("|                 DOMOSERV                 |\n");
    printf("|                Version 1.0               |\n");
    printf("|                                          |\n");
    printf("--------------------------------------------\n");

    Interface interface;

    return a.exec();
}
