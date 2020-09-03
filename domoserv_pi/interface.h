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
#include <../../ServerFire/src/serverfire.h>

#define className "General"

class Interface : public QObject
{
    Q_OBJECT
public:
    Interface(bool &exit);
    //~Interface();
    bool Test();
    void Init();

public slots:
    void ShowInfo(QString classText,QString text);
    void ReceiptDataFromServer(QString client, QString data);
    void StartUpdate();

private:
    QString ReadData(QString data, int level);
    CVOrder *cvOrder;
    ServerFire *server;
    bool _log = false;
    QString _linkLog;
    QTimer _update;
    bool _allDataTransmitted = true;
};

#endif // INTERFACE_H
