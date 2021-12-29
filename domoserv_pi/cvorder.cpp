#include "cvorder.h"

CVOrder::CVOrder()
{
    #define className "chauffage"
}

void CVOrder::Reload()
{
    _timerZ1->stop();
    _timerZ2->stop();
    _on = 1;
    _off = 0;
    _z1Eco = 0;
    _z1Hg = 1;
    _z2Eco = 2;
    _z2Hg = 3;
    _priority = 0;
    _StatusZ1 = 0;
    _StatusZ2 = 0;
    _CVStateZ1 = 0;
    _CVStateZ2 = 0;
    _lastStateZ1 = 0;
    _lastStateZ2 = 0;

    InterfaceI2C *i2c = this->findChild<InterfaceI2C*>();
    if(i2c) {
        i2c->deleteLater();
    }

    Init();
}

CVOrder::~CVOrder()
{
    _timerZ1->stop();
    _timerZ1->deleteLater();
    _timerZ2->stop();
    _timerZ2->deleteLater();
    _timerPing->stop();
    _timerPing->deleteLater();
    _abs->stop();
    _abs->deleteLater();
    _timerReadInput->stop();
    _timerReadInput->deleteLater();

    InterfaceI2C *i2c = this->findChild<InterfaceI2C*>();
    i2c->deleteLater();
}

void CVOrder::Init()
{
    //Create Database
    QSqlQuery req;
    bool ret = req.exec("CREATE TABLE CVOrder ('ID' SMALLINT, 'Name' TEXT, 'Value1' TEXT, 'Value2' TEXT, 'Value3' TEXT, 'Value4' TEXT)");
    if(ret)
        emit Info(className,"[\033[0;32m  OK  \033[0m] Table created ");
    if(ret)
    {
        req.exec("SELECT MAX(ID) FROM CVOrder");
        req.next();
        int id = req.value(0).toInt()+1;
        int test = req.exec("INSERT INTO CVOrder VALUES('" + QString::number(id) + "','ActualZ1','','','','')");
        id++;
        int test2 = req.exec("INSERT INTO CVOrder VALUES('" + QString::number(id) + "','ActualZ2','','','','')");
        id++;
        int test3 = req.exec("INSERT INTO CVOrder VALUES('" + QString::number(id) + "','Priority','0','','','')");
        id++;
        int test4 = req.exec("INSERT INTO CVOrder VALUES('" + QString::number(id) + "','Act_Network','0','30000','','')");
        id++;
        int test5 = req.exec("INSERT INTO CVOrder VALUES('" + QString::number(id) + "','StatusZ1','0','','','')");
        id++;
        int test6 = req.exec("INSERT INTO CVOrder VALUES('" + QString::number(id) + "','StatusZ2','0','','','')");

        if(test && test2 && test3 && test4 && test5 && test6)
            emit Info(className,"[\033[0;32m  OK  \033[0m] Rows created ");
        else if(test || test2 || test3 || test4 || test5 || test6)
            emit Info(className,"[\033[0;33m  OK  \033[0m] Rows created ");
        else
            emit Info(className,"[\033[0;31mFAILED\033[0m] Rows not created ");


        id++;
        test = req.exec("INSERT INTO CVOrder VALUES('" + QString::number(id) + "','GPIO','0','0','','')");
        id++;
        test2 = req.exec("INSERT INTO CVOrder VALUES('" + QString::number(id) + "','GPIO','1','1','','')");
        id++;
        test3 = req.exec("INSERT INTO CVOrder VALUES('" + QString::number(id) + "','GPIO','2','2','','')");
        id++;
        test4 = req.exec("INSERT INTO CVOrder VALUES('" + QString::number(id) + "','GPIO','3','3','','')");
        id++;
        test5 = req.exec("INSERT INTO CVOrder VALUES('" + QString::number(id) + "','GPIO','4','1','','')");

        if(test && test2 && test3 && test4 && test5)
            emit Info(className,"[\033[0;32m  OK  \033[0m] GPIO Rows created ");
        else if(test || test2 || test3 || test4 || test5)
            emit Info(className,"[\033[0;33m  OK  \033[0m] GPIO Rows created ");
        else
            emit Info(className,"[\033[0;31mFAILED\033[0m] GPIO Rows not created ");
    }

    req.exec("CREATE TABLE IF NOT EXISTS Temperature ('ID' INT, 'Date' TEXT, 'Time' TEXT, 'Pos' INT, 'Value' REAL)");
    req.exec("CREATE TABLE IF NOT EXISTS State ('ID' INT, 'Date' TEXT, 'Time' TEXT, 'Zone' INT, 'State' INT)");
    req.exec("CREATE TABLE IF NOT EXISTS Energy ('ID' INT, 'Date' TEXT, 'Time' TEXT, 'Wh' INT)");

    _timerZ1 = new QTimer;
    _timerZ2 = new QTimer;
    _timerPing = new QTimer;
    _abs = new QTimer;
    _linkHistory = "/home/pi/domoserv_pi/data/order/";
    QDir dir;
    dir.mkpath(_linkHistory);

    //define GPIO
    req.exec("SELECT * FROM CVOrder WHERE Name='GPIO'");
    while(req.next())
    {
        switch (req.value("Value1").toInt()) {
        case Z1Eco:
            _z1Eco = req.value("Value2").toInt();
            break;
        case Z1Hg:
            _z1Hg = req.value("Value2").toInt();
            break;
        case Z2Eco:
            _z2Eco = req.value("Value2").toInt();
            break;
        case Z2Hg:
            _z2Hg = req.value("Value2").toInt();
            break;
        case ReverseOnOff:
            if(req.value("Value2").toInt() == 1)
            {
                _off = 1;
                _on = 0;
            }
            else
            {
                _off = 0;
                _on = 1;
            }
            break;
        }
    }


    //Init WiringPi
#ifdef ACT_WIRING_PI
    if(wiringPiSetup() < 0)
        emit Info(className,"[\033[0;31mFAILED\033[0m] wiringPi not started");
    else
    {
        //Init pins
        pinMode(_z1Eco,OUTPUT);
        pinMode(_z1Hg,OUTPUT);
        pinMode(_z2Eco,OUTPUT);
        pinMode(_z2Hg,OUTPUT);

        ResetOutputState();

        emit Info(className,"[\033[0;32m  OK  \033[0m] wiringPi started");
    }
#endif

    //Init I2C
    emit Info(className,"Init I2C protocol");
    InterfaceI2C *i2c = new InterfaceI2C(this);
    i2c->setVersion(qApp->applicationVersion());
    connect(i2c,&InterfaceI2C::Info,this,&CVOrder::Info);
    connect(i2c,&InterfaceI2C::InputPressed,this,&CVOrder::I2CInputPressed);

    connect(_timerZ1,SIGNAL(timeout()),_timerZ1,SLOT(stop()));
    connect(_timerZ2,SIGNAL(timeout()),_timerZ2,SLOT(stop()));
    connect(_timerZ1,SIGNAL(timeout()),this,SLOT(RunChangeOrder()));
    connect(_timerZ2,SIGNAL(timeout()),this,SLOT(RunChangeOrder()));
    connect(_timerPing,SIGNAL(timeout()),this,SLOT(RunChangeOrder()));
    connect(_abs,SIGNAL(timeout()),this,SLOT(Reload()));

    //Init Prog
    emit Info(className,"Initialisation programmation...");
    InitProg();

    _activateClass = true;

    InitCPTEnergy();
    InitTemp();
}

