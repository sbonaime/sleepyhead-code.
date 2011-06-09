/*
SleepLib ZEO Loader Implementation

Author: Mark Watkins <jedimark64@users.sourceforge.net>
License: GPL
*/

//********************************************************************************************
/// IMPORTANT!!!
//********************************************************************************************
// Please INCREMENT the zeo_data_version in zel_loader.h when making changes to this loader
// that change loader behaviour or modify channels.
//********************************************************************************************


#include <wx/log.h>
#include "zeo_loader.h"
#include "sleeplib/machine.h"

ZEOLoader::ZEOLoader()
{
    //ctor
}

ZEOLoader::~ZEOLoader()
{
    //dtor
}
bool ZEOLoader::Open(wxString & path,Profile *profile)
{
    // ZEO folder structure detection stuff here.

    return false;
}
Machine *ZEOLoader::CreateMachine(Profile *profile)
{
    if (!profile) {  // shouldn't happen..
        wxLogError(wxT("No Profile!"));
        return NULL;
    }

    // NOTE: This only allows for one ZEO machine per profile..
    // Upgrading their ZEO will use this same record..

    vector<Machine *> ml=profile->GetMachines(MT_SLEEPSTAGE);

    for (vector<Machine *>::iterator i=ml.begin(); i!=ml.end(); i++) {
        if ((*i)->GetClass()==zeo_class_name)  {
            return (*i);
            break;
        }
    }

    wxLogDebug(wxT("Create ZEO Machine Record"));

    Machine *m=new SleepStage(profile,0);
    m->SetClass(zeo_class_name);
    m->properties[wxT("Brand")]=wxT("ZEO");
    m->properties[wxT("Model")]=wxT("Personal Sleep Coach");
    m->properties[wxT("DataVersion")]=wxString::Format("%li",zeo_data_version);

    profile->AddMachine(m);

    return m;
}



static bool zeo_initialized=false;

void ZEOLoader::Register()
{
    if (zeo_initialized) return;
    wxLogVerbose(wxT("Registering ZEOLoader"));
    RegisterLoader(new ZEOLoader());
    //InitModelMap();
    zeo_initialized=true;
}

