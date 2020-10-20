#include "interface.h"

//Version 1.02

//Admin
QString GetCVOrder("Config|General;CVOrder=%0");
QString GetLog("Config|General;GETLog=%0");
QString GetProg("Config|CVOrder;GETProg;%0");
QString GetConfig("Config|CVOrder;GETConfig;%0");
QString GetGPIO("Config|CVOrder;GPIO;");
QString GetAdminPort("Config|Server;GETPort;Port=%0");
QString GetAdminPassword("Config|Server;GetPassword;Password=%0");
QString GetAdminType("Config|Server;GETAdminSocket;AdminSocket=%0");
QString GetUserType("Config|Server;GETUserSocket;UserSocket=%0");
QString GetUserPort("Config|Server;GETWebPort;WebPort=%0");
QString GetUserPassword("Config|Server;GetWebPassword;WebPassword=%0");
QString GetAdminCrypto("Config|Server;GetAdminCrypto=%0;%1;%2");
QString GetUserCrypto("Config|Server;GetUserCrypto=%0;%1;%2");

//User
QString GetZOrder("CVOrder|GetZ%0Order=%1");
QString GetZStatus("CVOrder|GetZ%0Status=%1");
QString GetABS("CVOrder|GetABS=%0");
QString GetRemainingTimeZ("CVOrder|GetRemainingTimeZ%0=%1");
QString GetRemainingTimeABS("CVOrder|GetRemainingTimeABS=%1");
QString GetHistoryCPTEnergy("CVOrder|GetDataCPTEnergy=%0");
QString GetHistoryOrder("CVOrder|GetDataOrder=%0");
QString GetHistoryTemp("CVOrder|GetDataTemp=%0");
QString GetTemp("CVOrder|GetTemp;%0=%1");
QString Reload("CVOrder|Reload");

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

        server = new ServerFire(this);
        connect(server,&ServerFire::Info,this,&Interface::ShowInfo);
        connect(server,&ServerFire::ReceiptData,this,&Interface::ReceiptDataFromServer);

        //ADMIN SERVER
        QSqlQuery req;
        req.exec("SELECT Value1 FROM General WHERE Name='ActAdminServer'");
        req.next();
        if(req.value(0).toBool()) {
            req.exec("SELECT Value1 FROM General WHERE Name='Password'");
            req.next();
            QString password = req.value(0).toString();
            req.exec("SELECT Value1 FROM General WHERE Name='Port'");
            req.next();
            int port = req.value(0).toInt();
            req.exec("SELECT Value1 FROM General WHERE Name='WebAdminSocket'");
            req.next();
            int type = req.value(0).toBool() ? GlobalServer::Web : GlobalServer::TCP;

            int keySize, codeSize, charset = -1;
            req.exec("SELECT * FROM General WHERE Name='AdminCrypto'");
            if(req.next()) {
                keySize = req.value("Value1").toInt();
                codeSize = req.value("Value2").toInt();
                charset = req.value("Value3").toInt();
            }
            server->SetCrypto(keySize,codeSize,charset);
            server->RunServer(type,GlobalServer::Admin,port,password,QStringList());

            ShowInfo(className,"-------------------------ADMIN SERVER---------------------------");
            ShowInfo(className, QString("Type : %0").arg((type == GlobalServer::TCP) ? "TCP" : "WEB"));
            ShowInfo(className, QString("Password : %0").arg(password));
            ShowInfo(className, QString("Port : %0").arg(port));
            ShowInfo(className, QString("Key size : %0").arg(keySize));
            ShowInfo(className, QString("Code size : %0").arg(codeSize));
            ShowInfo(className, QString("Char format : %0").arg((charset == UTF8) ? "UTF-8" : "UTF-16"));
            ShowInfo(className, QString("Online : %0").arg(server->IsOnline(GlobalServer::Admin) ? "TRUE" : "FALSE"));
            ShowInfo(className,"----------------------------------------------------------------");
        }

        //USER SERVER
        req.exec("SELECT Value1 FROM General WHERE Name='WebPassword'");
        req.next();
        QString password = req.value(0).toString();
        req.exec("SELECT Value1 FROM General WHERE Name='WebPort'");
        req.next();
        int port = req.value(0).toInt();
        req.exec("SELECT Value1 FROM General WHERE Name='WebSocket'");
        req.next();
        int type = req.value(0).toBool() ? GlobalServer::Web : GlobalServer::TCP;

        int keySize, codeSize, charset = -1;
        req.exec("SELECT * FROM General WHERE Name='AdminCrypto'");
        if(req.next()) {
            keySize = req.value("Value1").toInt();
            codeSize = req.value("Value2").toInt();
            charset = req.value("Value3").toInt();
        }
        server->SetCrypto(keySize,codeSize,charset);
        server->RunServer(type,GlobalServer::User,port,password,QStringList());

        ShowInfo(className,"--------------------------USER SERVER---------------------------");
        ShowInfo(className, QString("Type : %0").arg((type == GlobalServer::TCP) ? "TCP" : "WEB"));
        ShowInfo(className, QString("Password : %0").arg(password));
        ShowInfo(className, QString("Port : %0").arg(port));
        ShowInfo(className, QString("Key size : %0").arg(keySize));
        ShowInfo(className, QString("Code size : %0").arg(codeSize));
        ShowInfo(className, QString("Char format : %0").arg((charset == UTF8) ? "UTF-8" : "UTF-16"));
        ShowInfo(className, QString("Online : %0").arg(server->IsOnline(GlobalServer::User) ? "TRUE" : "FALSE"));
        ShowInfo(className,"----------------------------------------------------------------");


        //Gestionnaire chauffage
        cvOrder = new CVOrder;
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

