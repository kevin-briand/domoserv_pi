#include "configure.h"
#include "cvorder.h"

Configure::Configure(int iofile)
{
    cout << "Connection au serveur..." << endl;
    
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(qApp->applicationDirPath() + "/bdd.db");
    db.setHostName("127.0.0.1");

    if(!db.open())
    {
        cout << "Impossible d'ouvrir la base de données";
        return;
    }
    
    QSqlQuery req;
    if(!req.exec("SELECT * FROM General WHERE Name='Port'"))
    {
        cout << "Base de données non créé";
        return;
    }

    switch (iofile) {
        case 0:
            GenerateConfigFile();
            break;
        case 1:
            ImportConfigFile();
            break;
        default:
            GeneralMenu();
            break;
    }
}

Configure::~Configure()
{

}

void Configure::GenerateConfigFile()
{
    cout << "Generate config file...";

    QFile f("Config.txt");
    f.remove();
    if(!f.open(QIODevice::WriteOnly)) {
        cout << "Error : file not open !";
        return;
    }
    QTextStream str(&f);
    QSqlQuery req;

    str << "{GENERAL}" << endl;
    req.exec("SELECT * FROM General");
    while(req.next()) {
        QString name = req.value("Name").toString();
        QString result;
        if(name == "CVOrder") result = QString("Chauffage Actif=%0").arg(req.value("Value1").toBool() ? "TRUE" : "FALSE");
        else if(name == "log") result = QString("Log Actif=%0").arg(req.value("Value1").toBool() ? "TRUE" : "FALSE");

        if(!result.isEmpty())
            str << result << endl;
    }

    str << endl << "{SERVEUR}" << endl;
    req.exec("SELECT * FROM General");
    while(req.next()) {
        QString name = req.value("Name").toString();
        QString result;
        if(name == "ActAdminServer") result = QString("Admin Actif=%0").arg(req.value("Value1").toBool() ? "TRUE" : "FALSE");
        else if(name == "WebAdminSocket") result = QString("Admin WebSocket=%0").arg(req.value("Value1").toBool() ? "TRUE" : "FALSE");
        else if(name == "WebSocket") result = QString("User WebSocket=%0").arg(req.value("Value1").toBool() ? "TRUE" : "FALSE");
        else if(name == "Port") result = QString("Admin Port=%0").arg(req.value("Value1").toInt());
        else if(name == "WebPort") result = QString("User Port=%0").arg(req.value("Value1").toInt());
        else if(name == "Password") result = QString("Admin Password=%0").arg(req.value("Value1").toString());
        else if(name == "WebPassword") result = QString("User Password=%0").arg(req.value("Value1").toString());
        else if(name == "AdminCrypto") result = QString("Admin Crypto= Taille clé=%0 Taille code=%1 Type=%2").arg(req.value("Value1").toInt())
                .arg(req.value("Value2").toInt()).arg(req.value("Value3").toBool() ? "UTF8" : "UTF16");
        else if(name == "UserCrypto") result = QString("User Crypto= Taille clé=%0 Taille code=%1 Type=%2").arg(req.value("Value1").toInt())
                .arg(req.value("Value2").toInt()).arg(req.value("Value3").toBool() ? "UTF8" : "UTF16");

        if(!result.isEmpty())
            str << result << endl;
    }

    str << endl << "{CHAUFFAGE}" << endl;
    req.exec("SELECT * FROM CVOrder");
    while(req.next()) {
        QString name = req.value("Name").toString();
        QString result;
        if(name == "Priority") result = QString("Priorité=%0").arg(req.value("Value1").toInt());
        else if(name == "Act_Network") result = QString("Interval ping=%0").arg(req.value("Value2").toInt());
        else if(name == "GPIO" && req.value("Value1").toInt() == 0) result = QString("GPIO Zone1 Eco=%0").arg(req.value("Value2").toInt());
        else if(name == "GPIO" && req.value("Value1").toInt() == 1) result = QString("GPIO Zone1 HorsGel=%0").arg(req.value("Value2").toInt());
        else if(name == "GPIO" && req.value("Value1").toInt() == 2) result = QString("GPIO Zone2 Eco=%0").arg(req.value("Value2").toInt());
        else if(name == "GPIO" && req.value("Value1").toInt() == 3) result = QString("GPIO Zone2 HorsGel=%0").arg(req.value("Value2").toInt());
        else if(name == "GPIO" && req.value("Value1").toInt() == 4) result = QString("GPIO Inverser On/Off=%0").arg(req.value("Value2").toBool() ? "TRUE" : "FALSE");
        else if(name == "GPIO" && req.value("Value1").toInt() == 5) result = QString("GPIO Impulsion Compteur=%0").arg(req.value("Value2").toInt());
        else if(name == "GPIO" && req.value("Value1").toInt() == 6) result = QString("GPIO Heures creuses=%0").arg(req.value("Value2").toInt());
        else if(name == "ActCPTEnergy") result = QString("Compteur Actif=%0").arg(req.value("Value1").toBool() ? "TRUE" : "FALSE");
        else if(name == "ActHCCPTEnergy") result = QString("Heures creuses Actif=%0").arg(req.value("Value1").toBool() ? "TRUE" : "FALSE");
        else if(name == "ImpWattCPTEnergy") result = QString("Wh/impulsion=%0").arg(req.value("Value1").toInt());
        else if(name == "IpPing") result = QString("Ip=%0").arg(req.value("Value1").toString());
        else if(name == "Prog") result = QString("Prog= Jour=%0 Heure=%1 Zone=%2 Etat=%3").arg(req.value("Value1").toString().split(" ").first().split("-").last())
                .arg(req.value("Value1").toString().split(" ").last()).arg(req.value("Value2").toInt()).arg(req.value("Value3").toBool() ? "Eco" : "Confort");

        if(!result.isEmpty())
            str << result << endl;
    }

    //I2C
    str << endl << "{I2C}" << endl;
    req.exec("SELECT * FROM General WHERE Name='I2C'");
    req.next();
    str << QString("Protocol=%0").arg(req.value("Value1").toBool() ? "TRUE" : "FALSE") << endl;
    req.exec("SELECT * FROM General WHERE Name='I2CTemp'");
    req.next();
    qDebug() << "t2";
    str << QString("Température=%0").arg(req.value("Value1").toBool() ? "TRUE" : "FALSE") << endl;
    req.exec("SELECT * FROM General WHERE Name='I2CScreen'");
    req.next();
    qDebug() << "t";
    str << QString("Ecran=%0").arg(req.value("Value1").toBool() ? "TRUE" : "FALSE") << endl;
    qDebug() << "end";


    //Enregistré
    str << endl << "{TEMPERATURE}" << endl;
    req.exec("SELECT * FROM CVOrder WHERE Name='Temp'");
    while(req.next()) {
        str << "Temp= " << (req.value("Value1").toBool() ? "Interieur=" : "Exterieur=") << req.value("Value2").toString() << endl;
    }

    //Dispo
    QDir dir;
    dir.setPath("/sys/bus/w1/devices");
    QStringList list = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for(int i = 0;i < list.count()-1;i++) {
        req.exec("SELECT * FROM CVOrder WHERE Name='Temp'");
        bool exist = false;
        while(req.next())
            if(req.value("Value2").toString() == list.at(i))
            {
                exist = true;
                list.removeAt(i);
                i--;
            }
        if(!exist)
        {
            QFile f(dir.path() + "/" + list.at(i) + "/w1_slave");
            if(!f.open(QIODevice::ReadOnly)) {
                cout << "Echec d'ouverture du fichier" << endl;
            }
            else {
                QString result = f.readAll();
                str << "Disponible=" << list.at(i) << " (t=" << result.split("=").last().toDouble() / 1000 << " degrés Celcius)" << endl;
                f.close();
            }
        }
    }
    cout << "Ok\n";
    GeneralMenu();
}

