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

#include "SleepLib/loader_plugins/prs1_loader.h"
#include "SleepLib/loader_plugins/cms50_loader.h"
#include "SleepLib/loader_plugins/zeo_loader.h"
#include "SleepLib/loader_plugins/resmed_loader.h"
//#include "qextserialport/qextserialenumerator.h"


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
  /*  QList<QextPortInfo> ports = QextSerialEnumerator::getPorts();
       qDebug() << "List of ports:";
       for (int i = 0; i < ports.size(); i++) {
           qDebug() << "port name:" << ports.at(i).portName;
           qDebug() << "friendly name:" << ports.at(i).friendName;
           qDebug() << "physical name:" << ports.at(i).physName;
           qDebug() << "enumerator name:" << ports.at(i).enumName;
           qDebug() << "vendor ID:" << QString::number(ports.at(i).vendorID, 16);
           qDebug() << "product ID:" << QString::number(ports.at(i).productID, 16);
           qDebug() << "===================================";
       }
*/

    a.setApplicationName("SleepyHead");
    int id=QFontDatabase::addApplicationFont(":/fonts/FreeSans.ttf");
    QStringList ffam=QFontDatabase::applicationFontFamilies(id);
    for (QStringList::iterator i=ffam.begin();i!=ffam.end();i++) {
        qDebug() << "Loaded Font: " << (*i);
    }

    a.setFont(QFont("FreeSans",10));

    PRS1Loader::Register();
    CMS50Loader::Register();
    ZEOLoader::Register();
    ResmedLoader::Register();

    MainWindow w;
    w.show();

    return a.exec();
}
