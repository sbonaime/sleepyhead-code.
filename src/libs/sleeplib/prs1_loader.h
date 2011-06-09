/*

SleepLib PRS1 Loader Header

Author: Mark Watkins <jedimark64@users.sourceforge.net>
License: GPL

*/

#ifndef PRS1LOADER_H
#define PRS1LOADER_H
//#include <map>
//using namespace std;
#include "machine.h" // Base class: MachineLoader
#include "machine_loader.h"
#include "profiles.h"


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


const int max_load_buffer_size=384*1024;


const wxString prs1_class_name=wxT("PRS1");

class PRS1Loader : public MachineLoader
{
public:
    PRS1Loader();
    virtual ~PRS1Loader();
    virtual bool Open(wxString & path,Profile *profile);
    virtual int Version() { return prs1_data_version; };
    virtual const wxString & ClassName() { return prs1_class_name; };
    Machine *CreateMachine(wxString serial,Profile *profile);

    static void Register();
protected:
    wxString last;
    map<wxString,Machine *> PRS1List;
    int OpenMachine(Machine *m,wxString path,Profile *profile);
    bool ParseProperties(Machine *m,wxString filename);
    bool OpenSummary(Session *session,wxString filename);
    bool OpenEvents(Session *session,wxString filename);
    bool OpenWaveforms(Session *session,wxString filename);
    bool Parse002(Session *session,unsigned char *buffer,int size,time_t timestamp);
    unsigned char * m_buffer;
};


#endif // PRS1LOADER_H
