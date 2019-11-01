﻿#include "configure.h"

//Version 1.1

Configure::Configure()
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

    GeneralMenu();
}

void Configure::GeneralMenu()
{
    int result = 0;

    cout << "1 - Etat serveur" << endl;
    cout << "2 - Configuration" << endl;
    cout << "3 - Test" << endl;
    cout << "4 - Quitter" << endl;

    while(result < 1 || result > 4)
    {
        cout << "Choix : ";
        cin >> result;
        cout << endl;
    }


    if(result == 1)
    {
        StateMenu();
    }
    else if(result == 2)
    {
        ConfigMenu();
    }
    else if(result == 3)
    {
        Test();
    }
    else if(result == 4)
    {
        this->deleteLater();
    }
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
        loop.exec();

        if(webSocket.state() == QAbstractSocket::ConnectedState)
            cout << "Status : " << GREEN << " En Service " << NOCOLOR << endl;
        else
            cout << "Status : " << RED << " Hors Service " << NOCOLOR << endl;
    }
    else
    {
        socket = new QTcpSocket;
        connect(socket,SIGNAL(readyRead()),this,SLOT(Receipt_Data()));
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
        connect(socket,SIGNAL(readyRead()),this,SLOT(Receipt_Data()));
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

void Configure::ConfigMenu()
{
    cout << "Configuration :" << endl;

    cout << "1 - Général" << endl;
    cout << "2 - Gestionnaire chauffage" << endl;
    cout << "3 - Serveur" << endl;
    cout << "4 - Retour" << endl;

    int result = 0;

    while(result < 1 || result > 4)
    {
        cout << "Choix : ";
        cin >> result;
        cout << endl;
    }

    switch (result) {
    case 1:
        ConfigGeneralMenu();
        break;
    case 2:
        ConfigCVOrderMenu();
        break;
    case 3:
        ConfigServerMenu();
        break;
    case 4:
        GeneralMenu();
    }
}

void Configure::ConfigGeneralMenu()
{
    cout << "Configuration Général :" << endl;

    QSqlQuery req;
    req.exec("SELECT * FROM General WHERE Name='CVOrder'");
    req.next();

    QString state;
    if(req.value("Value1").toBool())
        state = QString(GREEN) + "Actif" + QString(NOCOLOR);
    else
        state = QString(RED) + "Inactif" + QString(NOCOLOR);

    cout << "1 - Gestionnaire chauffage " << state.toStdString() << endl;
    cout << "2 - Retour" << endl;

    int result = 0;

    while(result < 1 || result > 2)
    {
        cout << "Choix : ";
        cin >> result;
        cout << endl;
    }

    QString value = "1";
    switch (result) {
    case 1:
        if(req.value("Value1").toBool())
            value = "0";
        req.exec("UPDATE General Set Value1='" + value + "' WHERE Name='CVOrder'");
        ConfigGeneralMenu();
        break;
    case 2:
        ConfigMenu();
    }
}

void Configure::ConfigCVOrderMenu()
{
    cout << "Configuration Gestionnaire chauffage :" << endl;

    QSqlQuery req;

    QString value;
    req.exec("SELECT * FROM CVOrder WHERE Name='Priority'");
    req.next();
    if(req.value("Value1").toInt() == horloge)
        value = "Horloge";
    else if(req.value("Value1").toInt() == network)
        value = "Réseau";
    else
        value = "Horloge + Réseau";
    cout << "1 - Changer la priorité : " << value.toStdString() << endl;

    req.exec("SELECT * FROM CVOrder WHERE Name='Act_Network'");
    req.next();
    cout << "2 - Temps entre chaque scan réseau : " << QString::number(req.value("Value2").toInt()/1000).toStdString() << " s" << endl;

    req.exec("SELECT * FROM CVOrder WHERE Name='GPIO' AND Value1='0'");
    req.next();
    cout << "3 - Changer GPIO Zone 1 Eco : " << req.value("Value2").toString().toStdString() << endl;

    req.exec("SELECT * FROM CVOrder WHERE Name='GPIO' AND Value1='1'");
    req.next();
    cout << "4 - Changer GPIO Zone 1 Hors Gel : " << req.value("Value2").toString().toStdString() << endl;

    req.exec("SELECT * FROM CVOrder WHERE Name='GPIO' AND Value1='2'");
    req.next();
    cout << "5 - Changer GPIO Zone 2 Eco : " << req.value("Value2").toString().toStdString() << endl;

    req.exec("SELECT * FROM CVOrder WHERE Name='GPIO' AND Value1='3'");
    req.next();
    cout << "6 - Changer GPIO Zone 2 Hors Gel : " << req.value("Value2").toString().toStdString() << endl;

    cout << "7 - Sonde de température" << endl;

    cout << "8 - Compteur d'énergie : ";
    req.exec("SELECT * FROM CVOrder WHERE Name='ActCPTEnergy'");
    req.next();
    if(req.value("Value1").toInt() == 0)
        cout << RED << "Désactivé" << NOCOLOR << endl;
    else {
        cout << GREEN << "Activé" << NOCOLOR << endl;
    }

    req.exec("SELECT * FROM CVOrder WHERE Name='ImpWattCPTEnergy'");
    req.next();
    cout << "9 - Nombre de watt par impulsion : " << QString::number(req.value("Value1").toInt()).toStdString() << " Wh" << endl;

    req.exec("SELECT * FROM CVOrder WHERE Name='GPIO' AND Value1='" + QString::number(ImpCPTEnergy) + "'");
    req.next();
    cout << "10 - Changer GPIO entrée compteur : " << req.value("Value2").toString().toStdString() << endl;

    req.exec("SELECT * FROM CVOrder WHERE Name='FileCPTEnergy'");
    req.next();
    cout << "11 - Emplacement dossier contenant les fichiers data compteur : " << req.value("Value1").toString().toStdString() << endl;

    cout << "12 - Réglage horloge" << endl;

    cout << "13 - Ajout/Suppression Ip" << endl;

    cout << "14 - Retour" << endl;

    int result = 0;

    while(result < 1 || result > 14)
    {
        cout << "Choix : ";
        cin >> result;
        cout << endl;
    }
    int p;
    string v;
    QString v2;
    switch (result) {
    case 1:
        cout << "1 - Horloge" << endl;
        cout << "2 - Réseau" << endl;
        cout << "3 - Horloge + Réseau" << endl;
        cout << "Choix : ";
        cin >> p;
        cout << endl;
        if(p == 1)
            p = horloge;
        else if(p == 2)
            p = network;
        else
            p = networkAndHorloge;
        req.exec("UPDATE CVOrder SET Value1='" + QString::number(p) + "' WHERE Name='Priority'");
        ConfigCVOrderMenu();
        break;
    case 2:
        cout << "Saisissez une nouvelle valeur en secondes" << endl;
        cin >> p;
        if(p > 30)
            req.exec("UPDATE CVOrder SET Value2='" + QString::number(p*1000) + "' WHERE Name='Act_Network'");
        ConfigCVOrderMenu();
        break;
    case 3:
        cout << "Saisissez une nouvelle valeur de 0 à 27 : ";
        cin >> p;
        cout << endl;
        if(p <= 27)
            req.exec("UPDATE CVOrder SET Value2='" + QString::number(p) + "' WHERE Name='GPIO' AND Value1='0'");
        ConfigCVOrderMenu();
        break;
    case 4:
        cout << "Saisissez une nouvelle valeur de 0 à 27 : ";
        cin >> p;
        cout << endl;
        if(p <= 27)
            req.exec("UPDATE CVOrder SET Value2='" + QString::number(p) + "' WHERE Name='GPIO' AND Value1='1'");
        ConfigCVOrderMenu();
        break;
    case 5:
        cout << "Saisissez une nouvelle valeur de 0 à 27 : ";
        cin >> p;
        cout << endl;
        if(p <= 27)
            req.exec("UPDATE CVOrder SET Value2='" + QString::number(p) + "' WHERE Name='GPIO' AND Value1='2'");
        ConfigCVOrderMenu();
        break;
    case 6:
        cout << "Saisissez une nouvelle valeur de 0 à 27 : ";
        cin >> p;
        cout << endl;
        if(p < 27)
            req.exec("UPDATE CVOrder SET Value2='" + QString::number(p) + "' WHERE Name='GPIO' AND Value1='3'");
        ConfigCVOrderMenu();
        break;
    case 7:
        SondeTempMenu();
        break;
    case 8:
        req.exec("SELECT * FROM CVOrder WHERE Name='ActCPTEnergy'");
        req.next();
        if(req.value("Value1").toInt() == 0) {
            req.exec("UPDATE CVOrder SET Value1='1' WHERE Name='ActCPTEnergy'");
        }
        else {
            req.exec("UPDATE CVOrder SET Value1='0' WHERE Name='ActCPTEnergy'");
        }
        ConfigCVOrderMenu();
        break;
    case 9:
        cout << "Saisissez une nouvelle valeur en wh ";
        cin >> p;
        if(p > 0)
            req.exec("UPDATE CVOrder SET Value1='" + QString::number(p) + "' WHERE Name='ImpWattCPTEnergy'");
        cout << endl;
        ConfigCVOrderMenu();
        break;
    case 10:
        cout << "Saisissez une nouvelle valeur de 0 à 27 : ";
        cin >> p;
        cout << endl;
        if(p < 27)
            req.exec("UPDATE CVOrder SET Value2='" + QString::number(p) + "' WHERE Name='GPIO' AND Value1='" + QString::number(ImpCPTEnergy) + "'");
        ConfigCVOrderMenu();
        break;
    case 11:
        cout << "Saisissez un nouveau chemin vers le dossier data du compteur : ";
        cin >> v;
        cout << endl;
        if(QString::fromStdString(v).split("/").last() == "" && QString::fromStdString(v).split("/").count() >= 2)
            req.exec("UPDATE CVOrder SET Value1='" + QString::fromStdString(v) + "' WHERE Name='FileCPTEnergy'");
        ConfigCVOrderMenu();
        break;
    case 12:
        ProgMenu();
        break;
    case 13:
        IpMenu();
        break;
    case 14:
        ConfigMenu();
    }
}

void Configure::IpMenu()
{
    cout << "Ip Actuellement contrôlées :" << endl;

    QSqlQuery req;
    req.exec("SELECT * FROM CVOrder WHERE Name='IpPing'");
    while(req.next()) {
        cout << req.value("Value1").toString().toStdString() << endl;
    }
    cout << endl;

    cout << "1 - Ajouter" << endl;
    cout << "2 - Supprimer" << endl;
    cout << "3 - Retour" << endl;

    int result = 0;
    while(result < 1 || result > 3) {
        cout << "Choix : ";
        cin >> result;
    }
    cout << endl;

    if(result == 1) {
        cout << "Saisissez une ip : ";
        string sip;
        cin >> sip;
        cout << endl;
        QString ip = QString::fromStdString(sip);

        //Add
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
                    cout << "Ip already exist" << endl;
                else
                {
                    req.exec("SELECT MAX(ID) FROM CVOrder");
                    req.next();
                    int id = req.value(0).toInt()+1;
                    req.exec("INSERT INTO CVOrder VALUES('" + QString::number(id) + "','IpPing','" + QString::number(sIp1) + "." + QString::number(sIp2) + "." +
                             QString::number(sIp3) + "." + QString::number(sIp4) + "','','','')");
                    cout << ip.toStdString() + " ajouté" << endl;
                }
            }
        }
        IpMenu();
    }
    else if(result == 2) {
        QSqlQuery req;
        req.exec("SELECT * FROM CVOrder WHERE Name='IpPing'");
        QStringList list;
        int r = 1;
        while(req.next()) {
            cout << r << " - " << req.value("Value1").toString().toStdString() << endl;
            r++;
            list.append(req.value(0).toString());
        }
        cout << r << " - " << "Retour" << endl;

        int result = 0;
        while(result < 1 || result > r) {
            cin >> result;
        }
        if(result == r) {
            IpMenu();
            return;
        }
        req.exec("DELETE FROM CVOrder WHERE Name='IpPing' AND ID='" + list.at(result-1) + "'");
        IpMenu();
    }
    else {
        ConfigCVOrderMenu();
    }
}

