/*
SleepLib ZEO Loader Implementation

Author: Mark Watkins <jedimark64@users.sourceforge.net>
License: GPL
*/

//********************************************************************************************
// IMPORTANT!!!
//********************************************************************************************
// Please INCREMENT the zeo_data_version in zel_loader.h when making changes to this loader
// that change loader behaviour or modify channels.
//********************************************************************************************


#include <QDir>
#include "zeo_loader.h"
#include "SleepLib/machine.h"

ZEOLoader::ZEOLoader()
{
    //ctor
}

ZEOLoader::~ZEOLoader()
{
    //dtor
}
int ZEOLoader::Open(QString & path,Profile *profile)
{
    Q_UNUSED(path)
    Q_UNUSED(profile)

    QString newpath;

    QString dirtag="zeo";
    if (path.toLower().endsWith(QDir::separator()+dirtag)) {
        return 0;
        //newpath=path;
    } else {
        newpath=path+QDir::separator()+dirtag.toUpper();
    }

    QString filename;

    if (path.toLower().endsWith(".csv")) {

    } else if (path.toLower().endsWith(".dat")) {
        // not supported.
    }
    // ZEO folder structure detection stuff here.

    return 0; // number of machines affected
}
Machine *ZEOLoader::CreateMachine(Profile *profile)
{
    if (!profile)
        return NULL;

    // NOTE: This only allows for one ZEO machine per profile..
    // Upgrading their ZEO will use this same record..

    QList<Machine *> ml=profile->GetMachines(MT_SLEEPSTAGE);

    for (QList<Machine *>::iterator i=ml.begin(); i!=ml.end(); i++) {
        if ((*i)->GetClass()==zeo_class_name)  {
            return (*i);
            break;
        }
    }

    qDebug("Create ZEO Machine Record");

    Machine *m=new SleepStage(profile,0);
    m->SetClass(zeo_class_name);
    m->properties[STR_PROP_Brand]="ZEO";
    m->properties[STR_PROP_Model]="Personal Sleep Coach";
    QString s;
    s.sprintf("%i",zeo_data_version);
    m->properties[STR_PROP_DataVersion]=s;

    profile->AddMachine(m);

    return m;
}



static bool zeo_initialized=false;

void ZEOLoader::Register()
{
    if (zeo_initialized) return;
    qDebug("Registering ZEOLoader");
    RegisterLoader(new ZEOLoader());
    //InitModelMap();
    zeo_initialized=true;
}

