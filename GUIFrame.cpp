///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version May  5 2011)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif //__BORLANDC__

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif //WX_PRECOMP

#include "GUIFrame.h"

///////////////////////////////////////////////////////////////////////////

GUIFrame::GUIFrame( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxFrame( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	m_mgr.SetManagedWindow(this);
	
	menubar = new wxMenuBar( 0 );
	FileMenu = new wxMenu();
	wxMenuItem* FileMenuImportSD;
	FileMenuImportSD = new wxMenuItem( FileMenu, wxID_ANY, wxString( _("&Import SD") ) + wxT('\t') + wxT("F9"), wxEmptyString, wxITEM_NORMAL );
	FileMenu->Append( FileMenuImportSD );
	
	wxMenuItem* FileMenuPreferences;
	FileMenuPreferences = new wxMenuItem( FileMenu, wxID_ANY, wxString( _("&Preferences") ) + wxT('\t') + wxT("F10"), wxEmptyString, wxITEM_NORMAL );
	FileMenu->Append( FileMenuPreferences );
	
	wxMenuItem* m_separator1;
	m_separator1 = FileMenu->AppendSeparator();
	
	wxMenuItem* FileMenuExit;
	FileMenuExit = new wxMenuItem( FileMenu, wxID_QUIT, wxString( _("E&xit") ) , wxEmptyString, wxITEM_NORMAL );
	FileMenu->Append( FileMenuExit );
	
	menubar->Append( FileMenu, _("&File") ); 
	
	ViewMenu = new wxMenu();
	wxMenuItem* ViewMenuSummary;
	ViewMenuSummary = new wxMenuItem( ViewMenu, wxID_ANY, wxString( _("&Summary") ) + wxT('\t') + wxT("F5"), wxEmptyString, wxITEM_NORMAL );
	ViewMenu->Append( ViewMenuSummary );
	
	wxMenuItem* ViewMenuDaily;
	ViewMenuDaily = new wxMenuItem( ViewMenu, wxID_ANY, wxString( _("&Daily") ) + wxT('\t') + wxT("F6"), wxEmptyString, wxITEM_NORMAL );
	ViewMenu->Append( ViewMenuDaily );
	
	wxMenuItem* m_separator3;
	m_separator3 = ViewMenu->AppendSeparator();
	
	ViewMenuSerial = new wxMenuItem( ViewMenu, wxID_ANY, wxString( _("Show Serial Numbers") ) , wxEmptyString, wxITEM_CHECK );
	ViewMenu->Append( ViewMenuSerial );
	
	wxMenuItem* m_separator2;
	m_separator2 = ViewMenu->AppendSeparator();
	
	wxMenuItem* ViewMenuFullscreen;
	ViewMenuFullscreen = new wxMenuItem( ViewMenu, wxID_ANY, wxString( _("Full Screen") ) + wxT('\t') + wxT("F11"), wxEmptyString, wxITEM_NORMAL );
	ViewMenu->Append( ViewMenuFullscreen );
	
	menubar->Append( ViewMenu, _("&View") ); 
	
	ProfileMenu = new wxMenu();
	menubar->Append( ProfileMenu, _("&Profiles") ); 
	
	ToolsMenu = new wxMenu();
	wxMenuItem* ToolsMenuScreenshot;
	ToolsMenuScreenshot = new wxMenuItem( ToolsMenu, wxID_ANY, wxString( _("Screenshot") ) + wxT('\t') + wxT("Shift+F12"), wxEmptyString, wxITEM_NORMAL );
	ToolsMenu->Append( ToolsMenuScreenshot );
	
	menubar->Append( ToolsMenu, _("Tools") ); 
	
	HelpMenu = new wxMenu();
	wxMenuItem* HelpMenuAbout;
	HelpMenuAbout = new wxMenuItem( HelpMenu, wxID_ANY, wxString( _("&About") ) , wxEmptyString, wxITEM_NORMAL );
	HelpMenu->Append( HelpMenuAbout );
	
	menubar->Append( HelpMenu, _("&Help") ); 
	
	this->SetMenuBar( menubar );
	
	statusBar = this->CreateStatusBar( 2, wxST_SIZEGRIP, wxID_ANY );
	main_auinotebook = new wxAuiNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_NB_DEFAULT_STYLE );
	m_mgr.AddPane( main_auinotebook, wxAuiPaneInfo() .Center() .MaximizeButton( false ).MinimizeButton( false ).PinButton( true ).Dock().Resizable().FloatingSize( wxDefaultSize ).DockFixed( false ) );
	
	
	
	m_mgr.Update();
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( GUIFrame::OnClose ) );
	this->Connect( FileMenuImportSD->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( GUIFrame::OnImportSD ) );
	this->Connect( FileMenuPreferences->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( GUIFrame::OnPreferencesClicked ) );
	this->Connect( FileMenuExit->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( GUIFrame::OnQuit ) );
	this->Connect( ViewMenuSummary->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( GUIFrame::OnViewMenuSummary ) );
	this->Connect( ViewMenuDaily->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( GUIFrame::OnViewMenuDaily ) );
	this->Connect( ViewMenuSerial->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( GUIFrame::OnShowSerial ) );
	this->Connect( ViewMenuFullscreen->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( GUIFrame::OnFullscreen ) );
	this->Connect( ToolsMenuScreenshot->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( GUIFrame::OnScreenshot ) );
	this->Connect( HelpMenuAbout->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( GUIFrame::OnAbout ) );
}

