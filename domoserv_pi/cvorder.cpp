#include "cvorder.h"

//Version 1.0
//ADD abs

CVOrder::CVOrder()
{
    #define className "chauffage"
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

        if(test && test2 && test3 && test4)
            emit Info(className,"[\033[0;32m  OK  \033[0m] Rows created ");
        else if(test || test2 || test3 || test4)
            emit Info(className,"[\033[0;33m  OK  \033[0m] Rows created ");
        else
            emit Info(className,"[\033[0;31mFAILED\033[0m] Rows not created ");
    }
    req.exec("SELECT * FROM CVOrder WHERE Name='GPIO'");
    if(!req.next())
    {
        req.exec("SELECT MAX(ID) FROM CVOrder");
        req.next();
        int id = req.value(0).toInt()+1;
        int test = req.exec("INSERT INTO CVOrder VALUES('" + QString::number(id) + "','GPIO','0','0','','')");
        id++;
        int test2 = req.exec("INSERT INTO CVOrder VALUES('" + QString::number(id) + "','GPIO','1','1','','')");
        id++;
        int test3 = req.exec("INSERT INTO CVOrder VALUES('" + QString::number(id) + "','GPIO','2','2','','')");
        id++;
        int test4 = req.exec("INSERT INTO CVOrder VALUES('" + QString::number(id) + "','GPIO','3','3','','')");
        id++;
        int test5 = req.exec("INSERT INTO CVOrder VALUES('" + QString::number(id) + "','GPIO','4','1','','')");

        if(test && test2 && test3 && test4 && test5)
            emit Info(className,"[\033[0;32m  OK  \033[0m] GPIO Rows created ");
        else if(test || test2 || test3 || test4 || test5)
            emit Info(className,"[\033[0;33m  OK  \033[0m] GPIO Rows created ");
        else
            emit Info(className,"[\033[0;31mFAILED\033[0m] GPIO Rows not created ");
    }


    //Init WiringPi
    if(wiringPiSetup() < 0)
        emit Info(className,"[\033[0;31mFAILED\033[0m] wiringPi not started");
    else
    {
        //Init pins
        pinMode(Z1ECO,OUTPUT);
        pinMode(Z1HG,OUTPUT);
        pinMode(Z2ECO,OUTPUT);
        pinMode(Z2HG,OUTPUT);

        ResetOutputState();

        emit Info(className,"[\033[0;32m  OK  \033[0m] wiringPi started");
    }


    connect(&_timerZ1,SIGNAL(timeout()),&_timerZ1,SLOT(stop()));
    connect(&_timerZ2,SIGNAL(timeout()),&_timerZ2,SLOT(stop()));
    connect(&_timerZ1,SIGNAL(timeout()),this,SLOT(RunChangeOrder()));
    connect(&_timerZ2,SIGNAL(timeout()),this,SLOT(RunChangeOrder()));
    connect(&_timerPing,SIGNAL(timeout()),this,SLOT(RunChangeOrder()));
    //Init Prog
    emit Info(className,"Initialisation programmation...");
    InitProg();
}

void CVOrder::SetOutputState(int digitalIO, int state)
{
    digitalWrite(digitalIO,state);
}

