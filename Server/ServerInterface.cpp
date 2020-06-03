#include "ServerInterface.h"
#include "ui_ServerWindow.h"
#include "Server.h"
#include <QMessageBox>
#include <QThread>
#include <QInputDialog>

ServerInterface::ServerInterface(QWidget *parent) : QWidget(parent), ui(new Ui::ServerWindow), server(new Server(this)) {
    ui->setupUi(this);

    QPixmap* serverIc = new QPixmap("/home/kostyanislv/Pictures/database.png");
    int h = ui->serverIcon->height();
    int w = ui->serverIcon->width();
    ui->serverIcon->clear();
    ui->serverIcon->setPixmap(serverIc->scaled(w, h, Qt::KeepAspectRatio));

    connect(ui->startStopButton, &QPushButton::clicked, this, &ServerInterface::startServerIteration);
    connect(server, &Server::logMessage, this, &ServerInterface::logMessage);
    connect(server, &Server::userOnline, this, &ServerInterface::addNewOnlineUser);
    connect(server, &Server::userOffline, this, &ServerInterface::userOffline);
    connect(ui->deleteButton, &QPushButton::clicked, this, &ServerInterface::deleteUserEvent);
}

ServerInterface::~ServerInterface() {
    delete ui;
}

void ServerInterface::startServerIteration() {
    if (server->isListening()) {
        server->stopServer();
        ui->startStopButton->setText(tr("Start Server"));
        logMessage(QStringLiteral("Server stopped"));
    }
    else {
        if (!server->listen(QHostAddress::Any, 3333)) {
            QMessageBox::critical(this, tr("Error"), tr("Unable to start the server"));
            return;
        }
        logMessage(QStringLiteral("Server connection..."));
        logMessage(QStringLiteral("Srever started"));
        ui->startStopButton->setText(tr("Stop Server"));
    }
    ui->deleteButton->isActiveWindow();
}

void ServerInterface::logMessage(const QString &msg) {
    ui->logEditor->appendPlainText(msg + '\n');
}

void ServerInterface::addNewOnlineUser(const QString &userName) {
    if(!listOfUsers.contains(userName)) {
        listOfUsers.append(userName);
        ui->usersList->appendPlainText(userName);
    }
}

void ServerInterface::userOffline(const QString &userName) {
    QMutableVectorIterator<QString> it(listOfUsers);
    while(it.hasNext()){
        if(it.next() == userName) {
            it.remove();
        }
    }
    ui->usersList->clear();
    if(listOfUsers.isEmpty()) return;
    for(int i = 0; i < listOfUsers.size(); i++) {
        ui->usersList->appendPlainText(listOfUsers[i]);
    }
}


void ServerInterface::deleteUserEvent() {
    QString userName = QInputDialog::getText(this, tr("Chose user to delete"), tr("Username"));
    if(userName.isEmpty()) return;
    if(listOfUsers.isEmpty()) return;
    if(!listOfUsers.contains(userName)) {
        QMessageBox::warning(this, tr("Error"), tr("This client is not on the server"));
        return;
    }
    this->server->deleteUser(userName);
}
