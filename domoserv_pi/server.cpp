#include "server.h"

//Version 1.01

Server::Server()
{
    #define className "Serveur"

    server = new QTcpServer;
    connect(server,SIGNAL(newConnection()),this,SLOT(NewConnexion()));
    UserServer = new QTcpServer;
    connect(UserServer,SIGNAL(newConnection()),this,SLOT(NewConnexion()));
#ifdef WEBSECURED
    webServer = new QWebSocketServer("webServer",QWebSocketServer::SecureMode);
#else
    webServer = new QWebSocketServer("webServer",QWebSocketServer::NonSecureMode);
#endif
    qDebug()<<"SSL version use for build: "<<QSslSocket::sslLibraryBuildVersionString();
    qDebug()<<"SSL version use for run-time: "<<QSslSocket::sslLibraryVersionNumber();
    connect(webServer,SIGNAL(newConnection()),this,SLOT(NewWebConnexion()));

    QSqlQuery req;
    req.exec("SELECT * FROM General WHERE Name='Port'");
    if(!req.next())
    {
        req.exec("SELECT MAX(ID) FROM General");
        req.next();
        int id = req.value(0).toInt()+1;
        req.exec("INSERT INTO General VALUES('" + QString::number(id) + "','Port','49152','','','')");
        id++;
        req.exec("INSERT INTO General VALUES('" + QString::number(id) + "','Password','','','','')");
        id++;
        req.exec("INSERT INTO General VALUES('" + QString::number(id) + "','WebSocket','','','','')");
    }
    req.exec("SELECT * FROM General WHERE Name='WebPort'");
    if(!req.next())
    {
        req.exec("SELECT MAX(ID) FROM General");
        req.next();
        int id = req.value(0).toInt()+1;
        req.exec("INSERT INTO General VALUES('" + QString::number(id) + "','WebPort','49155','','','')");
        id++;
        req.exec("INSERT INTO General VALUES('" + QString::number(id) + "','WebPassword','','','','')");
    }
}

void Server::Reload()
{
    for(int i=0;i<adminList.count();i++)
    {
        emit Info(className,"Forced disconnect user");
        adminList.at(i)->disconnectFromHost();
    }
    adminList.clear();
    for (int i=0;i<webUsersList.count();i++) {
        emit Info(className,"Forced disconnect web user");
        webUsersList.at(i)->close();
    }
    webUsersList.clear();
    dataSize = 0;
    password.clear();
    webPassword.clear();
    PKEY.clear();

    emit Info(className,"Closing server...");
    server->close();
    webServer->close();
    emit Info(className,"Server closed");

    server = new QTcpServer;
    connect(server,SIGNAL(newConnection()),this,SLOT(NewConnexion()));
    webServer = new QWebSocketServer("webServer",QWebSocketServer::NonSecureMode);
    connect(webServer,SIGNAL(newConnection()),this,SLOT(NewWebConnexion()));

    Init();
}

void Server::Init()
{
    emit Info(className,"Starting server");
    dataSize = 0;

    //Server
    //PKEY
    GeneratePKEY();
    emit Info(className,"PKEY generated : " + PKEY + "");

    //Password
    emit Info(className,"------------------Server Info-------------------------");
    QSqlQuery req;
    req.exec("SELECT Value1 FROM General WHERE Name='Password'");
    req.next();
    password = req.value(0).toString();
    emit Info(className,"Password = " + password + "");

    //Port
    req.exec("SELECT Value1 FROM General WHERE Name='Port'");
    req.next();
    emit Info(className,"Port = " + req.value(0).toString());
    emit Info(className,"------------------------------------------------------");

    //Run server
    if(StartServer())
        emit Info(className,"[\033[0;32m  OK  \033[0m] Server started");
    else
        emit Info(className,"[\033[0;31mFAILED\033[0m]starting server failed");



    //WebServer
    req.exec("SELECT Value1 FROM General WHERE Name='WebSocket'");
    req.next();
    if(req.value(0).toInt() == 1)
    {
        emit Info(className,"Starting Web Socket");
        emit Info(className,"------------------Web Server Info---------------------");

        //webPassword
        req.exec("SELECT Value1 FROM General WHERE Name='WebPassword'");
        req.next();
        webPassword = req.value(0).toString();
        emit Info(className,"Web Password = " + webPassword + "");

        //Port
        req.exec("SELECT Value1 FROM General WHERE Name='WebPort'");
        req.next();
        emit Info(className,"Web Port = " + req.value(0).toString() + "");
        emit Info(className,"------------------------------------------------------");

        if(StartWebServer())
            emit Info(className,"[\033[0;32m  OK  \033[0m] Web Socket started");
        else
            emit Info(className,"[\033[0;31mFAILED\033[0m]starting Web Socket failed");
    }
}

