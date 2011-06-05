/*
SleepLib CMS50X Loader Implementation

Author: Mark Watkins <jedimark64@users.sourceforge.net>
License: GPL
*/

#include <wx/log.h>
#include "cms50_loader.h"
#include "sleeplib/machine.h"

CMS50Loader::CMS50Loader()
{
    //ctor
}

CMS50Loader::~CMS50Loader()
{
    //dtor
}
bool CMS50Loader::Open(wxString & path,Profile *profile)
{
    // CMS50 folder structure detection stuff here.

    // Not sure whether to both supporting SpO2 files here, as the timestamps are utterly useless for overlay purposes.
    // May just ignore the crap and support my CMS50 logger

    return false;
}
Machine *CMS50Loader::CreateMachine(Profile *profile)
{
    if (!profile) {  // shouldn't happen..
        wxLogError(wxT("No Profile!"));
        return NULL;
    }

    // NOTE: This only allows for one CMS50 machine per profile..
    // Upgrading their oximeter will use this same record..

    vector<Machine *> ml=profile->GetMachines(MT_OXIMETER);

    for (vector<Machine *>::iterator i=ml.begin(); i!=ml.end(); i++) {
        if ((*i)->GetClass()==wxT("CMS50"))  {
            return (*i);
            break;
        }
    }

    wxLogDebug(wxT("Create CMS50 Machine Record"));

    Machine *m=new Oximeter(profile,0);
    m->SetClass(wxT("CMS50"));
    m->properties[wxT("Brand")]=wxT("Contec");
    m->properties[wxT("Model")]=wxT("CMS50X");

    profile->AddMachine(m);

    return m;
}



static bool cms50_initialized=false;

void CMS50Loader::Register()
{
    if (cms50_initialized) return;
    wxLogVerbose(wxT("Registering CMS50Loader"));
    RegisterLoader(new CMS50Loader());
    //InitModelMap();
    cms50_initialized=true;
}