void Configure::ImportConfigFile()
{
    cout << "Updating config...";
    QFile f("Config.txt");
    if(!f.open(QIODevice::ReadOnly)) {
        cout << "Error open file";
        return;
    }

    QTextStream str(&f);
    QSqlQuery req;

    if(!str.atEnd()) {
        req.exec("DELETE FROM CVOrder WHERE Name='IpPing'");
        req.exec("DELETE FROM CVOrder WHERE Name='Prog'");
        req.exec("DELETE FROM CVOrder WHERE Name='Temp'");
    }

    while(!str.atEnd()) {
        QString value = str.readLine();
        QString name = value.split("=").first();
        cout << value.toStdString();
        if(name == "Chauffage Actif") qDebug() << req.exec(QString("UPDATE General SET Value1='%0' WHERE Name='CVOrder'").arg(value.split("=").last().contains("TRUE") ? 1 : 0));
        else if(name == "log") qDebug() << req.exec(QString("UPDATE General SET Value1='%0' WHERE Name='log'").arg(value.split("=").last().contains("TRUE") ? 1 : 0));
        else if(name == "Admin Actif") qDebug() << req.exec(QString("UPDATE General SET Value1='%0' WHERE Name='ActAdminServer'").arg(value.split("=").last().contains("TRUE") ? 1 : 0));
        else if(name == "Admin WebSocket") qDebug() << req.exec(QString("UPDATE General SET Value1='%0' WHERE Name='WebAdminSocket'").arg(value.split("=").last().contains("TRUE") ? 1 : 0));
        else if(name == "User WebSocket") qDebug() << req.exec(QString("UPDATE General SET Value1='%0' WHERE Name='WebSocket'").arg(value.split("=").last().contains("TRUE") ? 1 : 0));
        else if(name == "Admin Port") qDebug() << req.exec(QString("UPDATE General SET Value1='%0' WHERE Name='Port'").arg(value.split("=").last().toInt()));
        else if(name == "User Port") qDebug() << req.exec(QString("UPDATE General SET Value1='%0' WHERE Name='WebPort'").arg(value.split("=").last().toInt()));
        else if(name == "Admin Password") qDebug() << req.exec(QString("UPDATE General SET Value1='%0' WHERE Name='Password'").arg(value.split("=").last()));
        else if(name == "User Password") qDebug() << req.exec(QString("UPDATE General SET Value1='%0' WHERE Name='WebPassword'").arg(value.split("=").last()));
        else if(name == "Admin Crypto") qDebug() << req.exec(QString("UPDATE General SET Value1='%0', Value2='%1', Value3='%2' WHERE Name='AdminCrypto'")
                                                 .arg(value.split("=").at(2).split(" ").first().toInt()).arg(value.split("=").at(3).split(" ").first().toInt())
                                                 .arg((value.split("=").last().toInt() == 1) ? "UTF16" : "UTF8"));
        else if(name == "User Crypto") qDebug() << req.exec(QString("UPDATE General SET Value1='%0', Value2='%1', Value3='%2' WHERE Name='UserCrypto'")
                                                .arg(value.split("=").at(2).split(" ").first().toInt()).arg(value.split("=").at(3).split(" ").first().toInt())
                                                .arg((value.split("=").last().toInt() == 1) ? "UTF16" : "UTF8"));
        else if(name == "Priorité") qDebug() << req.exec(QString("UPDATE CVOrder SET Value1='%0' WHERE Name='Priority'").arg(value.split("=").last().toInt()));
        else if(name == "Interval ping") qDebug() << req.exec(QString("UPDATE CVOrder SET Value2='%0' WHERE Name='Act_Network'").arg(value.split("=").last().toInt()));
        else if(name == "GPIO Zone1 Eco") qDebug() << req.exec(QString("UPDATE CVOrder SET Value2='%0' WHERE Name='GPIO' AND Value1='0'").arg(value.split("=").last().toInt()));
        else if(name == "GPIO Zone1 HorsGel") qDebug() << req.exec(QString("UPDATE CVOrder SET Value2='%0' WHERE Name='GPIO' AND Value1='1'").arg(value.split("=").last().toInt()));
        else if(name == "GPIO Zone2 Eco") qDebug() << req.exec(QString("UPDATE CVOrder SET Value2='%0' WHERE Name='GPIO' AND Value1='2'").arg(value.split("=").last().toInt()));
        else if(name == "GPIO Zone2 HorsGel") qDebug() << req.exec(QString("UPDATE CVOrder SET Value2='%0' WHERE Name='GPIO' AND Value1='3'").arg(value.split("=").last().toInt()));
        else if(name == "GPIO Inverser On/Off") qDebug() << req.exec(QString("UPDATE CVOrder SET Value2='%0' WHERE Name='GPIO' AND Value1='4'").arg(value.split("=").last().contains("TRUE") ? 1 : 0));
        else if(name == "GPIO Impulsion Compteur") qDebug() << req.exec(QString("UPDATE CVOrder SET Value2='%0' WHERE Name='GPIO' AND Value1='5'").arg(value.split("=").last().toInt()));
        else if(name == "GPIO Heures creuses") qDebug() << req.exec(QString("UPDATE CVOrder SET Value2='%0' WHERE Name='GPIO' AND Value1='6'").arg(value.split("=").last().toInt()));
        else if(name == "Compteur Actif") qDebug() << req.exec(QString("UPDATE CVOrder SET Value1='%0' WHERE Name='ActCPTEnergy'").arg(value.split("=").last().contains("TRUE") ? 1 : 0));
        else if(name == "Heures creuses Actif") qDebug() << req.exec(QString("UPDATE CVOrder SET Value1='%0' WHERE Name='ActHCCPTEnergy'").arg(value.split("=").last().contains("TRUE") ? 1 : 0));
        else if(name == "Wh/impulsion") qDebug() << req.exec(QString("UPDATE CVOrder SET Value1='%0' WHERE Name='ImpWattCPTEnergy'").arg(value.split("=").last().toInt()));
        else if(name == "Ip") qDebug() << CVOrder::AddIp(value.split("=").last());
        else if(name == "Prog") qDebug() << CVOrder::SetProg(QString("2018-01-%0 %1").arg(value.split("=").at(2).split(" ").first()).arg(value.split("=").at(3).split(" ").first()),
                                                 value.split("=").at(4).contains("0") ? 0 : 1,value.split("=").last().contains("Confort") ? 0 : 1);
        else if(name == "Temp") req.exec(QString("UPDATE CVOrder SET Value1='%0', Value2='%1' WHERE Name='Temp'").arg(value.split(" ").last().contains("Interieur") ? 0 : 1)
                                         .arg(value.split(" ").last().split("=").last()));
        else if(name == "Protocol") qDebug() << req.exec(QString("UPDATE General SET Value1='%0' WHERE Name='I2C'").arg(value.split("=").last().contains("TRUE") ? 1 : 0));
        else if(name == "Température") { qDebug() << req.exec(QString("UPDATE General SET Value1='%0' WHERE Name='I2CTemp'").arg(value.split("=").last().contains("TRUE") ? 1 : 0));
            if(value.split("=").last().contains("TRUE")) req.exec("DELETE FROM CVOrder WHERE Name='Temp' AND Value1='0'");//Remove 1 wire Indoor
        }
        else if(name == "Ecran") qDebug() << req.exec(QString("UPDATE General SET Value1='%0' WHERE Name='I2CScreen'").arg(value.split("=").last().contains("TRUE") ? 1 : 0));
        cout << endl;
    }
    cout << "Ok\n";
    GeneralMenu();
}

