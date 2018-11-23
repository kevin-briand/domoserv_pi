#ifndef INTERFACE_H
#define INTERFACE_H

#include <QObject>
#include <cvorder.h>
#include <server.h>
#include <QtNetwork>

class Interface : public QObject
{
    Q_OBJECT
public:
    Interface();
    bool Test();

public slots:
    void pong(quint64 msec);
};

#endif // INTERFACE_H
