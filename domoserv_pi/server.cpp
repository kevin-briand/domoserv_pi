#include "server.h"

//Version 1.0

Server::Server()
{
    #define className "Serveur"

    QSqlQuery req;
    req.exec("SELECT * FROM General WHERE Name='Port'");
    if(!req.next())
    {
        req.exec("SELECT MAX(ID) FROM General");
        req.next();
        int id = req.value(0).toInt();
        id++;
        req.exec("INSERT INTO General VALUES('" + QString::number(id) + "','Port','49152','','','')");
        id++;
        req.exec("INSERT INTO General VALUES('" + QString::number(id) + "','Password','','','','')");
        id++;
        req.exec("INSERT INTO General VALUES('" + QString::number(id) + "','WebSocket','','','','')");
    }
}

void Server::Init()
{
    emit Info(className,"Starting server");
    dataSize = 0;

    //Password
    QSqlQuery req;
    req.exec("SELECT Value1 FROM General WHERE Name='Password'");
    req.next();
    password = req.value(0).toString();
    emit Info(className,"Password = " + password.toLatin1() + "");

    //PKEY
    GeneratePKEY();
    emit Info(className,"PKEY generated : " + PKEY.toLatin1() + "");

    //Port
    req.exec("SELECT Value1 FROM General WHERE Name='Port'");
    req.next();
    emit Info(className,"Port = " + req.value(0).toString());

    //Run server
    if(StartServer())
        emit Info(className,"[\033[0;32m  OK  \033[0m] Server started");
    else
        emit Info(className,"[\033[0;31mFAILED\033[0m]starting server failed");
}

bool Server::StartServer()
{
    server = new QTcpServer;
    QSqlQuery req;
    req.exec("SELECT Value1 FROM General WHERE Name='Port'");
    req.next();
    bool ret = server->listen(QHostAddress::Any,req.value(0).toInt());
    connect(server,SIGNAL(newConnection()),this,SLOT(NewConnexion()));
    return ret;
}

void Server::NewConnexion()
{
    QTcpSocket *newCo = server->nextPendingConnection();

    connect(newCo,SIGNAL(readyRead()),this,SLOT(ReceiptData()));
    connect(newCo,SIGNAL(disconnected()),this,SLOT(Disconnect()));
    emit Info(className,"New user connected(" + newCo->peerAddress().toString().toLatin1() + ")");
}

void Server::Disconnect()
{
    QTcpSocket *co = qobject_cast<QTcpSocket *>(sender());
    usersList.removeOne(co);
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

    for(int i=0;i<usersList.count();i++)
    {
        usersList.at(i)->write(paquet);
    }
}

void Server::SendToUser(QTcpSocket *user, QString data)
{
    QByteArray paquet;

    QDataStream out(&paquet, QIODevice::WriteOnly);

    emit Info(className,"Send Data to user " + data);
    out << (quint16) 0;
    if(data == PKEY)
        out << data;
    else
        out << Encrypt(data);
    out.device()->seek(0);
    out << (quint16) (paquet.size() - sizeof(quint16));
}

void Server::ReceiptData()
{
    QTcpSocket *socket;
qDebug() << "Receipt";
    while(socket)
    {
        socket = qobject_cast<QTcpSocket *>(sender());

        if (socket == 0)
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
        emit Info(className,"receipt data from user " + Decrypt(data));
        if(!usersList.contains(socket))
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
        usersList.append(socket);
        SendToUser(socket,PKEY);
        emit Info(className,"New user accepted");
    }
    else
    {
        socket->close();
        emit Info(className,"New user refused");
    }
}

void Server::GeneratePKEY()
{
    srand(QTime::currentTime().msec());
    QString key;

    for(int i = 0;i<20;i++)
    {
        key.append(QString::number(rand() % 300) + " ");
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
