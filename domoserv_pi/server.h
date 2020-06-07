#ifndef SERVER_H
#define SERVER_H

#include <QObject>
#include <QtNetwork>
#include <QSqlQuery>
#include <QWebSocket>
#include <QWebSocketServer>
#include <QSslCertificate>
#include <QSslKey>
#include <QSslError>

//Dépot https://github.com/firedream89/CryptoFire
#include <../../CryptoFire/src/cryptofire.h>

//#define WEBSECURED

enum NetworkError
{
    noError,
    passwordError,
    dataError
};

enum Privilege
{
    Admin,
    User
};

class Server : public QObject
{
    Q_OBJECT
public:
    Server();
    ~Server();
    bool Stop();
    bool Reload();
    bool Init();
    bool StartServer();
    void SendToUser(QTcpSocket *user, QString data);

    bool StartWebServer();
    void SendToWebUser(QWebSocket *socket, QString data);


private slots:
    void NewConnexion();
    void ReceiptData();
    void Disconnect();
    void AddUserToList(QTcpSocket *socket, QString data);

    void NewWebConnexion();
    void WebDisconnect();
    void ReceiptMessage(QString text);

signals:
    void Receipt(QTcpSocket *client, QString data, int privilege);
    void WebReceipt(QWebSocket * socket, QString data, int privilege);
    void Info(QString classname, QString text);

private:
    template <class T>
    void DisconnectUsers(QList<T> list);

    QTcpServer *server;
    QTcpServer *UserServer;
    QWebSocketServer *webAdminServer;
    QWebSocketServer *webServer;
    QList<QTcpSocket*> adminList;
    QList<QTcpSocket*> usersList;
    QList<QWebSocket*> webAdminList;
    QList<QWebSocket*> webUsersList;
    quint16 dataSize;
    QString password;
    QString webPassword;
    CryptoFire *crypto;
};

class ServerException: public QException
{

public:
    ServerException(int const& message) : message(message) {}
    void raise() const override { throw *this; }
    ServerException *clone() const override { return new ServerException(*this); }
    int getMessage() const { return message; }

    enum exception {
        UnkwnownError,
        NotClosed,
        UnknownType
    };

private:
      int message;
};

#endif // SERVER_H