void Configure::ProgMenu()
{
    QSqlQuery req;
    cout << "Liste des horaires programmés :" << endl;
    req.exec("SELECT * FROM CVOrder WHERE Name='Prog' ORDER BY Value1 ASC");
    while(req.next()) {
        if(req.value("Value3").toInt() == eco)
            cout << req.value("Value1").toString().toStdString() << " Z" << req.value("Value2").toString().toStdString() << " : Eco" << endl;
        else if(req.value("Value3").toInt() == confort)
            cout << req.value("Value1").toString().toStdString() << " Z" << req.value("Value2").toString().toStdString() << " : Confort" << endl;
    }
    cout << "--------------------" << endl;

    cout << "1 - Nouvelle programmation" << endl;
    cout << "2 - Supprimer programmation" << endl;
    cout << "3 - Retour" << endl;

    int result = 0;
    while(result < 1 || result > 3)
    {
        cout << "Choix : ";
        cin >> result;
        cout << endl;
    }


    if(result == 1) {
        //Heure
        QString hour;
        int h = -1;
        int mn = -1;
        while((h < 0 || h > 24) && (mn < 0 || mn > 59)) {
            cout << "Indiquez une heure(format xx:xx) :" << endl;
            string r;
            cin >> r;
            hour = QString::fromStdString(r);
            h = hour.split(":").first().toInt();
            mn = hour.split(":").last().toInt();
        }

        //Jour
        int r = 0;
        QStringList day;
        while(r != 8) {
            cout << "Selectionner le(s) jour(s) :" << endl;
            if(!day.contains("1"))
                cout << "1 - Lundi" << endl;
            if(!day.contains("2"))
                cout << "2 - Mardi" << endl;
            if(!day.contains("3"))
                cout << "3 - Mercredi" << endl;
            if(!day.contains("4"))
                cout << "4 - Jeudi" << endl;
            if(!day.contains("5"))
                cout << "5 - Vendredi" << endl;
            if(!day.contains("6"))
                cout << "6 - Samedi" << endl;
            if(!day.contains("7"))
                cout << "7 - Dimanche" << endl;
            cout << "8 - Suivant" << endl;
            cout << "Choix : ";
            cin >> r;
            cout << endl;

            if(r > 0 && r < 8) {
                day.append(QString::number(r));
            }
        }

        //Zone
        r = 0;
        int zone = -1;
        while(r != 1 && r != 2) {
            cout << "Selectionner la zone :" << endl;
            cout << "1 - Zone 1" << endl;
            cout << "2 - Zone 2" << endl;
            cout << "Choix : ";
            cin >> r;
            cout << endl;
        }
        if(r == 1)
            zone = Z1;
        else
            zone = Z2;

        //Etat
        r = 0;
        int state = -1;
        while(r != 1 && r != 2) {
            cout << "Selectionner l'état :" << endl;
            cout << "1 - Confort" << endl;
            cout << "2 - Eco" << endl;
            cout << "Choix : ";
            cin >> r;
            cout << endl;
        }
        if(r == 1)
            state = confort;
        else
            state = eco;

        //Resumé
        cout << "Les horaires suivantes seront ajouté à la base de données :" << endl;
        for(int i=0;i<day.count();i++) {
            cout << DaytoString(day.at(i).toInt()).toStdString() << " " << hour.toStdString();
            if(zone == Z1)
                cout << " Z1";
            else
                cout << " Z2";
            if(state == confort)
                cout << " Confort";
            else
                cout << " Eco";
            cout << endl;
        }
        cout << "Enregistrer ?" << endl;

        r = 0;
        while(r != 1 && r != 2) {
            cout << "1 - Oui" << endl;
            cout << "2 - Non" << endl;
            cin >> r;
        }
        if(r == 1) {
            req.exec("SELECT MAX(ID) FROM CVOrder");
            req.next();

            int id = req.value(0).toInt();
            for(int i=0;i<day.count();i++) {
                QDate date;
                date.setDate(2018,01,day.at(i).toInt());
                QTime time;
                time.setHMS(h,mn,0);
                //---if exist
                req.exec("SELECT * FROM CVOrder WHERE Name='Prog' AND Value1='" + date.toString("yyyy-MM-dd") + time.toString(" hh:mm") + "' AND Value2='" + QString::number(zone) + "'");
                if(req.next())
                    continue;
                //---
                id++;
                req.exec("INSERT INTO CVOrder VALUES('" + QString::number(id) + "','Prog','" + date.toString("yyyy-MM-dd") + time.toString(" hh:mm") + "','" + QString::number(zone) + "','" + QString::number(state) + "','')");
            }
        }
        ProgMenu();

    }
    else if(result == 2) {
        int value = 1;
        QList<int> list;
        req.exec("SELECT * FROM CVOrder WHERE Name='Prog' ORDER BY Value1 ASC");
        while(req.next()) {
            list.append(req.value(0).toInt());
            if(req.value("Value3").toInt() == eco)
                cout << value << " - " << req.value("Value1").toString().toStdString() << " Z" << req.value("Value2").toString().toStdString() << " : Eco" << endl;
            else if(req.value("Value3").toInt() == confort)
                cout << value << " - " << req.value("Value1").toString().toStdString() << " Z" << req.value("Value2").toString().toStdString() << " : Confort" << endl;
            value++;
        }
        value = 0;
        while(value < 1 || value > list.count()) {
        cout << "Choix : ";
        cin >> value;
        cout << endl;
        }
        req.seek(value-1);
        if(req.value("Value3").toInt() == eco)
            cout << value << " - " << req.value("Value1").toString().toStdString() << " Z" << req.value("Value2").toString().toStdString() << " : Eco" << endl;
        else if(req.value("Value3").toInt() == confort)
            cout << value << " - " << req.value("Value1").toString().toStdString() << " Z" << req.value("Value2").toString().toStdString() << " : Confort" << endl;
        cout << "Supprimer ?" << endl;
        int v = 0;
        cout << "1 - Oui" << endl;
        cout << "2 - Non" << endl;
        cin >> v;
        cout << endl;
        if(v == 1) {
            req.exec("DELETE FROM CVOrder WHERE Name='Prog' AND ID='" + QString::number(list.at(value-1)) + "'");
        }
        ProgMenu();
    }
    else {
        ConfigCVOrderMenu();
    }
}

