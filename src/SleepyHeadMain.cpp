/***************************************************************
 * Name:      SleepyHeadMain.cpp
 * Purpose:   Code for Application Frame
 * Author:    Mark Watkins (jedimark64@users.sourceforge.net)
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

#include "version.h"
#include <wx/app.h>
#include <wx/icon.h>
#include <wx/aboutdlg.h>
#include <wx/msgdlg.h>
#include <wx/dirdlg.h>
#include <wx/progdlg.h>
#include <wx/bitmap.h>
#include <wx/log.h>
#include <wx/dcscreen.h>
#include <wx/dcmemory.h>
#include <wx/filedlg.h>
#include <wx/fs_mem.h>
#include <wx/aui/aui.h>
#include <wx/event.h>

#include "SleepyHeadMain.h"
#include "sleeplib/profiles.h"
#include "sleeplib/machine_loader.h"

// Gets rid of the GDIPLUS requirement
// But does it screw up windows drawing abilities?

//#if defined(__WXMSW__)
//extern "C" void *_GdipStringFormatCachedGenericTypographic = NULL;
//#endif


wxProgressDialog *loader_progress;
//helper functions
enum wxbuildinfoformat {
    short_f, long_f };

wxString wxbuildinfo(wxbuildinfoformat format)
{
    wxString wxbuild(wxVERSION_STRING);

    if (format == long_f )
    {
#if defined(__WXMSW__)
        wxbuild << _T("-Windows");
#elif defined(__WXMAC__)
        wxbuild << _T("-Mac");
#elif defined(__UNIX__)
        wxbuild << _T("-Linux");
#endif

#if wxUSE_UNICODE
        wxbuild << _T("-Unicode build");
#else
        wxbuild << _T("-ANSI build");
#endif // wxUSE_UNICODE
    }

    return wxbuild;
}

const long profile_version=0;

SleepyHeadFrame::SleepyHeadFrame(wxFrame *frame)
    : GUIFrame(frame)
{
    //GraphInit(); // Don't do this here: The first gGraphWindow must do it.

    wxString title=wxTheApp->GetAppName()+wxT(" v")+wxString(AutoVersion::_FULLVERSION_STRING,wxConvUTF8);
    SetTitle(title);

    logwindow=new wxLogWindow(this,wxT("Debug"),false,false); //new wxLogStderr(NULL); //

    profile=Profiles::Get();
    if (!profile) {
        wxLogError(wxT("Couldn't get active profile"));
        abort();
    }
    UpdateProfiles();

    if (!pref.Exists("ShowSerialNumbers")) pref["ShowSerialNumbers"]=false;
    if (!pref.Exists("fruitsalad")) pref["fruitsalad"]=true;
    if (!pref.Exists("LinkGraphMovement")) pref["LinkGraphMovement"]=true;
    if (!pref.Exists("UseAntiAliasing")) pref["UseAntiAliasing"]=false;
    if (!pref.Exists("ProfileVersion")) pref["ProfileVersion"]=(long)0;

    if (pref["ProfileVersion"].GetInteger()<profile_version) {
        if (wxMessageBox(title+wxT("\n\nChanges have been made that require the profiles database to be recreated\n\nWould you like to do this right now?"),wxT("Profile Database Changes"),wxYES_NO,this)==wxYES) {
            // Delete all machines from memory.
            pref["ProfileVersion"]=profile_version;


           // assert(1==0);
        }
    }

    ViewMenuSerial->Check(pref["ShowSerialNumbers"]);
    ViewMenuFruitsalad->Check(pref["fruitsalad"]);
    ViewMenuLinkGraph->Check(pref["LinkGraphMovement"]);
    ViewMenuAntiAliasing->Check(pref["UseAntiAliasing"]);

    // wxDisableAsserts();

    // Create AUINotebook Tabs
    wxCommandEvent dummy;
    OnViewMenuSummary(dummy);   // Summary Page
    OnViewMenuDaily(dummy);     // Daily Page

    this->Connect(wxID_ANY, wxEVT_DO_SCREENSHOT, wxCommandEventHandler(SleepyHeadFrame::DoScreenshot));

#if wxUSE_STATUSBAR
    //statusBar->SetStatusText(_("Hello!"), 0);
    statusBar->SetStatusText(wxbuildinfo(long_f), 1);
#endif
}

SleepyHeadFrame::~SleepyHeadFrame()
{
    GraphDone();
}
void SleepyHeadFrame::OnViewLog(wxCommandEvent & event)
{
    logwindow->Show();
}
void SleepyHeadFrame::UpdateProfiles()
{
    wxMenuItemList z=ProfileMenu->GetMenuItems();

    int i=ProfileMenuID;
    for (unsigned int j=0;j<z.size();j++) {
        wxMenuItem *mi=z[j];
        this->Disconnect(i,wxEVT_COMMAND_MENU_SELECTED,wxCommandEventHandler(SleepyHeadFrame::OnProfileSelected));
        ProfileMenu->Remove(mi);
        i++;
    }

    i=ProfileMenuID;

    for (map<wxString,Profile *>::iterator p=Profiles::profiles.begin();p!=Profiles::profiles.end();p++) {
        Profile &pro=*(Profiles::profiles[p->first]);
        wxMenuItem *item=ProfileMenu->AppendRadioItem(i,pro["Realname"],wxEmptyString);

        if (p->first==pref["Profile"].GetString()) {
            item->Check(true);
        }

        this->Connect(i,wxEVT_COMMAND_MENU_SELECTED,wxCommandEventHandler(SleepyHeadFrame::OnProfileSelected));
        i++;
    }
}

void SleepyHeadFrame::OnClose(wxCloseEvent &event)
{
    int idx=main_auinotebook->GetPageIndex(daily);
    if (idx!=wxNOT_FOUND) {
        daily->Close();
    }
    idx=main_auinotebook->GetPageIndex(summary);
    if (idx!=wxNOT_FOUND) {
        summary->Close();
    }
    //delete summary
    wxMenuItemList z=ProfileMenu->GetMenuItems();

    int i=ProfileMenuID;
    for (unsigned int j=0;j<z.size();j++) {
        wxMenuItem *mi=z[j];
        this->Disconnect(i,wxEVT_COMMAND_MENU_SELECTED,wxCommandEventHandler(SleepyHeadFrame::OnProfileSelected));
        ProfileMenu->Remove(mi);
        i++;
    }

    this->Disconnect(wxID_ANY, wxEVT_DO_SCREENSHOT, wxCommandEventHandler(SleepyHeadFrame::DoScreenshot));
    Destroy();
}

void SleepyHeadFrame::OnQuit(wxCommandEvent &event)
{
    Destroy();
}
void SleepyHeadFrame::OnFullscreen(wxCommandEvent& event)
{
    if (!IsFullScreen()) {
        ShowFullScreen(true,wxFULLSCREEN_NOBORDER|wxFULLSCREEN_NOCAPTION|wxFULLSCREEN_NOSTATUSBAR);
    } else {
        ShowFullScreen(false);
    }
}
void SleepyHeadFrame::OnProfileSelected(wxCommandEvent& event)
{
    int id=event.GetId()-ProfileMenuID;

    wxLogMessage(wxT("Profile Selected:")+wxString::Format(wxT("%i"),id));
    /*Machine *m=cpap_machines[id];

    if (m) {
        pref[wxT("DefaultMachine")]=(long)id;
    }
    int idx=main_auinotebook->GetPageIndex(daily);
    if (idx!=wxNOT_FOUND) {
        daily->RefreshData(m);
        daily->Refresh();
    }
    idx=main_auinotebook->GetPageIndex(summary);
    if (idx!=wxNOT_FOUND) {
        summary->ResetProfile(profile)
        summary->Refresh();
    } */
    //event.Skip();
    //Refresh();
}
void SleepyHeadFrame::OnScreenshot(wxCommandEvent& event)
{
    ToolsMenu->UpdateUI();
    //wxWindow::DoUpdateWindowUI();
    wxWindow::UpdateWindowUI();
    // TODO: Make sure the menu is closed.. (It pushes the Update event in front of the manual event we push next)


    wxCommandEvent MyEvent( wxEVT_DO_SCREENSHOT);
    wxPostEvent(this, MyEvent);
}

