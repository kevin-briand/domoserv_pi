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
    void Stop();
    void Reload();
    void Init();
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

#endif // SERVER_H
