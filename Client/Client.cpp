#include "Client.h"
#include <QTcpSocket>
#include <QDataStream>
#include <QJsonParseError>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

Client::Client(QObject *parent) : QObject(parent), clientSocket(new QTcpSocket(this)), loggedInValue(false) {
    connect(clientSocket, &QTcpSocket::connected, this, &Client::connected);
    connect(clientSocket, &QTcpSocket::disconnected, this, &Client::disconnectedFromServer);
    connect(clientSocket, &QTcpSocket::readyRead, this, &Client::doReadyRead);
    connect(clientSocket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), this, &Client::throwError);
    connect(clientSocket, &QTcpSocket::disconnected, this, [this]()->void{loggedInValue = false;});
}

void Client::logIn(const QString &userName) {
    if (clientSocket->state() == QAbstractSocket::ConnectedState) {
        QDataStream clientStream(clientSocket);
        clientStream.setVersion(QDataStream::Qt_5_7);
        QJsonObject message;
        message["type"] = QStringLiteral("login");
        message["username"] = userName;
        clientStream << QJsonDocument(message).toJson(QJsonDocument::Compact);
    }
}

void Client::sendTextMessage(const QString &text) {
    if (text.isEmpty()) return;
    QDataStream clientStream(clientSocket);
    clientStream.setVersion(QDataStream::Qt_5_7);
    QJsonObject message;
    message["type"] = QStringLiteral("message");
    message["text"] = text;
    clientStream << QJsonDocument(message).toJson();
}

void Client::disconnectFromHost()
{
    clientSocket->disconnectFromHost();
}

void Client::getServerResponseJSON(const QJsonObject &docObj) {
    const QJsonValue typeVal = docObj.value(QLatin1String("type"));
    if (typeVal.isNull() || !typeVal.isString())
        return;
    if (typeVal.toString().compare(QLatin1String("login"), Qt::CaseInsensitive) == 0) {
        if (loggedInValue)
            return;
        const QJsonValue resultVal = docObj.value(QLatin1String("success"));
        if (resultVal.isNull() || !resultVal.isBool())
            return;
        const bool loginSuccess = resultVal.toBool();
        if (loginSuccess) {
            emit loggedInProcess();
            return;
        }
        const QJsonValue reasonVal = docObj.value(QLatin1String("reason"));
        emit throwLoginError(reasonVal.toString());
    } else if (typeVal.toString().compare(QLatin1String("message"), Qt::CaseInsensitive) == 0) {
        const QJsonValue textVal = docObj.value(QLatin1String("text"));
        const QJsonValue senderVal = docObj.value(QLatin1String("sender"));
        if (textVal.isNull() || !textVal.isString())
            return;
        if (senderVal.isNull() || !senderVal.isString())
            return;
        emit messageReceived(senderVal.toString(), textVal.toString());
    }
    else if (typeVal.toString().compare(QLatin1String("newuser"), Qt::CaseInsensitive) == 0) {
        const QJsonValue usernameVal = docObj.value(QLatin1String("username"));
        if (usernameVal.isNull() || !usernameVal.isString())
            return;
        emit userJoined(usernameVal.toString());
    }
    else if (typeVal.toString().compare(QLatin1String("userdisconnected"), Qt::CaseInsensitive) == 0) {
        const QJsonValue usernameVal = docObj.value(QLatin1String("username"));
        if (usernameVal.isNull() || !usernameVal.isString())
            return;
        emit userLeft(usernameVal.toString());
    }
}

void Client::connectToServer(const QHostAddress &address, quint16 port)
{
    clientSocket->connectToHost(address, port);
}

void Client::doReadyRead() {
    QByteArray jsonData;
    QDataStream socketStream(clientSocket);
    socketStream.setVersion(QDataStream::Qt_5_7);
    for (;;) {
        socketStream.startTransaction();
        socketStream >> jsonData;
        if (socketStream.commitTransaction()) {
            QJsonParseError parseError;
            const QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData, &parseError);
            if (parseError.error == QJsonParseError::NoError) {
                if (jsonDoc.isObject())
                    getServerResponseJSON(jsonDoc.object());
            }
        }
        else break;
    }
}