GUIFrame::~GUIFrame()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( GUIFrame::OnClose ) );
	this->Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( GUIFrame::OnImportSD ) );
	this->Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( GUIFrame::OnPreferencesClicked ) );
	this->Disconnect( wxID_QUIT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( GUIFrame::OnQuit ) );
	this->Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( GUIFrame::OnViewMenuSummary ) );
	this->Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( GUIFrame::OnViewMenuDaily ) );
	this->Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( GUIFrame::OnShowSerial ) );
	this->Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( GUIFrame::OnFullscreen ) );
	this->Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( GUIFrame::OnScreenshot ) );
	this->Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( GUIFrame::OnAbout ) );
	
	m_mgr.UnInit();
	
}

DailyPanel::DailyPanel( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style ) : wxPanel( parent, id, pos, size, style )
{
	m_mgr.SetManagedWindow(this);
	
	HTMLInfo = new wxHtmlWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHW_SCROLLBAR_AUTO );
	m_mgr.AddPane( HTMLInfo, wxAuiPaneInfo() .Left() .Caption( wxT("Day Summary") ).CloseButton( false ).MaximizeButton( false ).MinimizeButton( false ).PinButton( true ).Dock().Resizable().FloatingSize( wxSize( 200,424 ) ).DockFixed( false ).Row( 0 ).Position( 1 ).MinSize( wxSize( 200,200 ) ).Layer( 0 ) );
	
	ScrolledWindow = new wxScrolledWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHSCROLL|wxVSCROLL );
	ScrolledWindow->SetScrollRate( 5, 5 );
	m_mgr.AddPane( ScrolledWindow, wxAuiPaneInfo() .Center() .Caption( wxT("Daily Information") ).CloseButton( false ).MaximizeButton( false ).MinimizeButton( false ).PinButton( true ).Dock().Resizable().FloatingSize( wxSize( -1,-1 ) ).Row( 0 ).Layer( 1 ).CentrePane() );
	
	fgSizer = new wxFlexGridSizer( 0, 1, 0, 0 );
	fgSizer->AddGrowableCol( 0 );
	fgSizer->SetFlexibleDirection( wxVERTICAL );
	fgSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_ALL );
	
	ScrolledWindow->SetSizer( fgSizer );
	ScrolledWindow->Layout();
	fgSizer->Fit( ScrolledWindow );
	Calendar = new wxCalendarCtrl( this, wxID_ANY, wxDefaultDateTime, wxDefaultPosition, wxDefaultSize, wxCAL_MONDAY_FIRST|wxCAL_SEQUENTIAL_MONTH_SELECTION|wxCAL_SHOW_SURROUNDING_WEEKS );
	m_mgr.AddPane( Calendar, wxAuiPaneInfo() .Left() .Caption( wxT("Selected Day") ).CloseButton( false ).MaximizeButton( false ).MinimizeButton( false ).PinButton( true ).PaneBorder( false ).Dock().Fixed().BottomDockable( false ).TopDockable( false ) );
	
	
	m_mgr.Update();
	
	// Connect Events
	Calendar->Connect( wxEVT_CALENDAR_MONTH_CHANGED, wxCalendarEventHandler( DailyPanel::OnCalendarMonth ), NULL, this );
	Calendar->Connect( wxEVT_CALENDAR_SEL_CHANGED, wxCalendarEventHandler( DailyPanel::OnCalendarDay ), NULL, this );
}

DailyPanel::~DailyPanel()
{
	// Disconnect Events
	Calendar->Disconnect( wxEVT_CALENDAR_MONTH_CHANGED, wxCalendarEventHandler( DailyPanel::OnCalendarMonth ), NULL, this );
	Calendar->Disconnect( wxEVT_CALENDAR_SEL_CHANGED, wxCalendarEventHandler( DailyPanel::OnCalendarDay ), NULL, this );
	
	m_mgr.UnInit();
	
}

