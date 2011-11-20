/*

SleepLib (DeVilbiss) Intellipap Loader Implementation

Author: Mark Watkins <jedimark64@users.sourceforge.net>
License: GPL

Notes: Intellipap requires the SmartLink attachment to access this data.
It does not seem to record multiple days, graph data is overwritten each time..

*/

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
    // Check for DV5MFirm.bin

    return 0;
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
