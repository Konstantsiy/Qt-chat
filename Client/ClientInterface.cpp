#include "ClientInterface.h"
#include "ui_chatwindow.h"
#include "Client.h"

#include <QInputDialog>

#include <QDateTime>
#include <QMessageBox>
#include <QStandardItemModel>
#include <QHostAddress>

ClientInterface::ClientInterface(QWidget *parent) : QWidget(parent), ui(new Ui::ChatWindow), client(new Client(this)), chatModel(new QStandardItemModel(this))  {
    ui->setupUi(this);
    QPixmap* emptyUser = new QPixmap("/home/kostyanislv/Pictures/icon-16.png");
    int h = ui->pictureLabel->height();
    int w = ui->pictureLabel->width();
    ui->pictureLabel->clear();
    ui->pictureLabel->setPixmap(emptyUser->scaled(w, h, Qt::KeepAspectRatio));

    chatModel->insertColumn(0);
    ui->chatView->setModel(chatModel);

    connect(client, &Client::connected, this, &ClientInterface::connectedToServer);
    connect(client, &Client::loggedInProcess, this, &ClientInterface::loggedIn);
    connect(client, &Client::throwLoginError, this, &ClientInterface::throwLoginError);
    connect(client, &Client::messageReceived, this, &ClientInterface::getMessageFromServer);
    connect(client, &Client::disconnectedFromServer, this, &ClientInterface::disconnectedFromServer);
    connect(client, &Client::throwError, this, &ClientInterface::catchError);
    connect(client, &Client::userJoined, this, &ClientInterface::userJoinedToChat);
    connect(client, &Client::userLeft, this, &ClientInterface::userLeftFromChat);

    connect(ui->connectButton, &QPushButton::clicked, this, &ClientInterface::catchNewConnection);

    connect(ui->sendButton, &QPushButton::clicked, this, &ClientInterface::sendMessage);
    connect(ui->messageEdit, &QLineEdit::returnPressed, this, &ClientInterface::sendMessage);
}

ClientInterface::~ClientInterface() {
    delete ui;
}

void ClientInterface::catchNewConnection() {
    QString hostAddress = QInputDialog::getText(this, tr("Server Address"), tr("Choose a host to connect"), QLineEdit::Normal, QStringLiteral("127.0.0.1"));
    if(QString::compare(hostAddress, "localhost")) hostAddress = QStringLiteral("127.0.0.1");
    if (hostAddress.isEmpty()) return;

    ui->connectButton->setEnabled(false);
    client->connectToServer(QHostAddress(hostAddress), 3333);
}

void ClientInterface::connectedToServer() {
    QString newUsername = QInputDialog::getText(this, tr("Chose Username"), tr("Username"));
    if (newUsername.isEmpty()){
        return client->disconnectFromHost();
    }
    catchLogin(newUsername);
}

void ClientInterface::catchLogin(const QString &userName) {
    client->logIn(userName);
}

void ClientInterface::loggedIn() {
    ui->sendButton->setEnabled(true);
    ui->messageEdit->setEnabled(true);
    ui->chatView->setEnabled(true);
    userNickname.clear();
}

void ClientInterface::throwLoginError(const QString &reason) {
    QMessageBox::critical(this, tr("Error"), reason);
    connectedToServer();
}

void ClientInterface::getMessageFromServer(const QString &sender, const QString &text) {
    int newRow = chatModel->rowCount();
    if (userNickname != sender) {
        userNickname = sender;
        QFont boldFont;
        boldFont.setBold(true);
        chatModel->insertRows(newRow, 2);
        chatModel->setData(chatModel->index(newRow, 0), sender + ' (' + QDateTime::currentDateTime().toString("HH:mm") + ')');
        chatModel->setData(chatModel->index(newRow, 0), int(Qt::AlignLeft | Qt::AlignVCenter), Qt::TextAlignmentRole);
        chatModel->setData(chatModel->index(newRow, 0), boldFont, Qt::FontRole);
        ++newRow;
    }
    else {
        chatModel->insertRow(newRow);
    }
    chatModel->setData(chatModel->index(newRow, 0), text);
    chatModel->setData(chatModel->index(newRow, 0), int(Qt::AlignLeft | Qt::AlignVCenter), Qt::TextAlignmentRole);
    ui->chatView->scrollToBottom();
}

