/********************************************************************
 Main
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*********************************************************************/

//#include <QtPlugin>
#include <QtGui/QApplication>
#include <QFontDatabase>
#include "mainwindow.h"
#include "SleepLib/profiles.h"

#include "SleepLib/loader_plugins/prs1_loader.h"
#include "SleepLib/loader_plugins/cms50_loader.h"
#include "SleepLib/loader_plugins/zeo_loader.h"



int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    a.setApplicationName("SleepyHead");
    QFontDatabase::addApplicationFont(":/fonts/freesans.ttf");
    a.setFont(QFont("FreeSans"));
    PRS1Loader::Register();
    CMS50Loader::Register();
    ZEOLoader::Register();




    /*Machine *m=new Machine(profile,0);
    m->SetClass("Journal");
    m->SetType(MT_JOURNAL);
    m->properties["Brand"]="Virtual";
    profile->AddMachine(m); */

    MainWindow w;
    w.show();

    return a.exec();
}
