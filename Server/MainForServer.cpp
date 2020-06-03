#include <QApplication>
#include "ServerInterface.h"
int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    ServerInterface serverIn;
    serverIn.show();
    return a.exec();
}
