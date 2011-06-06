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

#include "SleepyHeadMain.h"
#include "sleeplib/profiles.h"
#include "sleeplib/machine_loader.h"

#if defined(__WXMSW__)
extern "C" void *_GdipStringFormatCachedGenericTypographic = NULL;
#endif
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


SleepyHeadFrame::SleepyHeadFrame(wxFrame *frame)
    : GUIFrame(frame)
{
    wxString title=wxTheApp->GetAppName()+wxT(" v")+wxString(AutoVersion::FULLVERSION_STRING,wxConvUTF8);
    SetTitle(title);

    profile=Profiles::Get();
    if (!profile) {
        wxLogError(wxT("Couldn't get active profile"));
        abort();
    }
    UpdateProfiles();

    if (pref.Exists("ShowSerialNumbers")) ViewMenuSerial->Check(pref["ShowSerialNumbers"]);


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

#if defined(__UNIX__) // Borrowed.. this need fixing.
    int cx=r.x, cy=r.y;
    ClientToScreen(&cx,&cy);
    int border_width = cx - r.x;
    int title_bar_height = cy - r.y;
    r.width += (border_width * 2);
    r.height += title_bar_height; // + border_width;
#endif

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
}
void SleepyHeadFrame::OnShowSerial(wxCommandEvent& event)
{
    pref["ShowSerialNumbers"]=event.IsChecked();
}

