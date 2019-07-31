#include "interface.h"

//Version 1.02

Interface::Interface(bool &exit)
{
    //ARG
    bool srv = false;
    for(int i=0;i<qApp->arguments().count();i++)
        if(qApp->arguments().at(i) == "-server")
            srv = true;

    if(!srv)
    {
        Configure conf;
        exit = true;
    }
    else
    {
        Init();

        server = new Server;
        connect(server,SIGNAL(Info(QString,QString)),this,SLOT(ShowInfo(QString,QString)));
        server->Init();
        connect(server,SIGNAL(Receipt(QTcpSocket*,QString,int)),this,SLOT(ReceiptDataFromServer(QTcpSocket*,QString,int)));
        connect(server, &Server::WebReceipt, this, &Interface::ReceiptDataFromWebServer);

        cvOrder = new CVOrder;
        QSqlQuery req;
        req.exec("SELECT * FROM General WHERE Name='CVOrder'");
        req.next();
        if(req.value("Value1").toBool())
        {
            connect(cvOrder,SIGNAL(Info(QString,QString)),this,SLOT(ShowInfo(QString,QString)));
            cvOrder->Init();
        }


        Test();

        connect(&_update,&QTimer::timeout,this,&Interface::StartUpdate);
    }
}
/*
Interface::~Interface()
{
    server->deleteLater();
    cvOrder->deleteLater();
    QFile::remove(_linkLog);
}*/

bool Interface::Test()
{
#ifdef ACT_WIRING_PI_SPI
        qDebug() << "INIT WIRINGPI :" << wiringPiSetup();
        qDebug() << "TEST INIT SPI CHANNEL 0: " << wiringPiSPISetup(0,1000000);
        unsigned char buffer[100];
        buffer[0] = 0xFA;
        qDebug() <<  wiringPiSPIDataRW(0,buffer,100);
        qDebug() << buffer[0] << buffer[1] << buffer[2] << buffer[3] << buffer[4] << buffer[5];

        qDebug() << "TEST INIT SPI CHANNEL 1: " << wiringPiSPISetup(1,1000000);
        qDebug() <<  wiringPiSPIDataRW(1,buffer,100);
        qDebug() << buffer[0] << buffer[1] << buffer[2] << buffer[3] << buffer[4] << buffer[5];
#endif
    return true;
}

void Interface::StartUpdate()
{
    QProcess p;
    ShowInfo("Update",QString::number(p.startDetached("/home/pi/domoserv_pi/UPDATE")));
    qApp->exit(0);
}

void Interface::Init()
{
    //Init Database
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(qApp->applicationDirPath() + "/bdd.db");
    db.setHostName("127.0.0.1");

    if(!db.open())
        ShowInfo(className,"[\033[0;31mFAILED\033[0m] Database not started");
    else
        ShowInfo(className,"[\033[0;32m  OK  \033[0m] Database open ");

    QSqlQuery req;
    bool ret = req.exec("CREATE TABLE General ('ID' SMALLINT, 'Name' TEXT, 'Value1' TEXT, 'Value2' TEXT, 'Value3' TEXT, 'Value4' TEXT)");
    if(ret)
    {
        ShowInfo(className,"[\033[0;32m  OK  \033[0m] Table created ");
        req.exec("SELECT MAX(ID) FROM General");
        req.next();
        int id = req.value(0).toInt()+1;
        req.exec("INSERT INTO General VALUES('" + QString::number(id) + "','CVOrder','0','','','')");
    }
    req.exec("SELECT * FROM General WHERE Name='log'");
    if(!req.next())
    {
        req.exec("SELECT MAX(ID) FROM General");
        req.next();
        int id = req.value(0).toInt()+1;
        req.exec("INSERT INTO General VALUES('" + QString::number(id) + "','log','1','','','')");
    }
    _linkLog = "/home/pi/domoserv_pi/data/domoserv_pi.log";
    req.exec("SELECT Value1 FROM General WHERE Name='log'");
    req.next();
    if(req.value(0).toBool())
        _log = true;

    //_update.start(86400000*7);//24h * 7
}

