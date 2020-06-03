#ifndef CHATWINDOW_H
#define CHATWINDOW_H

#include <QWidget>
#include <QAbstractSocket>
class Client;
class QStandardItemModel;
namespace Ui { class ChatWindow; }
class ClientInterface : public QWidget {
    Q_OBJECT
    Q_DISABLE_COPY(ClientInterface)

private:
    Ui::ChatWindow *ui;
    Client *client;
    QStandardItemModel *chatModel;
    QString userNickname;
private slots:
    void catchNewConnection();
    void catchLogin(const QString &userNickname);
    void loggedIn();
    void connectedToServer();
    void getMessageFromServer(const QString &sender, const QString &text);
    void sendMessage();
    void disconnectedFromServer();
    void userJoinedToChat(const QString &username);
    void userLeftFromChat(const QString &username);
    void throwLoginError(const QString &reason);
    void catchError(QAbstractSocket::SocketError socketError);
public:
    explicit ClientInterface(QWidget *parent = nullptr);
    ~ClientInterface();
};

#endif // CHATWINDOW_H

