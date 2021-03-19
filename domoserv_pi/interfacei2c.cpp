#include "interfacei2c.h"

#define className "I2C"
#define Z1CONFORT 100
#define Z1ECO     101
#define Z1HG      102
#define Z2CONFORT 103
#define Z2ECO     104
#define Z2HG      105
#define SELECT    106
#define MODE      107


InterfaceI2C::InterfaceI2C(QObject *parent) : QObject(parent)
{
    activated = false;
    screen = false;
    screenSelected = temp;
    stateZ1 = CONFORT;
    stateZ2 = CONFORT;
    scanZ1 = false;
    scanZ2 = false;

    QSqlQuery req;
    req.exec("SELECT * FROM General WHERE Name='I2C'");
    if(!req.next()) {
        req.exec("SELECT MAX(ID) FROM General");
        req.next();
        int id = req.value(0).toInt() + 1;
        req.exec(QString("INSERT INTO General VALUES('" + QString::number(id) + "','I2C','0','32','','')"));
        id++;
        req.exec(QString("INSERT INTO General VALUES('" + QString::number(id) + "','I2CScreen','0','','','')"));
        id++;
        req.exec(QString("INSERT INTO General VALUES('" + QString::number(id) + "','I2CTemp','0','','','')"));
    }
    else {
        if(req.value("Value1").toBool() == true) {
            wiringPiSetup();

            int fd = pcf8574Setup(100, req.value("Value2").toByteArray().toInt());
            if (fd == -1) {
                emit Info(className, "Failed to init I2C communication.");
                return;
            }
            emit Info(className, "I2C communication successfully setup.");

            for (int i = 0; i < 6; ++i) {
                pinMode(100 + i, OUTPUT);
                digitalWrite(100 + i,1);
            }
            StartInput(SELECT);
            StartInput(MODE);

            req.exec("SELECT * FROM General WHERE Name='I2CScreen'");
            req.next();
            if(req.value("Value1").toBool()) {
                InitScreen();
            }
            req.exec("SELECT * FROM General WHERE Name='I2CTemp'");
            req.next();
            if(req.value("Value1").toBool()) {
                InitTemp();
            }
            activated = true;
        }
    }
}

InterfaceI2C::~InterfaceI2C()
{
    activated = false;
    screen = false;
    QProcess *p = this->findChild<QProcess*>("Screen");
    if(p) {
        p->close();
        p->deleteLater();
    }
}

void InterfaceI2C::TestOutput()
{
    int out = 0;
    while(1) {
    for (int i = 0; i < 8; ++i) {
        digitalWrite(100 + i, out);
        QEventLoop l;
        QTimer t;
        connect(&t, &QTimer::timeout, &l, &QEventLoop::quit);
        t.start(1000);
        l.exec();
    }
    out = (out == 0) ? 1 : 0;
    }
}

void InterfaceI2C::TestInput()
{
    //si P0 actif, P1 allumé sinon P1 éteint
    QTimer *t = new QTimer(this);
    t->setSingleShot(false);
    connect(t,&QTimer::timeout,this,&InterfaceI2C::ControlInput);
    t->setInterval(30);
    t->start();
}

void InterfaceI2C::ControlInput()
{
    if(digitalRead(100)) digitalWrite(101, 0);
    else digitalWrite(101, 1);
}

void InterfaceI2C::SetOutput(int state, int zone)
{
    int minPin = Z1CONFORT;
    if(zone == Z2)
        minPin = Z2CONFORT;
    for(int i = 0; i < 3; i++)
        digitalWrite(minPin + i,OFF);

    int output = -1;
    switch (zone) {
       case Z1:
        stateZ1 = state;
        switch (state) {
        case CONFORT:
           output = Z1CONFORT;
            break;
        case ECO:
            output = Z1ECO;
            break;
        case HG:
            output = Z1HG;
        }
        break;
    case Z2:
        stateZ2 = state;
        switch (state) {
        case CONFORT:
            output = Z2CONFORT;
            break;
        case ECO:
            output = Z2ECO;
            break;
        case HG:
            output = Z2HG;
        }
    }
    if(output == -1)
        return;

    qDebug() << output;
    digitalWrite(output, ON);
    UpdatingData();
}

void InterfaceI2C::setScanZone(int zone, bool scan)
{
    switch (zone) {
    case 0:
        scanZ1 = scan;
        break;
    case 1:
        scanZ2 = scan;
    }
    UpdatingData();
}

void InterfaceI2C::StartInput(int input)
{
    if(input == Z1CONFORT || input == Z1ECO || input == Z1HG || input == Z2CONFORT || input == Z2ECO || input == Z2HG) {
        emit Info(className,"error : already used on output");
        return;
    }

    pinMode(input,OUTPUT);

    QTimer *t = new QTimer(this);
    t->setSingleShot(false);
    t->setObjectName(QString::number(input));
    connect(t,&QTimer::timeout,this,&InterfaceI2C::isInputPressed);
    t->setInterval(30);
    t->start();
}