void Interface::ShowInfo(QString classText, QString text)
{
    QString result = QDateTime::currentDateTime().toString("dd-MM-yyyy hh:mm:ss") + " " + classText + "\t" + text + "\n";
    std::cout << result.toStdString();

    if(_log)
    {
        QFile f(_linkLog);
        if(!f.open(QIODevice::ReadWrite | QIODevice::Append))
            std::cout << "fail to open file 'log'\n";
        else
            f.write("\n" + result.toLatin1());
        f.close();
    }
}

void Interface::ReceiptDataFromServer(QTcpSocket *user, QString data, int privilege)
{
    if(data.contains("Reload"))
    {
        server->Reload();
        QSqlQuery req;
        req.exec("SELECT * FROM General WHERE Name='CVOrder'");
        req.next();
        if(req.value("Value1").toBool())
            cvOrder->Reload();
    }
    else
    {
        server->SendToUser(user,ReadData(data,privilege));
    }
}

void Interface::ReceiptDataFromWebServer(QWebSocket *user, QString data, int privilege)
{
    server->SendToWebUser(user,ReadData(data,privilege));
}

QString Interface::ReadData(QString data, int level)
{
    if(level == Admin)
    {
        if(data.split("|").count() != 2)
        {
            ShowInfo(className,"Data corrupted !");
            return QString("Error : Data corrupted");
        }
        else
        {
            QSqlQuery req;
            QStringList ddata = data.split("|");
            if(ddata.at(0) == "CVOrder" && ddata.last().split(";").count() == 2)
            {
                cvOrder->SetOrder(ddata.last().split(";").at(0).toInt(),ddata.last().split(";").at(1).toInt());
                return QString("OK");
            }
            else if(ddata.at(0) == "Config")
            {
                //General
                if(ddata.last().contains("General"))
                {
                    //GET
                    if(ddata.last().contains("GETCVOrder"))
                    {//Format : Config|General;GETCVOrder
                        req.exec("SELECT * FROM General WHERE Name='CVOrder'");
                        req.next();
                        QString result = "Config|General;CVOrder=" + req.value("Value1").toString();
                        return result;
                    }
                    if(ddata.last().contains("GETLog"))
                    {//Format : Config|General;GETLog
                        QFile f(_linkLog);
                        f.open(QIODevice::ReadOnly);
                        QString result = "Config|General;GETLog=" + f.readAll();
                        /*static qint64 offset = 0;
                        while(!f.atEnd())
                        {
                            
                        }
                        f.seek(offset);
                        f.read();
                        if(f.atEnd())
                            _allDataTransmitted = true;
                        else
                            _allDataTransmitted = false;
                        */
                        return result;
                    }
                    //SET
                    if(ddata.last().contains("SETCVOrder"))
                    {//Format : Config|General;SETCVOrder=value1
                        if(req.exec("UPDATE General SET Value1='" + QString::number(ddata.last().split("=").last().toInt()) + "' WHERE Name='CVOrder'"))
                            return QString("OK");
                        else
                            return QString("Error");
                    }
                }
                //CVOrder
                if(ddata.last().contains("CVOrder"))
                {
                    //GET
                    if(ddata.last().contains(";GETProg"))
                    {//Format : Config|CVOrder;GETProg;yyyy-MM-dd hh:mm#zone#state;yyyy-MM-dd hh:mm#zone#state
                        QString result;
                        result = "Config|CVOrder;GETProg";
                        result += cvOrder->GetProg();
                        return result;
                    }
                    else if(ddata.last().contains(";GETConfig"))
                    {//Format : Config|CVOrder;GETConfig;conf1=value1;conf2=value2
                        QString result;
                        result = "Config|CVOrder;GETConfig";
                        result += cvOrder->GetConfig();
                        return result;
                    }
                    else if(ddata.last().contains(";GPIO"))
                    {
                        QString result = "Config|CVOrder;GPIO;";

                        result += "Z1Eco=" + QString::number(cvOrder->GetGPIO(Z1Eco)) + ";";
                        result += "Z1HG=" + QString::number(cvOrder->GetGPIO(Z1Hg)) + ";";
                        result += "Z2Eco=" + QString::number(cvOrder->GetGPIO(Z2Eco)) + ";";
                        result += "Z2HG=" + QString::number(cvOrder->GetGPIO(Z2Hg)) + ";";
                        result += "ReverseOnOff=" + QString::number(cvOrder->GetGPIO(ReverseOnOff)) + ";";
                        result += "ImpCPTEnergy=" + QString::number(cvOrder->GetGPIO(ImpCPTEnergy)) + ";";
                        result += "HCCPTEnergy=" + QString::number(cvOrder->GetGPIO(HCCPTEnergy));

                        return result;
                    }
                    //SET
                    else if(ddata.last().contains(";SETProg"))
                    {//Format : Config|CVOrder;SETProg;yyyy-MM-dd hh:mm#zone#state;yyyy-MM-dd hh:mm#zone#state
                        QStringList result = ddata.last().split(";");
                        result.removeFirst();
                        result.removeFirst();
                        for(int i=0;i<result.count();i++)
                        {
                            cvOrder->SetProg(result.at(i).split("#").at(0),result.at(i).split("#").at(1).toInt(),result.at(i).split("#").at(2).toInt());
                        }
                        return QString("OK");
                    }
                    else if(ddata.last().contains(";DELProg"))
                    {//Format : Config|CVOrder;DELProg
                        cvOrder->RemoveProg(0);
                        return QString("OK");
                    }
                    else if(ddata.last().contains(";SETConfig"))
                    {//Format : Config|CVOrder;SETConfig;Conf1=Value1
                        if(ddata.last().split(";").last().contains("Priority"))
                            cvOrder->SetPriority(ddata.last().split(";").last().split("=").last().toInt());
                        else if(ddata.last().split(";").last().contains("RmIpPing"))
                            cvOrder->RemoveIp(ddata.last().split(";").last().split("=").last());
                        else if(ddata.last().split(";").last().contains("AddIpPing"))
                            cvOrder->AddIp(ddata.last().split(";").last().split("=").last());
                        else if(ddata.last().split(";").last().contains("timerNetwork"))
                            cvOrder->SetTimerNetwork(ddata.last().split(";").last().split("=").last().toInt());
                        else if(ddata.last().split(";").last().contains("ActCPTEnergy"))
                            cvOrder->UseCPTEnergy(ddata.last().split(";").last().split("=").last().toInt());
                        else if(ddata.last().split(";").last().contains("ActHCCPTEnergy"))
                            cvOrder->UseHCCPTEnergy(ddata.last().split(";").last().split("=").last().toInt());
                        else if(ddata.last().split(";").last().contains("FileCPTEnergy"))
                            ;//cvOrder->(ddata.last().split(";").last().split("=").last());
                        else if(ddata.last().split(";").last().contains("ImpWattCPTEnergy"))
                            cvOrder->SetImpWatt(ddata.last().split(";").last().split("=").last().toInt());
                        return QString("OK");
                    }
                    else if(ddata.last().contains(";SETGPIO"))
                    {
                        QStringList result = ddata.last().split(";");
                        for(int i=0;i<result.count();i++)
                        {//Format : Config|CVOrder;SETGPIO;Z1Eco=pin;Z1HG=pin2
                            if(result.at(i).contains("Z1Eco="))
                                cvOrder->SetGPIO(Z1Eco,result.at(i).split("=").last().toInt());
                            else if(result.at(i).contains("Z1HG="))
                                cvOrder->SetGPIO(Z1Hg,result.at(i).split("=").last().toInt());
                            else if(result.at(i).contains("Z2Eco="))
                                cvOrder->SetGPIO(Z2Eco,result.at(i).split("=").last().toInt());
                            else if(result.at(i).contains("Z2HG="))
                                cvOrder->SetGPIO(Z2Hg,result.at(i).split("=").last().toInt());
                            else if(result.at(i).contains("ReverseOnOff="))
                                cvOrder->ReverseGPIO(result.at(i).split("=").last().toInt());
                            else if(result.at(i).contains("ImpCPTEnergy="))
                                cvOrder->SetGPIO(ImpCPTEnergy,result.at(i).split("=").last().toInt());
                            else if(result.at(i).contains("HCCPTEnergy="))
                                cvOrder->SetGPIO(HCCPTEnergy,result.at(i).split("=").last().toInt());
                        }
                        return QString("OK");
                    }
                }
                //Server
                if(ddata.last().contains("Server"))
                {//Format : Config|Server;GETPort
                    //GET
                    if(ddata.last().contains("GETPort"))
                    {
                        req.exec("SELECT * FROM General WHERE Name='Port'");
                        req.next();
                        return QString("Config|Server;GETPort;Port=" + req.value("Value1").toString());
                    }
                    else if(ddata.last().contains("GETPassword"))
                    {
                        req.exec("SELECT * FROM General WHERE Name='Password'");
                        req.next();
                        return QString("Config|Server;GetPassword;Password=" + req.value("Value1").toString());
                    }
                    else if(ddata.last().contains("GETAdminSocket"))
                    {
                        req.exec("SELECT * FROM General WHERE Name='WebAdminSocket'");
                        req.next();
                        return QString("Config|Server;GETAdminSocket;AdminSocket=" + req.value("Value1").toString());
                    }
                    else if(ddata.last().contains("GETUserSocket"))
                    {
                        req.exec("SELECT * FROM General WHERE Name='WebSocket'");
                        req.next();
                        return QString("Config|Server;GETUserSocket;UserSocket=" + req.value("Value1").toString());
                    }
                    if(ddata.last().contains("GETWebPort"))
                    {
                        req.exec("SELECT * FROM General WHERE Name='WebPort'");
                        req.next();
                        return QString("Config|Server;GETWebPort;WebPort=" + req.value("Value1").toString());
                    }
                    else if(ddata.last().contains("GETWebPassword"))
                    {
                        req.exec("SELECT * FROM General WHERE Name='WebPassword'");
                        req.next();
                        return QString("Config|Server;GetWebPassword;WebPassword=" + req.value("Value1").toString());
                    }
                    //SET
                    if(ddata.last().contains("SETPort"))
                    {
                        if(req.exec("UPDATE General SET Value1='" + ddata.last().split("=").last()+ "' WHERE Name='Port'"))
                            return QString("OK");
                        else
                            return QString("Error");
                    }
                    else if(ddata.last().contains("SETPassword"))
                    {
                        if(req.exec("UPDATE General SET Value1='" + ddata.last().split("=").last()+ "' WHERE Name='Password'"))
                            return QString("OK");
                        else
                            return QString("Error");
                    }
                    else if(ddata.last().contains("SETAdminSocket"))
                    {
                        if(req.exec("UPDATE General SET Value1='" + ddata.last().split("=").last() + "' WHERE Name='WebAdminSocket'"))
                            return QString("OK");
                        else
                            return QString("Error");
                    }
                    else if(ddata.last().contains("SETUserSocket"))
                    {
                        if(req.exec("UPDATE General SET Value1='" + ddata.last().split("=").last() + "' WHERE Name='WebSocket'"))
                            return QString("OK");
                        else
                            return QString("Error");
                    }
                    if(ddata.last().contains("SETWebPort"))
                    {
                        ShowInfo(className,"Set WebPort = " + ddata.last().split("=").last());
                        if(req.exec("UPDATE General SET Value1='" + ddata.last().split("=").last()+ "' WHERE Name='WebPort'"))
                            return QString("OK");
                        else
                            return QString("Error");
                    }
                    else if(ddata.last().contains("SETWebPassword"))
                    {
                        if(req.exec("UPDATE General SET Value1='" + ddata.last().split("=").last()+ "' WHERE Name='WebPassword'"))
                            return QString("OK");
                        else
                            return QString("Error");
                    }
                }
            }
        }
    }
    else if(level == User)
    {
        if(data.contains("|")) {
            if(data.contains("CVOrder")) {
                QString first("CVOrder|");
                if(data.contains("=")) {//Set
                    if(data.contains("SetZ1Order")) {
                        cvOrder->SetOrder(data.split("=").last().toInt(),Z1);
                        return first + "GetZ1Order=" + QString::number(cvOrder->GetOrder(Z1));
                    }
                    else if(data.contains("SetZ2Order")) {
                        cvOrder->SetOrder(data.split("=").last().toInt(),Z2);
                        return first + "GetZ2Order=" + QString::number(cvOrder->GetOrder(Z2));
                    }
                    else if(data.contains("SetZ1Status")) {
                        cvOrder->SetStatus(data.split("=").last().toInt(),Z1);
                        return first + "GetZ1Status=" + QString::number(cvOrder->GetStatus(Z1));
                    }
                    else if(data.contains("SetZ2Status")) {
                        cvOrder->SetStatus(data.split("=").last().toInt(),Z2);
                        return first + "GetZ2Status=" + QString::number(cvOrder->GetStatus(Z2));
                    }
                    else if(data.contains("ABS")) {
                        cvOrder->ABS(data.split("=").last().toInt());
                        return first + "Reload";
                    }
                }
                else {//Get
                    if(data.contains("GetZ1Order")) {
                        return first + "GetZ1Order=" + QString::number(cvOrder->GetOrder(Z1));
                    }
                    else if(data.contains("GetZ2Order")) {
                        return first + "GetZ2Order=" + QString::number(cvOrder->GetOrder(Z2));
                    }
                    else if(data.contains("GetZ1Status")) {
                        return first + "GetZ1Status=" + QString::number(cvOrder->GetStatus(Z1));
                    }
                    else if(data.contains("GetZ2Status")) {
                        return first + "GetZ2Status=" + QString::number(cvOrder->GetStatus(Z2));
                    }
                    else if(data.contains("GetABS")) {
                        return first + "GetABS=" + QString::number(cvOrder->GetABS());
                    }
                    else if(data.contains("GetRemainingTimeZ1")) {
                        return first + "GetRemainingTimeZ1=" + QString::number(cvOrder->GetRemainingTime(Z1));
                    }
                    else if(data.contains("GetRemainingTimeZ2")) {
                        return first + "GetRemainingTimeZ2=" + QString::number(cvOrder->GetRemainingTime(Z2));
                    }
                    else if(data.contains("GetRemainingTimeABS")) {
                        return first + "GetRemainingTimeABS=" + QString::number(cvOrder->GetRemainingTime(3));
                    }
                    else if(data.contains("GetLog")) {
                        return first + "GetLog=" + cvOrder->GetLog();
                    }
                    else if(data.contains("GetDataCPTEnergy")) {
                        QStringList date = data.split(";").last().split("-");
                        if(date.count() != 3)
                            return QString("Error");
                        int day = date.at(0).toInt();
                        int month = date.at(1).toInt();
                        int year = date.at(2).toInt();
                        return first + "GetDataCPTEnergy=" + cvOrder->GetDataCPTEnergy(day,month,year);
                    }
                }
            }
        }
        else {
            ShowInfo(className,"data corrupted");
            return QString("Data corrupted");
        }
    }
    else
    {
        return QString("Privilege error");
    }
}