void SleepyHeadFrame::DoScreenshot( wxCommandEvent &event )
{
    wxRect r=GetRect();

#if defined(__UNIX__)
    // Height of statusbar needs adding in
    wxRect j=statusBar->GetRect();

    r.height += j.height;
#endif

#if defined(__WXMAC__)
    wxMessageBox(wxT("Sorry.. Screenshots don't work on your platform.\n\nPlease use your Mac's screenshot capability instead."),wxT("Naughty Apple!"),wxICON_EXCLAMATION,this);
#else
    wxScreenDC sdc;
    wxMemoryDC mdc;

    wxBitmap bmp(r.width, r.height,-1);
    //wxBitMap *bmp=wxEmptyImage(r.width,r.height);
    mdc.SelectObject(bmp);

    mdc.Blit((wxCoord)0, (wxCoord)0, (wxCoord)r.width, (wxCoord)r.height, &sdc, (wxCoord)r.x, (wxCoord)r.y);

    mdc.SelectObject(wxNullBitmap);

    wxDateTime d=wxDateTime::Now();

//    wxDirDialog sfs(this,_("Choose a Directory")); //,wxT(""),wxT(""),style=wxFD_OPEN);
    wxString filename=wxSaveFileSelector(_("Please give a filename for the screenshot"),wxT("png"),wxT("Sleepyhead-")+d.Format(wxT("%Y%m%d-%H%M%S")),this);
    if (!filename.IsEmpty()) {
        if (!filename.Lower().EndsWith(wxT(".png"))) filename+=wxT(".png");
        wxImage img=bmp.ConvertToImage();
        if (!img.SaveFile(filename, wxBITMAP_TYPE_PNG)) {
            wxLogError(wxT("Couldn't save screenshot ")+filename);
        }
    }
#endif
}
void SleepyHeadFrame::OnLinkGraphs( wxCommandEvent& event )
{
    pref["LinkGraphMovement"]=event.IsChecked();
}
void SleepyHeadFrame::OnShowSerial(wxCommandEvent& event)
{
    pref["ShowSerialNumbers"]=event.IsChecked();
}
void SleepyHeadFrame::OnFruitsalad(wxCommandEvent& event)
{
    pref["fruitsalad"]=event.IsChecked();
    Refresh();
}
void SleepyHeadFrame::OnAntiAliasing( wxCommandEvent& event )
{
    pref["UseAntiAliasing"]=event.IsChecked();
    Refresh();

}

void SleepyHeadFrame::OnAbout(wxCommandEvent &event)
{
    // wxAboutBox is fairly useless.
    wxString day=wxString(AutoVersion::_DATE,wxConvUTF8);
    wxString month=wxString(AutoVersion::_MONTH,wxConvUTF8);
    wxString year=wxString(AutoVersion::_YEAR,wxConvUTF8);
    wxString date=day+wxT("/")+month+wxT("/")+year;
    wxString msg=wxTheApp->GetAppName()+wxT(" v")+wxString(AutoVersion::_FULLVERSION_STRING,wxConvUTF8)+_(" alpha preview ")+date+_("\n\n")+wxT("\u00a9")+_("2011 Mark Watkins & Troy Schultz\n\n");
    msg+=_("Website: http://sleepyhead.sourceforge.net\n\n");
    msg+=_("License: GNU Public License (GPL)\n\n");
    msg+=_("This software is under active development, and is guaranteed to break and change regularly! Use at your own risk.\n\n");
    msg+=_("Relying on this softwares' accuracy for making personal medical decisions is probably grounds to get you committed. Please don't do it!\n\n");
    msg+=_("Combinations of letters which form those of trademarks belong to those who own them.");

    wxMessageBox(msg, _("Welcome to..."),wxOK,this);
}
void SleepyHeadFrame::OnImportSD(wxCommandEvent &event)
{
    wxDirDialog dd(this,_("Choose a Directory")); //,wxT(""),wxT(""),style=wxFD_OPEN);
    if (dd.ShowModal()!=wxID_OK) return;


    loader_progress=new wxProgressDialog(_("SleepyHead"),_("Please Wait..."),100,this, wxPD_APP_MODAL|wxPD_AUTO_HIDE|wxPD_SMOOTH);
    loader_progress->Hide();
    wxString path=dd.GetPath();

    loader_progress->Update(0);
    loader_progress->Show();
    profile->Import(path);
    loader_progress->Update(100);
    loader_progress->Show(false);
    loader_progress->Destroy();
    loader_progress=NULL;

    int idx=main_auinotebook->GetPageIndex(daily);
    if (idx!=wxNOT_FOUND) {
        daily->ResetDate();
        //daily->RefreshData();
    }
    idx=main_auinotebook->GetPageIndex(summary);
    if (idx!=wxNOT_FOUND) {
        summary->ResetProfile(profile); // resets the date ranges..
        summary->RefreshData();
        summary->Refresh();
    }

}
void SleepyHeadFrame::OnViewMenuDaily( wxCommandEvent& event )
{
    int idx=main_auinotebook->GetPageIndex(daily);
    if (idx==wxNOT_FOUND) {
        daily=new Daily(this,profile);
        main_auinotebook->AddPage(daily,_("Daily"),true,wxNullBitmap);
        //daily->ResetDate();
        //daily->RefreshData();

    } else {
        main_auinotebook->SetSelection(idx);
        daily->Refresh(true);
    }


}
void SleepyHeadFrame::OnViewMenuSummary( wxCommandEvent& event )
{

    int idx=main_auinotebook->GetPageIndex(summary);
    if (idx==wxNOT_FOUND) {
        summary=new Summary(this,profile);
        main_auinotebook->AddPage(summary,_("Summary"),true,wxNullBitmap);
        summary->ResetProfile(profile);
        summary->RefreshData();
        summary->Refresh();
    } else {
        main_auinotebook->SetSelection(idx);
        summary->Refresh(true);
        summary->AHI->Refresh(true);
        summary->USAGE->Refresh(true);
        summary->LEAK->Refresh(true);
        summary->PRESSURE->Refresh(true);
    }
}

