#include "interface.h"

//Version 1.01

Interface::Interface()
{  

    Init();

    server = new Server;
    connect(server,SIGNAL(Info(QString,QString)),this,SLOT(ShowInfo(QString,QString)));
    server->Init();
    connect(server,SIGNAL(Receipt(QTcpSocket*,QString)),this,SLOT(ReceiptDataFromServer(QTcpSocket*,QString)));
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

}

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
    req.exec("SELECT Value1 FROM General WHERE Name='log'");
    req.next();
    if(req.value(0).toBool())
        _log = true;
}

void Interface::ShowInfo(QString classText, QString text)
{
    QString result = QDateTime::currentDateTime().toString("dd-MM-yyyy hh:mm:ss") + " " + classText + "\t" + text + "\n";
    std::cout << result.toStdString();

    if(_log)
    {
        QFile f("log");
        if(!f.open(QIODevice::WriteOnly | QIODevice::Append))
            std::cout << "fail to open file 'log'\n";
        else
            f.write("\n" + result.toLatin1());
        f.close();
    }
}

void Interface::ReceiptDataFromServer(QTcpSocket *user, QString data)
{
    QEventLoop loop;
    QTimer timer;
    connect(&timer,SIGNAL(timeout()),&loop,SLOT(quit()));
    timer.start(500);
    loop.exec();
    if(data.split("|").count() != 2)
        ShowInfo(className,"Data corrupted !");
    else
    {
        QSqlQuery req;
        QStringList ddata = data.split("|");
        if(ddata.at(0) == "CVOrder" && ddata.last().split(";").count() == 2)
            cvOrder->SetOrder(ddata.last().split(";").at(0).toInt(),ddata.last().split(";").at(1).toInt());
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
                    server->SendToUser(user,result);
                    ShowInfo(className,"Send data");
                }
                //SET
                if(ddata.last().contains("SETCVOrder"))
                {//Format : Config|General;SETCVOrder=value1
                    req.exec("UPDATE General SET Value1='" + QString::number(ddata.last().split("=").last().toInt()) + "' WHERE Name='CVOrder'");
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
                    server->SendToUser(user,result);
                }
                else if(ddata.last().contains(";GETConfig"))
                {//Format : Config|CVOrder;GETConfig;conf1=value1;conf2=value2
                    QString result;
                    result = "Config|CVOrder;GETConfig";
                    result += cvOrder->GetConfig();
                    server->SendToUser(user,result);
                }
                else if(ddata.last().contains(";GPIO"))
                {
                    QString result = "Config|CVOrder;GPIO;";

                    result += "Z1Eco=" + QString::number(cvOrder->GetGPIO(Z1Eco)) + ";";
                    result += "Z1HG=" + QString::number(cvOrder->GetGPIO(Z1Hg)) + ";";
                    result += "Z2Eco=" + QString::number(cvOrder->GetGPIO(Z2Eco)) + ";";
                    result += "Z2HG=" + QString::number(cvOrder->GetGPIO(Z2Hg)) + ";";
                    result += "ReverseOnOff=" + QString::number(cvOrder->GetGPIO(ReverseOnOff));

                    server->SendToUser(user,result);
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
                }
                else if(ddata.last().contains(";DELProg"))
                {//Format : Config|CVOrder;DELProg
                    cvOrder->RemoveProg(0);
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
                    }
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
                    server->SendToUser(user,"Config|Server;GETPort;Port=" + req.value("Value1").toString());
                }
                else if(ddata.last().contains("GETPassword"))
                {
                    req.exec("SELECT * FROM General WHERE Name='Password'");
                    req.next();
                    server->SendToUser(user,"Config|Server;GetPassword;Password=" + req.value("Value1").toString());
                }
                else if(ddata.last().contains("GETWebSocket"))
                {
                    req.exec("SELECT * FROM General WHERE Name='WebSocket'");
                    req.next();
                    server->SendToUser(user,"Config|Server;GETWebSocket;WebSocket=" + req.value("Value1").toString());
                }
                if(ddata.last().contains("GETWebPort"))
                {
                    req.exec("SELECT * FROM General WHERE Name='WebPort'");
                    req.next();
                    server->SendToUser(user,"Config|Server;GETWebPort;WebPort=" + req.value("Value1").toString());
                }
                else if(ddata.last().contains("GETWebPassword"))
                {
                    req.exec("SELECT * FROM General WHERE Name='WebPassword'");
                    req.next();
                    server->SendToUser(user,"Config|Server;GetWebPassword;WebPassword=" + req.value("Value1").toString());
                }
                //SET
                if(ddata.last().contains("SETPort"))
                {
                    req.exec("UPDATE General SET Value1='" + ddata.last().split("=").last()+ "' WHERE Name='Port'");
                }
                else if(ddata.last().contains("SETPassword"))
                {
                    req.exec("UPDATE General SET Value1='" + ddata.last().split("=").last()+ "' WHERE Name='Password'");
                }
                else if(ddata.last().contains("SETWebSocket"))
                {
                    req.exec("UPDATE General SET Value1='" + ddata.last().split("=").last() + "' WHERE Name='WebSocket'");
                }
                if(ddata.last().contains("SETWebPort"))
                {
                    ShowInfo(className,"Set WebPort = " + ddata.last().split("=").last());
                    req.exec("UPDATE General SET Value1='" + ddata.last().split("=").last()+ "' WHERE Name='WebPort'");
                }
                else if(ddata.last().contains("SETWebPassword"))
                {
                    req.exec("UPDATE General SET Value1='" + ddata.last().split("=").last()+ "' WHERE Name='WebPassword'");
                }
            }
        }
        else if(ddata.at(0) == "Reload")
        {
            server->Reload();

            req.exec("SELECT * FROM General WHERE Name='CVOrder'");
            req.next();
            if(req.value("Value1").toBool())
                cvOrder->Reload();
        }
    }
}