void CVOrder::RunChangeOrder()
{
    static int lastStateZ1 = 0;
    static int lastStateZ2 = 0;

    switch (_priority) {
    case horloge://------------------------------
        if(!_timerZ1.isActive())
        {
            int state = _timerZ1.property("state").toInt();
            ChangeOrder(state,Z1);
            NextProgram(Z1);
        }
        if(!_timerZ2.isActive())
        {
            int state = _timerZ2.property("state").toInt();
            ChangeOrder(state,Z2);
            NextProgram(Z2);
        }
        break;
    case network://---------------------------
        if(!_timerZ1.isActive())//__________Z1
        {
            lastStateZ1 = _timerZ1.property("state").toInt();
            if(lastStateZ1 == confort)
            {
                QSqlQuery req;
                req.exec("SELECT * FROM CVOrder WHERE Name='Act_Network'");
                if(req.next())
                {
                    _timerPing.setSingleShot(false);
                    _timerPing.start(req.value("Value2").toInt());
                }
                else
                    emit Info(className,tr("_timerPing not started(Act_Network not found)"));
            }
            else
            {
                ChangeOrder(lastStateZ1,Z1);
                _timerPing.stop();
            }
            NextProgram(Z1);
        }
        else if(lastStateZ1 == confort && _timerPing.isActive())
        {
            if(PingNetwork())
                ChangeOrder(confort,Z1);
            else
                ChangeOrder(eco,Z1);
        }

        if(!_timerZ2.isActive())//__________Z2
        {
            lastStateZ2 = _timerZ2.property("state").toInt();
            if(lastStateZ2 == confort)
            {
                QSqlQuery req;
                req.exec("SELECT * FROM CVOrder WHERE Name='Act_Network'");
                if(req.next())
                {
                    _timerPing.setSingleShot(false);
                    _timerPing.start(req.value("Value2").toInt());
                }
                else
                    emit Info(className,tr("_timerPing not started(Act_Network not found)"));
            }
            else
            {
                ChangeOrder(lastStateZ2,Z2);
                _timerPing.stop();
            }
            NextProgram(Z2);
        }
        else if(lastStateZ2 == confort && _timerPing.isActive())
        {
            if(PingNetwork())
                ChangeOrder(confort,Z2);
            else
                ChangeOrder(eco,Z2);
        }
        break;
    case networkAndHorloge://-----------------------------
        if(!_timerZ1.isActive())//__________Z1
        {
            lastStateZ1 = _timerZ1.property("state").toInt();
            if(lastStateZ1 == confort)
            {
                QSqlQuery req;
                req.exec("SELECT * FROM CVOrder WHERE Name='Act_Network'");
                if(req.next())
                {
                    _timerPing.setSingleShot(false);
                    _timerPing.start(req.value("Value2").toInt());
                }
                else
                    emit Info(className,tr("_timerPing not started(Act_Network not found)"));
            }
            else
            {
                ChangeOrder(lastStateZ1,Z1);
                _timerPing.stop();
            }
            NextProgram(Z1);
        }
        else if(lastStateZ1 == confort && _timerPing.isActive())
        {
            if(PingNetwork())
            {
                ChangeOrder(confort,Z1);
                _timerPing.stop();
            }
        }

        if(!_timerZ2.isActive())//__________Z2
        {
            lastStateZ2 = _timerZ2.property("state").toInt();
            if(lastStateZ2 == confort)
            {
                QSqlQuery req;
                req.exec("SELECT * FROM CVOrder WHERE Name='Act_Network'");
                if(req.next())
                {
                    _timerPing.setSingleShot(false);
                    _timerPing.start(req.value("Value2").toInt());
                    emit Info(className,tr("_timerPing started"));
                }
                else
                    emit Info(className,tr("_timerPing not started(Act_Network not found)"));
            }
            else
            {
                ChangeOrder(lastStateZ2,Z2);
                _timerPing.stop();
            }
            NextProgram(Z2);
        }
        else if(lastStateZ2 == confort && _timerPing.isActive())
        {
            if(PingNetwork())
            {
                ChangeOrder(confort,Z2);
                _timerPing.stop();
            }
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
        SetOutputState(Z1ECO,OFF);
        SetOutputState(Z1HG,OFF);

        switch(order) {
        case eco:
            output = Z1ECO;
            break;
        case horsGel:
            output = Z1HG;
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
        SetOutputState(Z2ECO,OFF);
        SetOutputState(Z2HG,OFF);

        switch(order) {
        case eco:
            output = Z2ECO;
            break;
        case horsGel:
            output = Z2HG;
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
        emit Info(className,"Error bad zone " + QString::number(zone).toLatin1());
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
        break;
    }

    //Set Output
    if(order != confort)
        SetOutputState(output,ON);

    if(nameNewOrder.isEmpty())
        emit Info(className,"order change error");
    else
        emit Info(className,"Change order " + nameActualOrder.toLatin1() + " to " + nameNewOrder.toLatin1() + " in " + zoneSelect.toLatin1());
}

void CVOrder::ReceiptDataFromUser(QTcpSocket *user, QString data)
{
    if(data.split("|").count() != 2)
        emit Info(className,"Data corrupted !");
    else
    {
        if(data.split("|").at(0) == "CVOrder" && data.split(";").count() == 2)
            ChangeOrder(data.split("|").at(1).split(";").at(0).toInt(),data.split("|").at(1).split(";").at(1).toInt());
    }
}

void CVOrder::ResetOutputState()
{
    SetOutputState(Z1ECO,OFF);
    SetOutputState(Z2ECO,OFF);
    SetOutputState(Z1HG,OFF);
    SetOutputState(Z2HG,OFF);
}

void CVOrder::InitProg()
{
    QSqlQuery req;
    req.exec("SELECT * FROM CVOrder WHERE Name='ActualZ1'");
    if(req.next())
        ChangeOrder(req.value("Value4").toInt(),Z1);

    req.exec("SELECT * FROM CVOrder WHERE Name='ActualZ2'");
    if(req.next())
        ChangeOrder(req.value("Value4").toInt(),Z2);

    req.exec("SELECT * FROM CVOrder WHERE Name='Priority'");
    if(req.next())
        _priority = req.value("Value1").toInt();


    NextProgram(Z1);
    NextProgram(Z2);
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
    req.exec("SELECT * FROM CVOrder WHERE Value1 > '" + actual.toString("yyyy-MM-dd hh:mm") + "' AND Value2='" + QString::number(zone) + "' ORDER BY Value1 ASC");
    if(req.next())
    {
        fDate.append(req.value("Value1").toString());
        fDate.append(req.value("Value3").toString());
    }
    else
    {
        req.exec("SELECT * FROM CVOrder WHERE Value2='" + QString::number(zone) + "' ORDER BY Value1 ASC");
        if(req.next())
        {
            fDate.append(req.value("Value1").toString());
            fDate.append(req.value("Value3").toString());
        }
    }
    if(fDate.isEmpty())
    {
        emit Info(className,"no prog found in zone " + QString::number(zone).toLatin1());
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

    //Total in secondes
    int tSec = ((tDay * 24 + tHour) * 60 + tMinute) * 60;

    if(zone == Z1)
    {
        _timerZ1.setProperty("state",fDate.at(1).toInt());
        _timerZ1.start(tSec * 1000);
        req.exec("UPDATE FROM CVOrder SET Value1='" + fDate.at(0) + "', Value2='" + QString::number(zone) + "',Value3='" + fDate.at(1) + "',Value4='" +
                 QString::number(_CVStateZ1) + "' WHERE Name='ActualZ1'");
    }
    else if(zone == Z2)
    {
        _timerZ2.setProperty("state",fDate.at(1).toInt());
        _timerZ2.start(tSec * 1000);
        req.exec("UPDATE FROM CVOrder SET Value1='" + fDate.at(0) + "', Value2='" + QString::number(zone) + "',Value3='" + fDate.at(1) + "',Value4='" +
                 QString::number(_CVStateZ2) + "' WHERE Name='ActualZ2'");
    }
    emit Info(className,"Next prog in zone " + QString::number(zone+1).toLatin1() + " in " + QString::number(tSec).toLatin1() + " secondes");
    emit Info(className,"DEBUG : " + QString::number(tDay) + " " + QString::number(tHour) + " " + QString::number(tMinute));
}

void CVOrder::AddProg(int zone, int state, QString date)
{
    if(state < 0 && state > 2)
    {
        emit Info(className,"add prog failed(bad state)");
        return;
    }
    else if(zone < 0 && zone > 1)
    {
        emit Info(className,"add prog failed(bad zone)");
        return;
    }
    else if(date.split(" ").count() != 2 || date.split("-").count() != 3 || date.split(":").count() != 2)
    {
        emit Info(className,"add prog failed(bad date format)");
        return;
    }

    QString nameOrder;
    switch(state) {
    case confort:
        nameOrder = "Confort";
        break;
    case eco:
        nameOrder = "Eco";
        break;
    case horsGel:
        nameOrder = "Hors gel";
        break;
    default:
        break;
    }

    QSqlQuery req;
    req.exec("SELECT MAX(ID) FROM CVOrder");
    req.next();
    int id = req.value(0).toInt()+1;
    req.exec("INSERT INTO CVOrder VALUES('" + QString::number(id) + "','Prog','" + date + "','" + QString::number(zone) + "','" + QString::number(state) + "','')");
    emit Info(className,"add prog " + date.toLatin1() + " set " + nameOrder.toLatin1() + " in zone " + QString::number(zone+1).toLatin1());
}void CVOrder::RemoveProg(int zone, QString date)
{
    QSqlQuery req;
    req.exec("DELETE FROM CVOrder WHERE Value2='" + QString::number(zone) + "' AND Name='Prog' AND Value1='" + date + "'");
    emit Info(className,"remove prog " + date.toLatin1() + " in zone " + QString::number(zone+1).toLatin1());
}bool CVOrder::PingNetwork()
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
                    success = true;
    }
    return success;
}

void CVOrder::SetProg(QString date, int zone, int state)
{
    if(date.split("-").count() == 3 && date.split(":").count() == 2)
        if(date.split("-").at(0) == "2018" && date.split("-").at(1) == "01" && date.split("-").at(2).toInt() > 0 && date.split("-").at(2).toInt() < 8)
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
    QString result;
    QSqlQuery req;
    req.exec("SELECT * FROM CVOrder WHERE Name='Prog' ORDER BY Value1 ASC");
    while(req.next())
        result += ";" + req.value("Value1").toString() + "#" + req.value("Value2").toString() + "#" + req.value("Value3").toString();
    return result;
}

QString CVOrder::GetConfig()
{
    QSqlQuery req;
    req.exec("SELECT * FROM CVOrder WHERE Name='Priority'");
    req.next();
    QString result = ";" + req.value("Name").toString() + "=" + req.value("Value1").toString();

    req.exec("SELECT * FROM CVOrder WHERE Name='Act_Network'");
    req.next();
    result += ";" + req.value("Name").toString() + "=" + req.value("Value1").toString();
    result += "NetworkTimer=" + req.value("Value2").toString() + ";";

    req.exec("SELECT * FROM CVOrder WHERE Name='IpPing'");
    while(req.next())
        result += ";" + req.value("Name").toString() + "=" + req.value("Value1").toString();

    return result;
}

void CVOrder::SetPriority(int priority)
{
    if(priority >= 0 && priority <= 2)
    {
        QSqlQuery req;
        req.exec("UPDATE FROM CVOrder SET Value1='" + QString::number(priority) + "' WHERE Name='Priority'");
    }
    _priority = priority;
}

void CVOrder::AddIp(QString ip)
{
    if(ip.split(".").count() == 4)
    {
        unsigned int sIp1 = ip.split(".").at(0).toInt();
        unsigned int sIp2 = ip.split(".").at(1).toInt();
        unsigned int sIp3 = ip.split(".").at(2).toInt();
        unsigned int sIp4 = ip.split(".").at(3).toInt();
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
    req.exec("UPDATE FROM CVOrder SET Value2='" + QString::number(timer*1000) + "' WHERE Name='Act_Network'");
    emit Info(className,"timerNetwork set to " + QString::number(timer));
}

int CVOrder::GetGPIO(int pin)
{
    QSqlQuery req;
    req.exec("SELECT Value2 FROM CVOrder WHERE Name='GPIO' AND Value1='" + QString::number(pin) + "'");
    if(!req.next())
        emit Info(className,"GPIO pin not defined");
    else
        return req.value(0).toInt();
}
