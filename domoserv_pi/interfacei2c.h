#ifndef INTERFACEI2C_H
#define INTERFACEI2C_H

#include <QObject>
#include <QSqlQuery>
#include <QVariant>
#include <QProcess>
#include <QDesktopServices>
#include <QJsonDocument>
#include <QJsonObject>
#include <../dep/WiringPi/wiringPi/wiringPi.h>
#include <../dep/WiringPi/wiringPi/pcf8574.h>
#include <../dep/WiringPi/wiringPi/wiringPiI2C.h>
#include <iostream>
#include <QDebug>
#include <QEventLoop>
#include <QTimer>
#include <QUrl>
#include <QFile>

#define Z1      0
#define Z2      1
#define CONFORT 0
#define ECO     1
#define HG      2
#define ON      0
#define OFF     1

class InterfaceI2C : public QObject
{
    Q_OBJECT

    QStringList selectScreen = { "temp.py","z1.py","z2.py","version.py" };

public:
    enum selectScreen {
        temp,
        z1,
        z2,
        version
    };

    explicit InterfaceI2C(QObject *parent = nullptr);
    ~InterfaceI2C();
    static void TestOutput();
    void TestInput();
    void SetOutput(int state, int zone);
    void StartInput(int input);
    void InitTemp();
    QMap<QString,double> GetTemp();
    int GetHumidity();
    void UpdateScreen(QStringList data = QStringList());
    int KillPID();
    void PIDFinished();
    void InitScreen();
    void ChangeScreen();
    void UpdatingData(bool close = false);
    void setVersion(QString version) { appVersion = version; }
    void setScanZone(int zone, bool scan);

private slots:
    void RunTemp();
    void ControlInput();
    void isInputPressed();
    void processFinished();
    void restartScreen();
    void mainScreen(){ screenSelected = 3; ChangeScreen(); }

signals:
    void Info(QString className, QString text);
    void InputPressed(int input, int screen);

private:
    bool screen;
    bool temperature;
    bool activated;
    QMap<QString,double> bme280;
    int screenSelected;
    int stateZ1;
    int stateZ2;
    QString appVersion;
    bool scanZ1;
    bool scanZ2;


};

#endif // INTERFACEI2C_H
