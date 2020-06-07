#include "server.h"

//Version 1.01

Server::Server()
{
    #define className "Serveur"

    server = new QTcpServer;
    connect(server,SIGNAL(newConnection()),this,SLOT(NewConnexion()));
    UserServer = new QTcpServer;
    connect(UserServer,SIGNAL(newConnection()),this,SLOT(NewConnexion()));

    webServer = new QWebSocketServer("webServer",QWebSocketServer::NonSecureMode);
    webAdminServer = new QWebSocketServer("webAdminServer",QWebSocketServer::NonSecureMode);

    connect(webServer,SIGNAL(newConnection()),this,SLOT(NewWebConnexion()));
    connect(webAdminServer,SIGNAL(newConnection()),this,SLOT(NewWebConnexion()));

    QSqlQuery req;
    req.exec("SELECT * FROM General WHERE Name='Port'");
    if(!req.next())
    {
        req.exec("SELECT MAX(ID) FROM General");
        req.next();
        int id = req.value(0).toInt()+1;
        req.exec("INSERT INTO General VALUES('" + QString::number(id) + "','Port','49152','','','')");
        id++;
        req.exec("INSERT INTO General VALUES('" + QString::number(id) + "','ActAdminServer','0','','','')");
        id++;
        req.exec("INSERT INTO General VALUES('" + QString::number(id) + "','Password','','','','')");
        id++;
        req.exec("INSERT INTO General VALUES('" + QString::number(id) + "','WebSocket','0','','','')");
        id++;
        req.exec("INSERT INTO General VALUES('" + QString::number(id) + "','WebAdminSocket','0','','','')");
        id++;
        req.exec("INSERT INTO General VALUES('" + QString::number(id) + "','WebPort','49155','','','')");
        id++;
        req.exec("INSERT INTO General VALUES('" + QString::number(id) + "','WebPassword','','','','')");
    }
}

Server::~Server()
{
    Stop();

    server->deleteLater();
    UserServer->deleteLater();
    webAdminServer->deleteLater();
    webServer->deleteLater();
}

bool Server::Stop()
{
    //TCP SERVER
    emit Info(className,"Forced admin logout");
    DisconnectUsers(adminList);
    //WEB SERVER
    emit Info(className,"Forced web admin logout");
    DisconnectUsers(webAdminList);
    //TCP SERVER USER
    emit Info(className,"Forced admin logout");
    DisconnectUsers(usersList);
    //WEB SERVER USER
    emit Info(className,"Forced web user logout");
    DisconnectUsers(webUsersList);
    //VARIABLES
    dataSize = 0;
    password.clear();
    webPassword.clear();

    emit Info(className,"Server shutdown...");
    server->close();
    webAdminServer->close();
    UserServer->close();
    webServer->close();

    //TEST
    if(!server->isListening() && !webAdminServer->isListening() && !UserServer->isListening() && !webServer->isListening())
    {
        emit Info(className,"[\033[0;32m  OK  \033[0m] Closed server");
        return true;
    }
    else
    {
        emit Info(className,"[\033[0;31mFAILED\033[0m] Server not closed");
        return false;
    }
}

template <class T>
void Server::DisconnectUsers(QList<T> list)
{
    for(int i=0;i<list.count();i++)
    {
        if(typeid(list.at(i)) == typeid(QTcpSocket) || typeid(list.at(i)) == typeid(QWebSocket)) {
            list.at(i)->close();
        }
        else {
            throw ServerException::UnknownType;
        }
    }
    list.clear();
}

bool Server::Reload()
{

    if(!Stop())
        return false;

    //INIT SERVER
    server = new QTcpServer;
    connect(server,SIGNAL(newConnection()),this,SLOT(NewConnexion()));
    UserServer = new QTcpServer;
    connect(UserServer,SIGNAL(newConnection()),this,SLOT(NewConnexion()));

    webServer = new QWebSocketServer("webServer",QWebSocketServer::NonSecureMode);
    webAdminServer = new QWebSocketServer("webAdminServer",QWebSocketServer::NonSecureMode);

    return Init();
}