void CVOrder::I2CInputPressed(int input,int screenSelected)
{
    if(input == 107) {
        if(screenSelected == InterfaceI2C::z1) {
            SetOrder(GetOrder(Z1) == 0 ? 1 : 0,Z1);
        }
        else if(screenSelected == InterfaceI2C::z2) {
            SetOrder(GetOrder(Z2) == 0 ? 1 : 0,Z2);
        }
    }
}

void CVOrder::SetOutputState(int digitalIO, int state)
{
#ifdef ACT_WIRING_PI
        digitalWrite(digitalIO,state);
#endif
}

void CVOrder::RunChangeOrder()
{
    QTimer *t = qobject_cast<QTimer*>(sender());

    if(!t)
        return;

    if(_priority == horloge)//------------------------------
    {
        int zone = Z1;
        if(_timerZ2 == t)
            zone = Z2;

        //Récupération appartenance timer
        int state = t->property("state").toInt();
        //Changement de l'état
        ChangeOrder(state,zone);
        //Redémarrage timer
        NextProgram(zone);
    }
    else//---------------------------
    {
        if(_timerPing == t)
        {
            if(PingNetwork())
            {
                //I2C getScan
                InterfaceI2C *i2c = this->findChild<InterfaceI2C*>();
                if(i2c) {
                    i2c->setScanZone(Z1,false);
                    i2c->setScanZone(Z2,false);
                }

                if(_lastStateZ1 == confort)
                    ChangeOrder(confort,Z1);
                else if(_priority == networkAndHorloge)
                    ChangeOrder(eco,Z1);

                if(_lastStateZ2 == confort)
                    ChangeOrder(confort,Z2);
                else if(_priority == networkAndHorloge)
                    ChangeOrder(eco,Z2);

                if(_priority == networkAndHorloge)
                    _timerPing->stop();
            }
        }
        else if(_timerZ1 == t || _timerZ2 == t)
        {
            int zone = Z1;
            if(_timerZ1 == t)
                _lastStateZ1 = _timerZ1->property("state").toInt();
            else
            {
                _lastStateZ2 = _timerZ2->property("state").toInt();
                zone = Z2;
            }
            if(t->property("state").toInt() == confort)
            {
                QSqlQuery req;
                req.exec("SELECT * FROM CVOrder WHERE Name='Act_Network'");
                if(req.next())
                {
                    //I2C getScan
                    InterfaceI2C *i2c = this->findChild<InterfaceI2C*>();
                    if(i2c) i2c->setScanZone(zone,true);

                    _timerPing->setSingleShot(false);
                    _timerPing->start(req.value("Value2").toInt());
                }
                else
                    emit Info(className,tr("_timerPing not started(Act_Network not found)"));
            }
            else
            {
                ChangeOrder(t->property("state").toInt(),zone);
                _timerPing->stop();
            }
            NextProgram(zone);
        }
    }
}

bool CVOrder::ReadNetwork()
{
    QProcess proc;
    QSqlQuery req;
    QStringList ip;
    req.exec("SELECT * FROM CVOrder WHERE Name='IpPing'");
    while(req.next())
        ip.append(req.value("Value1").toString());

    bool success(false);
    for(int i=0;i<ip.count();i++)
    {
        proc.start("ping -c 5 " + ip.at(i));
        proc.waitForFinished();
        QByteArray ba = proc.readAll();
        QString result = ba;
        QStringList result2 = result.split("\n");

        for(int i2=0;i2<result2.count();i2++)//read output
            if(result2.at(i2).contains("packets transmitted"))
                if(result2.at(i2).split(" ").at(3).toInt() > 0)//host connected
                {
                    success = true;
                    emit Info(className,ip.at(i) + " found on network");
                }
    }
    return success;
}

