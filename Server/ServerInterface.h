#ifndef SERVERWINDOW_H
#define SERVERWINDOW_H

#include <QWidget>
#include <QVector>

namespace Ui {
class ServerWindow;
}

class Server;

class ServerInterface : public QWidget {
    Q_OBJECT
    Q_DISABLE_COPY(ServerInterface)

private:
    QVector<QString> listOfUsers;
    Ui::ServerWindow *ui;
    Server *server;
private slots:
    void startServerIteration();
    void addNewOnlineUser(const QString &userName);
    void userOffline(const QString &userName);
    void logMessage(const QString &msg);
    void deleteUserEvent();

public:
    explicit ServerInterface(QWidget *parent = nullptr);
    ~ServerInterface();
};

#endif // SERVERWINDOW_H
