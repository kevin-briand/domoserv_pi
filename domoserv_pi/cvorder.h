#ifndef CVORDER_H
#define CVORDER_H

#include <QObject>
#include <QtNetwork>
#include <QSqlDatabase>
#include <QSqlQuery>
extern "C" {
#include </usr/local/include/wiringPi.h>
}

//State
#define ON  0
#define OFF 1

//Zone
#define Z1  0
#define Z2  1

//Digital IO
#define Z1ECO   0
#define Z1HG    1
#define Z2ECO   2
#define Z2HG    3

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
    void ChangeOrder(int order, int zone);
    void InitProg();

private slots:
    void ReceiptDataFromUser(QTcpSocket *user, QString data);
    void ResetOutputState();
    void NextProgram(int zone);
    void RunChangeOrder();

private:
    void SetOutputState(int digitalIO,int state);
    int _CVStateZ1 = 0;
    int _CVStateZ2 = 0;
    QList<QDateTime> _horaireZ1;
    QList<QDateTime> _horaireZ2;
    QTimer _timerZ1;
    QTimer _timerZ2;

};

#endif // CVORDER_H
