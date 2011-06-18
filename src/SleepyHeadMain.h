/***************************************************************
 * Name:      SleepyHeadMain.h
 * Purpose:   Defines Application Frame
 * Author:    Mark Watkins (jedimark64@users.sourceforge.net)
 * Created:   2011-05-20
 * Copyright: Mark Watkins (http://sourceforge.net/projects/sleepyhead/)
 * License:   GPL
 **************************************************************/

#ifndef SLEEPYHEADMAIN_H
#define SLEEPYHEADMAIN_H


#include <wx/listbox.h>
#include <wx/treectrl.h>

#include "SleepyHeadApp.h"
#include "GUIFrame.h"
#include "sleeplib/machine.h"
#include "graphs/graph.h"

class Summary:public SummaryPanel
{
public:
    Summary(wxWindow *win,Profile *_profile);
    virtual ~Summary();
    void RefreshData();
    void ResetProfile(Profile *p);
	void AddData(HistoryData *d) { Data.push_back(d);  };

    HistoryData *ahidata,*pressure,*leak,*usage,*bedtime,*waketime,*pressure_iap,*pressure_eap;
    HistoryData *pressure_min,*pressure_max;

    gGraphWindow *AHI,*PRESSURE,*LEAK,*USAGE;

    gLayer *prmax,*prmin,*iap,*eap,*pr;

    wxBitmap Logo;

protected:
    virtual void OnRBSelect( wxCommandEvent& event );
	virtual void OnStartDateChanged( wxDateEvent& event );
	virtual void OnEndDateChanged( wxDateEvent& event );
    virtual void OnClose(wxCloseEvent &event);

    void EnableDatePickers(bool b);

    Profile *profile;
    list<HistoryData *> Data;
    Day *dummyday;
};

extern const wxEventType wxEVT_REFRESH_DAILY;
class Daily:public DailyPanel
{
public:
    Daily(wxWindow *win,Profile *p);
    virtual ~Daily();
    void ResetDate();
    void RefreshData();
    void DoRefreshData(wxCommandEvent& event);
  //  void SetProfile(Profile *p);
protected:
    virtual void OnCalendarDay( wxCalendarEvent& event );
	virtual void OnCalendarMonth( wxCalendarEvent& event );
    virtual void OnClose(wxCloseEvent &event);
    virtual void OnSelectSession( wxCommandEvent& event );
    virtual void OnEventTreeSelection( wxTreeEvent& event );
    virtual void OnWinScroll(wxScrollWinEvent& event);
	void AddCPAPData(gPointData *d) { CPAPData.push_back(d);  };
	void AddOXIData(gPointData *d) { OXIData.push_back(d);  };
	void UpdateCPAPGraphs(Day *day);
	void UpdateOXIGraphs(Day *day);

    bool foobar_datehack;
    gPointData *tap,*tap_eap,*tap_iap,*g_ahi,*frw,*prd,*leakdata,*pressure_iap,*pressure_eap,*snore;
    gPointData *pulse,*spo2;
    gPointData *flags[10];
    gGraphWindow *PRD,*FRW,*G_AHI,*TAP,*LEAK,*SF,*TAP_EAP,*TAP_IAP,*PULSE,*SPO2,*SNORE;

    Profile *profile;
    list<gPointData *> OXIData;
    list<gPointData *> CPAPData;
    wxHtmlWindow *HTMLInfo;
    wxTreeCtrl *EventTree;

    wxBitmap *ahi_bmp,*tap_bmp,*teap_bmp,*tiap_bmp;

    // Add it's own wxGLCanvas in place of scrolledwindow
    wxScrolledWindow *GraphWindow;
    wxFlexGridSizer* gwSizer;


};

const wxEventType wxEVT_DO_SCREENSHOT = wxNewEventType();
const wxEventType wxEVT_MACHINE_SELECTED = wxNewEventType();

const int ProfileMenuID=wxID_HIGHEST;

class SleepyHeadFrame: public GUIFrame
{
    public:
        SleepyHeadFrame(wxFrame *frame);
        ~SleepyHeadFrame();
    private:
        virtual void OnClose(wxCloseEvent& event);
        virtual void OnQuit(wxCommandEvent& event);
        virtual void OnAbout(wxCommandEvent& event);
        virtual void OnScreenshot(wxCommandEvent& event);
        virtual void OnFullscreen(wxCommandEvent& event);
        virtual void OnViewLog(wxCommandEvent& event);
        virtual void DoScreenshot(wxCommandEvent& event);
        virtual void OnImportSD(wxCommandEvent& event);
        virtual void OnViewMenuDaily(wxCommandEvent& event);
        virtual void OnViewMenuSummary(wxCommandEvent& event);
        virtual void OnShowSerial(wxCommandEvent& event);
        virtual void OnProfileSelected(wxCommandEvent& event);
        virtual void OnFruitsalad(wxCommandEvent& event);
		virtual void OnLinkGraphs( wxCommandEvent& event );
		virtual void OnAntiAliasing( wxCommandEvent& event );


        virtual void UpdateProfiles();

        Summary *summary;
        Daily *daily;
        Machine *machine;
        Profile *profile;
        wxLogWindow *logwindow;
        vector<Machine *>cpap_machines;
        int current_machine;

};

#endif // SLEEPYHEADMAIN_H