bool Server::Init()
{
    emit Info(className,"Starting server");
    dataSize = 0;

    //Server Admin
    QSqlQuery req;
    req.exec("SELECT Value1 FROM General WHERE Name='ActAdminServer'");
    req.next();
    if(req.value(0).toBool()) {
        //Password
        emit Info(className,"------------------Server Admin Info-------------------");
        req.exec("SELECT Value1 FROM General WHERE Name='Password'");
        req.next();
        password = req.value(0).toString();
        emit Info(className,"Password = " + password + "");

        //Port
        req.exec("SELECT Value1 FROM General WHERE Name='Port'");
        req.next();
        emit Info(className,"Port = " + req.value(0).toString());

        //Run server
        if(StartServer())
            emit Info(className,"[\033[0;32m  OK  \033[0m] Server started");
        else
            emit Info(className,"[\033[0;31mFAILED\033[0m] starting server failed");

        emit Info(className,"------------------------------------------------------");
    }


    //Server User
    emit Info(className,"------------------Server User Info--------------------");

    //webPassword
    req.exec("SELECT Value1 FROM General WHERE Name='WebPassword'");
    req.next();
    webPassword = req.value(0).toString();
    emit Info(className,"Password = " + webPassword + "");

    //Port
    req.exec("SELECT Value1 FROM General WHERE Name='WebPort'");
    req.next();
    emit Info(className,"Port = " + req.value(0).toString() + "");

    if(StartWebServer())
        emit Info(className,"[\033[0;32m  OK  \033[0m] Server started");
    else
        emit Info(className,"[\033[0;31mFAILED\033[0m] starting server failed");

    emit Info(className,"------------------------------------------------------");

    //CryptoFire
    crypto = new CryptoFire;
    if(crypto->Add_Encrypted_Key("Admin",password) &&
            crypto->Add_Encrypted_Key("User",webPassword))
        return true;
    return false;
}

bool Server::StartServer()
{    
    QSqlQuery req;
    req.exec("SELECT Value1 FROM General WHERE Name='Port'");
    req.next();
    int port = req.value(0).toInt();

    req.exec("SELECT Value1 FROM General WHERE Name='WebAdminSocket'");
    req.next();
    if(!req.value(0).toBool())
    {
        emit Info(className,"Server type : TCP");
        return server->listen(QHostAddress::Any,static_cast<quint16>(port));
    }
    else
    {
        emit Info(className,"Server type : Web Socket");
        return webAdminServer->listen(QHostAddress::Any,static_cast<quint16>(port));
    }
}

void Server::NewConnexion()
{
    while(server->hasPendingConnections())
    {
        QTcpSocket *newCo = server->nextPendingConnection();

        connect(newCo,SIGNAL(readyRead()),this,SLOT(ReceiptData()));
        connect(newCo,SIGNAL(disconnected()),this,SLOT(Disconnect()));
        emit Info(className,"New Admin connected(" + newCo->peerAddress().toString().toLatin1() + ")");

        SendToUser(newCo,"");
    }
    while(UserServer->hasPendingConnections())
    {
        QTcpSocket *newCo = UserServer->nextPendingConnection();

        connect(newCo,SIGNAL(readyRead()),this,SLOT(ReceiptData()));
        connect(newCo,SIGNAL(disconnected()),this,SLOT(Disconnect()));
        emit Info(className,"New User connected(" + newCo->peerAddress().toString().toLatin1() + ")");

        SendToUser(newCo,"");
    }
}

void Server::Disconnect()
{    
    QTcpSocket *co = qobject_cast<QTcpSocket *>(sender());
    QString privilege = "User";
    if(adminList.removeOne(co))
        privilege = "Admin";
    usersList.removeOne(co);

    emit Info(className,co->objectName() + " disconnected(" + privilege + ", " + co->peerAddress().toString().toLatin1() + ")");
}

void Server::SendToUser(QTcpSocket *user, QString data)
{
    QByteArray paquet;

    QDataStream out(&paquet, QIODevice::WriteOnly);

    quint16 empty(0);
    out << empty;

    if(data.isEmpty())
        out << crypto->Get_Key();
    else {
        if(usersList.contains(user) && (data != QString::number(dataError) || data != QString::number(passwordError)))
            crypto->Encrypt_Data(data,"User");
        else
            crypto->Encrypt_Data(data,"Admin");

        out << data;
    }

    out.device()->seek(0);
    out << static_cast<quint16>(static_cast<uint>(paquet.size()) - sizeof(quint16));

    user->write(paquet);
    user->flush();
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
            if(socket->bytesAvailable() < static_cast<uint>(sizeof(quint16)))
                 return;
            in >> dataSize;
        }

        if(socket->bytesAvailable() < dataSize)
            return;

        QString data;
        in >> data;

        if(!adminList.contains(socket) && !usersList.contains(socket))
        {
            AddUserToList(socket,data);
        }
        else
        {
            int privilege = User;
            if(adminList.contains(socket))
                privilege = Admin;
            if(privilege == User)
                crypto->Decrypt_Data(data,"User");
            else
                crypto->Decrypt_Data(data,"Admin");
             emit Receipt(socket,data,privilege);
        }
        dataSize = 0;
    }
}

