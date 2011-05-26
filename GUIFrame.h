///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version May  5 2011)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __GUIFrame__
#define __GUIFrame__

#include <wx/intl.h>

#include <wx/string.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/menu.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/statusbr.h>
#include <wx/aui/auibook.h>
#include <wx/frame.h>
#include <wx/aui/aui.h>
#include <wx/html/htmlwin.h>
#include <wx/sizer.h>
#include <wx/scrolwin.h>
#include <wx/calctrl.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

#define wxID_QUIT 1000

///////////////////////////////////////////////////////////////////////////////
/// Class GUIFrame
///////////////////////////////////////////////////////////////////////////////
class GUIFrame : public wxFrame 
{
	private:
	
	protected:
		wxMenuBar* menubar;
		wxMenu* FileMenu;
		wxMenu* ViewMenu;
		wxMenu* HelpMenu;
		wxStatusBar* statusBar;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnImportSD( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnPreferencesClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnQuit( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnViewMenuSummary( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnViewMenuDaily( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnAbout( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		wxAuiNotebook* main_auinotebook;
		
		GUIFrame( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("SleepyHead"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 1157,703 ), long style = wxDEFAULT_FRAME_STYLE|wxTAB_TRAVERSAL );
		wxAuiManager m_mgr;
		
		~GUIFrame();
	
};

///////////////////////////////////////////////////////////////////////////////
/// Class DailyPanel
///////////////////////////////////////////////////////////////////////////////
class DailyPanel : public wxPanel 
{
	private:
	
	protected:
		wxHtmlWindow* HTMLInfo;
		wxScrolledWindow* ScrolledWindow;
		wxFlexGridSizer* fgSizer;
		wxCalendarCtrl* Calendar;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnCalendarMonth( wxCalendarEvent& event ) { event.Skip(); }
		virtual void OnCalendarDay( wxCalendarEvent& event ) { event.Skip(); }
		
	
	public:
		
		DailyPanel( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 935,573 ), long style = wxTAB_TRAVERSAL ); wxAuiManager m_mgr;
		
		~DailyPanel();
	
};

///////////////////////////////////////////////////////////////////////////////
/// Class SummaryPanel
///////////////////////////////////////////////////////////////////////////////
class SummaryPanel : public wxPanel 
{
	private:
	
	protected:
		wxHtmlWindow* HTMLInfo;
		wxScrolledWindow* ScrolledWindow;
		wxFlexGridSizer* fgSizer;
	
	public:
		
		SummaryPanel( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 712,445 ), long style = wxTAB_TRAVERSAL ); wxAuiManager m_mgr;
		
		~SummaryPanel();
	
};

#endif //__GUIFrame__