int Configure::DaytoInt(QString day)
{
    if(day == "Lundi")
        return 1;
    else if(day == "Mardi")
        return 2;
    else if(day == "Mercredi")
        return 3;
    else if(day == "Jeudi")
        return 4;
    else if(day == "Vendredi")
        return 5;
    else if(day == "Samedi")
        return 6;
    else if(day == "Dimanche")
        return 7;
}

QString Configure::DaytoString(int day)
{
    switch (day) {
    case 1:
        return "Lundi";
    case 2:
        return "Mardi";
    case 3:
        return "Mercredi";
    case 4:
        return "Jeudi";
    case 5:
        return "Vendredi";
    case 6:
        return "Samedi";
    case 7:
        return "Dimanche";
    default:
        return "";
    }
}

void Configure::SondeTempMenu()
{
    cout << "Sonde de température : " << endl;
    cout << "-----------------------" << endl;

    QSqlQuery req;
    req.exec("SELECT * FROM CVOrder WHERE Name='Temp'");
    while(req.next())
    {
        QString emp;
        if(req.value("Value1").toInt() == Indoor)
            emp = "Intérieur";
        else if(req.value("Value1").toInt() == Outdoor)
            emp = "Extérieur";
        cout << emp.toStdString() << " " << req.value("Value2").toString().toStdString() << endl;
    }

    cout << "-----------------------" << endl;
    cout << "1 - Ajouter" << endl;
    cout << "2 - Supprimer" << endl;
    cout << "3 - Retour" << endl;
    int p = 0;
    while(p < 1 || p > 3)
    {
        cout << "Choix : ";
        cin >> p;
        cout << endl;
    }

    if(p == 1)
    {
        int count = 0;
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
                    count++;
                }
            if(!exist)
            {
                cout << i+1 << " - " << list.at(i).toStdString();
                QFile f(dir.path() + "/" + list.at(i) + "/w1_slave");
                if(!f.open(QIODevice::ReadOnly)) {
                    cout << "Echec d'ouverture du fichier" << endl;
                }
                else {
                    QString result = f.readAll();
                    cout << " : " << result.split("=").last().toDouble() / 1000 << " degrés Celcius" << endl;
                    f.close();
                    p = list.count();
                }
            }
        }
        if(count >= 2)
        {
            cout << "Seulement 2 sondes maximum !" << endl;
            SondeTempMenu();
            return;
        }
        cout << p << " - Retour" << endl;
        p = 0;

        while(p < 1)
        {
            cout << "Choix : ";
            cin >> p;
            cout << endl;
        }

        if(list.count() > p)
        {
            cout << "Selectionnez l'emplacement : " << endl;
            cout << "1 - Intérieur" << endl;
            cout << "2 - Extérieur" << endl;
            cout << "3 - Abandon" << endl;
        }
        else
        {
            SondeTempMenu();
            return;
        }
        int p2 = 0;
        while(p2 < 1 || p2 > 3)
        {
            cout << "Choix : ";
            cin >> p2;
            cout << endl;
        }

        int emp = 0;
        if(p2 == 1)
            emp = Indoor;
        else if(p2 == 2)
            emp = Outdoor;
        else
        {
            SondeTempMenu();
            return;
        }

        req.exec("SELECT * FROM CVOrder WHERE Name='Temp' AND Value1='" + QString::number(emp) + "'");
        if(req.next())
        {
            cout << "1 sonde existe déjà à cette emplacement" << endl;
            SondeTempMenu();
            return;
        }
        req.exec("SELECT MAX(ID) FROM CVOrder");
        req.next();
        req.exec("INSERT INTO CVOrder VALUES('" + QString::number(req.value(0).toInt()+1) + "','Temp','" + QString::number(emp) + "','" + list.at(p-1) + "','','')");
    }
    else if(p == 2)
    {
        req.exec("SELECT * FROM CVOrder WHERE Name='Temp'");
        p = 0;
        while(req.next())
        {
            p++;
            QString emp;
            if(req.value("Value1").toInt() == Indoor)
                emp = "Intérieur";
            else if(req.value("Value1").toInt() == Outdoor)
                emp = "Extérieur";
            cout << p << " - " << emp.toStdString() << " " << req.value("Value2").toString().toStdString() << endl;
        }
        cout << "3 - Retour" << endl;
        p = 0;

        while(p < 1)
        {
            cout << "Choix : ";
            cin >> p;
            cout << endl;
        }
        if(p == 1)
        {
            req.exec("SELECT ID FROM CVOrder WHERE Name='Temp'");
            req.next();
            int id = req.value(0).toInt();
            req.exec("DELETE FROM CVOrder WHERE Name='Temp' AND ID='" + QString::number(id) + "'");
        }
        else if(p == 2)
        {
            req.exec("SELECT ID FROM CVOrder WHERE Name='Temp'");
            req.next();
            req.next();
            int id = req.value(0).toInt();
            req.exec("DELETE FROM CVOrder WHERE Name='Temp' AND ID='" + QString::number(id) + "'");
        }
        else {
            SondeTempMenu();
            return;
        }
    }
    else {
        ConfigCVOrderMenu();
        return;
    }
    SondeTempMenu();
}

