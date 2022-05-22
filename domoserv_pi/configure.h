#ifndef CONFIGURE_H
#define CONFIGURE_H

#include <QObject>
#include <iostream>
#include <QTcpSocket>
#include <QSqlQuery>
#include <QSqlDatabase>
#include <QDataStream>
#include <QCoreApplication>
#include <QHostAddress>
#include <QWebSocket>
#include <cvorder.h>
#include <../dep/WiringPi/wiringPi/wiringPi.h>

#define GREEN "\033[0;32m"
#define RED "\033[0;31m"
#define NOCOLOR "\033[0m"

using namespace std;

class Configure : public QObject
{
    Q_OBJECT
public:
    Configure(int iofile = 0);
    ~Configure();

private:
    QTcpSocket *socket;
    QWebSocket webSocket;
    quint16 dataSize;
    QString _dataResult;
    void Scan();
    void GenerateConfigFile();
    void ImportConfigFile();
    void StateMenu();
    void ConfigMenu();
    void ConfigCVOrderMenu();
    void ConfigServerMenu();
    void ConfigGeneralMenu();
    void ProgMenu();
    void IpMenu();
    void Test();
    void SondeTempMenu();
    int DaytoInt(QString day);
    QString DaytoString(int day);
    int inChoice(int min, int max);
    int Question(QStringList Options, int max);

    QStringList list;

private slots:
    void endScan();
    void GeneralMenu();

};

#endif // CONFIGURE_H