void CVOrder::ChangeOrder(int order,int zone)
{
    int output = 0;
    QString zoneSelect;
    QString nameActualOrder;
    QString nameNewOrder;
    if(zone == Z1)
    {
        zoneSelect = "zone 1";
        SetOutputState(_z1Eco,_off);
        SetOutputState(_z1Hg,_off);

        switch(order) {
        case eco:
            output = _z1Eco;
            break;
        case horsGel:
            output = _z1Hg;
        }

        nameActualOrder = StateToString(_CVStateZ1);
        _CVStateZ1 = order;
    }
    else if(zone == Z2)
    {
        zoneSelect = "zone 2";
        SetOutputState(_z2Eco,_off);
        SetOutputState(_z2Hg,_off);

        switch(order) {
        case eco:
            output = _z2Eco;
            break;
        case horsGel:
            output = _z2Hg;
        }

        nameActualOrder = StateToString(_CVStateZ2);
        _CVStateZ2 = order;
    }
    else
    {
        emit Info(className,"Error bad zone " + QString::number(zone+1));
        return;
    }

    if(order > horsGel)
        return;
    nameNewOrder = StateToString(order);

    //Set Output
    if(order != confort)
        SetOutputState(output,_on);


    //I2C
    InterfaceI2C *i2c = this->findChild<InterfaceI2C*>();
    if(i2c) i2c->SetOutput(order,zone);

    //history
    QSqlQuery req;
    req.exec("SELECT MAX(ID) FROM State");
    req.next();
    int id = req.value(0).toInt()+1;
    req.exec("INSERT INTO State VALUES('" + QString::number(id) + "','" + QDate::currentDate().toString("yyyy-MM-dd") + "','" +
             QTime::currentTime().toString("hh:mm") + "','" + QString::number(zone) + "','" + QString::number(order) + "')");

    //save state
    req.exec("UPDATE CVOrder SET Value1='" + QString::number(order) + "' WHERE Name='ActualZ" + QString::number(zone+1) + "'");

    emit Info(className,"Change order " + nameActualOrder.toLatin1() + " to " + nameNewOrder.toLatin1() + " in " + zoneSelect.toLatin1());
}

QString CVOrder::StateToString(int state)
{
    switch(state) {
    case confort:
        return "Confort";
    case eco:
        return "Eco";
    case horsGel:
        return "Hors gel";
    default:
        return "Bad state";
    }
}

void CVOrder::ResetOutputState()
{
    SetOutputState(_z1Eco,_off);
    SetOutputState(_z2Eco,_off);
    SetOutputState(_z1Hg,_off);
    SetOutputState(_z2Hg,_off);
}

void CVOrder::InitProg()
{
    QSqlQuery req;
    req.exec("SELECT * FROM CVOrder WHERE Name='ActualZ1'");
    if(req.next())
        _lastStateZ1 = req.value("Value1").toInt();

    req.exec("SELECT * FROM CVOrder WHERE Name='ActualZ2'");
    if(req.next())
        _lastStateZ2 = req.value("Value1").toInt();

    //ABS
    if(_endABS)
    {
        QDateTime dt;
        int day = DayToInt(dt.currentDateTime().toString("ddd"));

        //find next hour
        QDate date;
        date.setDate(2018,1,day);
        QDateTime actual;
        actual.setDate(date);
        actual.setTime(QTime::currentTime());

        //Z1
        req.exec("SELECT * FROM CVOrder WHERE Value1 < '" + actual.toString("yyyy-MM-dd hh:mm") + "' AND Name='Prog' AND Value2='" + QString::number(Z1) + "' ORDER BY Value1 ASC");
        while(req.next());
        if(req.previous())
            _lastStateZ1 = req.value("Value3").toInt();
        else
            _lastStateZ1 = confort;

        //Z2
        req.exec("SELECT * FROM CVOrder WHERE Value1 < '" + actual.toString("yyyy-MM-dd hh:mm") + "' AND Name='Prog' AND Value2='" + QString::number(Z2) + "' ORDER BY Value1 ASC");
        while(req.next());
        if(req.previous())
            _lastStateZ2 = req.value("Value3").toInt();
        else
            _lastStateZ2 = confort;

        _endABS = false;
    }

    req.exec("SELECT * FROM CVOrder WHERE Name='Priority'");
    if(req.next())
        _priority = req.value("Value1").toInt();
    req.exec("SELECT * FROM CVOrder WHERE Name='StatusZ1'");
    if(req.next())
        _StatusZ1 = req.value("Value1").toInt();
    req.exec("SELECT * FROM CVOrder WHERE Name='StatusZ2'");
    if(req.next())
        _StatusZ2 = req.value("Value1").toInt();


    if(_StatusZ1 == Automatic)
    {
        NextProgram(Z1);
        ChangeOrder(_lastStateZ1,Z1);
    }
    else {
        req.exec("SELECT Value1 FROM CVOrder WHERE Name='ActualZ1'");
        req.next();
        ChangeOrder(req.value(0).toInt(),Z1);
    }
    if(_StatusZ2 == Automatic)
    {
        NextProgram(Z2);
        ChangeOrder(_lastStateZ2,Z2);
    }
    else {
        req.exec("SELECT Value1 FROM CVOrder WHERE Name='ActualZ2'");
        req.next();
        ChangeOrder(req.value(0).toInt(),Z2);
    }
}