void Configure::GeneralMenu()
{
    QStringList list;
    list.append("Etat serveur");
    list.append("Test");
    list.append("Import");
    list.append("Export");
    list.append("Scan réseau");
    list.append("Quitter");

    int result = Question(list,6);

    switch (result) {
    case 1:
        StateMenu();
        break;
    case 2:
        Test();
        break;
    case 3:
        ImportConfigFile();
        break;
    case 4:
        GenerateConfigFile();
        break;
    case 5:
        Scan();
        break;
    case 6:
        qApp->exit();
        break;
    }
}

void Configure::Scan()
{
    static int i = 1;
    static QString ip = "";

    if(i >= 255) {
        //Wait all process finished
        QList<QProcess*> lProc = this->findChildren<QProcess*>();
        for(int i=0;i<lProc.count();i++) {
            if(lProc.at(i)->isOpen())
                return;
        }
        static int listCount = 0;

        //All process finished
        if(i == 255) {
            listCount = list.count();
            cout << "\n";
            cout << list.count() << " hôtes trouvés\n Déconnectez les appareils utilisés pour le passage en confort puis taper 'y' pour continuer\n";
            string v;
            cin >> v;
            if(v != "y")
                i = 500;
        }
        //Find disconnected host
        if(i-255+list.count()-listCount < list.count())
            this->findChildren<QProcess*>().first()->start(QString("ping -c 2 -W 3 %0").arg(list.at(i-255 + list.count()-listCount)));
        else {
            cout << list.count() << " hôte(s) deconnecté(s), correct ?\n";
            string r;
            cin >> r;
            if(r == "y") {
                qDebug() << list;
                cout << "Souhaitez-vous ajouter les ip à la base de données ?\n";
                for(QString ip : list) {
                    cout << ip.toStdString() << "\n";
                }
                string v;
                cin >> v;
                if(v == "y") {
                    QSqlQuery req;
                    for(QString ip : list) {
                        req.exec("SELECT MAX(ID) FROM General");
                        req.next();
                        int id = req.value(0).toInt();
                        req.exec(QString("INSERT INTO CVOrder VALUES('%0','IpPing','%1','','','')").arg(id+1).arg(ip));
                    }
                }
            }
            //Reset var
            for(int i=0;i<lProc.count();i++) {
                if(lProc.count() == i+1)
                    connect(lProc.at(i), &QProcess::destroyed, this, &Configure::GeneralMenu);
                lProc.at(i)->deleteLater();
            }
            i = 1;
            ip.clear();
        }

        i++;
        return;
    }

    //Find ip
    if(ip.isEmpty()) {
        for(QHostAddress host : QNetworkInterface::allAddresses()) {
            if(host.toString().contains("192.168."))
                ip = host.toString();
        }
        ip.remove(ip.count()-ip.split(".").last().count(),ip.count());
        //Create process
        for(int i=0;i<5;i++) {
            QProcess *proc = new QProcess(this);
            connect(proc, SIGNAL(finished(int)), this, SLOT(endScan()));
        }
        QTextStream cout(stdout, QIODevice::WriteOnly);

        cout << "ip :" << ip << endl;
        cout << "Scan en cours...";
    }

    //Send ping
    QList<QProcess*> lProc = this->findChildren<QProcess*>();
    for(int i2=0;i2<lProc.count();i2++) {
        if(!lProc.at(i2)->isOpen()) {
            cout << "starting process" << endl;
            lProc.at(i2)->start(QString("ping -c 2 -W 3 %0").arg(ip + QString::number(i)));
            i++;
        }
    }
}