Interface::~Interface()
{
    server->deleteLater();
    cvOrder->deleteLater();
}

bool Interface::Test()
{
#ifdef ACT_WIRING_PI_I2C

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
    QSettings settings("domoserv_pi");
    _linkLog = settings.value("link").toString().isEmpty() ? "/home/pi/domoserv_pi/" : settings.value("link").toString();


    //Init Database Config
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
        req.exec("INSERT INTO General VALUES('" + QString::number(id) + "','CVOrder','1','','','')");
        id++;
        req.exec("INSERT INTO General VALUES('" + QString::number(id) + "','Port','49152','','','')");
        id++;
        req.exec("INSERT INTO General VALUES('" + QString::number(id) + "','ActAdminServer','1','','','')");
        id++;
        req.exec("INSERT INTO General VALUES('" + QString::number(id) + "','Password','admin','','','')");
        id++;
        req.exec("INSERT INTO General VALUES('" + QString::number(id) + "','WebSocket','1','','','')");
        id++;
        req.exec("INSERT INTO General VALUES('" + QString::number(id) + "','WebAdminSocket','1','','','')");
        id++;
        req.exec("INSERT INTO General VALUES('" + QString::number(id) + "','WebPort','49155','','','')");
        id++;
        req.exec("INSERT INTO General VALUES('" + QString::number(id) + "','WebPassword','user','','','')");
        id++;
        req.exec("INSERT INTO General VALUES('" + QString::number(id) + "','UserCrypto','50','4','0','')");
        id++;
        req.exec("INSERT INTO General VALUES('" + QString::number(id) + "','AdminCrypto','50','4','0','')");
        id++;
        req.exec("INSERT INTO General VALUES('" + QString::number(id) + "','log','1','','','')");
    }

    req.exec("SELECT Value1 FROM General WHERE Name='log'");
    req.next();
    if(req.value(0).toBool())
        _log = true;

    //_update.start(86400000*7);//24h * 7
}

void Interface::ShowInfo(QString classText, QString text)
{
    QString result = QDateTime::currentDateTime().toString("dd-MM-yyyy hh:mm:ss %1\t%2\n").arg(classText).arg(text);
    std::cout << result.toStdString();

    if(_log)
    {
        QFile f(_linkLog + "domoserv_pi.log");
        if(!f.open(QIODevice::ReadWrite | QIODevice::Append))
            std::cout << "fail to open file 'log', application need to run as admin\n";
        else {
            f.write("\n" + result.toLatin1());
            f.seek(0);
            QTextStream str(&f);
            QStringList v = str.readAll().split("\n");
            if(v.count() > 150) {
                while(v.count() > 150) {
                    v.removeFirst();
                }
            }
        }
        f.close();

        if(classText == "Server") {
            QFile f(_linkLog + "server.log");
            if(!f.open(QIODevice::ReadWrite | QIODevice::Append))
                std::cout << "fail to open file 'log', application need to run as admin\n";
            else {
                f.write("\n" + result.toLatin1());
            }
        }
    }
}