SummaryPanel::SummaryPanel( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style ) : wxPanel( parent, id, pos, size, style )
{
	m_mgr.SetManagedWindow(this);
	
	HTMLInfo = new wxHtmlWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHW_SCROLLBAR_AUTO );
	m_mgr.AddPane( HTMLInfo, wxAuiPaneInfo() .Right() .Caption( wxT("Information") ).CloseButton( false ).MaximizeButton( false ).MinimizeButton( false ).PinButton( true ).Dock().Resizable().FloatingSize( wxSize( 208,424 ) ).DockFixed( false ).Row( 0 ).Position( 2 ).MinSize( wxSize( 200,400 ) ) );
	
	ScrolledWindow = new wxScrolledWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHSCROLL|wxVSCROLL );
	ScrolledWindow->SetScrollRate( 5, 5 );
	m_mgr.AddPane( ScrolledWindow, wxAuiPaneInfo() .Center() .Caption( wxT("Overview") ).CloseButton( false ).MaximizeButton( false ).MinimizeButton( false ).PinButton( true ).Dock().Resizable().FloatingSize( wxDefaultSize ).DockFixed( false ).MinSize( wxSize( 440,400 ) ) );
	
	fgSizer = new wxFlexGridSizer( 0, 1, 0, 0 );
	fgSizer->SetFlexibleDirection( wxVERTICAL );
	fgSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_ALL );
	
	ScrolledWindow->SetSizer( fgSizer );
	ScrolledWindow->Layout();
	fgSizer->Fit( ScrolledWindow );
	m_panel1 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_panel1->SetMaxSize( wxSize( -1,42 ) );
	m_mgr.AddPane( m_panel1, wxAuiPaneInfo() .Bottom() .Caption( wxT("Date Range") ).CloseButton( false ).MaximizeButton( false ).MinimizeButton( false ).PinButton( true ).Dock().Resizable().FloatingSize( wxSize( -1,-1 ) ).DockFixed( false ).LeftDockable( false ).RightDockable( false ).Row( 0 ).Position( 1 ).BestSize( wxSize( 570,42 ) ).MinSize( wxSize( 570,42 ) ).MaxSize( wxSize( -1,42 ) ) );
	
	wxBoxSizer* bSizer1;
	bSizer1 = new wxBoxSizer( wxHORIZONTAL );
	
	rbAll = new wxRadioButton( m_panel1, wxID_RB, _("Everything"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	bSizer1->Add( rbAll, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );
	
	rbLastWeek = new wxRadioButton( m_panel1, wxID_ANY, _("Last Week"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer1->Add( rbLastWeek, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );
	
	rbLastMonth = new wxRadioButton( m_panel1, wxID_ANY, _("Last Month"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer1->Add( rbLastMonth, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );
	
	rbCustomDate = new wxRadioButton( m_panel1, wxID_ANY, _("Custom"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer1->Add( rbCustomDate, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );
	
	sdLabel = new wxStaticText( m_panel1, wxID_ANY, _("Start"), wxDefaultPosition, wxDefaultSize, 0 );
	sdLabel->Wrap( -1 );
	sdLabel->Enable( false );
	
	bSizer1->Add( sdLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );
	
	StartDatePicker = new wxDatePickerCtrl( m_panel1, wxID_ANY, wxDefaultDateTime, wxDefaultPosition, wxDefaultSize, wxDP_SPIN );
	StartDatePicker->Enable( false );
	
	bSizer1->Add( StartDatePicker, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0 );
	
	edLabel = new wxStaticText( m_panel1, wxID_ANY, _("End"), wxDefaultPosition, wxDefaultSize, 0 );
	edLabel->Wrap( -1 );
	edLabel->Enable( false );
	
	bSizer1->Add( edLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );
	
	EndDatePicker = new wxDatePickerCtrl( m_panel1, wxID_ANY, wxDefaultDateTime, wxDefaultPosition, wxDefaultSize, wxDP_SPIN );
	EndDatePicker->Enable( false );
	
	bSizer1->Add( EndDatePicker, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0 );
	
	m_panel1->SetSizer( bSizer1 );
	m_panel1->Layout();
	bSizer1->Fit( m_panel1 );
	
	m_mgr.Update();
	
	// Connect Events
	rbAll->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( SummaryPanel::OnRBSelect ), NULL, this );
	rbLastWeek->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( SummaryPanel::OnRBSelect ), NULL, this );
	rbLastMonth->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( SummaryPanel::OnRBSelect ), NULL, this );
	rbCustomDate->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( SummaryPanel::OnRBSelect ), NULL, this );
	StartDatePicker->Connect( wxEVT_DATE_CHANGED, wxDateEventHandler( SummaryPanel::OnStartDateChanged ), NULL, this );
	EndDatePicker->Connect( wxEVT_DATE_CHANGED, wxDateEventHandler( SummaryPanel::OnEndDateChanged ), NULL, this );
}

SummaryPanel::~SummaryPanel()
{
	// Disconnect Events
	rbAll->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( SummaryPanel::OnRBSelect ), NULL, this );
	rbLastWeek->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( SummaryPanel::OnRBSelect ), NULL, this );
	rbLastMonth->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( SummaryPanel::OnRBSelect ), NULL, this );
	rbCustomDate->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( SummaryPanel::OnRBSelect ), NULL, this );
	StartDatePicker->Disconnect( wxEVT_DATE_CHANGED, wxDateEventHandler( SummaryPanel::OnStartDateChanged ), NULL, this );
	EndDatePicker->Disconnect( wxEVT_DATE_CHANGED, wxDateEventHandler( SummaryPanel::OnEndDateChanged ), NULL, this );
	
	m_mgr.UnInit();
	
}
