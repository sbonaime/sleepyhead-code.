/*

SleepLib (DeVilbiss) Intellipap Loader Implementation

Author: Mark Watkins <jedimark64@users.sourceforge.net>
License: GPL

Notes: Intellipap requires the SmartLink attachment to access this data.
It does not seem to record multiple days, graph data is overwritten each time..

*/

#include <QDir>

#include "intellipap_loader.h"

Intellipap::Intellipap(Profile *p,MachineID id)
    :CPAP(p,id)
{
    m_class=intellipap_class_name;
    properties["Brand"]="DeVilbiss";
    properties["Model"]="Intellipap";
}

Intellipap::~Intellipap()
{
}

IntellipapLoader::IntellipapLoader()
{
    m_buffer=NULL;
}

IntellipapLoader::~IntellipapLoader()
{
    for (QHash<QString,Machine *>::iterator i=MachList.begin(); i!=MachList.end(); i++) {
        delete i.value();
    }
}

int IntellipapLoader::Open(QString & path,Profile *profile)
{
    // Check for SL directory
    QString newpath;

    QString dirtag="SL";
    if (path.endsWith(QDir::separator()+dirtag)) {
        return 0;
        //newpath=path;
    } else {
        newpath=path+QDir::separator()+dirtag;
    }

    unsigned char buf[27];
    QString filename=newpath+QDir::separator()+"U";
    QFile f(filename);
    if (!f.exists()) return 0;

    QHash<quint32,quint32> Sessions;

    quint32 ts1, ts2, length;
    unsigned char cs;
    f.open(QFile::ReadOnly);
    int cnt=0;
    QDateTime epoch(QDate(2000,1,1),QTime(0,0,0),Qt::UTC);
    int ep=epoch.toTime_t();
    do {
        cnt=f.read((char *)buf,9);
        ts1=(buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
        ts2=(buf[4] << 24) | (buf[5] << 16) | (buf[6] << 8) | buf[7];
        Sessions[ts1]=ts2;
        QDateTime d1=QDateTime::fromTime_t(ts1+ep);
        QDateTime d2=QDateTime::fromTime_t(ts2+ep);

        length=ts2-ts1;
        cs=buf[8];
        qDebug() << d1 << d2 << "Length: (" << length << ")";
    } while (cnt>0);
    f.close();

    filename=newpath+QDir::separator()+"L";
    f.setFileName(filename);
    if (!f.exists()) return 0;

    f.open(QFile::ReadOnly);


    // Check for DV5MFirm.bin

    return 1;
}

Machine *IntellipapLoader::CreateMachine(QString serial,Profile *profile)
{
    if (!profile)
        return NULL;
    qDebug() << "Create Machine " << serial;

    QVector<Machine *> ml=profile->GetMachines(MT_CPAP);
    bool found=false;
    QVector<Machine *>::iterator i;
    for (i=ml.begin(); i!=ml.end(); i++) {
        if (((*i)->GetClass()==intellipap_class_name) && ((*i)->properties["Serial"]==serial)) {
            MachList[serial]=*i; //static_cast<CPAP *>(*i);
            found=true;
            break;
        }
    }
    if (found) return *i;

    Machine *m=new Intellipap(profile,0);

    MachList[serial]=m;
    profile->AddMachine(m);

    m->properties["Serial"]=serial;
    return m;
}


bool intellipap_initialized=false;
void IntellipapLoader::Register()
{
    if (intellipap_initialized) return;
    qDebug() << "Registering IntellipapLoader";
    RegisterLoader(new IntellipapLoader());
    //InitModelMap();
    intellipap_initialized=true;
}
