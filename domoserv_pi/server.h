#ifndef SERVER_H
#define SERVER_H

#include <QObject>
#include <QtNetwork>
#include <QSqlQuery>
#include <QWebSocket>
#include <QWebSocketServer>



class Server : public QObject
{
    Q_OBJECT
public:
    Server();
    void Reload();
    void Init();
    bool StartServer();
    void SendToAll(QString data);
    void SendToUser(QTcpSocket *user, QString data);

    bool StartWebServer();
    void SendToWebUser(QString data);


private slots:
    void NewConnexion();
    void ReceiptData();
    void Disconnect();
    void AddUserToList(QTcpSocket *socket, QString data);

    void NewWebConnexion();
    void WebDisconnect();
    void ReceiptMessage(QByteArray text);

signals:
    void Receipt(QTcpSocket *client, QString data);
    void WebReceipt(QWebSocket * socket, QString data);
    void Info(QString classname, QString text);

private:
    QString Encrypt(QString text);
    QString Decrypt(QString text);
    void GeneratePKEY();

    QTcpServer *server;
    QWebSocketServer *webServer;
    QList<QTcpSocket*> usersList;
    QList<QWebSocket*> webUsersList;
    quint16 dataSize;
    QString password;
    QString webPassword;
    QString PKEY;
};

#endif // SERVER_H
