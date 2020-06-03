#ifndef CHATCLIENT_H
#define CHATCLIENT_H

#include <QObject>
#include <QTcpSocket>
class QHostAddress;
class QJsonDocument;
class Client : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY(Client)

private slots:
    void doReadyRead();
signals:
    void connected();
    void loggedInProcess();
    void throwLoginError(const QString &reason);
    void disconnectedFromServer();
    void messageReceived(const QString &sender, const QString &text);
    void throwError(QAbstractSocket::SocketError socketError);
    void userJoined(const QString &username);
    void userLeft(const QString &username);
private:
    QTcpSocket *clientSocket;
    bool loggedInValue;
    void getServerResponseJSON(const QJsonObject &doc);
public:
    explicit Client(QObject *parent = nullptr);
public slots:
    void logIn(const QString &userName);
    void sendTextMessage(const QString &text);
    void disconnectFromHost();
    void connectToServer(const QHostAddress &address, quint16 port);
};

#endif // CHATCLIENT_H