int CVOrder::DayToInt(QString day)
{
    if(day == "Mon" || day == "lun.")
        return 1;
    else if(day == "Tue" || day == "mar.")
        return 2;
    else if(day == "Wed" || day == "mer.")
        return 3;
    else if(day == "Thu" || day == "jeu.")
        return 4;
    else if(day == "Fri" || day == "ven.")
        return 5;
    else if(day == "Sat" || day == "sam.")
        return 6;
    else if(day == "Sun" || day == "dim.")
        return 7;
    else
        return -1;
}

void CVOrder::NextProgram(int zone)
{
    QDateTime dt;
    int day = DayToInt(dt.currentDateTime().toString("ddd"));

    //find next hour
    QDate date;
    date.setDate(2018,1,day);
    QDateTime actual;
    actual.setDate(date);
    actual.setTime(QTime::currentTime());


    QSqlQuery req;
    QStringList fDate;
    req.exec("SELECT * FROM CVOrder WHERE Value1 > '" + actual.toString("yyyy-MM-dd hh:mm") + "' AND Name='Prog' AND Value2='" + QString::number(zone) + "' ORDER BY Value1 ASC");
    if(req.next())
    {
        fDate.append(req.value("Value1").toString());
        fDate.append(req.value("Value3").toString());
    }
    else
    {
        req.exec("SELECT * FROM CVOrder WHERE Value2='" + QString::number(zone) + "' AND Name='Prog' ORDER BY Value1 ASC");
        if(req.next())
        {
            fDate.append(req.value("Value1").toString());
            fDate.append(req.value("Value3").toString());
        }
    }
    if(fDate.isEmpty())
    {
        emit Info(className,"no prog found in zone " + QString::number(zone+1).toLatin1());
        return;
    }

    //Day
    int tDay(0);
    int fDay = fDate.at(0).split(" ").at(0).split("-").last().toInt();
    if(fDay > day)
        tDay = fDay - day;
    else if(fDay < day)
        tDay = 7 - day + fDay;

    //hour
    int tHour(0);
    int fHour = fDate.at(0).split(" ").last().split(":").at(0).toInt();
    tHour = fHour - QTime::currentTime().hour();

    //minutes
    int tMinute(0);
    int fMinute = fDate.at(0).split(" ").last().split(":").last().toInt();
    if(fMinute > QTime::currentTime().minute())
        tMinute = fMinute - QTime::currentTime().minute();
    else if(fMinute < QTime::currentTime().minute())
    {
        tMinute = 60 - (QTime::currentTime().minute() - fMinute);
        tHour--;
    }

    if(tDay == 0 && tHour < 0)
        tDay = 7;

    //Total in secondes
    int tSec = ((tDay * 24 + tHour) * 60 + tMinute) * 60;

    if(zone == Z1)
    {
        _timerZ1->setProperty("state",fDate.at(1).toInt());
        _timerZ1->start(tSec * 1000);
        req.exec("UPDATE FROM CVOrder SET Value1='" + fDate.at(0) + "', Value2='" + QString::number(zone) + "',Value3='" + fDate.at(1) + "',Value4='" +
                 QString::number(_CVStateZ1) + "' WHERE Name='ActualZ1'");
    }
    else if(zone == Z2)
    {
        _timerZ2->setProperty("state",fDate.at(1).toInt());
        _timerZ2->start(tSec * 1000);
        req.exec("UPDATE FROM CVOrder SET Value1='" + fDate.at(0) + "', Value2='" + QString::number(zone) + "',Value3='" + fDate.at(1) + "',Value4='" +
                 QString::number(_CVStateZ2) + "' WHERE Name='ActualZ2'");
    }
    emit Info(className,"Next prog in zone " + QString::number(zone+1).toLatin1() + " in " + QString::number(tSec).toLatin1() + " secondes");
    emit Info(className,"DEBUG : " + QString::number(tDay) + " " + QString::number(tHour) + " " + QString::number(tMinute));
}

void CVOrder::RemoveProg(int zone, QString date)
{
    QSqlQuery req;
    if(date.isEmpty())
    {
        req.exec("DELETE FROM CVOrder WHERE Name='Prog'");
        emit Info(className,"remove all prog in zone 1 and zone 2");
    }
    else
    {
        req.exec("DELETE FROM CVOrder WHERE Value2='" + QString::number(zone) + "' AND Name='Prog' AND Value1='" + date + "'");
        emit Info(className,"remove prog " + date.toLatin1() + " in zone " + QString::number(zone+1).toLatin1());
    }
}

bool CVOrder::PingNetwork()
{
    QProcess proc;
    QSqlQuery req;
    QStringList ip;
    req.exec("SELECT * FROM CVOrder WHERE Name='IpPing'");
    while(req.next())
        ip.append(req.value("Value1").toString());

    bool success(false);
    for(int i=0;i<ip.count();i++)
    {
        proc.start("ping -c 2 " + ip.at(i));
        proc.waitForFinished();
        QByteArray ba = proc.readAll();

        success = (QString(ba).indexOf("ttl") >= 0) ? true : false;

        if(success)
            emit Info(className,ip.at(i) + " found on network");
    }
    return success;
}

QString CVOrder::SetProg(QString date, int zone, int state)
{
    if(date.split("-").count() == 3 && date.split(":").count() == 2)
        if(date.split("-").at(0) == "2018" && date.split("-").at(1) == "01" &&
                date.split("-").at(2).split(" ").first().toInt() > 0 && date.split("-").at(2).split(" ").first().toInt() < 8)
            if(zone >= 0 && zone <= 1)
                if(state >= 0 && state <= 2)
                {
                    QSqlQuery req;
                    req.exec("SELECT * FROM CVOrder WHERE Name='Prog' AND Value1='" + date + "' AND Value2='" + QString::number(zone) + "'");
                    if(req.next())
                        return tr("Prog already exist");
                    else
                    {
                        req.exec("SELECT MAX(ID) FROM CVOrder");
                        req.next();
                        int id = req.value(0).toInt()+1;
                        req.exec("INSERT INTO CVOrder VALUES('" + QString::number(id) + "','Prog','" + date + "','" + QString::number(zone) + "','" + QString::number(state) + "','')");
                        return tr("SetProg success");
                    }
                }
                else
                    return tr("SetProg failed(bad state)");
            else
                return tr("SetProg failed(bad zone)");
        else
            return tr("SetProg failed(bad date)");
    else
        return tr("SetProg failed(bad date format)");
}

