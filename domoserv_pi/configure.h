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
    QString Encrypt(QString text);
    QString Decrypt(QString text);
    QString PKEY;
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
    void Test();
    void SondeTempMenu();

private slots:
    void Receipt_Data();
    void Send_Data(QString data = nullptr);
};

#endif // CONFIGURE_H
