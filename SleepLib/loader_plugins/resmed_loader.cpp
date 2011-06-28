/*

SleepLib ResMed Loader Implementation

Author: Mark Watkins <jedimark64@users.sourceforge.net>
License: GPL
*/

#include <QString>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QProgressBar>

#include "resmed_loader.h"
#include "SleepLib/session.h"

ResmedLoader::ResmedLoader()
{
}
ResmedLoader::~ResmedLoader()
{
}

Machine *ResmedLoader::CreateMachine(QString serial,Profile *profile)
{
    qDebug(("Create ResMed Machine "+serial).toLatin1());
    assert(profile!=NULL);
    vector<Machine *> ml=profile->GetMachines(MT_CPAP);
    bool found=false;
    for (vector<Machine *>::iterator i=ml.begin(); i!=ml.end(); i++) {
        if (((*i)->GetClass()=="ResMed") && ((*i)->properties["Serial"]==serial)) {
            ResmedList[serial]=*i; //static_cast<CPAP *>(*i);
            found=true;
            break;
        }
    }
    if (found) return ResmedList[serial];

    Machine *m=new CPAP(profile,0);

    ResmedList[serial]=m;
    profile->AddMachine(m);

    m->properties["Serial"]=serial;

    return m;

}

bool ResmedLoader::Open(QString & path,Profile *profile)
{

    QString newpath;

    QString dirtag="DATALOG";
    if (path.endsWith("/"+dirtag)) {
        newpath=path;
    } else {
        newpath=path+"/"+dirtag;
    }

    QDir dir(newpath);

    if ((!dir.exists() || !dir.isReadable()))
        return 0;

    qDebug(("ResmedLoader::Open newpath="+newpath).toLatin1());
    dir.setFilter(QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files | QDir::Hidden | QDir::NoSymLinks);
    dir.setSorting(QDir::Name);
    QFileInfoList flist=dir.entryInfoList();


    list<QString> SerialNumbers;
    list<QString>::iterator sn;

    for (int i=0;i<flist.size();i++) {
        QFileInfo fi=flist.at(i);
        QString filename=fi.fileName();
    }
    return 0;
}

void ResInitModelMap()
{
   // ModelMap[34]="S9 with some weird feature";
};


bool resmed_initialized=false;
void ResmedLoader::Register()
{
    if (resmed_initialized) return;
    qDebug("Registering ResmedLoader");
    RegisterLoader(new ResmedLoader());
    ResInitModelMap();
    resmed_initialized=true;
}

