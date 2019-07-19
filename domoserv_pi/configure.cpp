#include "configure.h"

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

void Configure::Receipt_Message(QString text)
{
    _dataResult = text;
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
        connect(socket,SIGNAL(connected()),this,SLOT(Send_Data()));
        req.exec("SELECT * FROM General WHERE Name='Port'");
        req.next();
        socket->connectToHost("127.0.0.1",req.value("Value1").toInt());
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
        connect(socket,SIGNAL(connected()),this,SLOT(Send_Data()));
        req.exec("SELECT * FROM General WHERE Name='WebPort'");
        req.next();
        socket->connectToHost("127.0.0.1",req.value("Value1").toInt());
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

    cout << "9 - Retour" << endl;

    int result = 0;

    while(result < 1 || result > 9)
    {
        cout << "Choix : ";
        cin >> result;
        cout << endl;
    }
    int p;
    string v;
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
        ConfigMenu();
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

    req.exec("SELECT * FROM General WHERE Name='WebAdminSocket'");
    req.next();
    QString value = "TCP";
    if(req.value("Value1").toBool())
        value = "WebSocket";
    cout << "1 - Changer type serveur admin : " << value.toStdString() << endl;

    req.exec("SELECT * FROM General WHERE Name='Port'");
    req.next();
    cout << "2 - Changer port serveur admin : " << req.value("Value1").toString().toStdString() << endl;

    req.exec("SELECT * FROM General WHERE Name='Password'");
    req.next();
    cout << "3 - Changer mot de passe admin : " << req.value("Value1").toString().toStdString() << endl;

    req.exec("SELECT * FROM General WHERE Name='WebSocket'");
        req.next();
        value = "TCP";
        if(req.value("Value1").toBool())
            value = "WebSocket";
    cout << "4 - Changer type serveur utilisateur : " << value.toStdString() << endl;

    req.exec("SELECT * FROM General WHERE Name='WebPort'");
    req.next();
    cout << "5 - Changer port serveur utilisateur : " << req.value("Value1").toString().toStdString() << endl;

    req.exec("SELECT * FROM General WHERE Name='WebPassword'");
    req.next();
    cout << "6 - Changer mot de passe utilisateur : " << req.value("Value1").toString().toStdString() << endl;
    cout << "7 - Retour" << endl;

    int result = 0;

    while(result < 1 || result > 7)
    {
        cout << "Choix : ";
        cin >> result;
        cout << endl;
    }
    int p;
    string v;
    switch (result) {
    case 1:
        req.exec("SELECT * FROM General WHERE Name='WebAdminSocket'");
        req.next();
        value = "1";
        if(req.value("Value1").toBool())
            value = "0";
        req.exec("UPDATE General SET Value1='" + value + "' WHERE Name='WebAdminSocket'");
        ConfigServerMenu();
        break;
    case 2:
        cout << "Saisissez un nouveau port : ";
        cin >> p;
        cout << endl;
        if(p < 1000 || p > 65000)
            cout << "port invalide !" << endl;
        else
            req.exec("UPDATE General SET Value1='" + QString::number(p) + "' WHERE Name='Port'");
        ConfigServerMenu();
        break;
    case 3:
        cout << "Saisissez un nouveau mot de passe : ";
        cin >> v;
        cout << endl;
        req.exec("UPDATE General SET Value1='" + QString::fromStdString(v) + "' WHERE Name='Password'");
        ConfigServerMenu();
        break;
    case 4:
        req.exec("SELECT * FROM General WHERE Name='WebSocket'");
        req.next();
        value = "1";
        if(req.value("Value1").toBool())
            value = "0";
        req.exec("UPDATE General SET Value1='" + value + "' WHERE Name='WebSocket'");
        ConfigServerMenu();
        break;
    case 5:
        cout << "Saisissez un nouveau port : ";
        cin >> p;
        cout << endl;
        if(p < 1000 || p > 65000)
            cout << "port invalide !" << endl;
        else
            req.exec("UPDATE General SET Value1='" + QString::number(p) + "' WHERE Name='WebPort'");
        ConfigServerMenu();
        break;
    case 6:
        cout << "Saisissez un nouveau mot de passe : ";
        cin >> v;
        cout << endl;
        req.exec("UPDATE General SET Value1='" + QString::fromStdString(v) + "' WHERE Name='WebPassword'");
        ConfigServerMenu();
        break;
    case 7:
        ConfigMenu();
    }
}

void Configure::Send_Data(QString data)
{
    if(data.isEmpty())
    {
        QSqlQuery req;
        req.exec("SELECT * FROM General WHERE Name='Password'");
        req.next();
        data = req.value("Value1").toString();
    }
    QByteArray paquet;

    QDataStream out(&paquet, QIODevice::WriteOnly);

    out << (quint16) 0;
    out << Encrypt(data);
    out.device()->seek(0);
    out << (quint16) (paquet.size() - sizeof(quint16));

    socket->write(paquet);
}

void Configure::Receipt_Data()
{
    if (socket == nullptr)
        return;

    QDataStream in(socket);

    if(dataSize == 0)
    {
        if(socket->bytesAvailable() < (int)sizeof(quint16))
             return;
        in >> dataSize;
    }

    if(socket->bytesAvailable() < dataSize)
        return;

    QString data;
    in >> data;

    if(PKEY.isEmpty())
    {
        PKEY = data;
        qDebug() << "PKEY : " + PKEY;
    }
    else
    {
        _dataResult = Decrypt(data);
    }

    dataSize = 0;
}

QString Configure::Encrypt(QString text)
{
    QString crypt;
    QStringList k = PKEY.split(" ");
    int idk(0);
    for(int i = 0;i<text.count();i++)
    {
        if(idk == k.count())
        {
            idk = 0;
        }
        int t = text.at(i).unicode();
        t -= k.at(idk).toInt();
        if(t > 250)
        {
            t = t - 250;
        }
        else if(t < 0)
        {
            t = t + 250;
        }
        if(t == 34)//si '
        {
            t = 251;
        }
        else if(t == 39)//ou "
        {
            t = 252;
        }
        crypt += QChar(t).toLatin1();
        idk++;
    }
    return crypt;
}

QString Configure::Decrypt(QString text)
{
    QString decrypt;
    QStringList k = PKEY.split(" ");
    int idk(0);
    for(int i = 0;i<text.count();i++)
    {
        if(idk == k.count())
        {
            idk = 0;
        }
        int t = text.at(i).unicode();
        if(t == 251)//retour a '
        {
            t = 34;
        }
        else if(t == 252)//retour a "
        {
            t = 39;
        }
        t += k.at(idk).toInt();
        if(t < 0)
        {
            t = t + 250;
        }
        else if(t > 250)
        {
            t = t - 250;
        }
        decrypt += QChar(t).toLatin1();
        idk++;
    }
    return decrypt;
}
