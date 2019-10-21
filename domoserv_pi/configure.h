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
#include <wiringPi/wiringPi.h>
#include <../../CryptoFire/src/cryptofire.h>

#define GREEN "\033[0;32m"
#define RED "\033[0;31m"
#define NOCOLOR "\033[0m"

using namespace std;

class Configure : public QObject
{
    Q_OBJECT
public:
    explicit Configure();

    
signals:

public slots:
    
private:
    QTcpSocket *socket;
    QWebSocket webSocket;
    quint16 dataSize;
    QString _dataResult;
    void GeneralMenu();
    void StateMenu();
    void ConfigMenu();
    void ConfigCVOrderMenu();
    void ConfigServerMenu();
    void ConfigGeneralMenu();
    void ProgMenu();
    void Test();
    void SondeTempMenu();
    int DaytoInt(QString day);
    QString DaytoString(int day);

};

#endif // CONFIGURE_H
