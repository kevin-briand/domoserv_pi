#ifndef INTERFACE_H
#define INTERFACE_H

#include <QObject>
#include <cvorder.h>
#include <QtNetwork>
#include <stdio.h>
#include <iostream>
#include <QKeyEvent>
#include <QFile>
#include <QDesktopServices>
#include <configure.h>
#include "../dep/ServerFire/src/serverfire.h"
#include <interfacei2c.h>

#define className "General"

#define ACT_WIRING_PI_I2C

class Interface : public QObject
{
    Q_OBJECT
public:
    Interface(bool &exit);
    ~Interface();
    bool Test();
    void Init();

public slots:
    void ShowInfo(QString classText,QString text);
    void ReceiptDataFromServer(QString client, QString data);
    void StartUpdate();

private:
    QString isError(bool test) { return test ? QString("OK") : QString("Error"); }

    QString ReadData(QString data, int level);
    CVOrder *cvOrder;
    ServerFire *server;
    bool _log = false;
    QString _linkLog;
    QTimer _update;
    bool _allDataTransmitted = true;
};

#endif // INTERFACE_H
