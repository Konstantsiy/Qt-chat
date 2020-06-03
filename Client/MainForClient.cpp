#include <QApplication>
#include "ClientInterface.h"
int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    ClientInterface clientIn;
    clientIn.show();
    return a.exec();
}
