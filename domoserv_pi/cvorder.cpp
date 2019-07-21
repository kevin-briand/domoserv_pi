#include "cvorder.h"

//Version 1.02

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

    Init();
}
/*
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
}*/

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
        int test5 = req.exec("INSERT INTO VALUES('" + QString::number(id) + "','StatusZ1','0','','','')");
        id++;
        int test6 = req.exec("INSERT INTO VALUES('" + QString::number(id) + "','StatusZ2','0','','','')");

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

    //
    _timerZ1 = new QTimer;
    _timerZ2 = new QTimer;
    _timerPing = new QTimer;
    _abs = new QTimer;
    _linkHistory = "/home/pi/domoserv_pi/gestCV.log";

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

    switch (_priority) {
    case horloge://------------------------------
        if(_timerZ1 == t)
        {
            int state = _timerZ1->property("state").toInt();
            ChangeOrder(state,Z1);
            NextProgram(Z1);
        }
        if(_timerZ2 == t)
        {
            int state = _timerZ2->property("state").toInt();
            ChangeOrder(state,Z2);
            NextProgram(Z2);
        }
        break;
    case network://---------------------------
        if(_timerPing == t)
        {
            if(PingNetwork())
                ChangeOrder(confort,Z1);
            else
                ChangeOrder(eco,Z1);
        }
        if(_timerZ1 == t)//__________Z1
        {
            _lastStateZ1 = _timerZ1->property("state").toInt();
            if(_lastStateZ1 == confort)
            {
                QSqlQuery req;
                req.exec("SELECT * FROM CVOrder WHERE Name='Act_Network'");
                if(req.next())
                {
                    _timerPing->setSingleShot(false);
                    _timerPing->start(req.value("Value2").toInt());
                }
                else
                    emit Info(className,tr("_timerPing not started(Act_Network not found)"));
            }
            else
            {
                ChangeOrder(_lastStateZ1,Z1);
                _timerPing->stop();
            }
            NextProgram(Z1);
        }
        if(_timerZ2 == t)//__________Z2
        {
            _lastStateZ2 = _timerZ2->property("state").toInt();
            if(_lastStateZ2 == confort)
            {
                QSqlQuery req;
                req.exec("SELECT * FROM CVOrder WHERE Name='Act_Network'");
                if(req.next())
                {
                    _timerPing->setSingleShot(false);
                    _timerPing->start(req.value("Value2").toInt());
                }
                else
                    emit Info(className,tr("_timerPing not started(Act_Network not found)"));
            }
            else
            {
                ChangeOrder(_lastStateZ2,Z2);
                _timerPing->stop();
            }
            NextProgram(Z2);
        }
        break;
    case networkAndHorloge://-----------------------------
        if(_timerPing == t)
        {
            if(PingNetwork())
            {
                if(_lastStateZ1 == confort)
                    ChangeOrder(confort,Z1);
                if(_lastStateZ2 == confort)
                    ChangeOrder(confort,Z2);
                _timerPing->stop();
            }
        }
        if(_timerZ1 == t)//__________Z1
        {
            _lastStateZ1 = _timerZ1->property("state").toInt();
            if(_lastStateZ1 == confort)
            {
                QSqlQuery req;
                req.exec("SELECT * FROM CVOrder WHERE Name='Act_Network'");
                if(req.next())
                {
                    _timerPing->setSingleShot(false);
                    _timerPing->start(req.value("Value2").toInt());
                    emit Info(className,tr("_timerPing started"));
                }
                else
                    emit Info(className,tr("_timerPing not started(Act_Network not found)"));
            }
            else
            {
                ChangeOrder(_lastStateZ1,Z1);
                _timerPing->stop();
            }
            NextProgram(Z1);
        }

        if(_timerZ2 == t)//__________Z2
        {
            _lastStateZ2 = _timerZ2->property("state").toInt();
            if(_lastStateZ2 == confort)
            {
                QSqlQuery req;
                req.exec("SELECT * FROM CVOrder WHERE Name='Act_Network'");
                if(req.next())
                {
                    _timerPing->setSingleShot(false);
                    _timerPing->start(req.value("Value2").toInt());
                    emit Info(className,tr("_timerPing started"));
                }
                else
                    emit Info(className,tr("_timerPing not started(Act_Network not found)"));
            }
            else
            {
                ChangeOrder(_lastStateZ2,Z2);
                _timerPing->stop();
            }
            NextProgram(Z2);
        }
        break;
    default:
        break;
    }
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

        switch(_CVStateZ1) {
        case confort:
            nameActualOrder = "Confort";
            break;
        case eco:
            nameActualOrder = "Eco";
            break;
        case horsGel:
            nameActualOrder = "Hors gel";
            break;
        }
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

        switch(_CVStateZ2) {
        case confort:
            nameActualOrder = "Confort";
            break;
        case eco:
            nameActualOrder = "Eco";
            break;
        case horsGel:
            nameActualOrder = "Hors gel";
            break;
        }
        _CVStateZ2 = order;
    }
    else
    {
        emit Info(className,"Error bad zone " + QString::number(zone+1));
        return;
    }

    switch(order) {
    case confort:
        nameNewOrder = "Confort";
        break;
    case eco:
        nameNewOrder = "Eco";
        break;
    case horsGel:
        nameNewOrder = "Hors gel";
        break;
    default:
        emit Info(className,"Error bad order " + QString::number(order));
        return;
    }

    //Set Output
    if(order != confort)
        SetOutputState(output,_on);

    //history
    QFile f(_linkHistory);
    if(!f.open(QIODevice::WriteOnly | QIODevice::Append))
        emit Info(className,"Error to open file " + _linkHistory + " (" + f.errorString() + ")");
    else
    {
        QTextStream flux(&f);
        flux << "date=" << QDateTime::currentDateTime().toString("dd-MM-yyyy hh:mm") << ";zone=" << zone << ";order=" << order << "\n";
    }

    QSqlQuery req;
    req.exec("UPDATE CVOrder SET Value1='" + QString::number(order) + "' WHERE Name='ActualZ" + QString::number(zone+1) + "'");

    emit Info(className,"Change order " + nameActualOrder.toLatin1() + " to " + nameNewOrder.toLatin1() + " in " + zoneSelect.toLatin1());
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
        QString cDay = dt.currentDateTime().toString("ddd");
        int day(0);
        if(cDay == "Mon" || cDay == "lun.")
            day = 1;
        else if(cDay == "Tue" || cDay == "mar.")
            day = 2;
        else if(cDay == "Wed" || cDay == "mer.")
            day = 3;
        else if(cDay == "Thu" || cDay == "jeu.")
            day = 4;
        else if(cDay == "Fri" || cDay == "ven.")
            day = 5;
        else if(cDay == "Sat" || cDay == "sam.")
            day = 6;
        else if(cDay == "Sun" || cDay == "dim.")
            day = 7;

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