QString CVOrder::GetProg()
{
    if(!_activateClass)
        return QString("Not Activated !");

    QString result;
    QSqlQuery req;
    req.exec("SELECT * FROM CVOrder WHERE Name='Prog' ORDER BY Value1 ASC");
    while(req.next())
        result += ";" + req.value("Value1").toString() + "#" + req.value("Value2").toString() + "#" + req.value("Value3").toString();
    return result;
}

QString CVOrder::GetConfig()
{
    if(!_activateClass)
        return QString("Not Activated !");

    QSqlQuery req;
    req.exec("SELECT * FROM CVOrder WHERE Name='Priority'");
    req.next();
    QString result = req.value("Name").toString() + "=" + req.value("Value1").toString();

    req.exec("SELECT * FROM CVOrder WHERE Name='Act_Network'");
    req.next();
    result += ";" + req.value("Name").toString() + "=" + req.value("Value1").toString();
    result += ";NetworkTimer=" + req.value("Value2").toString() + ";";

    req.exec("SELECT * FROM CVOrder WHERE Name='IpPing'");
    while(req.next())
        result += ";" + req.value("Name").toString() + "=" + req.value("Value1").toString();

    req.exec("SELECT * FROM CVOrder WHERE Name='ActCPTEnergy'");
    req.next();
    result += ";" + req.value("Name").toString() + "=" + req.value("Value1").toString();

    req.exec("SELECT * FROM CVOrder WHERE Name='ActHCCPTEnergy'");
    req.next();
    result += ";" + req.value("Name").toString() + "=" + req.value("Value1").toString();

    req.exec("SELECT * FROM CVOrder WHERE Name='ImpWattCPTEnergy'");
    req.next();
    result += ";" + req.value("Name").toString() + "=" + req.value("Value1").toString();

    req.exec("SELECT * FROM CVOrder WHERE Name='FileCPTEnergy'");
    req.next();
    result += ";" + req.value("Name").toString() + "=" + req.value("Value1").toString();

    return result;
}

void CVOrder::SetPriority(int priority)
{
    if(priority >= 0 && priority <= 2)
    {
        QSqlQuery req;
        req.exec("UPDATE CVOrder SET Value1='" + QString::number(priority) + "' WHERE Name='Priority'");
        _priority = priority;
        emit Info(className,"Priority set to " + QString::number(priority));
    }
}

QString CVOrder::AddIp(QString ip)
{
    if(ip.split(".").count() == 4)
    {
        unsigned int sIp1 = ip.split(".").at(0).toUInt();
        unsigned int sIp2 = ip.split(".").at(1).toUInt();
        unsigned int sIp3 = ip.split(".").at(2).toUInt();
        unsigned int sIp4 = ip.split(".").at(3).toUInt();
        if(sIp1 <= 255 && sIp2 <= 255 && sIp3 <= 255 && sIp4 <= 255)
        {
            QSqlQuery req;
            req.exec("SELECT * FROM CVOrder WHERE Name='IpPing' AND Value1='" + QString::number(sIp1) + "." + QString::number(sIp2) + "." +
                     QString::number(sIp3) + "." + QString::number(sIp4) + "'");
            if(req.next())
                return "AddIp : Ip already exist";
            else
            {
                req.exec("SELECT MAX(ID) FROM CVOrder");
                req.next();
                int id = req.value(0).toInt()+1;
                req.exec("INSERT INTO CVOrder VALUES('" + QString::number(id) + "','IpPing','" + QString::number(sIp1) + "." + QString::number(sIp2) + "." +
                         QString::number(sIp3) + "." + QString::number(sIp4) + "','','','')");
                return ip + " added";
            }
        }
    }
    return QString("Error : bad ip format");
}

void CVOrder::RemoveIp(QString ip)
{
    QSqlQuery req;
    req.exec("DELETE FROM CVOrder WHERE Name='IpPing' AND Value1='" + ip + "'");
    emit Info(className,ip + " removed");
}

void CVOrder::SetTimerNetwork(int timer)
{
    if(timer <= 0)
    {
        emit Info(className,"SetTimeNetwork bad value");
        return;
    }
    QSqlQuery req;
    req.exec("UPDATE CVOrder SET Value2='" + QString::number(timer*1000) + "' WHERE Name='Act_Network'");
    emit Info(className,"timerNetwork set to " + QString::number(timer));
}

int CVOrder::GetGPIO(int pin)
{
    if(!_activateClass)
        return -1;

    QSqlQuery req;
    req.exec("SELECT Value2 FROM CVOrder WHERE Name='GPIO' AND Value1='" + QString::number(pin) + "'");
    if(!req.next())
        emit Info(className,"GPIO pin not defined");
    else
        return req.value(0).toInt();
    return -1;
}