void Configure::endScan()
{
    QTextStream cout(stdout, QIODevice::WriteOnly);
    cout << ".";
    QByteArray ba = qobject_cast<QProcess*>(sender())->readAll();

    if(QString(ba).indexOf("ttl") >= 0) {
        QString ip = QString(ba).split(" ").at(1);
        if(list.contains(ip))
            list.removeOne(ip);
        else
            list.append(ip);
    }

    qobject_cast<QProcess*>(sender())->close();
    Scan();
}

void Configure::Test()
{
    QSqlQuery req;
    QEventLoop loop;

    //Init WiringPi
    int z1Eco(0), z1HG(0), z2Eco(0), z2HG(0);
#ifdef ACT_WIRING_PI
    if(wiringPiSetup() < 0)
        cout << RED << "wiringPi not started" << NOCOLOR << endl;
    else
    {
        //Init pins
        req.exec("SELECT * FROM CVOrder WHERE Name='GPIO' AND Value1='0'");
        req.next();
        pinMode(req.value("Value2").toInt(),OUTPUT);
        z1Eco = req.value("Value2").toInt();
        req.exec("SELECT * FROM CVOrder WHERE Name='GPIO' AND Value1='1'");
        req.next();
        pinMode(req.value("Value2").toInt(),OUTPUT);
        z1HG= req.value("Value2").toInt();
        req.exec("SELECT * FROM CVOrder WHERE Name='GPIO' AND Value1='2'");
        req.next();
        pinMode(req.value("Value2").toInt(),OUTPUT);
        z2Eco = req.value("Value2").toInt();
        req.exec("SELECT * FROM CVOrder WHERE Name='GPIO' AND Value1='3'");
        req.next();
        pinMode(req.value("Value2").toInt(),OUTPUT);
        z2HG = req.value("Value2").toInt();

        cout << GREEN << "wiringPi started" << NOCOLOR << endl;
    }
#endif

    int on(1), off(0);
    req.exec("SELECT * FROM CVOrder WHERE Name='GPIO' AND Value1='4'");
    req.next();
    if(req.value("Value2").toInt() == 1)
    {
        on = 0;
        off = 1;
    }

    QTimer timer;
    connect(&timer,&QTimer::timeout,&loop,&QEventLoop::quit);
    cout << "Activation Z1 Eco" << endl;
    digitalWrite(z1Eco,on);
    timer.start(5000);
    loop.exec();
    digitalWrite(z1Eco,off);

    cout << "Activation Z1 Hors Gel" << endl;
    digitalWrite(z1HG,on);
    timer.start(5000);
    loop.exec();
    digitalWrite(z1HG,off);

    cout << "Activation Z2 Eco" << endl;
    digitalWrite(z2Eco,on);
    timer.start(5000);
    loop.exec();
    digitalWrite(z2Eco,off);

    cout << "Activation Z2 Hors Gel" << endl;
    digitalWrite(z2HG,on);
    timer.start(5000);
    loop.exec();
    digitalWrite(z1HG,off);

    cout << "Sondes de température connectées : " << endl;
    QDir dir;
    dir.setPath("/sys/bus/w1/devices");
    QStringList list = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for(int i = 0;i < list.count()-1;i++) {
        cout << i+1 << " - " << list.at(i).toStdString();
        QFile f(dir.path() + "/" + list.at(i) + "/w1_slave");
        if(!f.open(QIODevice::ReadOnly)) {
            cout << "Echec d'ouverture du fichier" << endl;
        }
        else {
            QString result = f.readAll();
            cout << " : " << result.split("=").last().toDouble() / 1000 << " degrés Celcius" << endl << endl;
            f.close();
        }
    }

    GeneralMenu();
}

