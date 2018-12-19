#ifndef INTERFACE_H
#define INTERFACE_H

#include <QObject>
#include <cvorder.h>
#include <server.h>
#include <QtNetwork>
#include <stdio.h>
#include <iostream>
#include <QKeyEvent>

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
    void ReceiptDataFromServer(QTcpSocket *user, QString data);

private:
    CVOrder *cvOrder;
    Server *server;
};

#endif // INTERFACE_H
