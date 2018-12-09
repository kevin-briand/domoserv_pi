#ifndef CVORDER_H
#define CVORDER_H

#include <QObject>
#include <QtNetwork>
#include <QSqlDatabase>
#include <QSqlQuery>
extern "C" {
#include </usr/local/include/wiringPi.h>
#include <QProcess>
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

enum priority{
    horloge,
    network,
    networkAndHorloge
};

enum GPIO{
    Z1Eco,
    Z1Hg,
    Z2Eco,
    Z2Hg,
    ReverseOnOff
};

class CVOrder : public QObject
{
    Q_OBJECT
public:
    CVOrder();
    void Init();
    void ChangeOrder(int order, int zone);
    void InitProg();
    void AddProg(int zone, int state, QString date);
    void RemoveProg(int zone, QString date);
    void SetPriority(int priority);
    void SetProg(QString date, int zone, int state);
    QString GetProg();
    QString GetConfig();
    void AddIp(QString ip);
    void RemoveIp(QString ip);
    void SetTimerNetwork(int timer);
    int GetGPIO(int pin);

private slots:
    void ReceiptDataFromUser(QTcpSocket *user, QString data);
    void ResetOutputState();
    void NextProgram(int zone);
    void RunChangeOrder();
    bool PingNetwork();

signals:
    void Info(QString textClass, QString text);

private:
    void SetOutputState(int digitalIO,int state);
    int _CVStateZ1 = 0;
    int _CVStateZ2 = 0;
    QTimer _timerZ1;
    QTimer _timerZ2;
    QTimer _timerPing;
    int _priority;
};

#endif // CVORDER_H