void Configure::StateMenu()
{
    QSqlQuery req;
    bool webSock = false;

    //Admin
    cout << "Etat serveur :" << endl;
    cout << "--------------------- Serveur Admin ---------------------" << endl;

    req.exec("SELECT * FROM General WHERE Name='WebAdminSocket'");
    req.next();
    if(req.value("Value1").toInt())
    {
        cout << "Type : WebSocket" << endl;
        webSock = true;
    }
    else
        cout << "Type : TCP" << endl;

    req.exec("SELECT * FROM General WHERE Name='Password'");
    req.next();
    cout << "Mot de passe : " << req.value("Value1").toString().toStdString() << endl;

    req.exec("SELECT * FROM General WHERE Name='Port'");
    req.next();
    cout << "Port : " << req.value("Value1").toInt() << endl;

    if(webSock)
    {
        req.exec("SELECT * FROM General WHERE Name='Port'");
        req.next();
        webSocket.open(QUrl("ws://127.0.0.1:" + req.value("Value1").toString()));
        QEventLoop loop;
        connect(&webSocket,&QWebSocket::connected,&loop,&QEventLoop::quit);
        connect(&webSocket,&QWebSocket::disconnected,&loop,&QEventLoop::quit);
        loop.exec();

        if(webSocket.state() == QAbstractSocket::ConnectedState)
            cout << "Status : " << GREEN << " En Service " << NOCOLOR << endl;
        else
            cout << "Status : " << RED << " Hors Service " << NOCOLOR << endl;
    }
    else
    {
        socket = new QTcpSocket;
        req.exec("SELECT * FROM General WHERE Name='Port'");
        req.next();
        socket->connectToHost("127.0.0.1",static_cast<quint16>(req.value("Value1").toInt()));
        if(socket->waitForConnected())
            cout << "Status : " << GREEN << " En Service " << NOCOLOR << endl;
        else
            cout << "Status : " << RED << " Hors Service " << NOCOLOR << endl;
        socket->close();
    }

    //User
    cout << endl << "--------------------- Serveur User ---------------------" << endl;

    webSock = false;
    req.exec("SELECT * FROM General WHERE Name='WebSocket'");
    req.next();
    if(req.value("Value1").toInt())
    {
        cout << "Type : WebSocket" << endl;
        webSock = true;
    }
    else
        cout << "Type : TCP" << endl;

    req.exec("SELECT * FROM General WHERE Name='WebPassword'");
    req.next();
    cout << "Mot de passe : " << req.value("Value1").toString().toStdString() << endl;

    req.exec("SELECT * FROM General WHERE Name='WebPort'");
    req.next();
    cout << "Port : " << req.value("Value1").toInt() << endl;

    if(webSock)
    {
        req.exec("SELECT * FROM General WHERE Name='WebPort'");
        req.next();
        QEventLoop loop;
        connect(&webSocket,&QWebSocket::connected,&loop,&QEventLoop::quit);
        connect(&webSocket,&QWebSocket::disconnected,&loop,&QEventLoop::quit);
        webSocket.open(QUrl("ws://127.0.0.1:" + req.value("Value1").toString()));
        loop.exec();

        if(webSocket.state() == QAbstractSocket::ConnectedState)
            cout << "Status : " << GREEN << " En Service " << NOCOLOR << endl << endl;
        else
            cout << "Status : " << RED << " Hors Service " << NOCOLOR << endl << endl;
    }
    else
    {
        socket = new QTcpSocket;
        req.exec("SELECT * FROM General WHERE Name='WebPort'");
        req.next();
        socket->connectToHost("127.0.0.1",static_cast<quint16>(req.value("Value1").toInt()));
        if(socket->waitForConnected())
            cout << "Status : \033[0;32m En Service \033[0m" << endl << endl;
        else
            cout << "Status : \033[0;31m Hors Service \033[0m" << endl << endl;
        socket->close();
    }

    GeneralMenu();
}

int Configure::Question(QStringList options, int max)
{
    for(int i = 0;i < options.count();i++) {
        cout << i+1 << " - " << options.at(i).toStdString() << "\n";
    }
    int result = -1;
    while(result < 1 || result > max)
    {
        cout << "Choix : ";
        cin >> result;
        cout << endl;
    }
    return result;
}
