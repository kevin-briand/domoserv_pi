#include "server.h"

Server::Server()
{
    printf("Starting server\n");
    dataSize = 0;

    //Password
    QFile f("config.conf");
    f.open(QIODevice::ReadOnly);
    QTextStream data(&f);
    while(!data.atEnd())
    {
        QString var = data.readLine();
        if(var.contains("SERVER_PASSWORD"))
            password = var.split("=").last();
    }
    f.close();
    printf("Password = " + password.toLatin1() + "\n");

    //PKEY
    GeneratePKEY();
    printf("PKEY generated : " + PKEY.toLatin1() + "\n");

    //Run server
    if(StartServer())
        printf("[  OK  ] Server started\n");
    else
        printf("[FAILED]starting server failed\n");
}

bool Server::StartServer()
{
    server = new QTcpServer;
    bool ret = server->listen(QHostAddress::Any,12120);
    QWebSocket ;
    connect(server,SIGNAL(newConnection()),this,SLOT(NewConnexion()));
    return ret;
}

void Server::NewConnexion()
{
    QTcpSocket *newCo = server->nextPendingConnection();
    connect(newCo,SIGNAL(readyRead()),this,SLOT(ReceiptData()));
    connect(newCo,SIGNAL(disconnected()),this,SLOT(Disconnect()));
    printf("New user connected(" + newCo->peerAddress().toString().toLatin1() + ")\n");
}

void Server::Disconnect()
{
    QTcpSocket *co = qobject_cast<QTcpSocket *>(sender());
    usersList.removeOne(co);
    printf("User disconnected(" + co->peerAddress().toString().toLatin1() + ")\n");
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

    out << (quint16) 0;
    out << Encrypt(data);
    out.device()->seek(0);
    out << (quint16) (paquet.size() - sizeof(quint16));

    user->write(paquet);
}

void Server::ReceiptData()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket *>(sender());

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

    if(!usersList.contains(socket))
        AddUserToList(socket,data);
    else
        emit Receipt(socket,Decrypt(data));

    dataSize = 0;
}

void Server::AddUserToList(QTcpSocket *socket, QString data)
{
    if(data == password)
    {
        usersList.append(socket);
        SendToUser(socket,PKEY);
        printf("New user accepted\n");
    }
    else
    {
        socket->close();
        printf("New user refused\n");
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
