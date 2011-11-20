#ifndef INTELLIPAP_LOADER_H
#define INTELLIPAP_LOADER_H

#include "SleepLib/machine.h" // Base class: MachineLoader
#include "SleepLib/machine_loader.h"
#include "SleepLib/profiles.h"


//********************************************************************************************
/// IMPORTANT!!!
//********************************************************************************************
// Please INCREMENT the following value when making changes to this loaders implementation.
//
const int intellipap_data_version=0;
//
//********************************************************************************************

class Intellipap:public CPAP
{
public:
    Intellipap(Profile *p,MachineID id=0);
    virtual ~Intellipap();
};


const int intellipap_load_buffer_size=1024*1024;


const QString intellipap_class_name="Intellipap";

class IntellipapLoader : public MachineLoader
{
public:
    IntellipapLoader();
    virtual ~IntellipapLoader();
    virtual int Open(QString & path,Profile *profile);
    virtual int Version() { return intellipap_data_version; }
    virtual const QString & ClassName() { return intellipap_class_name; }
    Machine *CreateMachine(QString serial,Profile *profile);
    static void Register();
protected:
    QString last;
    QHash<QString,Machine *> MachList;
    /*int OpenMachine(Machine *m,QString path,Profile *profile);
    bool ParseProperties(Machine *m,QString filename);
    bool OpenSummary(Session *session,QString filename);
    bool OpenEvents(Session *session,QString filename);
    bool OpenWaveforms(Session *session,QString filename);*/

    unsigned char * m_buffer;
};


#endif // INTELLIPAP_LOADER_H
