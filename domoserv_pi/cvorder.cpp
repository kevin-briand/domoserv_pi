#include "cvorder.h"

CVOrder::CVOrder()
{
    //Init Database
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(qApp->applicationDirPath() + "/bdd.db");
    db.setHostName("127.0.0.1");

    if(!db.open())
        printf("[\033[0;31mFAILED\033[0m] Database not started\n");
    else
        printf("[\033[0;32m  OK  \033[0m] Database open \n");

    //Create Database
    QSqlQuery req;
    bool ret = req.exec("CREATE TABLE CVOrder ('ID' SMALLINT, 'Name' TEXT, 'Value1' TEXT, 'Value2' TEXT, 'Value3' TEXT, 'Value4' TEXT)");
    if(ret)
        printf("[\033[0;32m  OK  \033[0m] Table created \n");


    //Init WiringPi
    if(wiringPiSetup() < 0)
        printf("[\033[0;31mFAILED\033[0m] wiringPi not started\n");
    else
    {
        //Init pins
        pinMode(Z1ECO,OUTPUT);
        pinMode(Z1HG,OUTPUT);
        pinMode(Z2ECO,OUTPUT);
        pinMode(Z2HG,OUTPUT);

        ResetOutputState();

        printf("[\033[0;32m  OK  \033[0m] wiringPi started\n");
    }


    connect(&_timerZ1,SIGNAL(timeout()),&_timerZ1,SLOT(stop()));
    connect(&_timerZ2,SIGNAL(timeout()),&_timerZ2,SLOT(stop()));
    connect(&_timerZ1,SIGNAL(timeout()),this,SLOT(RunChangeOrder()));
    connect(&_timerZ2,SIGNAL(timeout()),this,SLOT(RunChangeOrder()));
    //Init Prog
    printf("Initialisation programmation...\n");
    InitProg(); 
}

void CVOrder::SetOutputState(int digitalIO, int state)
{
    digitalWrite(digitalIO,state);
}

void CVOrder::RunChangeOrder()
{
    if(!_timerZ1.isActive())
    {
        int state = _timerZ1.property("state").toInt();
        ChangeOrder(state,Z1);
        NextProgram(Z1);
    }
    else if(!_timerZ2.isActive())
    {
        int state = _timerZ2.property("state").toInt();
        ChangeOrder(state,Z2);
        NextProgram(Z2);
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
        printf("Error bad zone " + QString::number(zone).toLatin1() + "\n");
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
        printf("order change error\n");
    else
        printf("Change order " + nameActualOrder.toLatin1() + " to " + nameNewOrder.toLatin1() + " in " + zoneSelect.toLatin1() + "\n");
}

void CVOrder::ReceiptDataFromUser(QTcpSocket *user, QString data)
{
    if(data.split("|").count() != 2)
        printf("Data corrupted !\n");
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
    _horaireZ1.clear();
    _horaireZ2.clear();

    QSqlQuery req;
    req.exec("SELECT * FROM CVOrder WHERE Name='Prog' ORDER BY Value1 ASC");
    while(req.next())
    {
        QDate d;
        d.setDate(2018,1,req.value("Value1").toString().split(" ").at(0).split("-").last().toInt());
        QTime t;
        t.setHMS(req.value("Value1").toString().split(" ").last().split(":").at(0).toInt(),
                 req.value("Value1").toString().split(" ").last().split(":").last().toInt(),req.value("Value3").toInt());
        QDateTime dt;
        dt.setDate(d);
        dt.setTime(t);

        if(req.value("Value2").toInt() == Z1)
        {
            _horaireZ1.append(dt);
        }
        else if(req.value("Value2").toInt() == Z2)
            _horaireZ2.append(dt);
    }


    NextProgram(Z1);
    NextProgram(Z2);
}

void CVOrder::NextProgram(int zone)
{
    QDateTime dt;
    QString cDay = dt.currentDateTime().toString("ddd");
    int day;
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

    QList<QDateTime> d;
    if(zone == Z1)
        d = _horaireZ1;
    else if(zone == Z2)
        d = _horaireZ2;

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
        req.exec("SELECT * FORM CVOrder WHERE Value2='" + QString::number(zone) + "' ORDER BY Value1 ASC");
        if(req.next())
        {
            fDate.append(req.value("Value1").toString());
            fDate.append(req.value("Value3").toString());
        }
    }
    if(fDate.isEmpty())
    {
        printf("no prog found in zone " + QString::number(zone).toLatin1() + "\n");
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
    }
    else if(zone == Z2)
    {
        _timerZ2.setProperty("state",fDate.at(1).toInt());
        _timerZ2.start(tSec * 1000);
    }
    printf("Next prog in zone " + QString::number(zone+1).toLatin1() + " in " + QString::number(tSec).toLatin1() + " secondes\n");
}