void Interface::ReceiptDataFromWebServer(QWebSocket *user, QString data)
{
    if(data.contains("|")) {
        if(data.contains("CVOrder")) {
            QString first("CVOrder|");
            if(data.contains("=")) {//Set
                if(data.contains("SetZ1Order")) {
                    cvOrder->SetOrder(data.split("=").last().toInt(),Z1);
                    server->SendToWebUser(user,first + "GetZ1Order=" + QString::number(cvOrder->GetOrder(Z1)));
                }
                else if(data.contains("SetZ2Order")) {
                    cvOrder->SetOrder(data.split("=").last().toInt(),Z2);
                    server->SendToWebUser(user,first + "GetZ2Order=" + QString::number(cvOrder->GetOrder(Z2)));
                }
                else if(data.contains("SetZ1Status")) {
                    cvOrder->SetStatus(data.split("=").last().toInt(),Z1);
                    server->SendToWebUser(user,first + "GetZ1Status=" + QString::number(cvOrder->GetStatus(Z1)));
                }
                else if(data.contains("SetZ2Status")) {
                    cvOrder->SetStatus(data.split("=").last().toInt(),Z2);
                    server->SendToWebUser(user,first + "GetZ2Status=" + QString::number(cvOrder->GetStatus(Z2)));
                }
                else if(data.contains("ABS")) {
                    cvOrder->ABS(data.split("=").last().toInt());
                    server->SendToWebUser(user,first + "Reload");
                }
            }
            else {//Get
                if(data.contains("GetZ1Order")) {
                    server->SendToWebUser(user,first + "GetZ1Order=" + QString::number(cvOrder->GetOrder(Z1)));
                }
                else if(data.contains("GetZ2Order")) {
                    server->SendToWebUser(user,first + "GetZ2Order=" + QString::number(cvOrder->GetOrder(Z2)));
                }
                else if(data.contains("GetZ1Status")) {
                    server->SendToWebUser(user,first + "GetZ1Status=" + QString::number(cvOrder->GetStatus(Z1)));
                }
                else if(data.contains("GetZ2Status")) {
                    server->SendToWebUser(user,first + "GetZ2Status=" + QString::number(cvOrder->GetStatus(Z1)));
                }
                else if(data.contains("GetABS")) {
                    server->SendToWebUser(user,first + "GetABS=" + QString::number(cvOrder->GetABS()));
                }
            }
        }
    }
    else {
        ShowInfo(className,"web data corrupted");
    }
}
