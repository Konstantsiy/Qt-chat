#ifndef SERVERWORKER_H
#define SERVERWORKER_H

#include <QObject>
#include <QTcpSocket>
#include <QReadWriteLock>
class QJsonObject;
class ClientOperator : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY(ClientOperator)

private:
    QTcpSocket *serverSocket;
    QString userNameString;
    mutable QReadWriteLock userNameLock;   
    int threadId;
public:
    explicit ClientOperator(QObject *parent = nullptr);
    virtual bool setSocketDescriptor(qintptr socketDescriptor);
    QString getUserName() const;
    void setUserName(const QString &userName);
    void sendServerResponseJSON(const QJsonObject &json);
    void setThreadId(const int& id);
    int getThreadIt();
public slots:
    void disconnectFromClient();
private slots:
    void processRequest();
signals:
    void getRequestJSON(const QJsonObject &jsonDoc);
    void catchError();
    void logMessage(const QString &msg);
    void disconnectedFromClient();

};

#endif // SERVERWORKER_H