void CVOrder::NextProgram(int zone)
{
    QDateTime dt;
    QString cDay = dt.currentDateTime().toString("ddd");
    int day(0);
    if(cDay == "Mon")
        day = 1;
    else if(cDay == "Tue")
        day = 2;
    else if(cDay == "Wed")
        day = 3;
    else if(cDay == "Thu")
        day = 4;
    else if(cDay == "Fri")
        day = 5;
    else if(cDay == "Sat")
        day = 6;
    else if(cDay == "Sun")
        day = 7;

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

void CVOrder::SetProg(QString date, int zone, int state)
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
                        emit Info(className,tr("Prog already exist"));
                    else
                    {
                        req.exec("SELECT MAX(ID) FROM CVOrder");
                        req.next();
                        int id = req.value(0).toInt()+1;
                        req.exec("INSERT INTO CVOrder VALUES('" + QString::number(id) + "','Prog','" + date + "','" + QString::number(zone) + "','" + QString::number(state) + "','')");
                        emit Info(className,tr("SetProg success"));
                    }
                }
                else
                    emit Info(className,tr("SetProg failed(bad state)"));
            else
                emit Info(className,tr("SetProg failed(bad zone)"));
        else
            emit Info(className,tr("SetProg failed(bad date)"));
    else
        emit Info(className,tr("SetProg failed(bad date format)"));
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
    QString result = ";" + req.value("Name").toString() + "=" + req.value("Value1").toString();

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

