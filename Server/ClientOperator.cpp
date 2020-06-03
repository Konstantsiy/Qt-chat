#include "ClientOperator.h"
#include <QDataStream>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonObject>

ClientOperator::ClientOperator(QObject *parent) : QObject(parent), serverSocket(new QTcpSocket(this)) {
    connect(serverSocket, &QTcpSocket::readyRead, this, &ClientOperator::processRequest);
    connect(serverSocket, &QTcpSocket::disconnected, this, &ClientOperator::disconnectedFromClient);
    connect(serverSocket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), this, &ClientOperator::catchError);
}

void ClientOperator::setThreadId(const int &id) {
    this->threadId = id;
}

int ClientOperator::getThreadIt() {
    return this->threadId;
}

bool ClientOperator::setSocketDescriptor(qintptr socketDescriptor) {
    return serverSocket->setSocketDescriptor(socketDescriptor);
}

void ClientOperator::sendServerResponseJSON(const QJsonObject &json) {
    const QByteArray jsonData = QJsonDocument(json).toJson();
    emit logMessage("Sending to " + getUserName() + " - " + QString::fromUtf8(jsonData));
    QDataStream socketStream(serverSocket);
    socketStream.setVersion(QDataStream::Qt_5_7);
    socketStream << jsonData;
}

void ClientOperator::disconnectFromClient() {
    serverSocket->disconnectFromHost();
}

QString ClientOperator::getUserName() const {
    userNameLock.lockForRead();
    const QString result = userNameString;
    userNameLock.unlock();
    return result;
}

void ClientOperator::setUserName(const QString &userName) {
    userNameLock.lockForWrite();
    userNameString = userName;
    userNameLock.unlock();
}

void ClientOperator::processRequest() {
    QByteArray jsonData; // объект для получения JSON из сокета
    QDataStream socketStream(serverSocket);
    // согласовываем версию для сериализации
    socketStream.setVersion(QDataStream::Qt_5_7);
    for (;;) {
        // запускает новую транзакцию чтения в потоке
        socketStream.startTransaction();
        socketStream >> jsonData;
        if (socketStream.commitTransaction()) { // если данные прочитанны успешно
            QJsonParseError parseError;
            // создаём JSON-документ с полученными данными
            const QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData, &parseError);
            if (parseError.error == QJsonParseError::NoError) {
                if (jsonDoc.isObject())
                    emit getRequestJSON(jsonDoc.object());
                else
                    emit logMessage("Invalid message: " + QString::fromUtf8(jsonData));
            } else {
                emit logMessage("Invalid message: " + QString::fromUtf8(jsonData));
            }
            // читаем JSON, пока они доступны
        } else {
            break;
        }
    }
}