Summary::Summary(wxWindow *win,Profile *_profile)
:SummaryPanel(win),profile(_profile)
{
    AddData(ahidata=new HistoryData(profile));
    AddData(pressure=new HistoryCodeData(profile,CPAP_PressureAverage));
    AddData(pressure_min=new HistoryCodeData(profile,CPAP_PressureMin));
    AddData(pressure_max=new HistoryCodeData(profile,CPAP_PressureMax));

    AddData(pressure_eap=new HistoryCodeData(profile,BIPAP_EAPAverage));
    AddData(pressure_iap=new HistoryCodeData(profile,BIPAP_IAPAverage));

   // pressure->ForceMinY(3);
   // pressure->ForceMaxY(12);
    AddData(leak=new HistoryCodeData(profile,CPAP_LeakMedian));
    AddData(usage=new UsageHistoryData(profile,UHD_Hours));
    AddData(waketime=new UsageHistoryData(profile,UHD_Waketime));
    AddData(bedtime=new UsageHistoryData(profile,UHD_Bedtime));

    AHI=new gGraphWindow(ScrolledWindow,-1,wxT("AHI"),wxPoint(0,0), wxSize(400,180), wxNO_BORDER);
    AHI->SetMargins(10,15,65,80);
    AHI->AddLayer(new gFooBar());
    AHI->AddLayer(new gYAxis(wxBLACK));
    AHI->AddLayer(new gBarChart(ahidata,wxRED));
   // AHI->AddLayer(new gXAxis(NULL,wxBLACK));
    //AHI->AddLayer(new gLineChart(ahidata,wxRED));
    fgSizer->Add(AHI,1,wxEXPAND);

    PRESSURE=new gGraphWindow(ScrolledWindow,-1,wxT("Pressure"),wxPoint(0,0), wxSize(400,180), wxNO_BORDER);
    PRESSURE->SetMargins(10,15,65,80);
    PRESSURE->AddLayer(new gYAxis(wxBLACK));
    PRESSURE->AddLayer(new gXAxis(wxBLACK));
    PRESSURE->AddLayer(new gFooBar());

    PRESSURE->AddLayer(prmax=new gLineChart(pressure_max,wxBLUE,6192,false,true,true));
    PRESSURE->AddLayer(prmin=new gLineChart(pressure_min,wxRED,6192,false,true,true));
    PRESSURE->AddLayer(eap=new gLineChart(pressure_eap,wxBLUE,6192,false,true,true));
    PRESSURE->AddLayer(iap=new gLineChart(pressure_iap,wxRED,6192,false,true,true));
    PRESSURE->AddLayer(pr=new gLineChart(pressure,wxDARK_GREEN,6192,false,true,true));

    fgSizer->Add(PRESSURE,1,wxEXPAND);

    LEAK=new gGraphWindow(ScrolledWindow,-1,wxT("Mask Leak"),wxPoint(0,0), wxSize(400,180), wxNO_BORDER);
    LEAK->SetMargins(10,15,65,80);
    //LEAK->AddLayer(new gBarChart(leak,wxYELLOW));
    LEAK->AddLayer(new gXAxis(wxBLACK));
    LEAK->AddLayer(new gYAxis(wxBLACK));
    LEAK->AddLayer(new gFooBar());
    LEAK->AddLayer(new gLineChart(leak,wxPURPLE,6192,false,false,true));
    fgSizer->Add(LEAK,1,wxEXPAND);


    USAGE=new gGraphWindow(ScrolledWindow,-1,wxT("Usage (Hours)"),wxPoint(0,0), wxSize(400,180), wxNO_BORDER);
    USAGE->SetMargins(10,15,65,80);
    USAGE->AddLayer(new gFooBar());
    USAGE->AddLayer(new gYAxis(wxBLACK));
    USAGE->AddLayer(new gBarChart(usage,wxGREEN));
    //USAGE->AddLayer(new gXAxis(wxBLACK));

    //USAGE->AddLayer(new gLineChart(usage,wxGREEN));
    fgSizer->Add(USAGE,1,wxEXPAND);

    //    Logo.LoadFile(wxT("./pic.png"));
    //wxMemoryFSHandler::AddFile(_T("test.png"), Logo, wxBITMAP_TYPE_PNG);
//    RefreshData();
    dummyday=new Day(NULL);
}
Summary::~Summary()
{
    delete dummyday;
//    wxMemoryFSHandler::RemoveFile(_T("test.png"));
}
void Summary::ResetProfile(Profile *p)
{
    profile=p;

    if (profile->FirstDay().IsValid()) {
        wxDateTime last=profile->LastDay()+wxTimeSpan::Day();
        wxDateTime first=profile->FirstDay()+wxTimeSpan::Day();
        wxDateTime start=last-wxTimeSpan::Days(30);

        StartDatePicker->SetRange(first,last);
        EndDatePicker->SetRange(first,last);
        if (start<first) start=first;

        StartDatePicker->SetValue(start);
        EndDatePicker->SetValue(last);

        for (list<HistoryData *>::iterator h=Data.begin();h!=Data.end();h++) {
            (*h)->SetProfile(p);
            (*h)->ResetDateRange();
            (*h)->Reload(NULL);
        }
    }

}
void Summary::RefreshData()
{
    wxDateTime first=StartDatePicker->GetValue()-wxTimeSpan::Day();
    wxDateTime last=EndDatePicker->GetValue();

    for (list<HistoryData *>::iterator h=Data.begin();h!=Data.end();h++) {
        //(*h)->Update(dummyday);
        (*h)->SetDateRange(first,last);
    }

    wxString submodel=_("Unknown Model");
    double ahi=ahidata->GetAverage();
    double avp=pressure->GetAverage();
    double apmin=pressure_min->GetAverage();
    double apmax=pressure_max->GetAverage();
    double aeap=pressure_eap->GetAverage();
    double aiap=pressure_iap->GetAverage();
    double bt=fmod(bedtime->GetAverage(),12.0);
    double ua=usage->GetAverage();
    double wt=waketime->GetAverage(); //fmod(bt+ua,12.0);

    wxString html=wxT("<html><body leftmargin=0 rightmargin=0 topmargin=0 marginwidth=0 marginheight=0>");

    //html=html+wxT("<img src=\"memory:test.png\" width='180'>");
    html=html+wxT("<table cellspacing=2 cellpadding=0>\n");
    if (avp>0) {

        html=html+wxT("<tr><td colspan=2 align=center><i>")+_("Machine Information has been removed because this page has become machine agnostic. Not sure what to display here.")+wxT("</i></td></tr>\n");
        /*if (machine->properties.find(wxT("SubModel"))!=machine->properties.end())
            submodel=wxT(" <br>\n ")+machine->properties[wxT("SubModel")];
        html=html+wxT("<tr><td colspan=2 align=center><b>")+machine->properties[wxT("Brand")]+wxT("</b> <br/>")+machine->properties[wxT("Model")]+wxT("&nbsp;")+machine->properties[wxT("ModelNumber")]+submodel+wxT("</td></tr>\n");
        html=html+wxT("<tr><td colspan=2 align=center>")+_("Firmware")+wxT(" ")+machine->properties[wxT("SoftwareVersion")]+wxT("</td></tr>"); */
        html=html+wxT("<tr><td>&nbsp;</td><td>&nbsp;</td></tr>\n");
        html=html+wxT("<tr><td colspan=2 align=left><i>")+_("Indice Averages")+wxT("</i></td></tr>\n");
        html=html+wxT("<tr><td><b>")+_("AHI")+wxT("</b></td><td>")+wxString::Format(wxT("%0.2f"),ahi)+wxT("</td></tr>\n");
        html=html+wxT("<tr><td><b>")+_("Pressure&nbsp;Avg")+wxT("</b></td><td>")+wxString::Format(wxT("%0.2fcmH2O"),avp)+wxT("</td></tr>\n");
        if (aiap>0) {
            html=html+wxT("<tr><td><b>")+_("IPAP&nbsp;Avg")+wxT("</b></td><td>")+wxString::Format(wxT("%0.2fcmH2O"),aiap)+wxT("</td></tr>\n");
            html=html+wxT("<tr><td><b>")+_("EPAP&nbsp;Avg")+wxT("</b></td><td>")+wxString::Format(wxT("%0.2fcmH2O"),aeap)+wxT("</td></tr>\n");
            iap->SetVisible(true);
            eap->SetVisible(true);
            prmax->SetVisible(false);
            prmin->SetVisible(false);
            pr->SetVisible(false);
        } else {
            if (apmin!=apmax) {
                prmax->SetVisible(true);
                prmin->SetVisible(true);
                pr->SetVisible(true);
                iap->SetVisible(false);
                eap->SetVisible(false);
                html=html+wxT("<tr><td><b>")+_("Pressure&nbsp;Min")+wxT("</b></td><td>")+wxString::Format(wxT("%0.2fcmH2O"),apmin)+wxT("</td></tr>\n");
                html=html+wxT("<tr><td><b>")+_("Pressure&nbsp;Max")+wxT("</b></td><td>")+wxString::Format(wxT("%0.2fcmH2O"),apmax)+wxT("</td></tr>\n");
            } else {
                pr->SetVisible(true);
                prmax->SetVisible(false);
                prmin->SetVisible(false);
                iap->SetVisible(false);
                eap->SetVisible(false);
                //prmax->SetVisible(false);
                //prmin->SetVisible(false);
            }
        }
        html=html+wxT("<tr><td>&nbsp;</td><td>&nbsp;</td></tr>\n");
        html=html+wxT("<tr><td><b>")+_("Mask Leaks")+wxT("</b></td><td>")+wxString::Format(wxT("%0.2f"),leak->GetAverage())+wxT("</td></tr>\n");

        html=html+wxT("<tr><td>&nbsp;</td><td>&nbsp;</td></tr>\n");
        html=html+wxT("<tr><td><b>")+_("Bedtime")+wxT("</b></td><td>")+wxString::Format(wxT("%02.0f:%02i"),bt,int(bt*60) % 60)+wxT("</td></tr>\n");
        html=html+wxT("<tr><td><b>")+_("Waketime")+wxT("</b></td><td>")+wxString::Format(wxT("%02.0f:%02i"),wt,int(wt*60) % 60)+wxT("</td></tr>\n");
        html=html+wxT("<tr><td><b>")+_("Hours/Night")+wxT("</b></td><td>")+wxString::Format(wxT("%02.0f:%02i"),ua,int(ua*60)%60)+wxT("</td></tr>\n");
        html=html+wxT("</table>");
    } else {
        html+=wxT("<div align=center><h1>Welcome</h1><br/>&nbsp;<br/><i>Please import some data</i></div>");
    }

    html+=wxT("</body></html>");
    HTMLInfo->SetPage(html);
}
void Summary::EnableDatePickers(bool b)
{
        StartDatePicker->Enable(b);
        sdLabel->Enable(b);
        EndDatePicker->Enable(b);
        edLabel->Enable(b);
}
void Summary::OnRBSelect( wxCommandEvent& event )
{
    wxDateTime start=StartDatePicker->GetValue();
    wxDateTime end=EndDatePicker->GetValue();

    if (rbCustomDate->GetValue()) {
        EnableDatePickers(true);
    } else if (rbAll->GetValue()) {
        start=profile->FirstDay()+wxTimeSpan::Day();
        end=profile->LastDay()+wxTimeSpan::Day();
        EnableDatePickers(false);
    } else if (rbLastMonth->GetValue()) {
        end=profile->LastDay()+wxTimeSpan::Day();
        start=end-wxTimeSpan::Days(30-1);
        EnableDatePickers(false);
    } else if (rbLastWeek->GetValue()) {
        end=profile->LastDay()+wxTimeSpan::Day();
        start=end-wxTimeSpan::Days(7-1);
        EnableDatePickers(false);
    }

    EndDatePicker->SetRange(start,profile->LastDay()+wxTimeSpan::Day());
    StartDatePicker->SetRange(profile->FirstDay()+wxTimeSpan::Day(),end);

    StartDatePicker->SetValue(start);
    EndDatePicker->SetValue(end);

    RefreshData();
}

void Summary::OnStartDateChanged( wxDateEvent& event )
{
    wxDateTime st=StartDatePicker->GetValue();
    EndDatePicker->SetRange(st,profile->LastDay()+wxTimeSpan::Day());

    RefreshData();
}
void Summary::OnEndDateChanged( wxDateEvent& event )
{
    wxDateTime et=EndDatePicker->GetValue();
    StartDatePicker->SetRange(profile->FirstDay()+wxTimeSpan::Day(),et);

    RefreshData();
}
void Summary::OnClose(wxCloseEvent &event)
{
    Destroy();
}