void SleepyHeadFrame::OnAbout(wxCommandEvent &event)
{
    wxString msg = wxbuildinfo(long_f);
    msg=wxTheApp->GetAppName()+wxT(" v")+wxString(AutoVersion::FULLVERSION_STRING,wxConvUTF8)+wxT("\nAuthors: Mark Watkins / Troy Schultz\nThis is alpha software is guaranteed to break regularly!\nUse at your own risk.\n\nLicense: GPL"); //,AutoVersion::DATE,AutoVersion::MONTH,AutoVersion::YEAR

    wxMessageBox(msg, _("Welcome to..."),0,this);
}
void SleepyHeadFrame::OnImportSD(wxCommandEvent &event)
{
    wxDirDialog dd(this,_("Choose a Directory")); //,wxT(""),wxT(""),style=wxFD_OPEN);
    if (dd.ShowModal()!=wxID_OK) return;


    loader_progress=new wxProgressDialog(wxT("SleepyHead"),wxT("Please Wait..."),100,this, wxPD_APP_MODAL|wxPD_AUTO_HIDE|wxPD_SMOOTH);
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
        daily->RefreshData();
        daily->Refresh();
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
        main_auinotebook->AddPage(daily,_("Daily"),true);
        daily->RefreshData();
        daily->Refresh();

    } else {
        main_auinotebook->SetSelection(idx);
    }


}
void SleepyHeadFrame::OnViewMenuSummary( wxCommandEvent& event )
{

    int idx=main_auinotebook->GetPageIndex(summary);
    if (idx==wxNOT_FOUND) {
        summary=new Summary(this,profile);
        main_auinotebook->AddPage(summary,_("Summary"),true);
        summary->ResetProfile(profile);
        summary->RefreshData();
        summary->Refresh();
    } else {
        main_auinotebook->SetSelection(idx);
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
    AHI->AddLayer(new gBarChart(ahidata,wxRED));
   // AHI->AddLayer(new gXAxis(NULL,wxBLACK));
    //AHI->AddLayer(new gLineChart(ahidata,wxRED));
    fgSizer->Add(AHI,1,wxEXPAND);

    PRESSURE=new gGraphWindow(ScrolledWindow,-1,wxT("Pressure"),wxPoint(0,0), wxSize(400,180), wxNO_BORDER);
    PRESSURE->SetMargins(10,15,65,80);
    PRESSURE->AddLayer(new gYAxis(wxBLACK));
    PRESSURE->AddLayer(new gXAxis(wxBLACK));
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
    LEAK->AddLayer(new gLineChart(leak,wxPURPLE,6192,false,false,true));
    fgSizer->Add(LEAK,1,wxEXPAND);


    USAGE=new gGraphWindow(ScrolledWindow,-1,wxT("Usage (Hours)"),wxPoint(0,0), wxSize(400,180), wxNO_BORDER);
    USAGE->SetMargins(10,15,65,80);
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

    HTMLInfo=new wxHtmlWindow(this);
    EventTree=new wxTreeCtrl(this);

    this->Connect(wxEVT_COMMAND_TREE_SEL_CHANGED, wxTreeEventHandler( Daily::OnEventTreeSelection), NULL, this);
    Notebook->AddPage(HTMLInfo,wxT("Details"),false,NULL);
    Notebook->AddPage(EventTree,wxT("Events"),false,NULL);
    AddCPAPData(tap_eap=new TAPData(CPAP_EAP));
    AddCPAPData(tap_iap=new TAPData(CPAP_IAP));
    AddCPAPData(tap=new TAPData(CPAP_Pressure));

    TAP=new gGraphWindow(ScrolledWindow,-1,wxT("Time@Pressure"),wxPoint(0,0), wxSize(600,60), wxNO_BORDER);
    TAP->SetMargins(20,15,5,50);
    TAP->AddLayer(new gCandleStick(tap));

    TAP_IAP=new gGraphWindow(ScrolledWindow,-1,wxT("Time@IPAP"),wxPoint(0,0), wxSize(600,60), wxNO_BORDER);
    TAP_IAP->SetMargins(20,15,5,50);
    TAP_IAP->AddLayer(new gCandleStick(tap_iap));

    TAP_EAP=new gGraphWindow(ScrolledWindow,-1,wxT("Time@EPAP"),wxPoint(0,0), wxSize(600,60), wxNO_BORDER);
    TAP_EAP->SetMargins(20,15,5,50);
    TAP_EAP->AddLayer(new gCandleStick(tap_eap));

    G_AHI=new gGraphWindow(ScrolledWindow,-1,wxT("Event Breakdown"),wxPoint(0,0), wxSize(600,60), wxNO_BORDER);
    G_AHI->SetMargins(20,15,5,50);
    AddCPAPData(g_ahi=new AHIData());
    gCandleStick *l=new gCandleStick(g_ahi);
    l->AddName(wxT("H"));
    l->AddName(wxT("OA"));
    l->AddName(wxT("CA"));
    l->AddName(wxT("RE"));
    l->AddName(wxT("FL"));
    l->AddName(wxT("CSR"));
    l->color.clear();
    l->color.push_back(wxBLUE);
    l->color.push_back(wxAQUA);
    l->color.push_back(wxPURPLE);
    l->color.push_back(wxYELLOW);
    l->color.push_back(wxBLACK);
    l->color.push_back(wxGREEN2);
    G_AHI->AddLayer(l);

    AddOXIData(pulse=new SkipZeroData(OXI_Pulse,0,32768));
    //pulse->ForceMinY(40);
    //pulse->ForceMaxY(120);

    PULSE=new gGraphWindow(ScrolledWindow,-1,wxT("Pulse"),wxPoint(0,0), wxSize(600,130), wxNO_BORDER);
    PULSE->AddLayer(new gLineChart(pulse,wxRED,32768,false,false,true));
    PULSE->AddLayer(new gXAxis(wxBLACK));

    AddOXIData(spo2=new SkipZeroData(OXI_SPO2,0,32768));
    //spo2->ForceMinY(60);
    //spo2->ForceMaxY(100);
    SPO2=new gGraphWindow(ScrolledWindow,-1,wxT("SpO2"),wxPoint(0,0), wxSize(600,130), wxNO_BORDER);
    SPO2->AddLayer(new gLineChart(spo2,wxBLUE,32768,false,false,true));
    SPO2->AddLayer(new gXAxis(wxBLACK));
    SPO2->LinkZoom(PULSE);
    PULSE->LinkZoom(SPO2);


    AddCPAPData(leakdata=new PressureData(CPAP_Leak,0));
    //leakdata->ForceMinY(0);
    //leakdata->ForceMaxY(120);
    LEAK=new gGraphWindow(ScrolledWindow,-1,wxT("Mask Leaks"),wxPoint(0,0), wxSize(600,130), wxNO_BORDER);
    LEAK->AddLayer(new gLineChart(leakdata,wxPURPLE,4096,false,false,true));
    LEAK->AddLayer(new gXAxis(wxBLACK));

    AddCPAPData(pressure_iap=new PressureData(CPAP_IAP));
    AddCPAPData(pressure_eap=new PressureData(CPAP_EAP));
    AddCPAPData(prd=new PressureData(CPAP_Pressure));
    PRD=new gGraphWindow(ScrolledWindow,-1,wxT("Pressure"),wxPoint(0,0), wxSize(600,130), wxNO_BORDER);
    PRD->AddLayer(new gLineChart(prd,wxDARK_GREEN,4096,false,false,true));
    PRD->AddLayer(new gLineChart(pressure_iap,wxBLUE,4096,false,true,true));
    PRD->AddLayer(new gLineChart(pressure_eap,wxRED,4096,false,true,true));
    PRD->AddLayer(new gXAxis(wxBLACK));

    AddCPAPData(frw=new FlowData());
    FRW=new gGraphWindow(ScrolledWindow,-1,wxT("Flow Rate"),wxPoint(0,0), wxSize(600,150), wxNO_BORDER);

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

    FRW->AddLayer(new gLineOverlayBar(flags[0],wxGREEN2,wxT("CSR")));
    FRW->AddLayer(new gLineChart(frw,wxBLACK,200000,true));
    FRW->AddLayer(new gLineOverlayBar(flags[7],wxRED,wxT("PR"),LOT_Dot));
    FRW->AddLayer(new gLineOverlayBar(flags[6],wxYELLOW,wxT("RE")));
    FRW->AddLayer(new gLineOverlayBar(flags[9],wxDARK_GREEN,wxT("U0E")));
    FRW->AddLayer(new gLineOverlayBar(flags[5],wxRED,wxT("VS")));
    FRW->AddLayer(new gLineOverlayBar(flags[4],wxBLACK,wxT("FL")));
    FRW->AddLayer(new gLineOverlayBar(flags[3],wxBLUE,wxT("H")));
    FRW->AddLayer(new gLineOverlayBar(flags[2],wxAQUA,wxT("OA")));
    FRW->AddLayer(new gLineOverlayBar(flags[1],wxPURPLE,wxT("CA")));
    FRW->AddLayer(new gXAxis(wxBLACK));

    SF=new gGraphWindow(ScrolledWindow,-1,wxT("Event Flags"),wxPoint(0,0), wxSize(600,180), wxNO_BORDER);
  //  SF->SetMargins(10,15,20,80);

    SF->LinkZoom(FRW);
    FRW->LinkZoom(SF);
    #if defined(__UNIX__)
 //   SF->LinkZoom(PRD); // Uncomment to link in more graphs.. Too slow on windows.
 //   SF->LinkZoom(LEAK);
    #endif

    const int sfc=9;

    SF->AddLayer(new gFlagsLine(flags[9],wxDARK_GREEN,wxT("U0E"),8,sfc));
    SF->AddLayer(new gFlagsLine(flags[8],wxRED,wxT("VS2"),6,sfc));
    SF->AddLayer(new gFlagsLine(flags[6],wxYELLOW,wxT("RE"),7,sfc));
    SF->AddLayer(new gFlagsLine(flags[5],wxRED,wxT("VS"),5,sfc));
    SF->AddLayer(new gFlagsLine(flags[4],wxBLACK,wxT("FL"),4,sfc));
    SF->AddLayer(new gFlagsLine(flags[3],wxBLUE,wxT("H"),3,sfc));
    SF->AddLayer(new gFlagsLine(flags[2],wxAQUA,wxT("OA"),2,sfc));
    SF->AddLayer(new gFlagsLine(flags[1],wxPURPLE,wxT("CA"),1,sfc));
    SF->AddLayer(new gFlagsLine(flags[0],wxGREEN2,wxT("CSR"),0,sfc));
    SF->AddLayer(new gXAxis(wxBLACK));
    SF->AddLayer(new gFooBar());

    fgSizer->Add(SF,1,wxEXPAND);
    fgSizer->Add(FRW,1,wxEXPAND);
    fgSizer->Add(PULSE,1,wxEXPAND);
    fgSizer->Add(SPO2,1,wxEXPAND);
    fgSizer->Add(PRD,1,wxEXPAND);
    fgSizer->Add(LEAK,1,wxEXPAND);
    fgSizer->Add(G_AHI,1,wxEXPAND);
    fgSizer->Add(TAP,1,wxEXPAND);
    fgSizer->Add(TAP_IAP,1,wxEXPAND);
    fgSizer->Add(TAP_EAP,1,wxEXPAND);


    ResetDate();
}
Daily::~Daily()
{
    this->Disconnect(wxEVT_COMMAND_TREE_SEL_CHANGED, wxTreeEventHandler( Daily::OnEventTreeSelection), NULL, this);
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
    }
}

