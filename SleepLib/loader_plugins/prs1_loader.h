/*

SleepLib PRS1 Loader Header

Author: Mark Watkins <jedimark64@users.sourceforge.net>
License: GPL

*/

#ifndef PRS1LOADER_H
#define PRS1LOADER_H
//#include <map>
//using namespace std;
#include "SleepLib/machine.h" // Base class: MachineLoader
#include "SleepLib/machine_loader.h"
#include "SleepLib/profiles.h"


//********************************************************************************************
/// IMPORTANT!!!
//********************************************************************************************
// Please INCREMENT the following value when making changes to this loaders implementation.
//
const int prs1_data_version=1;
//
//********************************************************************************************

class PRS1:public CPAP
{
public:
    PRS1(Profile *p,MachineID id=0);
    virtual ~PRS1();
};


const int max_load_buffer_size=1024*1024;


const QString prs1_class_name="PRS1";

class PRS1Loader : public MachineLoader
{
public:
    PRS1Loader();
    virtual ~PRS1Loader();
    virtual int Open(QString & path,Profile *profile);
    virtual int Version() { return prs1_data_version; }
    virtual const QString & ClassName() { return prs1_class_name; }
    Machine *CreateMachine(QString serial,Profile *profile);

    static void Register();
protected:
    QString last;
    map<QString,Machine *> PRS1List;
    int OpenMachine(Machine *m,QString path,Profile *profile);
    bool ParseProperties(Machine *m,QString filename);
    bool OpenSummary(Session *session,QString filename);
    bool OpenEvents(Session *session,QString filename);
    bool OpenWaveforms(Session *session,QString filename);
    bool Parse002(Session *session,unsigned char *buffer,int size,qint64 timestamp);
    bool Parse002ASV(Session *session,unsigned char *buffer,int size,qint64 timestamp);

    unsigned char * m_buffer;
};


#endif // PRS1LOADER_H