Daily::Daily(wxWindow *win,Profile *p)
:DailyPanel(win),profile(p)
{
    GraphWindow = new wxScrolledWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHSCROLL|wxVSCROLL );
	GraphWindow->SetScrollRate( 5, 5 );

	m_mgr.AddPane( GraphWindow, wxAuiPaneInfo() .Center() .Caption( wxT("Graphs") ).CloseButton( false ).MaximizeButton( false ).MinimizeButton( false ).PinButton( true ).Dock().Resizable().FloatingSize( wxDefaultSize ).DockFixed( false ).MinSize( wxSize( 440,400 ) ) );
    m_mgr.Update();
	gwSizer = new wxFlexGridSizer( 0, 1, 0, 0 );
	gwSizer->AddGrowableCol( 0 );
	gwSizer->SetFlexibleDirection( wxVERTICAL );
	gwSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_ALL );

	GraphWindow->SetSizer( gwSizer );
	GraphWindow->Layout();
	gwSizer->Fit(GraphWindow);


    tiap_bmp=teap_bmp=tap_bmp=ahi_bmp=NULL;
    HTMLInfo=new wxHtmlWindow(this);
    HTMLInfo->SetBorders(4);
    EventTree=new wxTreeCtrl(this);

    Notebook->AddPage(HTMLInfo,wxT("Details"),false,wxNullBitmap);
    Notebook->AddPage(EventTree,wxT("Events"),false,wxNullBitmap);


    AddCPAPData(flags[0]=new FlagData(CPAP_CSR,7,1,0));
    AddCPAPData(flags[1]=new FlagData(CPAP_ClearAirway,6));
    AddCPAPData(flags[2]=new FlagData(CPAP_Obstructive,5));
    AddCPAPData(flags[3]=new FlagData(CPAP_Hypopnea,4));
    AddCPAPData(flags[4]=new FlagData(CPAP_FlowLimit,3));
    AddCPAPData(flags[5]=new FlagData(CPAP_VSnore,2));
    AddCPAPData(flags[6]=new FlagData(CPAP_RERA,1));
    AddCPAPData(flags[7]=new FlagData(PRS1_PressurePulse,1));
    AddCPAPData(flags[8]=new FlagData(PRS1_VSnore2,1));
    AddCPAPData(flags[9]=new FlagData(PRS1_Unknown0E,1));


    AddCPAPData(frw=new WaveData(CPAP_FlowRate));
    FRW=new gGraphWindow(GraphWindow,-1,wxT("Flow Rate"),wxPoint(0,0), wxSize(600,150), wxNO_BORDER);
    gLineChart *g;
    FRW->AddLayer(new gYAxis(wxBLACK));
    FRW->AddLayer(new gXAxis(wxBLACK));
    FRW->AddLayer(new gFooBar());
    FRW->AddLayer(new gLineOverlayBar(flags[0],wxGREEN2,wxT("CSR")));
    FRW->AddLayer(g=new gLineChart(frw,wxBLACK,200000,true));
    g->ReportEmpty(true);
    FRW->AddLayer(new gLineOverlayBar(flags[7],wxRED,wxT("PR"),LOT_Dot));
    FRW->AddLayer(new gLineOverlayBar(flags[6],wxYELLOW,wxT("RE")));
    FRW->AddLayer(new gLineOverlayBar(flags[9],wxDARK_GREEN,wxT("U0E")));
    FRW->AddLayer(new gLineOverlayBar(flags[5],wxRED,wxT("VS")));
    FRW->AddLayer(new gLineOverlayBar(flags[4],wxBLACK,wxT("FL")));
    FRW->AddLayer(new gLineOverlayBar(flags[3],wxBLUE,wxT("H")));
    FRW->AddLayer(new gLineOverlayBar(flags[2],wxAQUA,wxT("OA")));
    FRW->AddLayer(new gLineOverlayBar(flags[1],wxPURPLE,wxT("CA")));

    SF=new gGraphWindow(GraphWindow,-1,wxT("Event Flags"),wxPoint(0,0), wxSize(600,180), wxNO_BORDER);

    AddCPAPData(pressure_iap=new EventData(CPAP_IAP));
    AddCPAPData(pressure_eap=new EventData(CPAP_EAP));
    AddCPAPData(prd=new EventData(CPAP_Pressure));
    PRD=new gGraphWindow(GraphWindow,-1,wxT("Pressure"),wxPoint(0,0), wxSize(600,130), wxNO_BORDER);
    PRD->AddLayer(new gXAxis(wxBLACK));
    PRD->AddLayer(new gYAxis(wxBLACK));
    PRD->AddLayer(new gFooBar());
    PRD->AddLayer(new gLineChart(prd,wxDARK_GREEN,4096,false,false,true));
    PRD->AddLayer(new gLineChart(pressure_iap,wxBLUE,4096,false,true,true));
    PRD->AddLayer(new gLineChart(pressure_eap,wxRED,4096,false,true,true));


    AddCPAPData(leakdata=new EventData(CPAP_Leak,0));
    //leakdata->ForceMinY(0);
    //leakdata->ForceMaxY(120);
    LEAK=new gGraphWindow(GraphWindow,-1,wxT("Leaks"),wxPoint(0,0), wxSize(600,130), wxNO_BORDER);
    LEAK->AddLayer(new gXAxis(wxBLACK));
    LEAK->AddLayer(new gYAxis(wxBLACK));
    LEAK->AddLayer(new gFooBar());
    LEAK->AddLayer(new gLineChart(leakdata,wxPURPLE,4096,false,false,false));






  //  SF->SetMargins(10,15,20,80);

 //  #endif

    const int sfc=9;


    SF->SetLeftMargin(SF->GetLeftMargin()+gYAxis::Margin);
    SF->SetBlockZoom(true);
    SF->AddLayer(new gXAxis(wxBLACK));
    SF->AddLayer(new gFlagsLine(flags[9],wxDARK_GREEN,wxT("U0E"),8,sfc));
    SF->AddLayer(new gFlagsLine(flags[8],wxRED,wxT("VS2"),6,sfc));
    SF->AddLayer(new gFlagsLine(flags[6],wxYELLOW,wxT("RE"),7,sfc));
    SF->AddLayer(new gFlagsLine(flags[5],wxRED,wxT("VS"),5,sfc));
    SF->AddLayer(new gFlagsLine(flags[4],wxBLACK,wxT("FL"),4,sfc));
    SF->AddLayer(new gFlagsLine(flags[3],wxBLUE,wxT("H"),3,sfc));
    SF->AddLayer(new gFlagsLine(flags[2],wxAQUA,wxT("OA"),2,sfc));
    SF->AddLayer(new gFlagsLine(flags[1],wxPURPLE,wxT("CA"),1,sfc));
    SF->AddLayer(new gFlagsLine(flags[0],wxGREEN2,wxT("CSR"),0,sfc));
    SF->AddLayer(new gFooBar(wxGREEN,wxDARK_GREY,true));


    AddCPAPData(snore=new EventData(CPAP_SnoreGraph,0));
    //snore->ForceMinY(0);
    //snore->ForceMaxY(15);
    SNORE=new gGraphWindow(GraphWindow,-1,wxT("Snore"),wxPoint(0,0), wxSize(600,130), wxNO_BORDER);
    SNORE->AddLayer(new gXAxis(wxBLACK));
    SNORE->AddLayer(new gYAxis(wxBLACK));
    SNORE->AddLayer(new gFooBar());
    SNORE->AddLayer(new gLineChart(snore,wxDARK_GREY,4096,false,false,true));

    AddOXIData(pulse=new EventData(OXI_Pulse,0,65536,true));
    //pulse->ForceMinY(40);
    //pulse->ForceMaxY(120);

    PULSE=new gGraphWindow(GraphWindow,-1,wxT("Pulse"),wxPoint(0,0), wxSize(600,130), wxNO_BORDER);
    PULSE->AddLayer(new gXAxis(wxBLACK));
    PULSE->AddLayer(new gYAxis(wxBLACK));
    PULSE->AddLayer(new gFooBar());
    PULSE->AddLayer(new gLineChart(pulse,wxRED,65536,false,false,true));

    AddOXIData(spo2=new EventData(OXI_SPO2,0,65536,true));
    //spo2->ForceMinY(60);
    //spo2->ForceMaxY(100);
    SPO2=new gGraphWindow(GraphWindow,-1,wxT("SpO2"),wxPoint(0,0), wxSize(600,130), wxNO_BORDER);
    SPO2->AddLayer(new gXAxis(wxBLACK));
    SPO2->AddLayer(new gYAxis(wxBLACK));
    SPO2->AddLayer(new gFooBar());
    SPO2->AddLayer(new gLineChart(spo2,wxBLUE,65536,false,false,true));
    SPO2->LinkZoom(PULSE);
    PULSE->LinkZoom(SPO2);


