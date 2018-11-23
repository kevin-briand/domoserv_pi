#ifndef CVORDER_H
#define CVORDER_H

#include <QObject>
#include <QtNetwork>
#include <../../../wiringPi-HEAD-8d188fa/wiringPi/wiringPi.h>


//State
#define ON  1
#define OFF 0

//Digital IO
#define CONFORT 0
#define ECO     1
#define HG      2

enum order{
    confort,
    eco,
    horsGel
};

class CVOrder : public QObject
{
    Q_OBJECT
public:
    CVOrder();
    void ChangeOrder(int order);

public slots:
    void ReceiptDataFromUser(QTcpSocket *user, QString data);

private:
    void SetOutputState(int digitalIO,int state);
    int CVState = 0;


};

#endif // CVORDER_H
