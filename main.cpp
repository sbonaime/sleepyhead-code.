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

    QFontDatabase::addApplicationFont(":/fonts/freesans.ttf");
    a.setFont(QFont("FreeSans"));
    PRS1Loader::Register();
    CMS50Loader::Register();
    ZEOLoader::Register();

    Profiles::Scan();

    //loader_progress->Show();

    pref["AppName"]="SleepyHead";
    //pref["Version"]=wxString(AutoVersion::_FULLVERSION_STRING,wxConvUTF8);
    pref["Profile"]=getUserName();
    pref["LinkGraphMovement"]=true;
    pref["fruitsalad"]=true;

    profile=Profiles::Get(pref["Profile"].toString());

    profile->LoadMachineData();

    /*Machine *m=new Machine(profile,0);
    m->SetClass("Journal");
    m->SetType(MT_JOURNAL);
    m->properties["Brand"]="Virtual";
    profile->AddMachine(m); */

    MainWindow w;
    w.show();

    return a.exec();
}