//    #if defined(__UNIX__)
    FRW->LinkZoom(SF);
    FRW->LinkZoom(PRD);
    FRW->LinkZoom(LEAK);
    FRW->LinkZoom(SNORE);
    SF->LinkZoom(FRW);
    SF->LinkZoom(PRD); // Uncomment to link in more graphs.. Too slow on windows.
    SF->LinkZoom(LEAK);
    SF->LinkZoom(SNORE);
    PRD->LinkZoom(SF);
    PRD->LinkZoom(FRW);
    PRD->LinkZoom(LEAK);
    PRD->LinkZoom(SNORE);
    LEAK->LinkZoom(SF);
    LEAK->LinkZoom(FRW);
    LEAK->LinkZoom(PRD);
    LEAK->LinkZoom(SNORE);
    SNORE->LinkZoom(SF);
    SNORE->LinkZoom(FRW);
    SNORE->LinkZoom(PRD);
    SNORE->LinkZoom(LEAK);




    AddCPAPData(tap_eap=new TAPData(CPAP_EAP));
    AddCPAPData(tap_iap=new TAPData(CPAP_IAP));
    AddCPAPData(tap=new TAPData(CPAP_Pressure));



    TAP=new gGraphWindow(GraphWindow,-1,wxT(""),wxPoint(0,0), wxSize(600,30), wxNO_BORDER);  //Time@Pressure
    //TAP->SetMargins(20,15,5,50);
    TAP->SetMargins(0,1,0,1);
    TAP->AddLayer(new gCandleStick(tap));

    TAP_EAP=new gGraphWindow(GraphWindow,-1,wxT(""),wxPoint(0,0), wxSize(600,30), wxNO_BORDER); //Time@EPAP
    TAP_EAP->SetMargins(0,1,0,1);
    TAP_EAP->AddLayer(new gCandleStick(tap_eap));

    TAP_IAP=new gGraphWindow(GraphWindow,-1,wxT(""),wxPoint(0,0), wxSize(600,30), wxNO_BORDER); //Time@IPAP
    TAP_IAP->SetMargins(0,1,0,1);
    TAP_IAP->AddLayer(new gCandleStick(tap_iap));

    G_AHI=new gGraphWindow(GraphWindow,-1,wxT(""),wxPoint(0,0), wxSize(600,30), wxNO_BORDER); //Event Breakdown")
    G_AHI->SetMargins(0,1,0,1);
    AddCPAPData(g_ahi=new AHIData());
    gCandleStick *l=new gCandleStick(g_ahi);
    l->AddName(wxT("H"));
    l->AddName(wxT("OA"));
    l->AddName(wxT("CA"));
    l->AddName(wxT("RE"));
    l->AddName(wxT("FL"));
    l->AddName(wxT("CSR"));
    l->color.clear();
    l->color.push_back(*wxBLUE);
    l->color.push_back(*wxAQUA);
    l->color.push_back(wxColor(0xff,0x40,0xff,0xff)); //wxPURPLE);
    l->color.push_back(*wxYELLOW);
    l->color.push_back(*wxBLACK);
    l->color.push_back(*wxGREEN2);
    G_AHI->AddLayer(l);


    G_AHI->Hide();
    TAP->Hide();
    TAP_IAP->Hide();
    TAP_EAP->Hide();

    gwSizer->Add(SF,1,wxEXPAND);
    gwSizer->Add(FRW,1,wxEXPAND);
    gwSizer->Add(PRD,1,wxEXPAND);
    gwSizer->Add(LEAK,1,wxEXPAND);
    gwSizer->Add(SNORE,1,wxEXPAND);
    //gwSizer->Add(TAP,1,wxEXPAND);
    gwSizer->Add(PULSE,1,wxEXPAND);
    gwSizer->Add(SPO2,1,wxEXPAND);


    gwSizer->Layout();
    //fgSizer->Add(G_AHI,1,wxEXPAND);
    //fgSizer->Add(TAP,1,wxEXPAND);
    //fgSizer->Add(TAP_IAP,1,wxEXPAND);
    //fgSizer->Add(TAP_EAP,1,wxEXPAND);

    this->Connect(wxEVT_COMMAND_TREE_SEL_CHANGED, wxTreeEventHandler(Daily::OnEventTreeSelection), NULL, this);
    this->Connect(wxID_ANY, wxEVT_REFRESH_DAILY, wxCommandEventHandler(Daily::RefreshData));


    //this->Connect(wxEVT_SCROLLWIN_THUMBTRACK
    //EVT_SCROLLWIN_THUMBTRACK(Daily::OnWinScroll)
    //this->Connect(GraphWindow->GetId(),wxEVT_SCROLLWIN_THUMBTRACK, wxScrollWinEventHandler(Daily::OnWinScroll));

    Refresh(); // Important. Don't change the order of the next two lines.
    ResetDate();
}

Daily::~Daily()
{
    if (ahi_bmp) {
        wxMemoryFSHandler::RemoveFile(_T("ahi.png"));
        delete ahi_bmp;
    }
    if (tap_bmp) {
        wxMemoryFSHandler::RemoveFile(_T("tap.png"));
        delete tap_bmp;
    }
    if (tiap_bmp) {
        wxMemoryFSHandler::RemoveFile(_T("tiap.png"));
        delete tiap_bmp;
    }
    if (teap_bmp) {
        wxMemoryFSHandler::RemoveFile(_T("teap.png"));
        delete teap_bmp;
    }

    //this->Disconnect(wxEVT_SCROLLWIN_THUMBTRACK, wxScrollWinEventHandler(Daily::OnWinScroll));
    //this->Disconnect(wxEVT_SCROLLWIN_THUMBTRACK, EVT_SCROLLWIN_THUMBTRACK(Daily::OnWinScroll));
    //this->Disconnect(wxEVT_SCROLLWIN_THUMBRELEASE, EVT_SCROLLWIN_THUMBRELEASE(Daily::OnWinScroll));

    this->Disconnect(wxID_ANY, wxEVT_REFRESH_DAILY, wxCommandEventHandler(Daily::RefreshData));
    this->Disconnect(wxEVT_COMMAND_TREE_SEL_CHANGED, wxTreeEventHandler( Daily::OnEventTreeSelection), NULL, this);
}
void Daily::OnWinScroll(wxScrollWinEvent &event)
{
    wxLogMessage(wxT("OnWinScroll"));
    Update();
}
void Daily::OnClose(wxCloseEvent &event)
{
    Destroy();
}
void Daily::OnEventTreeSelection( wxTreeEvent& event )
{
    wxTreeItemId id=event.GetItem();

    if (!EventTree->ItemHasChildren(id)) {
        wxDateTime d;
        d.ParseFormat(EventTree->GetItemText(id),wxT("%Y-%m-%d %H:%M:%S"));
        double st=(d-wxTimeSpan::Seconds(180)).GetMJD();
        double et=(d+wxTimeSpan::Seconds(180)).GetMJD();
        FRW->SetXBounds(st,et);
        SF->SetXBounds(st,et);
        PRD->SetXBounds(st,et);
        LEAK->SetXBounds(st,et);
        SNORE->SetXBounds(st,et);
    }
}

