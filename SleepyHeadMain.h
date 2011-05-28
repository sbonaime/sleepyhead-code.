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



#include "SleepyHeadApp.h"

#include "GUIFrame.h"
#include "sleeplib/machine.h"
#include "graphs/graph.h"
//#include "graphs/sleepflagsgraph.h"
//#include "graphs/cpap_wavegraph.h"
//#include "graphs/cpap_pressure.h"

class Summary:public SummaryPanel
{
public:
    Summary(wxWindow *win);
    virtual ~Summary();
    Machine *machine;
    HistoryData *ahidata,*pressure,*leak,*usage,*bedtime,*waketime;
    gGraphWindow *AHI,*PRESSURE,*LEAK,*USAGE;
    void RefreshData();
	void AddData(HistoryData *d) { Data.push_back(d);  };
protected:
    list<HistoryData *> Data;
};


class Daily:public DailyPanel
{
public:
    Daily(wxWindow *win);
    virtual ~Daily();
    void RefreshData();

    Machine *machine;
protected:
    virtual void OnCalendarDay( wxCalendarEvent& event );
	virtual void OnCalendarMonth( wxCalendarEvent& event );
	void AddData(gPointData *d) { Data.push_back(d);  };
	void UpdateGraphs(wxDateTime date);

    bool foobar_datehack;
    gPointData *tap,*g_ahi,*frw,*prd,*leakdata,*pressure_iap,*pressure_eap;
    gPointData *flags[10];
    gGraphWindow *PRD,*FRW,*G_AHI,*TAP,*LEAK,*SF;


    list<gPointData *> Data;
};

const wxEventType wxEVT_DO_SCREENSHOT = wxNewEventType();

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
        virtual void DoScreenshot(wxCommandEvent& event);
        virtual void OnImportSD(wxCommandEvent& event);
        virtual void OnViewMenuDaily(wxCommandEvent& event);
        virtual void OnViewMenuSummary(wxCommandEvent& event);

        Summary *summary;
        Daily *daily;
        Machine *machine;
        Profile *profile;

};

#endif // SLEEPYHEADMAIN_H