void Configure::ConfigServerMenu()
{
    cout << "Configuration Serveur :" << endl;

    QSqlQuery req;

    req.exec("SELECT * FROM General WHERE Name='ActAdminServer'");
    req.next();
    QString value = QString(RED) + "Inactif" + QString(NOCOLOR);
    if(req.value("Value1").toBool())
        value = QString(GREEN) + "Actif" + QString(NOCOLOR);
    cout << "1 - Serveur admin : " << value.toStdString() << endl;

    req.exec("SELECT * FROM General WHERE Name='WebAdminSocket'");
    req.next();
    value = "TCP";
    if(req.value("Value1").toBool())
        value = "WebSocket";
    cout << "2 - Changer type serveur admin : " << value.toStdString() << endl;

    req.exec("SELECT * FROM General WHERE Name='Port'");
    req.next();
    cout << "3 - Changer port serveur admin : " << req.value("Value1").toString().toStdString() << endl;

    req.exec("SELECT * FROM General WHERE Name='Password'");
    req.next();
    cout << "4 - Changer mot de passe admin : " << req.value("Value1").toString().toStdString() << endl;

    req.exec("SELECT * FROM General WHERE Name='WebSocket'");
        req.next();
        value = "TCP";
        if(req.value("Value1").toBool())
            value = "WebSocket";
    cout << "5 - Changer type serveur utilisateur : " << value.toStdString() << endl;

    req.exec("SELECT * FROM General WHERE Name='WebPort'");
    req.next();
    cout << "6 - Changer port serveur utilisateur : " << req.value("Value1").toString().toStdString() << endl;

    req.exec("SELECT * FROM General WHERE Name='WebPassword'");
    req.next();
    cout << "7 - Changer mot de passe utilisateur : " << req.value("Value1").toString().toStdString() << endl;
    cout << "8 - Retour" << endl;

    int result = 0;

    while(result < 1 || result > 8)
    {
        cout << "Choix : ";
        cin >> result;
        cout << endl;
    }
    int p;
    string v;
    switch (result) {
    case 1:
        req.exec("SELECT * FROM General WHERE Name='ActAdminServer'");
        req.next();
        value = "1";
        if(req.value("Value1").toBool())
            value = "0";
        req.exec("UPDATE General SET Value1='" + value + "' WHERE Name='ActAdminServer'");
        ConfigServerMenu();
        break;
    case 2:
        req.exec("SELECT * FROM General WHERE Name='WebAdminSocket'");
        req.next();
        value = "1";
        if(req.value("Value1").toBool())
            value = "0";
        req.exec("UPDATE General SET Value1='" + value + "' WHERE Name='WebAdminSocket'");
        ConfigServerMenu();
        break;
    case 3:
        cout << "Saisissez un nouveau port : ";
        cin >> p;
        cout << endl;
        if(p < 1000 || p > 65000)
            cout << "port invalide !" << endl;
        else
            req.exec("UPDATE General SET Value1='" + QString::number(p) + "' WHERE Name='Port'");
        ConfigServerMenu();
        break;
    case 4:
        cout << "Saisissez un nouveau mot de passe : ";
        cin >> v;
        cout << endl;
        req.exec("UPDATE General SET Value1='" + QString::fromStdString(v) + "' WHERE Name='Password'");
        ConfigServerMenu();
        break;
    case 5:
        req.exec("SELECT * FROM General WHERE Name='WebSocket'");
        req.next();
        value = "1";
        if(req.value("Value1").toBool())
            value = "0";
        req.exec("UPDATE General SET Value1='" + value + "' WHERE Name='WebSocket'");
        ConfigServerMenu();
        break;
    case 6:
        cout << "Saisissez un nouveau port : ";
        cin >> p;
        cout << endl;
        if(p < 1000 || p > 65000)
            cout << "port invalide !" << endl;
        else
            req.exec("UPDATE General SET Value1='" + QString::number(p) + "' WHERE Name='WebPort'");
        ConfigServerMenu();
        break;
    case 7:
        cout << "Saisissez un nouveau mot de passe : ";
        cin >> v;
        cout << endl;
        req.exec("UPDATE General SET Value1='" + QString::fromStdString(v) + "' WHERE Name='WebPassword'");
        ConfigServerMenu();
        break;
    case 8:
        ConfigMenu();
    }
}
