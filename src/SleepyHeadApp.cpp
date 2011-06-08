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
#include "sleeplib/loader_plugins/cms50_loader.h"
#include "sleeplib/loader_plugins/zeo_loader.h"

#include "sleeplib/profiles.h"

IMPLEMENT_APP(SleepyHeadApp);

bool SleepyHeadApp::OnInit()
{
    // Initialize the logger


    // It helps to allocate the logger on the heap.. This show work for all platforms now :)

    wxLog *logger=new wxLogStderr(NULL); //new wxLogWindow(NULL,wxT("Debug"),true,false); //new wxLogStderr(NULL); //
    wxLog::SetActiveTarget(logger);
    //wxLog::SetLogLevel(wxLOG_Max);

	wxLogDebug( wxVERSION_STRING );
	wxLogDebug( wxT("Application Initialze...") );

    wxInitAllImageHandlers();
    wxFileSystem::AddHandler(new wxMemoryFSHandler);  // This is damn handy..
    //wxDateTime::SetCountry(wxDateTime::USA);

    SetAppName(_("SleepyHead"));

    PRS1Loader::Register();
    CMS50Loader::Register();
    ZEOLoader::Register();

    Profiles::Scan();

    //loader_progress->Show();

    pref["AppName"]=wxT("Awesome Sleep Application");
    pref["Version"]=wxString(AutoVersion::FULLVERSION_STRING,wxConvUTF8);

    pref["Profile"]=wxGetUserId();
    profile=Profiles::Get(pref["Profile"]);

    profile->LoadMachineData();
   // loader_progress->Show(false);

    SleepyHeadFrame* frame = new SleepyHeadFrame(0L);

    //logger->GetFrame()->Reparent(frame);
    frame->Show();

    return true;
}

int SleepyHeadApp::OnExit()
{
    //delete loader_progress;
    wxLogDebug(wxT("Closing Profiles..."));
    Profiles::Done();
//    wxLog::SetActiveTarget(NULL);
    return true;
}