void Daily::ResetDate()
{
    foobar_datehack=false; // this exists due to a wxGTK bug.
  //  RefreshData();

    Update();
    wxDateTime date;
    if (profile->LastDay().IsValid()) {
        date=profile->LastDay()+wxTimeSpan::Day();
        Calendar->SetDate(date);
    } else {
        Calendar->SetDate(wxDateTime::Today());
    }

    wxCalendarEvent ev;
    ev.SetDate(date);
    OnCalendarMonth(ev);
    RefreshData();
}
void Daily::RefreshData()
{
    wxCommandEvent MyEvent( wxEVT_REFRESH_DAILY);
    wxPostEvent(this, MyEvent);
}
void Daily::RefreshData(wxCommandEvent& event)
{
    wxDateTime date=Calendar->GetDate();
    date.ResetTime();
    date.SetHour(0);
    date-=wxTimeSpan::Days(1);

    Day *cpap=profile->GetDay(date,MT_CPAP);
    Day *oxi=profile->GetDay(date,MT_OXIMETER);
    if (cpap) UpdateCPAPGraphs(cpap);
    if (oxi) UpdateOXIGraphs(oxi);

    wxString html=wxT("<html><body leftmargin=0 rightmargin=0 topmargin=0 marginwidth=0 marginheight=0>");
    html=html+wxT("<table cellspacing=0 cellpadding=2 border=0 width='100%'>\n");

    CPAPMode mode=MODE_UNKNOWN;
    PRTypes pr;
    wxString epr,modestr;
    float iap90,eap90;

    if (cpap) {
        mode=(CPAPMode)cpap->summary_max(CPAP_Mode);

        EventTree->DeleteAllItems();
        wxTreeItemId root=EventTree->AddRoot(wxT("Events"));
        map<MachineCode,wxTreeItemId> mcroot;

        for (vector<Session *>::iterator s=cpap->begin();s!=cpap->end();s++) {

            map<MachineCode,vector<Event *> >::iterator m;

            wxTreeItemId ti,sroot;

            for (m=(*s)->events.begin();m!=(*s)->events.end();m++) {
                MachineCode code=m->first;
                if (code==CPAP_Leak) continue;
                if (code==CPAP_SnoreGraph) continue;
                if (code==PRS1_Unknown12) continue;
                wxTreeItemId mcr;
                if (mcroot.find(code)==mcroot.end()) {
                    wxString s=DefaultMCLongNames[m->first];
                    if (s.IsEmpty())  {
                        s=wxString::Format(wxT("Fixme: %i"),code);
                    }

                    mcr=mcroot[code]=EventTree->AppendItem(root,s);
                } else {
                    mcr=mcroot[code];
                }
                for (vector<Event *>::iterator e=(*s)->events[code].begin();e!=(*s)->events[code].end();e++) {
                    wxDateTime t=(*e)->time();
                    if (code==CPAP_CSR) {
                        t-=wxTimeSpan::Seconds((*(*e))[0]/2);
                    }
                    EventTree->AppendItem(mcr,t.Format(wxT("%Y-%m-%d %H:%M:%S")),-1,-1);
                }
            }
        }
        EventTree->SortChildren(root);
        EventTree->Expand(root);

        //Logo.LoadFile(wxT("./pic.png"));
        if (ahi_bmp) {
            wxMemoryFSHandler::RemoveFile(_T("ahi.png"));
            delete ahi_bmp;
        }
        if (tap_bmp) {
            wxMemoryFSHandler::RemoveFile(_T("tap.png"));
            delete tap_bmp;
        }
        if (tiap_bmp) {
            wxMemoryFSHandler::RemoveFile(_T("tiap.png"));
            delete tiap_bmp;
            tiap_bmp=NULL;
        }
        if (teap_bmp) {
            wxMemoryFSHandler::RemoveFile(_T("teap.png"));
            delete teap_bmp;
            teap_bmp=NULL;
        }
        wxRect r=HTMLInfo->GetRect();
        int w=r.width-27;
        ahi_bmp=G_AHI->RenderBitmap(w,25);
        tap_bmp=TAP->RenderBitmap(w,25);
        if (ahi_bmp)
            wxMemoryFSHandler::AddFile(_T("ahi.png"), *ahi_bmp, wxBITMAP_TYPE_PNG);
        if (tap_bmp)
            wxMemoryFSHandler::AddFile(_T("tap.png"), *tap_bmp, wxBITMAP_TYPE_PNG);

        if (mode==MODE_BIPAP) {
            teap_bmp=TAP_EAP->RenderBitmap(w,25);
            tiap_bmp=TAP_IAP->RenderBitmap(w,25);
            wxMemoryFSHandler::AddFile(_T("teap.png"), *teap_bmp, wxBITMAP_TYPE_PNG);
            wxMemoryFSHandler::AddFile(_T("tiap.png"), *tiap_bmp, wxBITMAP_TYPE_PNG);
        }

        pr=(PRTypes)cpap->summary_max(CPAP_PressureReliefType);
        if (pr==PR_NONE)
            epr=_(" No Pressure Relief");
        else epr=PressureReliefNames[pr]+wxString::Format(wxT(" x%i"),(int)cpap->summary_max(CPAP_PressureReliefSetting));
        modestr=CPAPModeNames[mode];

        float ahi=(cpap->count(CPAP_Obstructive)+cpap->count(CPAP_Hypopnea)+cpap->count(CPAP_ClearAirway))/cpap->hours();
        float csr=(100.0/cpap->hours())*(cpap->sum(CPAP_CSR)/3600.0);
        float oai=cpap->count(CPAP_Obstructive)/cpap->hours();
        float hi=cpap->count(CPAP_Hypopnea)/cpap->hours();
        float cai=cpap->count(CPAP_ClearAirway)/cpap->hours();
        float rei=cpap->count(CPAP_RERA)/cpap->hours();
        float vsi=cpap->count(CPAP_VSnore)/cpap->hours();
        float fli=cpap->count(CPAP_FlowLimit)/cpap->hours();
//        float p90=cpap->percentile(CPAP_Pressure,0,0.9);
        eap90=cpap->percentile(CPAP_EAP,0,0.9);
        iap90=cpap->percentile(CPAP_IAP,0,0.9);
        wxString submodel=_("Unknown Model");


        html=html+wxT("<tr><td colspan=4 align=center><i>")+_("Machine Information")+wxT("</i></td></tr>\n");
        if (cpap->machine->properties.find(wxT("SubModel"))!=cpap->machine->properties.end())
            submodel=wxT(" <br>")+cpap->machine->properties[wxT("SubModel")];
        html=html+wxT("<tr><td colspan=4 align=center><b>")+cpap->machine->properties[wxT("Brand")]+wxT("</b> <br>")+cpap->machine->properties[wxT("Model")]+wxT(" ")+cpap->machine->properties[wxT("ModelNumber")]+submodel+wxT("</td></tr>\n");
        if (pref.Exists("ShowSerialNumbers") && pref["ShowSerialNumbers"]) {
            html=html+wxT("<tr><td colspan=4 align=center>")+cpap->machine->properties[wxT("Serial")]+wxT("</td></tr>\n");
        }
        //html=html+wxT("<tr><td colspan=4><hr></td></tr>\n");
        //html=html+wxT("<tr><td colspan=4 align=center><hr></td></tr>\n");
        //html=html+wxT("<tr><td colspan=4 align=center><i>")+_("Sleep Times")+wxT("</i></td></tr>\n");
        html=html+wxT("<tr><td align='center'><b>Date</b></td><td align='center'><b>Sleep</b></td><td align='center'><b>Wake</b></td><td align='center'><b>Hours</b></td></tr>");
        //html=html+wxT("<tr><td><b>")+_("Date")+wxT("</b></td><td>")+cpap->first().Format(wxT("%x"))+wxT("</td></tr>\n");
        html=html+wxT("<tr><td align='center'>")+cpap->first().Format(wxT("%x"))+wxT("</td><td align='center'>")+cpap->first().Format(wxT("%H:%M"))+wxT("</td><td align='center'>")+cpap->last().Format(wxT("%H:%M"))+wxT("</td><td align='center'>")+cpap->total_time().Format(wxT("%H:%M"))+wxT("</td></tr>\n");

//        html=html+wxT("<tr><td><b>")+_("Sleep")+wxT("</b></td><td>")+cpap->first().Format(wxT("%H:%M"))+wxT("</td></tr>\n");
        //html=html+wxT("<tr><td><b>")+_("Wake")+wxT("</b></td><td>")+cpap->last().Format(wxT("%H:%M"))+wxT("</td></tr>\n");
//        html=html+wxT("<tr><td><b>")+_("Total Time")+wxT("</b></td><td colspan=3><i>")+cpap->total_time().Format(wxT("%H:%M&nbsp;hours"))+wxT("</i></td></tr>\n");
       // html=html+wxT("<tr><td colspan=4>&nbsp;</td></tr>\n");

        html=html+wxT("<tr><td colspan=4 align=center><hr></td></tr>\n");
        //pref["fruitsalad"]=false;
        if (pref.Exists("fruitsalad") && pref["fruitsalad"]) {
            html=html+wxT("<tr><td colspan=2><table cellspacing=0 cellpadding=2 border=0 width='100%'>");
            html=html+wxT("<tr><td align='right' bgcolor='#F88017'><b><font color='black'>")+_("AHI")+wxT("</font></b></td><td  bgcolor='#F88017'><b><font color='black'>")+wxString::Format(wxT("%0.2f"),ahi)+wxT("</font></b></td></tr>\n");
            html=html+wxT("<tr><td align='right' bgcolor='#4040ff'><b><font color='white'>")+_("Hypopnea")+wxT("</font></b></td><td bgcolor='#4040ff'><font color='white'>")+wxString::Format(wxT("%0.2f"),hi)+wxT("</font></td></tr>\n");
            html=html+wxT("<tr><td align='right' bgcolor='#40afbf'><b>")+_("Obstructive")+wxT("</b></td><td bgcolor='#40afbf'>")+wxString::Format(wxT("%0.2f"),oai)+wxT("</td></tr>\n");
            html=html+wxT("<tr><td align='right' bgcolor='#ff80ff'><b>")+_("ClearAirway")+wxT("</b></td><td bgcolor='#ff80ff'>")+wxString::Format(wxT("%0.2f"),cai)+wxT("</td></tr>\n");
            html=html+wxT("</table></td><td colspan=2><table cellspacing=0 cellpadding=2 border=0 width='100%'>");
            html=html+wxT("<tr><td align='right' bgcolor='#ffff80'><b>")+_("RERA")+wxT("</b></td><td bgcolor='#ffff80'>")+wxString::Format(wxT("%0.2f"),rei)+wxT("</td></tr>\n");
            html=html+wxT("<tr><td align='right' bgcolor='#404040'><b><font color='white'>")+_("FlowLimit")+wxT("</font></b></td><td bgcolor='#404040'><font color='white'>")+wxString::Format(wxT("%0.2f"),fli)+wxT("</font></td></tr>\n");
            html=html+wxT("<tr><td align='right' bgcolor='#ff4040'><b>")+_("Vsnore")+wxT("</b></td><td bgcolor='#ff4040'>")+wxString::Format(wxT("%0.2f"),vsi)+wxT("</td></tr>\n");
            html=html+wxT("<tr><td align='right' bgcolor='#80ff80'><b>")+_("PB/CSR")+wxT("</b></td><td bgcolor='#80ff80'>")+wxString::Format(wxT("%0.2f%%"),csr)+wxT("</td></tr>\n");
            html=html+wxT("</table></td></tr>");
        } else {
            html=html+wxT("<tr><td colspan=2><table cellspacing=0 cellpadding=2 border=0 width='100%'>");
            html=html+wxT("<tr><td align='right'><b><font color='black'>")+_("AHI")+wxT("</font></b></td><td><b><font color='black'>")+wxString::Format(wxT("%0.2f"),ahi)+wxT("</font></b></td></tr>\n");
            html=html+wxT("<tr><td align='right'><b>")+_("Hypopnea")+wxT("</b></td><td>")+wxString::Format(wxT("%0.2f"),hi)+wxT("</td></tr>\n");
            html=html+wxT("<tr><td align='right'><b>")+_("Obstructive")+wxT("</b></td><td>")+wxString::Format(wxT("%0.2f"),oai)+wxT("</td></tr>\n");
            html=html+wxT("<tr><td align='right'><b>")+_("ClearAirway")+wxT("</b></td><td>")+wxString::Format(wxT("%0.2f"),cai)+wxT("</td></tr>\n");
            html=html+wxT("</table></td><td colspan=2><table cellspacing=0 cellpadding=2 border=0 width='100%'>");
            html=html+wxT("<tr><td align='right'><b>")+_("RERA")+wxT("</b></td><td>")+wxString::Format(wxT("%0.2f"),rei)+wxT("</td></tr>\n");
            html=html+wxT("<tr><td align='right'><b>")+_("FlowLimit")+wxT("</b></td><td>")+wxString::Format(wxT("%0.2f"),fli)+wxT("</td></tr>\n");
            html=html+wxT("<tr><td align='right'><b>")+_("Vsnore")+wxT("</b></td><td>")+wxString::Format(wxT("%0.2f"),vsi)+wxT("</td></tr>\n");
            html=html+wxT("<tr><td align='right'><b>")+_("PB/CSR")+wxT("</b></td><td>")+wxString::Format(wxT("%0.2f%%"),csr)+wxT("</td></tr>\n");
            html=html+wxT("</table></td></tr>");
        }

       // html=html+wxT("<tr><td colspan=4>&nbsp;</td></tr>\n");
        //html=html+wxT("<tr><td colspan=4>&nbsp;</td></tr>\n");
        html=html+wxT("<tr><td colspan=4 align=center><i>")+_("Event Breakdown")+wxT("</i></td></tr>\n");
        html=html+wxT("<tr><td colspan=4 align=left><img src=\"memory:ahi.png\"></td></tr>\n");
        //html=html+wxT("<tr><td colspan=4>&nbsp;</td></tr>\n");

        //html=html+wxT("<tr><td colspan=4 align=center><i>")+_("Other Information")+wxT("</i></td></tr>\n");

        html=html+wxT("</table>");

        html=html+wxT("<table cellspacing=0 cellpadding=0 border=0 width='100%'>\n");
        //html=html+wxT("<tr><td colspan=4>&nbsp;</td></tr>\n");
        html=html+wxT("<tr height='2'><td colspan=4 height='2'><hr></td></tr>\n");
        //html=html+wxT("<tr><td colspan=4 align=center><hr></td></tr>\n");


        if (mode==MODE_BIPAP) {
            html=html+wxT("<tr><td colspan=4 align='center'><i>")+_("90%&nbsp;EPAP ")+wxString::Format(wxT("%.1fcmH2O"),eap90)+wxT("</td></tr>\n");
            html=html+wxT("<tr><td colspan=4 align='center'><i>")+_("90%&nbsp;IPAP ")+wxString::Format(wxT("%.1fcmH2O"),iap90)+wxT("</td></tr>\n");
        } else if (mode==MODE_APAP) {
            html=html+wxT("<tr><td colspan=4 align='center'><i>")+_("90%&nbsp;Pressure ")+wxString::Format(wxT("%.2fcmH2O"),cpap->summary_weighted_avg(CPAP_PressurePercentValue))+wxT("</i></td></tr>\n");
        } else if (mode==MODE_CPAP) {
            html=html+wxT("<tr><td colspan=4 align='center'><i>")+_("Pressure ")+wxString::Format(wxT("%.2fcmH2O"),cpap->summary_min(CPAP_PressureMin))+wxT("</i></td></tr>\n");
        }
        html=html+wxT("<tr><td colspan=4 align=center>&nbsp;</td></tr>\n");

        html=html+wxT("<tr><td> </td><td><b>Min</b></td><td><b>Avg</b></td><td><b>Max</b></td></tr>");

        if (mode==MODE_APAP) {
            html=html+wxT("<tr><td>Pressure</td><td>")+wxString::Format(wxT("%.2f"),cpap->summary_min(CPAP_PressureMinAchieved));
            html=html+wxT("</td><td>")+wxString::Format(wxT("%.2f"),cpap->summary_weighted_avg(CPAP_PressureAverage));
            html=html+wxT("</td><td>")+wxString::Format(wxT("%.2f"),cpap->summary_max(CPAP_PressureMaxAchieved))+wxT("</td></tr>");

          //  html=html+wxT("<tr><td><b>")+_("90%&nbsp;Pressure")+wxT("</b></td><td>")+wxString::Format(wxT("%.1fcmH2O"),p90)+wxT("</td></tr>\n");
        } else if (mode==MODE_BIPAP) {
            html=html+wxT("<tr><td>EPAP</td><td>")+wxString::Format(wxT("%.2f"),cpap->summary_min(BIPAP_EAPMin));
            html=html+wxT("</td><td>")+wxString::Format(wxT("%.2f"),cpap->summary_weighted_avg(BIPAP_EAPAverage));
            html=html+wxT("</td><td>")+wxString::Format(wxT("%.2f"),cpap->summary_max(BIPAP_EAPMax))+wxT("</td></tr>");

            html=html+wxT("<tr><td>IPAP</td><td>")+wxString::Format(wxT("%.2f"),cpap->summary_min(BIPAP_IAPMin));
            html=html+wxT("</td><td>")+wxString::Format(wxT("%.2f"),cpap->summary_weighted_avg(BIPAP_IAPAverage));
            html=html+wxT("</td><td>")+wxString::Format(wxT("%.2f"),cpap->summary_max(BIPAP_IAPMax))+wxT("</td></tr>");

        }
        html=html+wxT("<tr><td>Leak"); //</td><td>")+wxString::Format(wxT("%.2f"),cpap->summary_weighted_avg(CPAP_LeakAverage))
        html=html+wxT("</td><td>")+wxString::Format(wxT("%.2f"),cpap->summary_min(CPAP_LeakMinimum));
        html=html+wxT("</td><td>")+wxString::Format(wxT("%.2f"),cpap->summary_weighted_avg(CPAP_LeakAverage));
        html=html+wxT("</td><td>")+wxString::Format(wxT("%.2f"),cpap->summary_max(CPAP_LeakMaximum))+wxT("</td><tr>");

        html=html+wxT("<tr><td>Snore"); //</td><td>")+wxString::Format(wxT("%.2f"),cpap->summary_weighted_avg(CPAP_LeakAverage))
        html=html+wxT("</td><td>")+wxString::Format(wxT("%.2f"),cpap->summary_min(CPAP_SnoreMinimum));
        html=html+wxT("</td><td>")+wxString::Format(wxT("%.2f"),cpap->summary_avg(CPAP_SnoreAverage));
        html=html+wxT("</td><td>")+wxString::Format(wxT("%.2f"),cpap->summary_max(CPAP_SnoreMaximum))+wxT("</td><tr>");

       // html=html+wxT("<tr><td colspan=4>&nbsp;</td></tr>\n");

        /*if (mode!=MODE_BIPAP) {
            TAP_EAP->Show(false);
            TAP_IAP->Show(false);
            TAP->Show(true);
        } else {
            TAP->Show(false);
            TAP_IAP->Show(true);
            TAP_EAP->Show(true);
        } */
        //G_AHI->Show(true);
        FRW->Show(true);
        PRD->Show(true);
        LEAK->Show(true);
        SF->Show(true);
        SNORE->Show(true);
    } else {
        html+=_("<tr><td colspan=2 align=center><i>No CPAP data available</i></td></tr>");
        html=html+wxT("<tr><td>&nbsp;</td><td>&nbsp;</td></tr>\n");

        //TAP_EAP->Show(false);
        //TAP_IAP->Show(false);
        //G_AHI->Show(false);
        FRW->Show(false);
        PRD->Show(false);
        LEAK->Show(false);
        SF->Show(false);
        SNORE->Show(false);
    }
    if (oxi) {
        html=html+wxT("<tr><td>Pulse");
        html=html+wxT("</td><td>")+wxString::Format(wxT("%.2fbpm"),oxi->summary_min(OXI_PulseMin));
        html=html+wxT("</td><td>")+wxString::Format(wxT("%.2fbpm"),oxi->summary_avg(OXI_PulseAverage));
        html=html+wxT("</td><td>")+wxString::Format(wxT("%.2fbpm"),oxi->summary_max(OXI_PulseMax))+wxT("</td><tr>");

        html=html+wxT("<tr><td>SpO2");
        html=html+wxT("</td><td>")+wxString::Format(wxT("%.2f%%"),oxi->summary_min(OXI_SPO2Min));
        html=html+wxT("</td><td>")+wxString::Format(wxT("%.2f%%"),oxi->summary_avg(OXI_SPO2Average));
        html=html+wxT("</td><td>")+wxString::Format(wxT("%.2f%%"),oxi->summary_max(OXI_SPO2Max))+wxT("</td><tr>");

        //html=html+wxT("<tr><td colspan=4>&nbsp;</td></tr>\n");

        PULSE->Show(true);
        SPO2->Show(true);
    } else {
        PULSE->Show(false);
        SPO2->Show(false);
    }
    gwSizer->Layout();
    GraphWindow->FitInside();

    if (cpap) {


        if (mode==MODE_BIPAP) {
            html=html+wxT("<tr><td colspan=4 align=center><i>")+_("Time@EPAP")+wxT("</i></td></tr>\n");
            html=html+wxT("<tr><td colspan=4 align=center><img src=\"memory:teap.png\" ></td></tr>\n");
            html=html+wxT("<tr><td colspan=4 align=center><i>")+_("Time@IPAP")+wxT("</i></td></tr>\n");
            html=html+wxT("<tr><td colspan=4 align=center><img src=\"memory:tiap.png\"  ></td></tr>\n");

        } else if (mode==MODE_APAP) {
            html=html+wxT("<tr><td colspan=4 align=center><i>")+_("Time@Pressure")+wxT("</i></td></tr>\n");
            html=html+wxT("<tr><td colspan=4 align=center><img src=\"memory:tap.png\"></td></tr>\n");
        }

        html=html+wxT("</table><hr>");
        html=html+wxT("<div align=left>");
        html=html+wxT("<table cellspacing=0 cellpadding=0 border=0 width='100%'>\n");
//        fgSizer->Layout();

        if (cpap->summary_avg(CPAP_BrokenSummary)==1) {
            html=html+wxT("<tr><td colspan=2 align=center><i>")+_("No System Settings Recorded")+wxT("</i></td></tr>\n");
        } else {
            //html=html+wxT("<tr><td colspan=2 align=center><i>")+_("System Settings")+wxT("</i></td></tr>\n");
            html=html+wxT("<tr><td><b>Mode</b></td><td><b>")+modestr+wxT("</b> with ")+epr+wxT("</td></tr>\n");

            if (mode==MODE_CPAP) {
                html=html+wxT("<tr><td><b>")+_("Pressure")+wxT("</b></td><td>")+wxString::Format(wxT("%.1fcmH2O"),cpap->summary_min(CPAP_PressureMin))+wxT("</td></tr>\n");
            } else if (mode==MODE_APAP) {
                html=html+wxT("<tr><td><b>")+_("Pressure")+wxT("</b></td><td>")+wxString::Format(wxT("%.2fcmH20 Min<br>"),cpap->summary_min(CPAP_PressureMin))+wxString::Format(wxT("%.2fcmH2O Max"),cpap->summary_max(CPAP_PressureMax))+wxT("</td></tr>\n");
            } else if (mode==MODE_BIPAP) {
                html=html+wxT("<tr><td><b>")+_("Pressure")+wxT("</b></td><td>")+wxString::Format(wxT("%.2fcmH2O EPAP<br>"),cpap->summary_min(CPAP_PressureMin))+wxString::Format(wxT("%.2fcmH2O IPAP"),cpap->summary_max(CPAP_PressureMax))+wxT("</td></tr>\n");
            }
            html=html+wxT("<tr><td><b>")+_("Ramp")+wxT("</b></td><td>")+wxString::Format(wxT("%.2fcmH2O"),cpap->summary_min(CPAP_RampStartingPressure))+wxString::Format(wxT(" @ %imin"),(int)cpap->summary_max(CPAP_RampTime))+wxT("</td></tr>\n");

            html=html+wxT("<tr><td colspan=4>&nbsp;</td></tr>\n");
            // check HumidiferStatus..
            wxString str;
            if (bool(cpap->summary_max(CPAP_HumidifierStatus))) {
                str=wxString::Format(wxT("x%i"),(int)cpap->summary_max(CPAP_HumidifierSetting));
            } else str=wxT("No");
            html=html+wxT("<tr><td><b>")+_("Humidifier")+wxT("</b></td><td>")+str+wxT("</td></tr>\n");
            html=html+wxT("<tr><td><b>")+_("SystemLock")+wxT("</b></td><td>")+(bool(cpap->summary_max(PRS1_SystemLockStatus)) ? _("On") : _("Off"))+wxT("</td></tr>\n");
            html=html+wxT("<tr><td><b>")+_("Auto-Off")+wxT("</b></td><td>")+(bool(cpap->summary_max(PRS1_AutoOff)) ? _("On") : _("Off"))+wxT("</td></tr>\n");
            html=html+wxT("<tr><td><b>")+_("Mask-Alert")+wxT("</b></td><td>")+(bool(cpap->summary_max(PRS1_MaskAlert)) ? _("On") : _("Off"))+wxT("</td></tr>\n");
            html=html+wxT("<tr><td><b>")+_("Show-AHI")+wxT("</b></td><td>")+(bool(cpap->summary_max(PRS1_ShowAHI)) ? _("On") : _("Off"))+wxT("</td></tr>\n");
            html=html+wxT("<tr><td><b>")+_("Hose-Size")+wxT("</b></td><td>")+(bool(cpap->summary_max(PRS1_HoseDiameter)) ? _("22mm") : _("15mm"))+wxT("</td></tr>\n");
            if (bool(cpap->summary_max(PRS1_SystemResistanceStatus))) {
                str=wxString::Format(wxT("x%i"),(int)cpap->summary_max(PRS1_SystemResistanceSetting));
            } else str=wxT("No");
            html=html+wxT("<tr><td><b>")+_("Sys-Resist.")+wxT("</b></td><td>")+str+wxT("</td></tr>\n");
        }
        html=html+wxT("</table><hr><div align=center>");
        html=html+wxT("<table cellspacing=0 cellpadding=0 border=0>\n");
            //html=html+wxT("<tr><td>&nbsp;</td><td>&nbsp;</td></tr>\n");
        //html=html+wxT("<tr><td colspan=2 align=center><hr></td></tr>\n");

        html=html+wxT("<tr><td colspan=2 align=center><i>")+_("Session Files")+wxT("</i></td></tr>\n");

        for (vector<Session *>::iterator i=cpap->begin();i!=cpap->end();i++) {
            html=html+wxT("<tr><td colspan=2 align=left>")+(*i)->first().Format(wxT("%d-%m-%Y&nbsp;%H:%M"))+wxT("&nbsp;");
            html=html+(*i)->last().Format(wxT("%H:%M"))+wxT("&nbsp;");
            html=html+wxString::Format(wxT("#%06li"),(*i)->session())+wxT("</td></tr>\n");
        }

    }
    /*if (!cpap && !oxi) {
        html+=_("<tr><td colspan=2><i>No data available for this day</i></td></tr>");
    } */
    html+=wxT("</table></div></body></html>");
    HTMLInfo->SetPage(html);
    Refresh();
    event.Skip();
}
void Daily::OnSelectSession( wxCommandEvent& event )
{
    if (event.IsSelection()) {
        int sessid=event.GetSelection();

        wxLogMessage(wxT("Selected:")+wxString::Format(wxT("%i"),sessid));
    }
}