void ClientInterface::sendMessage() {
    client->sendTextMessage(ui->messageEdit->text());
    int newRow = chatModel->rowCount();
    chatModel->insertRow(newRow);
    chatModel->setData(chatModel->index(newRow, 0), ui->messageEdit->text());
    chatModel->setData(chatModel->index(newRow, 0), int(Qt::AlignRight | Qt::AlignVCenter), Qt::TextAlignmentRole);

    ui->messageEdit->clear();
    ui->chatView->scrollToBottom();
    userNickname.clear();
}

void ClientInterface::disconnectedFromServer() {
    QMessageBox::warning(this, tr("Disconnected"), tr("The host terminated the connection"));
    ui->sendButton->setEnabled(false);
    ui->messageEdit->setEnabled(false);
    ui->chatView->setEnabled(false);
    ui->connectButton->setEnabled(true);
    userNickname.clear();
}

void ClientInterface::userJoinedToChat(const QString &username) {
    const int newRow = chatModel->rowCount();
    chatModel->insertRow(newRow);
    chatModel->setData(chatModel->index(newRow, 0), tr("%1 Joined the Chat").arg(username));
    chatModel->setData(chatModel->index(newRow, 0), Qt::AlignCenter, Qt::TextAlignmentRole);
    chatModel->setData(chatModel->index(newRow, 0), QBrush(Qt::blue), Qt::ForegroundRole);
    ui->chatView->scrollToBottom();
    userNickname.clear();
}

void ClientInterface::userLeftFromChat(const QString &username) {
    const int newRow = chatModel->rowCount();
    chatModel->insertRow(newRow);
    chatModel->setData(chatModel->index(newRow, 0), tr("%1 Left the Chat").arg(username));
    chatModel->setData(chatModel->index(newRow, 0), Qt::AlignCenter, Qt::TextAlignmentRole);
    chatModel->setData(chatModel->index(newRow, 0), QBrush(Qt::red), Qt::ForegroundRole);
    ui->chatView->scrollToBottom();
    userNickname.clear();
}

void ClientInterface::catchError(QAbstractSocket::SocketError socketError) {
    switch (socketError) {
    case QAbstractSocket::RemoteHostClosedError:
    case QAbstractSocket::ProxyConnectionClosedError:
        return;
    case QAbstractSocket::ConnectionRefusedError:
        QMessageBox::critical(this, tr("Error"), tr("The connection was disconnected by another node (or by timeout)"));
        break;
    case QAbstractSocket::HostNotFoundError:
        QMessageBox::critical(this, tr("Error"), tr("No host address was found"));
        break;
    case QAbstractSocket::SocketResourceError:
        QMessageBox::critical(this, tr("Error"), tr("Too many connections opened"));
        break;
    case QAbstractSocket::SocketTimeoutError:
        QMessageBox::warning(this, tr("Error"), tr("Socket operation timed out"));
        return;
    case QAbstractSocket::NetworkError:
        QMessageBox::critical(this, tr("Error"), tr("Network error occurred"));
        break;
    case QAbstractSocket::UnknownSocketError:
        QMessageBox::critical(this, tr("Error"), tr("An unknown error occured"));
        break;
    case QAbstractSocket::TemporaryError:
    case QAbstractSocket::OperationError:
        QMessageBox::warning(this, tr("Error"), tr("Operation failed, please try again"));
        return;
    default:
        Q_UNREACHABLE();
    }
    ui->connectButton->setEnabled(true);
    ui->sendButton->setEnabled(false);
    ui->messageEdit->setEnabled(false);
    ui->chatView->setEnabled(false);
    userNickname.clear();
}