void InterfaceI2C::isInputPressed()
{
    QTimer *t = qobject_cast<QTimer*>(sender());
    if(!t) return;

    int input = t->objectName().toInt();
    if(input >= 100) {
        if(digitalRead(input) && !t->property("pressed").toBool()) {
            t->setProperty("pressed",true);
            emit InputPressed(input,screenSelected);
            if(input == SELECT) {
                qDebug() << "input SELECT pressed !";
                ChangeScreen();
            }
            else if(input == MODE) {
                qDebug() << "input MODE pressed !";
            }
        }
        else if(!digitalRead(input)) {
            t->setProperty("pressed",false);
        }
    }
}

void InterfaceI2C::InitTemp()
{
    QTimer *t = new QTimer(this);
    t->setSingleShot(false);
    connect(t,&QTimer::timeout,this,&InterfaceI2C::RunTemp);
    t->setInterval(5000);
    t->start();
}

void InterfaceI2C::RunTemp()
{
    QProcess *p = new QProcess(this);
    p->setObjectName("temp");
    connect(p,QOverload<int,QProcess::ExitStatus>::of(&QProcess::finished),this,&InterfaceI2C::processFinished);

    p->start("bme280");
}

QMap<QString,double> InterfaceI2C::GetTemp()
{
    return bme280;
}

void InterfaceI2C::processFinished()
{
    QByteArray data = this->findChild<QProcess*>("temp")->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);

    QMap<QString,double> value;
    value.insert("temperature",doc.object()["temperature"].toDouble());
    value.insert("humidity",doc.object()["humidity"].toDouble());

    bme280 = value;
    this->findChild<QProcess*>("temp")->deleteLater();

    QStringList list;
    list.append(QString::number(value.value("temperature")));
    list.append(QString::number(value.value("humidity")));

    UpdatingData();
}

void InterfaceI2C::InitScreen()
{
    QProcess *p = new QProcess(this);
    connect(p,QOverload<int,QProcess::ExitStatus>::of(&QProcess::finished),this,&InterfaceI2C::restartScreen);
    p->setObjectName("Screen");
    p->start("python3 /home/pi/domoserv_pi/build/" + selectScreen.at(screenSelected));

    screen = true;
}

void InterfaceI2C::ChangeScreen()
{
    QProcess *p = this->findChild<QProcess*>("Screen");
    screen = false;
    if(screenSelected >= 3)
        screenSelected = 0;
    else
        screenSelected++;

    if(p) {
        QTimer *t = this->findChild<QTimer*>("mainScreen");
        if(!t) { 
            t = new QTimer(this);
            t->setObjectName("mainScreen");
            connect(t,&QTimer::timeout,this,&InterfaceI2C::mainScreen);
        }
        t->stop();

        if(screenSelected != 0)
            t->start(30000);

        screen = true;
    }
    UpdatingData();
}

void InterfaceI2C::restartScreen()
{
    QProcess *p = this->findChild<QProcess*>("Screen");
    if(p && screen) {
        p->start("python3 /home/pi/domoserv_pi/build/" + selectScreen.at(screenSelected));
    }
}

void InterfaceI2C::UpdatingData()
{
    QFile f("/home/pi/domoserv_pi/build/temp.txt");
    f.resize(0);
    if(!f.open(QIODevice::WriteOnly)) return;
    QTextStream str(&f);
    str << "temp=" << bme280.value("temperature") << endl;
    str << "hum=" << bme280.value("humidity") << endl;
    str << "screen=" << screenSelected << endl;
    str << "z1=" << stateZ1 << endl;
    str << "z2=" << stateZ2 << endl;
    str << "scanZ1=" << int(scanZ1) << endl;
    str << "scanZ2=" << int(scanZ2) << endl;
    str << "version=" << appVersion;
    f.close();
}

int InterfaceI2C::KillPID()
{
    QProcess *p = new QProcess(this);
    p->setObjectName("kill");
    connect(p,QOverload<int,QProcess::ExitStatus>::of(&QProcess::finished),this,&InterfaceI2C::PIDFinished);
    p->start("ps aux");
}

void InterfaceI2C::PIDFinished()
{
    QStringList data = QString(this->findChild<QProcess*>("kill")->readAll()).split("\n");
    QString result;
    if(!data.isEmpty()) {
        for(QString value : data) {
            if(value.contains("screen")) {
                for(QString val : value.split(" ")) {
                    if(!val.isEmpty() && val != "pi" && val != "root") {
                        result = val;
                        break;
                    }
                }
            }
        }
        if(!result.isEmpty())
            this->findChild<QProcess*>("kill")->start("kill " + result);
    }
    if(result.isEmpty()) {
        this->findChild<QProcess*>("kill")->deleteLater();
    }
}