void Server::AddUserToList(QTcpSocket *socket, QString data)
{
    QString nameList = "User";
    QList<QTcpSocket*> list = usersList;
    if(socket->parent() == server) {
        nameList = "Admin";
        list = adminList;
    }

    crypto->Decrypt_Data(data,nameList);
    int lastError = noError;
    if(data.contains(QString::number(noError)))
    {
        if(data.split(" ").count() == 2) {
            socket->setObjectName(data.split(" ").last());
            list.append(socket);
        }
        else {
            lastError = passwordError;
        }
    }
    else {
        lastError = dataError;
    }

    SendToUser(socket, QString::number(lastError));

    QString auth = "refused";
    if(lastError == noError)
        auth = "accepted";
    emit Info(className,"New " + nameList + " " + auth);

    socket->flush();
    socket->close();
}

bool Server::StartWebServer()
{
    QSqlQuery req;
    req.exec("SELECT Value1 FROM General WHERE Name='WebPort'");
    req.next();
    int port = req.value(0).toInt();

    req.exec("SELECT Value1 FROM General WHERE Name='WebSocket'");
    req.next();
    if(req.value(0).toBool())
    {
        emit Info(className,"Server type : Web Socket");
        return webServer->listen(QHostAddress::Any,static_cast<quint16>(port));
    }
    else
    {
        emit Info(className,"Server type : TCP");
        return UserServer->listen(QHostAddress::Any,static_cast<quint16>(port));
    }
}

void Server::NewWebConnexion()
{
    while(webServer->hasPendingConnections())
    {
        QWebSocket *socket = webServer->nextPendingConnection();
        socket->setParent(webServer);
        connect(socket,&QWebSocket::textMessageReceived,this,&Server::ReceiptMessage);
        connect(socket,&QWebSocket::disconnected,this,&Server::WebDisconnect);
        emit Info(className,"New Web user connected(" + socket->peerAddress().toString() + ")");

        SendToWebUser(socket,crypto->Get_Key());
    }
    while(webAdminServer->hasPendingConnections())
    {
        QWebSocket *socket = webAdminServer->nextPendingConnection();
        socket->setParent(webAdminServer);
        connect(socket,&QWebSocket::textMessageReceived,this,&Server::ReceiptMessage);
        connect(socket,&QWebSocket::disconnected,this,&Server::WebDisconnect);

        emit Info(className,"New Web Admin connected(" + socket->peerAddress().toString() + ")");

        SendToWebUser(socket,crypto->Get_Key());
    }
}

void Server::WebDisconnect()
{
    QWebSocket *socket = qobject_cast<QWebSocket *>(sender());
    emit Info(className,"Web user disconnected(" + socket->objectName() + ", " + socket->peerAddress().toString() + ")");
    webUsersList.removeOne(socket);
    webAdminList.removeOne(socket);
    socket->deleteLater();
}

void Server::ReceiptMessage(QString text)
{
    QWebSocket *socket = qobject_cast<QWebSocket *>(sender());
    if(!socket)
        return;

    QList<QWebSocket*> list = webUsersList;
    QString nameList = "User";
    int privilege = User;
    if(socket->parent() == webAdminServer) {
        list = webAdminList;
        nameList = "Admin";
        privilege = Admin;
    }

    crypto->Decrypt_Data(text,nameList);
    if(list.contains(socket))
    {
        emit WebReceipt(socket,text,privilege);
    }
    else if(text.contains(QString::number(noError)))
    {
        if(text.split(" ").count() == 2) {
            socket->setObjectName(text.split(" ").last());
            SendToWebUser(socket, QString::number(noError));
            list.append(socket);
            emit Info(className,"New web " + nameList + " accepted");
        }
        else {
            SendToWebUser(socket,QString::number(passwordError));
            emit Info(className,"New web " + nameList + " refused");
        }
    }
    else
    {
        SendToWebUser(socket,QString::number(dataError));
        emit Info(className,"New web " + nameList + " refused");
    }
    socket->flush();
    socket->close();
}

void Server::SendToWebUser(QWebSocket *socket, QString data)
{
    if(socket)
    {
        if(webUsersList.contains(socket) && (data != QString::number(dataError) || data != QString::number(passwordError))) {
            crypto->Encrypt_Data(data,"User");
            socket->sendTextMessage(data);
        }
        else {
            socket->sendTextMessage(data);
        }
    }
}
