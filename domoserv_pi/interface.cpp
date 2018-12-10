#include "interface.h"

//Version 1.0

Interface::Interface()
{  
    Init();

    server = new Server;
    connect(server,SIGNAL(Info(QString,QString)),this,SLOT(ShowInfo(QString,QString)));
    server->Init();
    connect(server,SIGNAL(Receipt(QTcpSocket*,QString)),this,SLOT(ReceiptDataFromServer(QTcpSocket*,QString)));

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
        ShowInfo(className,"[\033[0;32m  OK  \033[0m] Table created ");
    if(ret)
    {
        req.exec("SELECT MAX(ID) FROM General");
        req.next();
        int id = req.value(0).toInt()+1;
        int test = req.exec("INSERT INTO General VALUES('" + QString::number(id) + "','CVOrder','0','','','')");
    }
}

void Interface::ShowInfo(QString classText, QString text)
{
    printf(QDateTime::currentDateTime().toString("dd-MM-yyyy hh:mm:ss").toLatin1() + " " + classText.toLatin1() + "\t" + text.toLatin1() + "\n");
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
            cvOrder->ChangeOrder(ddata.last().split(";").at(0).toInt(),ddata.last().split(";").at(1).toInt());
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
                if(ddata.last().contains("GETProg"))
                {//Format : Config|CVOrder;GETProg;yyyy-MM-dd hh:mm#zone#state;yyyy-MM-dd hh:mm#zone#state
                    QString result;
                    result = "Config|CVOrder;GETProg";
                    result += cvOrder->GetProg();
                    server->SendToUser(user,result);
                }
                else if(ddata.last().contains("GETConfig"))
                {//Format : Config|CVOrder;GETConfig;conf1=value1;conf2=value2
                    QString result;
                    result = "Config|CVOrder;GETConfig";
                    result += cvOrder->GetConfig();
                    server->SendToUser(user,result);
                }
                else if(ddata.last().contains("GPIO"))
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
                else if(ddata.last().contains("SETProg"))
                {//Format : Config|CVOrder;SETProg;yyyy-MM-dd hh:mm#zone#state;yyyy-MM-dd hh:mm#zone#state
                    QStringList result = ddata.last().split(";");
                    result.removeFirst();
                    result.removeFirst();
                    for(int i=0;i<result.count();i++)
                    {
                        cvOrder->SetProg(result.at(i).split("#").at(0),result.at(i).split("#").at(1).toInt(),result.at(i).split("#").at(2).toInt());
                    }
                }
                else if(ddata.last().contains("SETConfig"))
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
            }
        }
        else if(ddata.at(0) == "Reload")
        {
            Init();
            server->Init();
            cvOrder->Init();
        }
    }
}
