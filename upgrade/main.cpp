#include <QtGui/QApplication>
#include <QTimer>
#include "UpdateWindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    UpdateWindow w;

    w.checkForUpdates();
   // w.show();
    
    return a.exec();
}