void Daily::ResetDate()
{
    foobar_datehack=false; // this exists due to a wxGTK bug.
  //  RefreshData();
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
}
void Daily::RefreshData()
{
    wxDateTime date=Calendar->GetDate();
    date.ResetTime();
    date.SetHour(0);
    date-=wxTimeSpan::Days(1);

    Day *d=NULL;
    Day *oxi=NULL;
    if (profile->daylist.find(date)!=profile->daylist.end()) {
        vector<Day *>::iterator di;
        for (di=profile->daylist[date].begin();di!=profile->daylist[date].end();di++) {
            if (!d && ((*di)->machine_type()==MT_CPAP)) {
                d=(*di);
                UpdateCPAPGraphs(d);
            }
            if ((*di)->machine_type()==MT_OXIMETER) {
                oxi=(*di);
                UpdateOXIGraphs(oxi);
            }

        }
    }

    if (d) {
        CPAPMode mode=(CPAPMode)d->summary_max(CPAP_Mode);
        if (mode!=MODE_BIPAP) {
            TAP_EAP->Show(false);
            TAP_IAP->Show(false);
            TAP->Show(true);
        } else {
            TAP->Show(false);
            TAP_IAP->Show(true);
            TAP_EAP->Show(true);
        }

        EventTree->DeleteAllItems();
        wxTreeItemId root=EventTree->AddRoot(wxT("Events"));
        map<MachineCode,wxTreeItemId> mcroot;

        for (vector<Session *>::iterator s=d->begin();s!=d->end();s++) {

            map<MachineCode,vector<Event *> >::iterator m;

            wxTreeItemId ti,sroot;

            for (m=(*s)->events.begin();m!=(*s)->events.end();m++) {
                MachineCode code=m->first;
                if (code==CPAP_Leak) continue;
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


        PRTypes pr=(PRTypes)d->summary_max(CPAP_PressureReliefType);
        wxString epr=PressureReliefNames[pr]+wxString::Format(wxT(" x%i"),(int)d->summary_max(CPAP_PressureReliefSetting));
        wxString modestr=CPAPModeNames[mode];

        float ahi=(d->count(CPAP_Obstructive)+d->count(CPAP_Hypopnea)+d->count(CPAP_ClearAirway))/d->hours();
        float csr=(100.0/d->hours())*(d->sum(CPAP_CSR)/3600.0);
        float oai=d->count(CPAP_Obstructive)/d->hours();
        float hi=d->count(CPAP_Hypopnea)/d->hours();
        float cai=d->count(CPAP_ClearAirway)/d->hours();
        float rei=d->count(CPAP_RERA)/d->hours();
        float vsi=d->count(CPAP_VSnore)/d->hours();
        float fli=d->count(CPAP_FlowLimit)/d->hours();
//        float p90=d->percentile(CPAP_Pressure,0,0.9);
        float eap90=d->percentile(CPAP_EAP,0,0.9);
        float iap90=d->percentile(CPAP_IAP,0,0.9);
        wxString submodel=_("Unknown Model");

        wxString html=wxT("<html><body leftmargin=0 rightmargin=0 topmargin=0 marginwidth=0 marginheight=0><table cellspacing=2 cellpadding=0>\n");
        html=html+wxT("<tr><td colspan=2 align=center><i>")+_("Machine Information")+wxT("</i></td></tr>\n");
        if (d->machine->properties.find(wxT("SubModel"))!=d->machine->properties.end())
            submodel=wxT(" <br>")+d->machine->properties[wxT("SubModel")];
        html=html+wxT("<tr><td colspan=2 align=center><b>")+d->machine->properties[wxT("Brand")]+wxT("</b> <br>")+d->machine->properties[wxT("Model")]+wxT(" ")+d->machine->properties[wxT("ModelNumber")]+submodel+wxT("</td></tr>\n");
        if (pref.Exists("ShowSerialNumbers") && pref["ShowSerialNumbers"]) {
            html=html+wxT("<tr><td colspan=2 align=center>")+d->machine->properties[wxT("Serial")]+wxT("</td></tr>\n");
        }
        html=html+wxT("<tr><td>&nbsp;</td><td>&nbsp;</td></tr>\n");
        html=html+wxT("<tr><td colspan=2 align=center><i>")+_("Sleep Times")+wxT("</i></td></tr>\n");
        html=html+wxT("<tr><td><b>")+_("Date")+wxT("</b></td><td>")+d->first().Format(wxT("%x"))+wxT("</td></tr>\n");
        html=html+wxT("<tr><td><b>")+_("Sleep")+wxT("</b></td><td>")+d->first().Format(wxT("%H:%M"))+wxT("</td></tr>\n");
        html=html+wxT("<tr><td><b>")+_("Wake")+wxT("</b></td><td>")+d->last().Format(wxT("%H:%M"))+wxT("</td></tr>\n");
        html=html+wxT("<tr><td><b>")+_("Total Time")+wxT("</b></td><td><i>")+d->total_time().Format(wxT("%H:%M&nbsp;hours"))+wxT("</i></td></tr>\n");
        html=html+wxT("<tr><td>&nbsp;</td><td>&nbsp;</td></tr>\n");
        html=html+wxT("<tr><td colspan=2 align=center><i>")+_("Indices")+wxT("</i></td></tr>\n");
        html=html+wxT("<tr><td><b>")+_("AHI")+wxT("</b></td><td>")+wxString::Format(wxT("%0.2f"),ahi)+wxT("</td></tr>\n");
        html=html+wxT("<tr><td><b>")+_("Obstructive")+wxT("</b></td><td>")+wxString::Format(wxT("%0.2f"),oai)+wxT("</td></tr>\n");
        html=html+wxT("<tr><td><b>")+_("Hypopnea")+wxT("</b></td><td>")+wxString::Format(wxT("%0.2f"),hi)+wxT("</td></tr>\n");
        html=html+wxT("<tr><td><b>")+_("ClearAirway")+wxT("</b></td><td>")+wxString::Format(wxT("%0.2f"),cai)+wxT("</td></tr>\n");
        html=html+wxT("<tr><td><b>")+_("RERA")+wxT("</b></td><td>")+wxString::Format(wxT("%0.2f"),rei)+wxT("</td></tr>\n");
        html=html+wxT("<tr><td><b>")+_("FlowLimit")+wxT("</b></td><td>")+wxString::Format(wxT("%0.2f"),fli)+wxT("</td></tr>\n");
        html=html+wxT("<tr><td><b>")+_("Vsnore")+wxT("</b></td><td>")+wxString::Format(wxT("%0.2f"),vsi)+wxT("</td></tr>\n");
        html=html+wxT("<tr><td><b>")+_("CSR")+wxT("</b></td><td>")+wxString::Format(wxT("%0.2f%%"),csr)+wxT("</td></tr>\n");
        html=html+wxT("<tr><td>&nbsp;</td><td>&nbsp;</td></tr>\n");
        html=html+wxT("<tr><td colspan=2 align=center><i>")+_("Other Information")+wxT("</i></td></tr>\n");

        html=html+wxT("<tr><td><b>")+_("Mode")+wxT("</b></td><td>")+modestr+wxT("</td></tr>\n");
        html=html+wxT("<tr><td><b>")+_("Relief")+wxT("</b></td><td>")+epr+wxT("</td></tr>\n");
        if (mode==MODE_CPAP) {
            html=html+wxT("<tr><td><b>")+_("Pressure")+wxT("</b></td><td>")+wxString::Format(wxT("%.1fcmH2O"),d->summary_min(CPAP_PressureMin))+wxT("</td></tr>\n");
        } else if (mode==MODE_APAP) {
            html=html+wxT("<tr><td><b>")+_("90%&nbsp;Pressure")+wxT("</b></td><td>")+wxString::Format(wxT("%.1fcmH2O"),d->summary_weighted_avg(CPAP_PressurePercentValue))+wxT("</td></tr>\n");
            html=html+wxT("<tr><td><b>")+_("Avg&nbsp;Pressure")+wxT("</b></td><td>")+wxString::Format(wxT("%.2fcmH2O"),d->summary_weighted_avg(CPAP_PressureAverage))+wxT("</td></tr>\n");
            html=html+wxT("<tr><td><b>")+_("Min&nbsp;Reached")+wxT("</b></td><td>")+wxString::Format(wxT("%.1fcmH2O"),d->summary_min(CPAP_PressureMinAchieved))+wxT("</td></tr>\n");
            html=html+wxT("<tr><td><b>")+_("Max&nbsp;Reached")+wxT("</b></td><td>")+wxString::Format(wxT("%.1fcmH2O"),d->summary_max(CPAP_PressureMaxAchieved))+wxT("</td></tr>\n");
          //  html=html+wxT("<tr><td><b>")+_("90%&nbsp;Pressure")+wxT("</b></td><td>")+wxString::Format(wxT("%.1fcmH2O"),p90)+wxT("</td></tr>\n");
        } else if (mode==MODE_BIPAP) {
            html=html+wxT("<tr><td><b>")+_("Avg IPAP")+wxT("</b></td><td>")+wxString::Format(wxT("%.2fcmH2O"),d->summary_avg(BIPAP_IAPAverage))+wxT("</td></tr>\n");
            html=html+wxT("<tr><td><b>")+_("Avg EPAP")+wxT("</b></td><td>")+wxString::Format(wxT("%.1fcmH2O"),d->summary_avg(BIPAP_EAPAverage))+wxT("</td></tr>\n");
            html=html+wxT("<tr><td><b>")+_("Min IPAP")+wxT("</b></td><td>")+wxString::Format(wxT("%.2fcmH2O"),d->summary_min(BIPAP_IAPMin))+wxT("</td></tr>\n");
            html=html+wxT("<tr><td><b>")+_("Max IPAP")+wxT("</b></td><td>")+wxString::Format(wxT("%.1fcmH2O"),d->summary_max(BIPAP_IAPMax))+wxT("</td></tr>\n");
            html=html+wxT("<tr><td><b>")+_("Min EPAP")+wxT("</b></td><td>")+wxString::Format(wxT("%.1fcmH2O"),d->summary_min(BIPAP_EAPMin))+wxT("</td></tr>\n");
            html=html+wxT("<tr><td><b>")+_("Max EPAP")+wxT("</b></td><td>")+wxString::Format(wxT("%.1fcmH2O"),d->summary_max(BIPAP_EAPMax))+wxT("</td></tr>\n");
            html=html+wxT("<tr><td><b>")+_("90%&nbsp;IPAP")+wxT("</b></td><td>")+wxString::Format(wxT("%.1fcmH2O"),iap90)+wxT("</td></tr>\n");
            html=html+wxT("<tr><td><b>")+_("90%&nbsp;EPAP")+wxT("</b></td><td>")+wxString::Format(wxT("%.1fcmH2O"),eap90)+wxT("</td></tr>\n");

        }
        html=html+wxT("<tr><td><b>")+_("Avg Leak")+wxT("</b></td><td>")+wxString::Format(wxT("%.2f"),d->summary_weighted_avg(CPAP_LeakAverage))+wxT("</td></tr>\n");
        html=html+wxT("<tr><td>&nbsp;</td><td>&nbsp;</td></tr>\n");

        if (oxi) {
            html=html+wxT("<tr><td colspan=2 align=center><i>")+_("Oximeter Information")+wxT("</i></td></tr>\n");
            html=html+wxT("<tr><td><b>")+_("Pulse Avg")+wxT("</b></td><td>")+wxString::Format(wxT("%.1fcmH2O"),oxi->summary_avg(OXI_PulseAverage))+wxT("</td></tr>\n");
            html=html+wxT("<tr><td><b>")+_("Pulse Min")+wxT("</b></td><td>")+wxString::Format(wxT("%.1fcmH2O"),oxi->summary_min(OXI_PulseMin))+wxT("</td></tr>\n");
            html=html+wxT("<tr><td><b>")+_("Pulse Max")+wxT("</b></td><td>")+wxString::Format(wxT("%.1fcmH2O"),oxi->summary_max(OXI_PulseMax))+wxT("</td></tr>\n");
            html=html+wxT("<tr><td><b>")+_("SpO2 Avg")+wxT("</b></td><td>")+wxString::Format(wxT("%.1fcmH2O"),oxi->summary_avg(OXI_SPO2Average))+wxT("</td></tr>\n");
            html=html+wxT("<tr><td><b>")+_("SpO2 Min")+wxT("</b></td><td>")+wxString::Format(wxT("%.1fcmH2O"),oxi->summary_min(OXI_SPO2Min))+wxT("</td></tr>\n");
            html=html+wxT("<tr><td><b>")+_("SpO2 Max")+wxT("</b></td><td>")+wxString::Format(wxT("%.1fcmH2O"),oxi->summary_max(OXI_SPO2Max))+wxT("</td></tr>\n");
            PULSE->Show(true);
            SPO2->Show(true);
        } else {
            PULSE->Show(false);
            SPO2->Show(false);
        }
        fgSizer->Layout();
//        fgSizer->Layout();
        ScrolledWindow->FitInside();

        html=html+wxT("<tr><td>&nbsp;</td><td>&nbsp;</td></tr>\n");

        if (d->summary_avg(CPAP_BrokenSummary)==1) {
            html=html+wxT("<tr><td colspan=2 align=center><i>")+_("No System Settings Recorded")+wxT("</i></td></tr>\n");
        } else {
            html=html+wxT("<tr><td colspan=2 align=center><i>")+_("System Settings")+wxT("</i></td></tr>\n");

            if (mode==MODE_CPAP) {
                html=html+wxT("<tr><td><b>")+_("Pressure")+wxT("</b></td><td>")+wxString::Format(wxT("%.1fcmH2O"),d->summary_min(CPAP_PressureMin))+wxT("</td></tr>\n");
            } else if (mode==MODE_APAP) {
                html=html+wxT("<tr><td><b>")+_("Min&nbsp;Pressure")+wxT("</b></td><td>")+wxString::Format(wxT("%.1fcmH2O"),d->summary_min(CPAP_PressureMin))+wxT("</td></tr>\n");
                html=html+wxT("<tr><td><b>")+_("Max&nbsp;Pressure")+wxT("</b></td><td>")+wxString::Format(wxT("%.1fcmH2O"),d->summary_max(CPAP_PressureMax))+wxT("</td></tr>\n");
            } else if (mode==MODE_BIPAP) {
                html=html+wxT("<tr><td><b>")+_("IPAP&nbsp;Pressure")+wxT("</b></td><td>")+wxString::Format(wxT("%.1fcmH2O"),d->summary_min(CPAP_PressureMin))+wxT("</td></tr>\n");
                html=html+wxT("<tr><td><b>")+_("EPAP&nbsp;Pressure")+wxT("</b></td><td>")+wxString::Format(wxT("%.1fcmH2O"),d->summary_max(CPAP_PressureMax))+wxT("</td></tr>\n");
            }
            html=html+wxT("<tr><td><b>")+_("Ramp-Time")+wxT("</b></td><td>")+wxString::Format(wxT("%imin"),(int)d->summary_max(CPAP_RampTime))+wxT("</td></tr>\n");
            html=html+wxT("<tr><td><b>")+_("Ramp-Prs.")+wxT("</b></td><td>")+wxString::Format(wxT("%.1fcmH2O"),d->summary_min(CPAP_RampStartingPressure))+wxT("</td></tr>\n");

            // check HumidiferStatus..
            wxString str;
            if (bool(d->summary_max(CPAP_HumidifierStatus))) {
                str=wxString::Format(wxT("x%i"),(int)d->summary_max(CPAP_HumidifierSetting));
            } else str=wxT("No");
            html=html+wxT("<tr><td><b>")+_("Humidifier")+wxT("</b></td><td>")+str+wxT("</td></tr>\n");
            html=html+wxT("<tr><td><b>")+_("System-Lock")+wxT("</b></td><td>")+(bool(d->summary_max(PRS1_SystemLockStatus)) ? _("On") : _("Off"))+wxT("</td></tr>\n");
            html=html+wxT("<tr><td><b>")+_("Auto-Off")+wxT("</b></td><td>")+(bool(d->summary_max(PRS1_AutoOff)) ? _("On") : _("Off"))+wxT("</td></tr>\n");
            html=html+wxT("<tr><td><b>")+_("Mask-Alert")+wxT("</b></td><td>")+(bool(d->summary_max(PRS1_MaskAlert)) ? _("On") : _("Off"))+wxT("</td></tr>\n");
            html=html+wxT("<tr><td><b>")+_("Show-AHI")+wxT("</b></td><td>")+(bool(d->summary_max(PRS1_ShowAHI)) ? _("On") : _("Off"))+wxT("</td></tr>\n");
            html=html+wxT("<tr><td><b>")+_("Hose-Size")+wxT("</b></td><td>")+(bool(d->summary_max(PRS1_HoseDiameter)) ? _("22mm") : _("15mm"))+wxT("</td></tr>\n");
            if (bool(d->summary_max(PRS1_SystemResistanceStatus))) {
                str=wxString::Format(wxT("x%i"),(int)d->summary_max(PRS1_SystemResistanceSetting));
            } else str=wxT("No");
            html=html+wxT("<tr><td><b>")+_("Sys-Resist.")+wxT("</b></td><td>")+str+wxT("</td></tr>\n");
        }
        html=html+wxT("<tr><td>&nbsp;</td><td>&nbsp;</td></tr>\n");
        html=html+wxT("<tr><td colspan=2 align=center><i>")+_("Session Files")+wxT("</i></td></tr>\n");

        for (vector<Session *>::iterator i=d->begin();i!=d->end();i++) {

            html=html+wxT("<tr><td colspan=2 align=center>")+(*i)->first().Format(wxT("%d-%m-%Y %H:%M:%S"))+wxT(" ")+wxString::Format(wxT("%05i"),(*i)->session())+wxT("</td></tr>\n");
            html=html+wxT("<tr><td colspan=2 align=center>")+(*i)->last().Format(wxT("%d-%m-%Y %H:%M:%S"))+wxT(" ")+wxString::Format(wxT("%05i"),(*i)->session())+wxT("</td></tr>\n");
        }

        html=html+wxT("</table>");
        html+=wxT("</body></html>");
        HTMLInfo->SetPage(html);
    } else {
        HTMLInfo->SetPage(_("<i>No data available for this day</i>"));
    }

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


