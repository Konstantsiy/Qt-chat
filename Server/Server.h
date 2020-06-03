#ifndef CHATSERVER_H
#define CHATSERVER_H

#include <QTcpServer>
#include <QVector>
#include "ServerInterface.h"
class QThread;
class ClientOperator;
class QJsonObject;
class Server : public QTcpServer {
    Q_OBJECT
    Q_DISABLE_COPY(Server)

private:
    const int threadCount;
    QVector<int> threadsLoad;
    QVector<ClientOperator *> clients;
    QVector<QThread *> availableThreads;

private slots:
    void sendResponseJSON(const QJsonObject &message, ClientOperator *exclude);
    void getRequestJSON(ClientOperator *sender, const QJsonObject &doc);
    void clientDisconnected(ClientOperator *sender, int threadIndex);
    void throwClientError(ClientOperator *sender);
public slots:
    void stopServer();
    void deleteUser(const QString &userName);
private:
    void LoggetOutJSON(ClientOperator *sender, const QJsonObject &doc);
    void getLoggetInJSON(ClientOperator *sender, const QJsonObject &doc);
    void sendJson(ClientOperator *destination, const QJsonObject &message);  
signals:
    void logMessage(const QString &msg);
    void userOnline(const QString &userName);
    void userOffline(const QString &userName);
    void stopAllClients();
public:
    explicit Server(QObject *parent = nullptr);
    ~Server();
protected:
    void incomingConnection(qintptr socketDescriptor) override;
};

#endif // CHATSERVER_H