void CVOrder::SetGPIO(int pin,int newPin)
{
    QSqlQuery req;
    if(pin >= 0 && pin <= 6)
    {
        if(pin == ReverseOnOff)
        {
            ReverseGPIO(newPin);
            return;
        }
        if(newPin >= 0 && newPin <= 27)
        {
            if(req.exec("UPDATE CVOrder SET Value2='" + QString::number(newPin) + "' WHERE Name='GPIO' AND Value1='" + QString::number(pin) + "'"))
                emit Info(className,"GPIO pin " + QString::number(pin) + " set to " + QString::number(newPin));
            else
                emit Info(className,"GPIO pin change failed");
        }
        else
            emit Info(className,"GPIO newPin out of range");
    }
    else
        emit Info(className,"GPIO pin out of range");
}

void CVOrder::ReverseGPIO(bool reverse)
{
    QString result("0");
    if(reverse)
        result = "1";

    QSqlQuery req;
    if(req.exec("UPDATE CVOrder SET Value2='" + result + "' WHERE Name='GPIO' AND Value1='" + QString::number(ReverseOnOff) + "'"))
        emit Info(className,"GPIO Reverse On <-> Off set to true");
    else
        emit Info(className,"GPIO Reverse On <-> Off set to false");
}

int CVOrder::GetOrder(int zone)
{
    if(!_activateClass)
        return -1;

    if(zone == Z1)
        return _CVStateZ1;
    else if(zone == Z2)
        return _CVStateZ2;
    else
        return -1;
}

int CVOrder::GetStatus(int zone)
{
    if(!_activateClass)
        return -1;

    if(zone == Z1)
        return _StatusZ1;
    else if(zone == Z2)
        return _StatusZ2;
    else 
        return -1;
}

void CVOrder::SetOrder(int order, int zone)
{
    if(order < 0 || order > 2)
    {
        emit Info(className,"SetOrder : order out of range");
        return;
    }
    if(zone < 0 || zone > 1)
    {
        emit Info(className,"SetOrder : zone out of range");
        return;
    }
    ChangeOrder(order,zone);
}

void CVOrder::SetStatus(int status, int zone)
{
    if(zone < 0 || zone > 1)
    {
        emit Info(className,"SetStatus : zone out of range");
        return;
    }
    if(status < 0 || status > 2)
    {
        emit Info(className,"SetStatus : status out of range");
        return;
    }

    if(zone == Z1)
    {
        _StatusZ1 = status;
        if(status == Manual)
            _timerZ1->stop();
        else if(status == Automatic)
            NextProgram(Z1);
    }
    else if(zone == Z2)
    {
        _StatusZ2 = status;
        if(status == Manual)
            _timerZ2->stop();
        else if(status == Automatic)
            NextProgram(Z2);
    }


    QSqlQuery req;
    req.exec("UPDATE CVOrder SET Value1='" + QString::number(status) + "' WHERE Name='StatusZ" + QString::number(zone+1) + "'");
}

void CVOrder::ABS(int day)
{
    if(day == 0)
    {
        if(!_abs->isActive())
            return;
        else {
            _abs->stop();
            Reload();
            return;
        }
    }
    else if(day < 1 || day > 30)
    {
        emit Info(className,"ABS : day out of range");
        return;
    }

    int sec = day * 24 * 60 * 60;


    _timerZ1->stop();
    _timerZ2->stop();
    _timerPing->stop();

    SetOrder(horsGel,Z1);
    SetOrder(horsGel,Z2);

    _abs->setSingleShot(true);
    _abs->start(sec * 1000);

    _endABS = true;
}

int CVOrder::GetABS()
{
    if(!_activateClass)
        return -1;

    if(_abs->remainingTime() == 0)
        return 0;
    return _abs->remainingTime() / 1000;
}

QString CVOrder::GetLog()
{
    if(!_activateClass)
        return QString("Not Activated !");

    QFile f(_linkHistory);
    if(f.open(QIODevice::ReadOnly))
        return QString(f.readAll());
    else
        return QString();
}

int CVOrder::GetRemainingTime(int zone)
{
    if(!_activateClass)
        return -1;

    if(zone == Z1)
        return _timerZ1->remainingTime() / 1000;
    else if(zone == Z2)
        return _timerZ2->remainingTime() / 1000;
    else if(zone == frostFree)
        return _abs->remainingTime() / 1000;
    else
        return -1;
}

