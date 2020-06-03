#include "Server.h"
#include "ClientOperator.h"
#include "ServerInterface.h"
#include <QThread>
#include <functional>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QTimer>

Server::Server(QObject *parent) : QTcpServer(parent), threadCount(qMax(QThread::idealThreadCount(), 1)) {
    availableThreads.reserve(threadCount); // резервируем место в памяти под потоки
    threadsLoad.reserve(threadCount);
}

Server::~Server() {
    for (QThread *singleThread : availableThreads) {
        singleThread->quit();
        singleThread->wait();
    }
}

void Server::incomingConnection(qintptr socketDescriptor) {
    ClientOperator *worker = new ClientOperator;
    if (!worker->setSocketDescriptor(socketDescriptor)) {
        worker->deleteLater(); // отмечаем готовность к удалению объекта
        return;
    }
    int threadIndex = availableThreads.size();
    if (threadIndex < threadCount) { // если мы можем позволить себе создание нового потока, создаём его
        availableThreads.append(new QThread(this));
        threadsLoad.append(1);
        availableThreads.last()->start();
    }
    else {
        // находим поток с наименьшим количеством клиентов и используем его
        threadIndex = std::distance(threadsLoad.cbegin(), std::min_element(threadsLoad.cbegin(), threadsLoad.cend()));
        ++threadsLoad[threadIndex];
    }
    worker->setThreadId(threadIndex);
    worker->moveToThread(availableThreads.at(threadIndex));
    connect(availableThreads.at(threadIndex), &QThread::finished, worker, &QObject::deleteLater); // для удаления клиентскоко помошника на сервере
    connect(worker, &ClientOperator::disconnectedFromClient, this, std::bind(&Server::clientDisconnected, this, worker, threadIndex));
    connect(worker, &ClientOperator::catchError, this, std::bind(&Server::throwClientError, this, worker));
    connect(worker, &ClientOperator::getRequestJSON, this, std::bind(&Server::getRequestJSON, this, worker, std::placeholders::_1));
    connect(this, &Server::stopAllClients, worker, &ClientOperator::disconnectFromClient);

    clients.append(worker);
    emit logMessage("New client сonnected");
}

void Server::deleteUser(const QString &userName) {
    for(int i = 0; i < clients.size(); i++) {
        if(userName == clients[i]->getUserName()) {
            clientDisconnected(clients[i], clients[i]->getThreadIt());
        }
    }
}

void Server::sendJson(ClientOperator *adressPoint, const QJsonObject &message) {
    Q_ASSERT(adressPoint);
    QTimer::singleShot(0, adressPoint, std::bind(&ClientOperator::sendServerResponseJSON, adressPoint, message));
}

void Server::sendResponseJSON(const QJsonObject &message, ClientOperator *exclude) {
    for (ClientOperator *worker : clients) {
        Q_ASSERT(worker);
        if (worker == exclude)
            continue;
        sendJson(worker, message);
    }
}

void Server::getRequestJSON(ClientOperator *sender, const QJsonObject &json) {
    Q_ASSERT(sender);
    emit logMessage(QString::fromUtf8(QJsonDocument(json).toJson()));
    if (sender->getUserName().isEmpty())
        return LoggetOutJSON(sender, json);
    getLoggetInJSON(sender, json);
}

void Server::clientDisconnected(ClientOperator *sender, int threadIdx) {
    --threadsLoad[threadIdx];
    clients.removeAll(sender); // удаляем отключившегося клиента из списка
    const QString userName = sender->getUserName();
    if (!userName.isEmpty()) {
        QJsonObject disconnectedMessage;
        disconnectedMessage["type"] = QStringLiteral("userdisconnected");
        disconnectedMessage["username"] = userName;
        sendResponseJSON(disconnectedMessage, nullptr);
        emit logMessage(userName + " disconnected");
        emit userOffline(userName);
    }
    sender->deleteLater();
}

void Server::throwClientError(ClientOperator *sender) {
    Q_UNUSED(sender)
    emit logMessage("Error from " + sender->getUserName());
}

void Server::stopServer() {
    emit stopAllClients();
    close();
}

void Server::LoggetOutJSON(ClientOperator *sender, const QJsonObject &docObj) {
    Q_ASSERT(sender);
    const QJsonValue typeValue = docObj.value(QLatin1String("type"));
    if (typeValue.isNull() || !typeValue.isString())
        return;
    if (typeValue.toString().compare(QLatin1String("login"), Qt::CaseInsensitive) != 0)
        return;
    const QJsonValue usernameValue = docObj.value(QLatin1String("username"));
    if (usernameValue.isNull() || !usernameValue.isString())
        return;
    const QString newUserName = usernameValue.toString().simplified();
    if (newUserName.isEmpty())
        return;
    for (ClientOperator *worker : qAsConst(clients)) {
        if (worker == sender)
            continue;
        if (worker->getUserName().compare(newUserName, Qt::CaseInsensitive) == 0) {
            QJsonObject message;
            message["type"] = QStringLiteral("login");
            message["success"] = false;
            message["reason"] = QStringLiteral("A user with this name is already registered");
            sendJson(sender, message);
            return;
        }
    }
    sender->setUserName(newUserName);
    QJsonObject responseMessage;
    responseMessage["type"] = QStringLiteral("login");
    responseMessage["success"] = true;
    sendJson(sender, responseMessage);
    QJsonObject connectedMessage;
    connectedMessage["type"] = QStringLiteral("newuser");
    connectedMessage["username"] = newUserName;
    sendResponseJSON(connectedMessage, sender);
}

void Server::getLoggetInJSON(ClientOperator *sender, const QJsonObject &docObj) {
    Q_ASSERT(sender);
    const QJsonValue typeVal = docObj.value(QLatin1String("type"));
    if (typeVal.isNull() || !typeVal.isString())
        return;
    if (typeVal.toString().compare(QLatin1String("message"), Qt::CaseInsensitive) != 0)
        return;
    const QJsonValue textVal = docObj.value(QLatin1String("text"));
    if (textVal.isNull() || !textVal.isString())
        return;
    const QString text = textVal.toString().trimmed();
    if (text.isEmpty())
        return;
    QJsonObject message;
    message["type"] = QStringLiteral("message");
    message["text"] = text;
    message["sender"] = sender->getUserName();
    sendResponseJSON(message, sender);
    emit userOnline(sender->getUserName());
}


