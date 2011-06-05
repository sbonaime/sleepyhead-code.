/*
SleepLib CMS50X Loader Implementation

Author: Mark Watkins <jedimark64@users.sourceforge.net>
License: GPL
*/

#include "cms50_loader.h"

CMS50Loader::CMS50Loader()
{
    //ctor
}

CMS50Loader::~CMS50Loader()
{
    //dtor
}


bool cms50_initialized=false;
void CMS50Loader::Register()
{
    if (cms50_initialized) return;
    wxLogVerbose(wxT("Registering CMS50Loader"));
    RegisterLoader(new CMS50Loader());
    //InitModelMap();
    cms50_initialized=true;
}