void CVOrder::InitCPTEnergy()
{
    //Create Rows
    QSqlQuery req;
    req.exec("SELECT * FROM CVOrder WHERE Name='ActCPTEnergy'");
    if(!req.next())
    {
        req.exec("SELECT MAX(ID) FROM CVOrder");
        req.next();
        int id = req.value(0).toInt();
        id++;
        int test = req.exec("INSERT INTO CVOrder VALUES('" + QString::number(id) + "','ActCPTEnergy','0','','','')");
        id++;
        int test2 = req.exec("INSERT INTO CVOrder VALUES('" + QString::number(id) + "','ActHCCPTEnergy','0','','','')");
        id++;
        int test3 = req.exec("INSERT INTO CVOrder VALUES('" + QString::number(id) + "','GPIO','" + QString::number(ImpCPTEnergy) + "','4','','')");
        id++;
        int test4 = req.exec("INSERT INTO CVOrder VALUES('" + QString::number(id) + "','GPIO','" + QString::number(HCCPTEnergy) + "','5','','')");
        id++;
        int test5 = req.exec("INSERT INTO CVOrder VALUES('" + QString::number(id) + "','FileCPTEnergy','/home/pi/domoserv_pi/data/cptenergy/','','','')");
        id++;
        int test6 = req.exec("INSERT INTO CVOrder VALUES('" + QString::number(id) + "','ImpWattCPTEnergy','1','','','')");

        if(test && test2 && test3 && test4 && test5 && test6)
            emit Info(className,"[\033[0;32m  OK  \033[0m] Rows created ");
        else if(test || test2 || test3 || test4 || test5 || test6)
            emit Info(className,"[\033[0;33m  OK  \033[0m] Rows created ");
        else
            emit Info(className,"[\033[0;31mFAILED\033[0m] Rows not created ");
    }

    //Create directory
    req.exec("SELECT Value1 FROM CVOrder WHERE Name='FileCPTEnergy'");
    req.next();
    QDir dir;
    dir.mkpath(req.value(0).toString());
    _pathEnergy = req.value(0).toString();

    //Set Var
    req.exec("SELECT * FROM CVOrder WHERE Name='ActCPTEnergy'");
    req.next();
    if(req.value("Value1").toInt() == 0)
        return;

    req.exec("SELECT * FROM CVOrder WHERE Name='GPIO' AND Value1='" + QString::number(ImpCPTEnergy) + "'");
    req.next();
    _ImpCPTEnergy = req.value("Value2").toInt();

    req.exec("SELECT * FROM CVOrder WHERE Name='GPIO' AND Value1='" + QString::number(HCCPTEnergy) + "'");
    req.next();
    _HCCPTEnergy = req.value("Value2").toInt();

    req.exec("SELECT * FROM CVOrder WHERE Name='ImpWattCPTEnergy'");
    req.next();
    _WattCPTEnergy = req.value("Value1").toInt();


    //Set GPIO
#ifdef ACT_WIRING_PI
    pinMode(_ImpCPTEnergy,INPUT);
    pullUpDnControl(_ImpCPTEnergy,PUD_UP) ;
    req.exec("SELECT * FROM CVOrder WHERE Name='ActHCCPTEnergy'");
    req.next();
    if(req.value("value1").toInt() == 1)
    {
        pinMode(_HCCPTEnergy,INPUT);
        pullUpDnControl(_HCCPTEnergy,PUD_UP) ;
    }
#endif

    //Run timer
    _timerReadInput = new QTimer;
    connect(_timerReadInput,&QTimer::timeout,this,&CVOrder::TestInput);
    _timerReadInput->setSingleShot(false);
    _timerReadInput->start(30);//30ms

    emit Info(className,"[\033[0;32m  OK  \033[0m] Compteur d'énergie activé");
}

void CVOrder::AddImp()
{
    //sauvegarde en fichier toute les 30mn
    static int totalImp = 0;
    static int minute = 0;
    bool save = false;
    QString time = "00";

    int currentMinute = QTime::currentTime().minute();

    if(currentMinute >= 0 && currentMinute <= 29)
    {
        if(minute > 29 && totalImp > 0)
        {
            save = true;
        }
    }
    else if(currentMinute > 29 && currentMinute <= 60)
    {
        if(minute < 30 && totalImp > 0)
        {
            save = true;
            time = "30";
        }  
    }
    if(save)
    {
        int total = totalImp * _WattCPTEnergy;
        QSqlQuery req;
        req.exec("SELECT MAX(ID) FROM Energy");
        req.next();
        int id = req.value(0).toInt()+1;
        req.exec("INSERT INTO Energy VALUES('" + QString::number(id) + "','" + QDate::currentDate().toString("yyyy-MM-dd") + "','" +
                 QTime::currentTime().toString("hh:") + time + "','" + QString::number(total) + "')");
        totalImp = 0;
    }
    minute = currentMinute;
    totalImp++;
}

void CVOrder::TestInput()
{
#ifdef ACT_WIRING_PI
    if(digitalRead(_ImpCPTEnergy) == LOW)
    {
        AddImp();
        while(digitalRead(_ImpCPTEnergy) == LOW)
            delay(10);
    }
#endif
}

int CVOrder::GetImpWatt()
{
    return _WattCPTEnergy;
}

void CVOrder::SetImpWatt(int watt)
{
    if(watt > 0 && watt < 100)
    {
        QSqlQuery req;
        req.exec("UPDATE CVOrder SET Value1='" + QString::number(watt) + "' WHERE Name='ImpWattCPTEnergy'");
        _WattCPTEnergy = watt;
    }
}

void CVOrder::UseCPTEnergy(bool value)
{
    QSqlQuery req;
    if(value)
        req.exec("UPDATE CVOrder SET Value1='1' WHERE Name='ActCPTEnergy'");
    else
        req.exec("UPDATE CVOrder SET Value1='0' WHERE Name='ActCPTEnergy'");
}

void CVOrder::UseHCCPTEnergy(bool value)
{
    QSqlQuery req;
    if(value)
        req.exec("UPDATE CVOrder SET Value1='1' WHERE Name='ActHCCPTEnergy'");
    else
        req.exec("UPDATE CVOrder SET Value1='0' WHERE Name='ActHCCPTEnergy'");
}

void CVOrder::StopCPTEnergy()
{
    _timerReadInput->stop();
}

QString CVOrder::GetDataCPTEnergy(QDate first, QDate end)
{
    QSqlQuery req;
    QString result;

    if(first != end) {
        QDate date(first);
        int day = 0;
        while(date.operator<(end)) {
            req.exec("SELECT * FROM Energy WHERE Date BETWEEN '" + date.toString("yyyy-MM-dd") + "' AND '" + date.toString("yyyy-MM-dd") + "'");
            while(req.next())
            {
                day += req.value(3).toInt();
            }
            result += date.toString("yyyy-MM-dd") + "|00:00|" + QString::number(day) + "\r";
            date = date.addDays(1);
            day = 0;
        }
    }
    else {
        req.exec("SELECT * FROM Energy WHERE Date BETWEEN '" + first.toString("yyyy-MM-dd") + "' AND '" + end.toString("yyyy-MM-dd") + "'");
        while(req.next())
        {
            result += req.value(1).toString() + "|" + req.value(2).toString() + "|" + req.value(3).toString() + "\r";
        }
    }
    return result;
}

