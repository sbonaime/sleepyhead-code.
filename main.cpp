/*
 Main
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

//#include <QtPlugin>
#include <QtGui/QApplication>
#include <QFontDatabase>
#include <QStringList>
#include <QDebug>
#include "mainwindow.h"
#include "SleepLib/profiles.h"


MainWindow *mainwin=NULL;

void MyOutputHandler(QtMsgType type, const char *msg) {
    if (!mainwin) {

        return;
    }
    switch (type) {
        case QtDebugMsg:
            mainwin->Log(msg);
            break;
        case QtWarningMsg:
            mainwin->Log(QString("Warning: ")+msg);
            break;
        case QtFatalMsg:
            mainwin->Log(QString("Fatal: ")+msg);
            break;
        case QtCriticalMsg:
            mainwin->Log(QString("Critical: ")+msg);
            break;
            // Popup a messagebox
            //abort();
    }
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    a.setApplicationName("SleepyHead");

    int id=QFontDatabase::addApplicationFont(":/fonts/FreeSans.ttf");
    QStringList ffam=QFontDatabase::applicationFontFamilies(id);
    for (QStringList::iterator i=ffam.begin();i!=ffam.end();i++) {
        qDebug() << "Loaded Font: " << (*i);
    }

    a.setFont(QFont("FreeSans",10));


    MainWindow w;
    mainwin=&w;
    qInstallMsgHandler(MyOutputHandler);

    w.show();
    return a.exec();
}
