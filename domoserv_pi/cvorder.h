#ifndef CVORDER_H
#define CVORDER_H

#include <QObject>
#include <QtNetwork>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QProcess>
#include <QtDBus/QtDBus>
#include <interfacei2c.h>

#ifndef Q_OS_WIN
    #include <../dep/WiringPi/wiringPi/wiringPi.h>
    #define ACT_WIRING_PI
#endif

#include <interfacei2c.h>

//Zone
#define Z1  0
#define Z2  1
#define frostFree 2

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
    ReverseOnOff,
    ImpCPTEnergy,
    HCCPTEnergy
};

enum Status{
    Automatic,
    SemiAutomatic,
    Manual
};

enum Emplacement{
    Indoor,
    Outdoor
};

class CVOrder : public QObject
{
    Q_OBJECT
public:
    CVOrder();
    ~CVOrder();
    void Init();
    void InitProg();
    void RemoveProg(int zone, QString date = nullptr);
    void SetPriority(int priority);
    static QString SetProg(QString date, int zone, int state);
    QString GetProg();
    QString GetConfig();
    static QString AddIp(QString ip);
    void RemoveIp(QString ip);
    void SetTimerNetwork(int timer);
    int GetGPIO(int pin);
    void SetGPIO(int pin, int newPin);
    void ReverseGPIO(bool reverse);
    int GetOrder(int zone);
    int GetStatus(int zone);
    void SetStatus(int status, int zone);
    void SetOrder(int order, int zone);
    void ABS(int day);
    int GetABS();
    QString GetLog();
    int GetRemainingTime(int zone);
    int GetImpWatt();
    void SetImpWatt(int watt);
    void UseHCCPTEnergy(bool value);
    void UseCPTEnergy(bool value);
    void StopCPTEnergy();
    QString GetDataCPTEnergy(QDate first, QDate end);
    void InitTemp();
    QString GetDataOrder(QDate first, QDate end);
    QString GetDataTemp(QDate first, QDate end);
    QString GetTemp(int emp);

public slots:
    void Reload();

private slots:
    void ResetOutputState();
    void NextProgram(int zone);
    void RunChangeOrder();
    bool PingNetwork();
    void AddImp();
    void TestInput();
    void AddTempToFile();
    void I2CInputPressed(int intput, int screenSelected);

signals:
    void Info(QString textClass, QString text);

private:
    QSqlQuery request(int type, QString arg1, QString arg2, QString arg3 = nullptr, QString arg4 = nullptr, QString arg5 = nullptr);
    void SetOutputState(int digitalIO,int state);
    void ChangeOrder(int order, int zone);
    void InitCPTEnergy();
    bool ReadNetwork();
    QString StateToString(int state);
    int DayToInt(QString day);
    int _CVStateZ1 = 0;
    int _CVStateZ2 = 0;
    int _lastStateZ1 = 0;
    int _lastStateZ2 = 0;
    int _StatusZ1 = 0;
    int _StatusZ2 = 0;
    QTimer *_timerZ1;
    QTimer *_timerZ2;
    QTimer *_timerPing;
    QTimer *_abs;
    int _priority = 0;
    int _z1Eco = 0;
    int _z1Hg = 1;
    int _z2Eco = 2;
    int _z2Hg = 3;
    int _on = 1;
    int _off = 0;
    int _ImpCPTEnergy = 4;
    int _HCCPTEnergy = 5;
    int _WattCPTEnergy = 1;
    bool _endABS = false;
    bool _activateClass = false;
    QString _linkHistory;
    QString _pathEnergy;
    QTimer *_timerReadInput;
    QTimer _timerReadTemp;
    QString _pathTemp;
    QSqlDatabase dbData;
};

#endif // CVORDER_H