///usr/local/bin/upx ./bin/Windows/SleepyHead

void Daily::OnCalendarDay( wxCalendarEvent& event )
{
    if (foobar_datehack) {
        OnCalendarMonth(event);
        foobar_datehack=false;
    }
    RefreshData();
}
void Daily::UpdateCPAPGraphs(Day *day)
{
    //if (!day) return;
    if (day) {
        day->OpenEvents();
        day->OpenWaveforms();
    }
    for (list<gPointData *>::iterator g=CPAPData.begin();g!=CPAPData.end();g++) {
        (*g)->Update(day);
    }
};

void Daily::UpdateOXIGraphs(Day *day)
{
    //if (!day) return;
    if (day) {
        day->OpenEvents();
        day->OpenWaveforms();
    }
    for (list<gPointData *>::iterator g=OXIData.begin();g!=OXIData.end();g++) {
        (*g)->Update(day);
    }
};


void Daily::OnCalendarMonth( wxCalendarEvent& event )
{
    wxDateTime et=event.GetDate();
    if (!et.IsValid()) {
        foobar_datehack=true;
        return;
    }

	wxDateTime::Month m=et.GetMonth();
	int y=et.GetYear();

    static wxFont f=*wxNORMAL_FONT; //wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
    f.SetWeight(wxBOLD);

	//if (!machine) return;
	for (int i=1;i<31;i++) {
	    int j=wxDateTime::GetNumberOfDays(m,y);
		if (i>j) break;
		wxDateTime d(i,m,y,0,0,0,0);
		d-=wxTimeSpan::Days(1);

		if ((profile->daylist.find(d)!=profile->daylist.end())) {
#if wxCHECK_VERSION(2,9,0)
			Calendar->Mark(i,true);
#else
            wxCalendarDateAttr *a=new wxCalendarDateAttr();
            a->SetFont(f);
            //wxNORM
			Calendar->SetAttr(i,a);
#endif
		} else {
#if wxCHECK_VERSION(2,9,0)
			Calendar->Mark(i,false);
#else
		    Calendar->ResetAttr(i);
#endif
//			Calendar->SetAttr(i,NULL);
		}
	}
}