void CVOrder::InitTemp()
{
    QSqlQuery req;
    req.exec("SELECT * FROM CVOrder WHERE Name='TempPath'");
    if(!req.next()) {
        req.exec("SELECT MAX(ID) FROM CVOrder");
        req.next();
        int id = req.value(0).toInt()+1;
        req.exec("INSERT INTO CVOrder VALUES('" + QString::number(id) + "','TempPath','/home/pi/domoserv_pi/data/temperature/','','','')");
    }

    req.exec("SELECT Value1 FROM CVOrder WHERE Name='TempPath'");
    req.next();
    _pathTemp = req.value(0).toString();

    emit Info(className,"Lecture température démarré");
    connect(&_timerReadTemp,&QTimer::timeout,this,&CVOrder::AddTempToFile);
    _timerReadTemp.setSingleShot(false);
    _timerReadTemp.start(1800*1000);
}

void CVOrder::AddTempToFile()
{
    int minute = QTime::currentTime().minute();
    QString strMinute;

    if(minute < 30) {
        strMinute = "30";
    }
    else if(minute < 60) {
        strMinute = "00";
    }

    QSqlQuery req;
    req.exec("SELECT * FROM CVOrder WHERE Name='Temp'");
    while(req.next()) {
        QString id = req.value("Value2").toString();
        int emp = req.value("Value1").toInt();


        QFile f("/sys/bus/w1/devices/" + id + "/w1_slave");
        if(!f.open(QIODevice::ReadOnly)) {
            emit Info(className,"Echec d'ouverture du fichier " + f.fileName());
        }
        else {
            QString result = f.readAll();
            QSqlQuery req2;
            req2.exec("SELECT MAX(ID) FROM Temperature");
            req2.next();
            int id = req2.value(0).toInt()+1;
            int r = static_cast<int>(result.split("=").last().toDouble() / 100);
            double r2 = static_cast<double>(r);
            req2.exec("INSERT INTO Temperature VALUES('" + QString::number(id) + "','" + QDate::currentDate().toString("yyyy-MM-dd") +
                     "','" + QTime::currentTime().toString("hh:") + strMinute + "','" + QString::number(emp) + "','" + QString::number(r2 / 10) + "')");
        }
    }

    //I2C
    InterfaceI2C *i2c = this->findChild<InterfaceI2C*>();
    if(i2c && i2c->IsTempActiv()) {
        QSqlQuery req2;
        req2.exec("SELECT MAX(ID) FROM Temperature");
        req2.next();
        int id = req2.value(0).toInt()+1;
        req.exec("INSERT INTO Temperature VALUES('" + QString::number(id) + "','" + QDate::currentDate().toString("yyyy-MM-dd") +
                  "','" + QTime::currentTime().toString("hh:") + strMinute + "','" + QString::number(Indoor) + "','" + QString::number(i2c->GetTemp().value("temperature")) + "')");
    }
}

QString CVOrder::GetDataOrder(QDate first, QDate end)
{
    QSqlQuery req;
    QString result;
    req.exec("SELECT * FROM State WHERE Date BETWEEN '" + first.toString("yyyy-MM-dd") + "' AND '" + end.toString("yyyy-MM-dd") + "' ORDER BY Date ASC, Time ASC");
    while(req.next())
    {
        result += req.value(1).toString() + "|" + req.value(2).toString() + "|" + req.value(3).toString() + "|" + req.value(4).toString() + "\r";
    }
    return result;
}

QString CVOrder::GetDataTemp(QDate first, QDate end)
{
    QSqlQuery req;
    QString result;
    req.exec("SELECT * FROM Temperature WHERE Date BETWEEN '" + first.toString("yyyy-MM-dd") + "' AND '" + end.toString("yyyy-MM-dd") + "' ORDER BY Date ASC, Time ASC");
    while(req.next())
    {
        result += req.value(1).toString() + "|" + req.value(2).toString() + "|" + req.value(3).toString() + "|" + req.value(4).toString() + ",";
    }
    result = result.remove(result.count()-1,result.count()-1);
    return result;
}

QString CVOrder::GetTemp(int emp)
{
    QSqlQuery req;
    double temp = 0;
    //I2C
    InterfaceI2C *i2c = this->findChild<InterfaceI2C*>();
    if(emp == Indoor && i2c && i2c->IsTempActiv()) {
        temp = i2c->GetTemp().value("temperature");
    }
    else {
        req.exec("SELECT * FROM CVOrder WHERE Name='Temp' AND Value1='" + QString::number(emp) + "'");
        if(req.next()) {
            QFile f("/sys/bus/w1/devices/" + req.value("Value2").toString() + "/w1_slave");
            if(!f.open(QIODevice::ReadOnly)) {
                emit Info(className,"Echec d'ouverture du fichier " + f.fileName());
            }
            else {
                QString result = f.readAll();
                int r = static_cast<int>(result.split("=").last().toDouble() / 100);
                temp = static_cast<double>(r) / 10;
            }
        }
    }

    req.exec("SELECT Value FROM Temperature WHERE Date='" + QDate::currentDate().toString("yyyy-MM-dd") + "' AND Pos='" + QString::number(emp) + "' ORDER BY Value DESC");
    req.next();
    QString max = req.value(0).toString();
    req.last();
    QString min = req.value(0).toString();

    return QString(min + ":" + max + ":" + QString::number(temp));
}
