bool Server::StartServer()
{
    QSqlQuery req;
    req.exec("SELECT Value1 FROM General WHERE Name='Port'");
    req.next();
    return server->listen(QHostAddress::Any,req.value(0).toInt());
}

void Server::NewConnexion()
{
    while(server->hasPendingConnections())
    {
        QTcpSocket *newCo = server->nextPendingConnection();

        connect(newCo,SIGNAL(readyRead()),this,SLOT(ReceiptData()));
        connect(newCo,SIGNAL(disconnected()),this,SLOT(Disconnect()));
        emit Info(className,"New Admin connected(" + newCo->peerAddress().toString().toLatin1() + ")");
    }
    while(UserServer->hasPendingConnections())
    {
        QTcpSocket *newCo = UserServer->nextPendingConnection();

        connect(newCo,SIGNAL(readyRead()),this,SLOT(ReceiptData()));
        connect(newCo,SIGNAL(disconnected()),this,SLOT(Disconnect()));
        emit Info(className,"New Admin connected(" + newCo->peerAddress().toString().toLatin1() + ")");
    }
}

void Server::Disconnect()
{
    QTcpSocket *co = qobject_cast<QTcpSocket *>(sender());
    adminList.removeOne(co);
    emit Info(className,"User disconnected(" + co->peerAddress().toString().toLatin1() + ")");
}

void Server::SendToAll(QString data)
{
    QByteArray paquet;

    QDataStream out(&paquet, QIODevice::WriteOnly);

    out << (quint16) 0;
    out << Encrypt(data);
    out.device()->seek(0);
    out << (quint16) (paquet.size() - sizeof(quint16));

    for(int i=0;i<adminList.count();i++)
    {
        adminList.at(i)->write(paquet);
    }
}

void Server::SendToUser(QTcpSocket *user, QString data)
{
    QByteArray paquet;

    QDataStream out(&paquet, QIODevice::WriteOnly);

    quint16 empty(0);
    out << empty;

    if(data == PKEY)
        out << data;
    else
        out << Encrypt(data);

    out.device()->seek(0);
    out << (quint16) (paquet.size() - sizeof(quint16));

    //quint64* sizePaquet = reinterpret_cast<quint16*>(paquet.size());
     //sizePaquet -= sizeof(quint16);
     //out << sizePaquet;

     user->write(paquet);
}

void Server::ReceiptData()
{
    QTcpSocket *socket = new QTcpSocket;
    while(socket)
    {
        socket = qobject_cast<QTcpSocket *>(sender());

        if (!socket)
            return;

        QDataStream in(socket);

        if(dataSize == 0)
        {
            if(socket->bytesAvailable() < (uint)sizeof(quint16))
                 return;
            in >> dataSize;
        }

        if(socket->bytesAvailable() < dataSize)
            return;

        QString data;
        in >> data;
        qDebug() << data;
        if(!adminList.contains(socket))
            AddUserToList(socket,data);
        else
            emit Receipt(socket,Decrypt(data));

        dataSize = 0;
    }
}

