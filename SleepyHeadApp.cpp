/***************************************************************
 * Name:      SleepyHeadApp.cpp
 * Purpose:   Code for Application Class
 * Author:    Mark Watkins (jedimark64@gmail.com)
 * Created:   2011-05-20
 * Copyright: Mark Watkins (http://sourceforge.net/projects/sleepyhead/)
 * License:   GPL
 **************************************************************/

#ifdef WX_PRECOMP
#include "wx_pch.h"
#endif

#ifdef __BORLANDC__
#pragma hdrstop
#endif //__BORLANDC__

#include <iostream>

#include <wx/log.h>
#include <wx/progdlg.h>
#include <wx/datetime.h>
#include <wx/fs_mem.h>

#include "SleepyHeadApp.h"
#include "SleepyHeadMain.h"
#include "version.h"

#include "sleeplib/prs1_loader.h"
#include "sleeplib/profiles.h"

IMPLEMENT_APP(SleepyHeadApp);

bool SleepyHeadApp::OnInit()
{
    // Initialize the logger

    wxLog *logger=new wxLogStream(&std::cout);
    wxLog::SetActiveTarget(logger);

   // wxFileSystem::AddHandler(new wxMemoryFSHandler);

    wxInitAllImageHandlers();
    //wxDateTime::SetCountry(wxDateTime::USA);

    SetAppName(_("SleepyHead"));
    PRS1Loader::Register();
    Profiles::Scan();

    //loader_progress->Show();

    pref["AppName"]=wxT("Awesome Sleep Application");
    pref["Version"]=wxString(AutoVersion::FULLVERSION_STRING,wxConvUTF8);

    pref["Profile"]=wxGetUserId();
    profile=Profiles::Get(pref["Profile"]);

    profile->LoadMachineData();
   // loader_progress->Show(false);

    SleepyHeadFrame* frame = new SleepyHeadFrame(0L);

    frame->Show();

    return true;
}

int SleepyHeadApp::OnExit()
{
    //delete loader_progress;
    wxLogMessage(wxT("Closing Profiles..."));
    Profiles::Done();

    return true;
}