void Interface::ReceiptDataFromServer(QString client, QString data)
{
    if(data.contains("Reload"))
    {
        QSqlQuery req;
        req.exec("SELECT * FROM General WHERE Name='CVOrder'");
        req.next();
        if(req.value("Value1").toBool())
            cvOrder->Reload();
    }
    else
    {
        int privilege = client.contains("A") ? GlobalServer::Admin : GlobalServer::User;
        server->SendDataToClient(client, ReadData(data, privilege));
    }
}

QString Interface::ReadData(QString data, int level)
{
    if(level == GlobalServer::Admin)
    {
        if(data.split("|").count() != 2)
        {
            ShowInfo(className,"Data corrupted !");
            return QString(GlobalServer::dataError);
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
                        return GetCVOrder.arg(req.value("Value1").toInt());
                    }
                    if(ddata.last().contains("GETLog"))
                    {//Format : Config|General;GETLog
                        QFile f(_linkLog);
                        f.open(QIODevice::ReadOnly);
                        return GetLog.arg(QString(f.readAll()));
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
                        return GetProg.arg(cvOrder->GetProg());
                    }
                    else if(ddata.last().contains(";GETConfig"))
                    {//Format : Config|CVOrder;GETConfig;conf1=value1;conf2=value2
                        return GetConfig.arg(cvOrder->GetConfig());
                    }
                    else if(ddata.last().contains(";GPIO"))
                    {
                        QString result;

                        result += "Z1Eco=" + QString::number(cvOrder->GetGPIO(Z1Eco)) + ";";
                        result += "Z1HG=" + QString::number(cvOrder->GetGPIO(Z1Hg)) + ";";
                        result += "Z2Eco=" + QString::number(cvOrder->GetGPIO(Z2Eco)) + ";";
                        result += "Z2HG=" + QString::number(cvOrder->GetGPIO(Z2Hg)) + ";";
                        result += "ReverseOnOff=" + QString::number(cvOrder->GetGPIO(ReverseOnOff)) + ";";
                        result += "ImpCPTEnergy=" + QString::number(cvOrder->GetGPIO(ImpCPTEnergy)) + ";";
                        result += "HCCPTEnergy=" + QString::number(cvOrder->GetGPIO(HCCPTEnergy));

                        return GetGPIO + result;
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
                        QString var = ddata.last().split(";").last();
                        if(var.contains("Priority"))
                            cvOrder->SetPriority(var.split("=").last().toInt());
                        else if(var.contains("RmIpPing"))
                            cvOrder->RemoveIp(var.split("=").last());
                        else if(var.contains("AddIpPing"))
                            cvOrder->AddIp(var.split("=").last());
                        else if(var.contains("timerNetwork"))
                            cvOrder->SetTimerNetwork(var.split("=").last().toInt());
                        else if(var.contains("ActCPTEnergy"))
                            cvOrder->UseCPTEnergy(var.split("=").last().toInt());
                        else if(var.contains("ActHCCPTEnergy"))
                            cvOrder->UseHCCPTEnergy(var.split("=").last().toInt());
                        else if(var.contains("FileCPTEnergy"))
                            ;//cvOrder->(var.split("=").last());
                        else if(var.contains("ImpWattCPTEnergy"))
                            cvOrder->SetImpWatt(var.split("=").last().toInt());
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
                        return GetAdminPort.arg(req.value("Value1").toString());
                    }
                    else if(ddata.last().contains("GETPassword"))
                    {
                        req.exec("SELECT * FROM General WHERE Name='Password'");
                        req.next();
                        return GetAdminPassword.arg(req.value("Value1").toString());
                    }
                    else if(ddata.last().contains("GETAdminSocket"))
                    {
                        req.exec("SELECT * FROM General WHERE Name='WebAdminSocket'");
                        req.next();
                        return GetAdminType.arg(req.value("Value1").toString());
                    }
                    else if(ddata.last().contains("GETUserSocket"))
                    {
                        req.exec("SELECT * FROM General WHERE Name='WebSocket'");
                        req.next();
                        return GetUserType.arg(req.value("Value1").toString());
                    }
                    if(ddata.last().contains("GETWebPort"))
                    {
                        req.exec("SELECT * FROM General WHERE Name='WebPort'");
                        req.next();
                        return GetUserPort.arg(req.value("Value1").toString());
                    }
                    else if(ddata.last().contains("GETWebPassword"))
                    {
                        req.exec("SELECT * FROM General WHERE Name='WebPassword'");
                        req.next();
                        return GetUserPassword.arg(req.value("Value1").toString());
                    }
                    else if(ddata.last().contains("GetAdminCrypto")) {
                        QSqlQuery req;
                        req.exec("SELECT * FROM General WHERE Name='AdminCrypto'");
                        if(!req.next()) return "Error";
                        return GetAdminCrypto.arg(req.value("Value1").toInt()).arg(req.value("Value2").toInt()).arg(req.value("Value3").toInt());
                    }
                    else if(ddata.last().contains("GetUserCrypto")) {
                        QSqlQuery req;
                        req.exec("SELECT * FROM General WHERE Name='UserCrypto'");
                        if(!req.next()) return "Error";
                        return GetUserCrypto.arg(req.value("Value1").toInt()).arg(req.value("Value2").toInt()).arg(req.value("Value3").toInt());
                    }
                    //SET
                    if(ddata.last().contains("SETPort")) {
                        return isError(req.exec("UPDATE General SET Value1='" + ddata.last().split("=").last()+ "' WHERE Name='Port'"));
                    }
                    else if(ddata.last().contains("SETPassword")) {
                        return isError(req.exec("UPDATE General SET Value1='" + ddata.last().split("=").last()+ "' WHERE Name='Password'"));
                    }
                    else if(ddata.last().contains("SETAdminSocket")) {
                        return isError(req.exec("UPDATE General SET Value1='" + ddata.last().split("=").last() + "' WHERE Name='WebAdminSocket'"));
                    }
                    else if(ddata.last().contains("SETUserSocket")) {
                        return isError(req.exec("UPDATE General SET Value1='" + ddata.last().split("=").last() + "' WHERE Name='WebSocket'"));
                    }
                    if(ddata.last().contains("SETWebPort")) {
                        return isError(req.exec("UPDATE General SET Value1='" + ddata.last().split("=").last()+ "' WHERE Name='WebPort'"));
                    }
                    else if(ddata.last().contains("SETWebPassword")){
                        return isError(req.exec("UPDATE General SET Value1='" + ddata.last().split("=").last()+ "' WHERE Name='WebPassword'"));
                    }
                    else if(ddata.last().contains("SetAdminCrypto")) {
                        QStringList list = ddata.last().split("=").last().split(";");
                        if(list.count() != 3) return "Error";
                        QSqlQuery req;
                        return isError(req.exec(QString("UPDATE General SET Value1='%0', Value2='%1', Value3='%2' WHERE Name='AdminCrypto'").arg(list.at(0)).arg(list.at(1)).arg(list.at(2))));
                    }
                    else if(ddata.last().contains("SetUserCrypto")) {
                        QStringList list = ddata.last().split("=").last().split(";");
                        if(list.count() != 3) return "Error";
                        QSqlQuery req;
                        return isError(req.exec(QString("UPDATE General SET Value1='%0', Value2='%1', Value3='%2' WHERE Name='UserCrypto'").arg(list.at(0)).arg(list.at(1)).arg(list.at(2))));
                    }
                }
            }
        }
    }
    else if(level == GlobalServer::User)
    {
        if(data.contains("|")) {
            if(data.contains("CVOrder")) {
                if(data.contains("=")) {//Set
                    if(data.contains("SetZ1Order")) {
                        cvOrder->SetOrder(data.split("=").last().toInt(),Z1);
                        return GetZOrder.arg(1).arg(cvOrder->GetOrder(Z1));
                    }
                    else if(data.contains("SetZ2Order")) {
                        cvOrder->SetOrder(data.split("=").last().toInt(),Z2);
                        return GetZOrder.arg(2).arg(cvOrder->GetOrder(Z2));
                    }
                    else if(data.contains("SetZ1Status")) {
                        cvOrder->SetStatus(data.split("=").last().toInt(),Z1);
                        return GetZStatus.arg(1).arg(cvOrder->GetStatus(Z1));
                    }
                    else if(data.contains("SetZ2Status")) {
                        cvOrder->SetStatus(data.split("=").last().toInt(),Z2);
                        return GetZStatus.arg(2).arg(cvOrder->GetStatus(Z2));
                    }
                    else if(data.contains("ABS")) {
                        cvOrder->ABS(data.split("=").last().toInt());
                        return Reload;
                    }
                }
                else {//Get
                    if(data.contains("GetZ1Order")) {
                        return GetZOrder.arg(1).arg(cvOrder->GetOrder(Z1));
                    }
                    else if(data.contains("GetZ2Order")) {
                        return GetZOrder.arg(2).arg(cvOrder->GetOrder(Z2));
                    }
                    else if(data.contains("GetZ1Status")) {
                        return GetZStatus.arg(1).arg(cvOrder->GetStatus(Z1));
                    }
                    else if(data.contains("GetZ2Status")) {
                        return GetZStatus.arg(2).arg(cvOrder->GetStatus(Z2));
                    }
                    else if(data.contains("GetABS")) {
                        return GetABS.arg(cvOrder->GetABS());
                    }
                    else if(data.contains("GetRemainingTimeZ1")) {
                        int value = 0;
                        if(cvOrder->GetABS() > 0) {
                            value = cvOrder->GetABS();
                        }
                        else {
                            value = cvOrder->GetRemainingTime(Z1);
                        }
                        return GetRemainingTimeZ.arg(1).arg(value);
                    }
                    else if(data.contains("GetRemainingTimeZ2")) {
                        int value = 0;
                        if(cvOrder->GetABS() > 0) {
                            value = cvOrder->GetABS();
                        }
                        else {
                            value = cvOrder->GetRemainingTime(Z2);
                        }
                        return GetRemainingTimeZ.arg(2).arg(value);
                    }
                    else if(data.contains("GetRemainingTimeABS")) {
                        return GetRemainingTimeABS.arg(cvOrder->GetRemainingTime(frostFree));
                    }
                    else if(data.contains("GetDataCPTEnergy")  || data.contains("GetDataOrder") || data.contains("GetDataTemp")) {
                        QStringList listDate = data.split(";;").last().split(":");
                        if(listDate.count() != 2)
                            return QString("Error, bad format");
                        QStringList date = listDate.first().split("-");
                        QStringList endDate = listDate.last().split("-");
                        if(date.count() != 3 || endDate.count() != 3)
                            return QString("Error, bad format");
                        QDate d(date.at(0).toInt(),date.at(1).toInt(),date.at(2).toInt());
                        QDate f(endDate.at(0).toInt(),endDate.at(1).toInt(),endDate.at(2).toInt());

                        if(data.contains("GetDataCPTEnergy")) {
                            return GetHistoryCPTEnergy.arg(cvOrder->GetDataCPTEnergy(d,f));
                        }
                        else if(data.contains("GetDataOrder")) {
                            return GetHistoryOrder.arg(cvOrder->GetDataOrder(d,f));
                        }
                        else {
                            return GetHistoryTemp.arg(cvOrder->GetDataTemp(d,f));
                        }
                    }
                    else if(data.contains("GetTemp")) {
                        if(data.split(";").count() == 2) {
                            int emp = data.split(";").last().toInt();
                            return GetTemp.arg(emp).arg(cvOrder->GetTemp(emp));
                        }
                        else {
                            return QString("Error");
                        }
                    }
                    else if(data.contains("GetCPTEnergyThisYear")) {

                    }
                }
            }
        }
        else {
            ShowInfo(className,"data corrupted");
            return QString(GlobalServer::dataError);
        }
    }
    else
    {
        return QString("Privilege error");
    }
}