void Server::AddUserToList(QTcpSocket *socket, QString data)
{
    if(data == password)
    {
        adminList.append(socket);
        SendToUser(socket,PKEY);
        emit Info(className,"New user accepted");
    }
    else
    {
        socket->close();
        emit Info(className,"New user refused");
    }
}

bool Server::StartWebServer()
{
#ifdef WEBSECURED
        SecureWebSocket();
#endif
    QSqlQuery req;
    req.exec("SELECT Value1 FROM General WHERE Name='WebPort'");
    req.next();
    return webServer->listen(QHostAddress::Any,req.value(0).toInt());
}

void Server::SecureWebSocket()
{
#ifdef WEBSECURED
    QSslConfiguration sslConfiguration;
    QFile certFile(QStringLiteral("CA/cacert.pem"));
    QFile keyFile(QStringLiteral("CA/private/cakey.pem"));
    certFile.open(QIODevice::ReadOnly);
    keyFile.open(QIODevice::ReadOnly);
    QSslCertificate certificate(&certFile, QSsl::Pem);
    QSslKey sslKey(&keyFile, QSsl::Rsa, QSsl::Pem);
    certFile.close();
    keyFile.close();
    sslConfiguration.setPeerVerifyMode(QSslSocket::VerifyNone);
    sslConfiguration.setLocalCertificate(certificate);
    sslConfiguration.setPrivateKey(sslKey);
    sslConfiguration.setProtocol(QSsl::TlsV1SslV3);
    webServer->setSslConfiguration(sslConfiguration);

    connect(webServer, &QWebSocketServer::sslErrors, this, &Server::SslErrors);
#endif

}

void Server::SslErrors(const QList<QSslError> &err)
{
#ifdef WEBSECURED
    for (int i = 0;i<err.count();i++) {
        emit Info(className,"SSL Error : " + err.at(i).errorString());
    }
#endif
}

void Server::NewWebConnexion()
{
    if(webServer->hasPendingConnections())
    {
        QWebSocket *socket = webServer->nextPendingConnection();
        connect(socket,&QWebSocket::textMessageReceived,this,&Server::ReceiptMessage);
        connect(socket,&QWebSocket::disconnected,this,&Server::WebDisconnect);

        emit Info(className,"New Web user connected(" + socket->peerAddress().toString() + ")");
    }
}

void Server::WebDisconnect()
{
    QWebSocket *socket = qobject_cast<QWebSocket *>(sender());
    emit Info(className,"Web user disconnected(" + socket->peerAddress().toString() + ")");
    webUsersList.removeOne(socket);
    socket->deleteLater();
}

void Server::ReceiptMessage(QString text)
{
    QWebSocket *socket = qobject_cast<QWebSocket *>(sender());
    if(socket)
    {
        if(!webUsersList.contains(socket))
        {
            if(text == webPassword)
            {
                webUsersList.append(socket);
                socket->sendTextMessage(PKEY);
                emit Info(className,"new Web user accepted(" + socket->peerAddress().toString() + ")");
            }
            else
            {
                emit Info(className,"new Web user refused(" + socket->peerAddress().toString() + ")");
                socket->close();
            }
        }
        else {
            #ifdef WEBSECURED
                emit WebReceipt(socket,text);
            #else
                emit WebReceipt(socket,Decrypt(text));
            #endif
        }
    }

}

void Server::SendToWebUser(QWebSocket *socket, QString data)
{
    if(socket)
    {
        #ifdef WEBSECURED
            socket->sendTextMessage(data);
        #else
            socket->sendTextMessage(Encrypt(data));
        #endif
    }
}

void Server::GeneratePKEY()
{
    srand(QTime::currentTime().msec());
    QString key;

    for(int i = 0;i<50;i++)
    {
        key.append(QString::number(rand() % 250) + " ");
    }
    key.remove(key.count()-1,key.count()-1);
    PKEY = key;
}

QString Server::Encrypt(QString text)
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

QString Server::Decrypt(QString text)
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
