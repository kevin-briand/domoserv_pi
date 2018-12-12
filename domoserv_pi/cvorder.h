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

//Zone
#define Z1  0
#define Z2  1

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
    void Reload();
    void Init();
    void ChangeOrder(int order, int zone);
    void InitProg();
    void AddProg(int zone, int state, QString date);
    void RemoveProg(int zone, QString date = 0);
    void SetPriority(int priority);
    void SetProg(QString date, int zone, int state);
    QString GetProg();
    QString GetConfig();
    void AddIp(QString ip);
    void RemoveIp(QString ip);
    void SetTimerNetwork(int timer);
    int GetGPIO(int pin);
    void SetGPIO(int pin, int newPin);
    void ReverseGPIO(bool reverse);

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
    int _priority = 0;
    int _z1Eco = 0;
    int _z1Hg = 1;
    int _z2Eco = 2;
    int _z2Hg = 3;
    int _on = 1;
    int _off = 0;
};

#endif // CVORDER_H