void CVOrder::AddIp(QString ip)
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
                emit Info(className,"AddIp : Ip already exist");
            else
            {
                req.exec("SELECT MAX(ID) FROM CVOrder");
                req.next();
                int id = req.value(0).toInt()+1;
                req.exec("INSERT INTO CVOrder VALUES('" + QString::number(id) + "','IpPing','" + QString::number(sIp1) + "." + QString::number(sIp2) + "." +
                         QString::number(sIp3) + "." + QString::number(sIp4) + "','','','')");
                emit Info(className,ip + " added");
            }
        }
    }
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
    else if(zone == 3)
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
        int test5 = req.exec("INSERT INTO CVOrder VALUES('" + QString::number(id) + "','FileCPTEnergy','/home/pi/domoserv_pi/,'','','')");
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

    int currentMinute = QTime::currentTime().minute();

    if(currentMinute >= 0 && currentMinute <= 29)
    {
        if(minute > 29 && totalImp > 0)
        {
            int total = totalImp * _WattCPTEnergy;

            QFile f(QDate::currentDate().toString("dd-MM-yyyy") + ".log");
            if(!f.open(QIODevice::WriteOnly | QIODevice::Append))
                    emit Info(className,"Error open file " + QDate::currentDate().toString("dd-MM-yyyy") + ".log : " + f.errorString());
            else {
                QTextStream flux(&f);
                flux << QDateTime::currentDateTime().toString("hh:00") << "|" << total << "\n";
            }

            totalImp = 0;
        }
    }
    else if(currentMinute > 29 && currentMinute <= 60)
    {
        if(minute < 30 && totalImp > 0)
        {
            int total = totalImp * _WattCPTEnergy;

            QFile f(QDate::currentDate().toString("dd-MM-yyyy") + ".log");
            if(!f.open(QIODevice::WriteOnly | QIODevice::Append))
                    emit Info(className,"Error open file " + QDate::currentDate().toString("dd-MM-yyyy") + ".log : " + f.errorString());
            else {
                QTextStream flux(&f);
                flux << QDateTime::currentDateTime().toString("hh:30") << "|" << total << "\n";
            }

            totalImp = 0;
        }  
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
    {
        req.exec("UPDATE CVOrder SET Value1='1' WHERE Name='ActCPTEnergy'");
        InitCPTEnergy();
    }
    else
    {
        req.exec("UPDATE CVOrder SET Value1='0' WHERE Name='ActCPTEnergy'");
        StopCPTEnergy();
    }
}

void CVOrder::UseHCCPTEnergy(bool value)
{
    QSqlQuery req;
    if(value)
        req.exec("UPDATE CVOrder SET Value1='1' WHERE Name='ActHCCPTEnergy'");
    else
        req.exec("UPDATE CVOrder SET Value1='0' WHERE Name='ActHCCPTEnergy'");

    StopCPTEnergy();
    InitCPTEnergy();
}

void CVOrder::StopCPTEnergy()
{
    _timerReadInput->stop();
}

QString CVOrder::GetDataCPTEnergy(int day,int month,int year)
{
    QDate date;
    date.setDate(year,month,day);
    QFile f(date.toString("dd-MM-yyyy") + ".log");
    if(!f.open(QIODevice::ReadOnly))
    {
        emit Info(className,"Open file " + f.fileName() + " failed");
        return QString();
    }

    QTextStream flux(&f);
    return flux.readAll();
}
