#ifndef INTERFACE_H
#define INTERFACE_H

#include <QObject>
#include <cvorder.h>
#include <server.h>
#include <QtNetwork>
#include <stdio.h>
#include <iostream>
#include <QKeyEvent>
#include <QFile>
#include <QDesktopServices>

//TEST
#ifndef Q_OS_WIN
    #include </home/firedream/raspi/sysroot/usr/include/wiringPi/wiringPiSPI.h>
    //#define ACT_WIRING_PI_SPI
#endif


#define className "General"

class Interface : public QObject
{
    Q_OBJECT
public:
    Interface();
    bool Test();
    void Init();

public slots:
    void ShowInfo(QString classText,QString text);
    void ReceiptDataFromServer(QTcpSocket *user, QString data, int privilege);
    void ReceiptDataFromWebServer(QWebSocket *user, QString data, int privilege);
    void StartUpdate();

private:
    QString ReadData(QString data, int level);
    CVOrder *cvOrder;
    Server *server;
    bool _log = false;
    QString _linkLog;
    QTimer _update;
};

#endif // INTERFACE_H
