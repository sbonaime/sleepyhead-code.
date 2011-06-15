/*
gGraph Graphing System Header File

Author: Mark Watkins <jedimark64@users.sourceforge.net>
License: LGPL
*/
#ifndef GRAPH_H
#define GRAPH_H

#include "gl_pbuffer.h"
extern pBuffer *pbuffer;

//#undef Yield

//#include <wx/dcgraph.h>
#include <wx/glcanvas.h>
#include <wx/geometry.h>

#include <sleeplib/machine.h>
#include <list>


#if !wxCHECK_VERSION(2,9,0)
extern wxColor *wxYELLOW;
#endif
extern wxColor *wxAQUA;
extern wxColor *wxPURPLE;
extern wxColor *wxGREEN2;
extern wxColor *wxLIGHT_YELLOW;
extern wxColor *wxDARK_GREEN;
extern wxColor *wxDARK_GREY;

#define MIN(a,b) (a<b) ? a : b;
#define MAX(a,b) (a<b) ? b : a;

class Point3D
{
public:
    Point3D() {x=0; y=0; z=0;};
    Point3D(double _x,double _y, double _z) { x=_x; y=_y; z=_z; };

    double x,y,z;
};

enum gDataType { gDT_Point, gDT_Point3D, gDT_Stacked, gDT_Segmented };

class gLayer;

class gGraphData
{
public:
    gGraphData(int mp,gDataType t=gDT_Point);
    virtual ~gGraphData();

    virtual void Reload(Day *day=NULL) {};

    virtual void Update(Day *day=NULL);
    //inline wxRealPoint & operator [](int i) { return vpoint[seg][i]; };
    //inline vector<double> & Vec(int i) { return yaxis[i]; };

    //virtual inline const int & NP(int i) { return vnp[i]; };
    //virtual inline const int & MP(int i) { return vsize[i]; };
    inline const gDataType & Type() { return type; };

    virtual inline double MaxX() { return max_x; };
    virtual inline double MinX() { return min_x; };
    virtual inline double MaxY() { return max_y; };
    virtual inline double MinY() { return min_y; };
    virtual inline void SetMaxX(double v) { max_x=v; if (max_x>real_max_x) max_x=real_max_x; };
    virtual inline void SetMinX(double v) { min_x=v; if (min_x<real_min_x) min_x=real_min_x; };
    virtual inline void SetMaxY(double v) { max_y=v; if (max_y>real_max_y) max_y=real_max_y; };
    virtual inline void SetMinY(double v) { min_y=v; if (min_y<real_min_y) min_y=real_min_y; };
    virtual inline double RealMaxX() { return real_max_x; };
    virtual inline double RealMinX() { return real_min_x; };
    virtual inline double RealMaxY() { return real_max_y; };
    virtual inline double RealMinY() { return real_min_y; };

    virtual inline void ForceMinY(double v) { force_min_y=v; };
    virtual inline void ForceMaxY(double v) { force_max_y=v; };

    inline void ResetX() { min_x=real_min_x; max_x=real_max_x; };
    inline void ResetY() { min_y=real_min_y; max_y=real_max_y; };

    virtual inline int VC() { return vc; };

    vector<int> np;
    vector<int> maxsize;
    const bool & IsReady() { return m_ready; };

    void AddLayer(gLayer *g) { notify_layers.push_back(g); };
protected:
    virtual void AddSegment(int max_points) {};

    double real_min_x, real_max_x, real_min_y, real_max_y;
    double min_x, max_x, min_y, max_y;

    double force_min_y,force_max_y;

    int vc;
    gDataType type;
    int max_points;
    bool m_ready;

    list<gLayer *> notify_layers;

};

class gPoint3DData:public gGraphData
{
public:
    gPoint3DData(int mp);
    virtual ~gPoint3DData();
    virtual void Reload(Day *day=NULL) {};
    virtual void AddSegment(int max_points);
    vector<Point3D *> point;
};

class gPointData:public gGraphData
{
public:
    gPointData(int mp);
    virtual ~gPointData();
    virtual void Reload(Day *day=NULL){};
    virtual void AddSegment(int max_points);
    vector<wxPoint2DDouble *> point;
};


extern wxGLContext *shared_context;
class gGraphWindow:public wxGLCanvas //Window // rename to gGraphWindow
{
    public:
        gGraphWindow();
        gGraphWindow(wxWindow *parent, wxWindowID id,const wxString & title=wxT("Graph"),const wxPoint &pos = wxDefaultPosition,const wxSize &size = wxDefaultSize,long flags = 0);

        wxBitmap * RenderBitmap(int width,int height);

        virtual ~gGraphWindow();
        virtual void OnPaint(wxPaintEvent & event);
        virtual void OnSize(wxSizeEvent & event);
        //virtual void OnMouseWheel(wxMouseEvent &event);
        virtual void OnMouseMove(wxMouseEvent &event);
        virtual void OnMouseLeftDown(wxMouseEvent &event);
        virtual void OnMouseLeftDClick(wxMouseEvent &event);
        virtual void OnMouseLeftRelease (wxMouseEvent  &event);
        virtual void OnMouseRightDown(wxMouseEvent &event);
        virtual void OnMouseRightRelease(wxMouseEvent &event);

        int GetScrX(void) const { return m_scrX; };
        int GetScrY(void) const { return m_scrY; };

        // For mouse to screen use only.. work in OpenGL points where possible

        const wxString & Title(void ) { return m_title; };

        void SetMargins(float top, float right, float bottom, float left);  // OpenGL width of each corners margin

        float GetTopMargin(void) const { return m_marginTop; };
        float GetBottomMargin(void) const { return m_marginBottom; };
        float GetLeftMargin(void) const { return m_marginLeft; };
        float GetRightMargin(void) const { return m_marginRight; };

        void SetTopMargin(float i) { m_marginTop=i; };
        void SetBottomMargin(float i) { m_marginBottom=i; };
        void SetLeftMargin(float i) { m_marginLeft=i; };
        void SetRightMargin(float i) { m_marginRight=i; };

        inline float Width() { return m_scrX-m_marginLeft-m_marginRight; };  // Width of OpenGL main drawing area
        inline int Height() { return m_scrY-m_marginTop-m_marginBottom; };   // Height of ""...

        void LinkZoom(gGraphWindow *g) { link_zoom.push_back(g); }; // Linking graphs changes zoom behaviour..
        void LinkMove(gGraphWindow *g) { link_move.push_back(g); }; // Linking graphs changes zoom behaviour..

        virtual double MinX();
        virtual double MaxX();
        virtual double MinY();
        virtual double MaxY();

        virtual double RealMinX();
        virtual double RealMaxX();
        virtual double RealMinY();
        virtual double RealMaxY();

        virtual void SetMinX(double v);
        virtual void SetMaxX(double v);
        virtual void SetMinY(double v);
        virtual void SetMaxY(double v);

        virtual void ResetXBounds();
        virtual void SetXBounds(double minx, double maxx);
        virtual void ZoomX(double mult,int origin_px);

        virtual void ZoomXPixels(int x1, int x2);           // Zoom between two selected points on screen
        virtual void ZoomXPixels(int x1,int x2,double &rx1,double &rx2);

        virtual void MoveX(int i);                          // Move x bounds by i Pixels
        virtual void MoveX(int i,double &min, double & max);

        inline float x2p(double x) {
            double xx=max_x-min_x;
            double w=((Width()/xx)*(x-min_x));
            return w+GetLeftMargin();
        };
        inline double p2x(float px) {
            double xx=max_x-min_x;
            double wx=px-GetLeftMargin();
            double ww=wx/Width();
            return min_x+(xx*ww);
        };
        inline int y2p(double y) {
            double yy=max_y-min_y;
            double h=(Height()/yy)*(y-min_y);
            return h+GetTopMargin();
        };
        inline double p2y(float py) {
            double yy=max_y-min_y;
            double hy=py-GetTopMargin();
            double hh=hy/Height();
            return min_y+(yy*hh);
        };

        void Render(float scrx,float scry);

        //virtual void Update();
        //virtual void Update();
        void AddLayer(gLayer *l);

        virtual void DataChanged(gLayer *layer);

        double max_x,min_x,max_y,min_y;
        double rmax_x,rmin_x,rmax_y,rmin_y;

        void SetBlockZoom(bool b) { m_block_zoom=b; };
        void SetBlockMove(bool b) { m_block_move=b; };

        wxGLContext *gl_context;
        //FTFont *texfont;

    protected:
        list<gGraphWindow *>link_zoom;
        list<gGraphWindow *>link_move;

        bool m_block_move;
        bool m_block_zoom;
        bool m_drag_foobar;
        double m_foobar_pos,m_foobar_moved;
        std::list<gLayer *> layers;
        wxColour m_bgColour;	//!< Background Colour
        wxColour m_fgColour;	//!< Foreground Colour
        wxColour m_axColour;	//!< Axes Colour
        wxString m_title;
        int    m_scrX;      //!< Current view's X dimension
        int    m_scrY;      //!< Current view's Y dimension
        wxPoint m_mouseLClick,m_mouseRClick,m_mouseRClick_start;

        float m_marginTop, m_marginRight, m_marginBottom, m_marginLeft;

        wxRect m_mouseRBrect,m_mouseRBlast;
        bool m_mouseLDown,m_mouseRDown,m_datarefresh;

        gLayer *foobar;
        gLayer *xaxis;
        gLayer *yaxis;
        gLayer *gtitle;
        DECLARE_DYNAMIC_CLASS(gGraphWindow)
        DECLARE_EVENT_TABLE()

        wxDateTime ti;
        gLayer *lastlayer;

};

//Borrows a lot of concepts from wxmathplot
class gLayer
{
    public:
        gLayer(gPointData *g=NULL,wxString title=wxT(""));
        virtual ~gLayer();
        //virtual void Update() { data=gd; };
        virtual void Plot(gGraphWindow & w,float scrx,float scry);
        vector<wxColor> color;

        virtual void SetData(gPointData * gd) { data=gd; };
        virtual gPointData * GetData() { return data; };

        virtual void DataChanged(gGraphData *src) {
            for (list<gGraphWindow *>::iterator i=m_graph.begin();i!=m_graph.end();i++) {
                if (src) {
                    (*i)->DataChanged(this);
                } else {
                    (*i)->DataChanged(NULL);
                }
            }

        }; // Notify signal sent from gGraphData.. pass on to the graph so it can que a refresh and update stuff.

        virtual double MinX() { if (data) return data->MinX(); return 0;};
        virtual double MaxX() { if (data) return data->MaxX(); return 0;};
        virtual double MinY() { if (data) return data->MinY(); return 0;};
        virtual double MaxY() { if (data) return data->MaxY(); return 0;};

        virtual double RealMinX() { if (data) return data->RealMinX(); return 0;};
        virtual double RealMaxX() { if (data) return data->RealMaxX(); return 0;};
        virtual double RealMinY() { if (data) return data->RealMinY(); return 0;};
        virtual double RealMaxY() { if (data) return data->RealMaxY(); return 0;};

        virtual void SetMinX(double v) { if (data) data->SetMinX(v); };
        virtual void SetMaxX(double v) { if (data) data->SetMaxX(v); };
        virtual void SetMinY(double v) { if (data) data->SetMinY(v); };
        virtual void SetMaxY(double v) { if (data) data->SetMaxY(v); };

        /*virtual inline int x2p(gGraphWindow & g,double x) {
            double xx=data->MaxX()-data->MinX();
            double w=(g.Width()/xx)*(x-data->MinX());
            return w+g.GetLeftMargin();
        };
        virtual inline double p2x(gGraphWindow & g,int px) {
            double xx=data->MaxX()-data->MinX();
            double wx=px-g.GetLeftMargin();
            double ww=wx/g.Width();
            return data->MinX()+(xx*ww);
        };
        virtual inline int y2p(gGraphWindow & g,double y) {
            double yy=data->MaxY()-data->MinY();
            double h=(g.Height()/yy)*(y-data->MinY());
            return h+g.GetTopMargin();
        };
        virtual inline double p2y(gGraphWindow & g,int py) {
            double yy=data->MaxY()-data->MinY();
            double hy=py-g.GetTopMargin();
            double hh=hy/g.Height();
            return data->MinY()+(yy*hh);
        }; */
        void NotifyGraphWindow(gGraphWindow *g) { m_graph.push_back(g); };
        void SetVisible(bool v) { m_visible=v; };
        bool IsVisible() { return m_visible; };

    protected:
        bool m_visible;
        bool m_movable;
        wxString m_title;
        gPointData *data;   // Data source
        list<gGraphWindow *> m_graph; // notify list of graphs that attach this layer.
};

class gGraphTitle:public gLayer
{
    public:
        gGraphTitle(const wxString & _title,wxOrientation o=wxVERTICAL,const wxColor * color=wxBLACK);
        virtual ~gGraphTitle();
        virtual void Plot(gGraphWindow & w,float scrx,float scry);
        wxOrientation Orientation() { return m_orientation; };
        static const int Margin=20;

    protected:
        wxString m_title;
        wxOrientation m_orientation;
        //wxFont *m_font;
        wxColor *m_color;
        wxCoord m_textheight,m_textwidth;
};

class gCandleStick:public gLayer
{
    public:
        gCandleStick(gPointData *d=NULL,wxOrientation o=wxHORIZONTAL);
        virtual ~gCandleStick();

        virtual void Plot(gGraphWindow & w,float scrx,float scry);
        void AddName(wxString name) { m_names.push_back(name); };

    protected:
        wxOrientation m_direction;
        vector<wxString> m_names;

};

class gXAxis:public gLayer
{
    public:
        gXAxis(const wxColor * col=wxBLACK);
        virtual ~gXAxis();
        virtual void Plot(gGraphWindow & w,float scrx,float scry);
        static const int Margin=40; // How much room does this take up. (Bottom margin)
    protected:
//        virtual const wxString & Format(double v) { static wxString t; wxDateTime d; d.Set(v); t=d.Format(wxT("%H:%M")); return t; };
};
class gYAxis:public gLayer
{
    public:
        gYAxis(const wxColor * col=wxBLACK);
        virtual ~gYAxis();
        virtual void Plot(gGraphWindow & w,float scrx,float scry);
        void SetShowMinorLines(bool b) { m_show_minor_lines=b; };
        void SetShowMajorLines(bool b) { m_show_major_lines=b; };
        bool ShowMinorLines() { return m_show_minor_lines; };
        bool ShowMajorLines() { return m_show_major_lines; };
        virtual const wxString & Format(double v) { static wxString t; t=wxString::Format(wxT("%.1f"),v); return t; };
        static const int Margin=40; // Left margin space
    protected:
        bool m_show_major_lines;
        bool m_show_minor_lines;
  //      virtual const wxString & Format(double v) { static wxString t; t=wxString::Format(wxT("%.1f"),v); return t; };
};
class gFooBar:public gLayer
{
    public:
        gFooBar(const wxColor * color1=wxGREEN,const wxColor * color2=wxDARK_GREY);
        virtual ~gFooBar();
        virtual void Plot(gGraphWindow & w,float scrx,float scry);
        static const int Margin=15;
    protected:
};


class gLineChart:public gLayer
{
    public:
        gLineChart(gPointData *d=NULL,const wxColor * col=wxBLACK,int dlsize=4096,bool accelerate=false,bool _hide_axes=false,bool _square_plot=false);
        virtual ~gLineChart();

        virtual void Plot(gGraphWindow & w,float scrx,float scry);

        void SetSquarePlot(bool b) { m_square_plot=b; };
        bool GetSquarePlot() { return m_square_plot; };
        void ReportEmpty(bool b) { m_report_empty=b; };
        bool GetReportEmpty() { return m_report_empty; };

    protected:
        bool m_accelerate;
        int m_drawlist_size;
        bool m_report_empty;
        wxRealPoint *m_drawlist;
        bool m_hide_axes;
        bool m_square_plot;
        //gYAxis * Yaxis;
        //gFooBar *foobar;

};

enum LO_Type { LOT_Bar, LOT_Dot };
class gLineOverlayBar:public gLayer
{
    public:
        gLineOverlayBar(gPointData *d=NULL,const wxColor * col=wxBLACK,wxString _label=wxT(""),LO_Type _lot=LOT_Bar);
        virtual ~gLineOverlayBar();

        virtual void Plot(gGraphWindow & w,float scrx,float scry);

    protected:
        wxString label;
        LO_Type lo_type;
};

class gFlagsLine:public gLayer
{
    public:
        gFlagsLine(gPointData *d=NULL,const wxColor * col=wxBLACK,wxString _label=wxT(""),int _line_num=0,int _total_lines=0);
        virtual ~gFlagsLine();

        virtual void Plot(gGraphWindow & w,float scrx,float scry);

    protected:
        wxString label;
        int line_num,total_lines;
};


class gBarChart:public gLayer
{
    public:
        gBarChart(gPointData *d=NULL,const wxColor *col=NULL,wxOrientation o=wxHORIZONTAL);
        virtual ~gBarChart();

        virtual void Plot(gGraphWindow & w,float scrx,float scry);

    protected:
        wxOrientation m_direction;

        // d.Set(i+2400000.5+.000001); // JDN vs MJD vs Rounding errors

        virtual const wxString & FormatX(double v) { static wxString t; wxDateTime d; d.Set(v+2400000.5+1); t=d.Format(wxT("%d %b")); return t; };
        //virtual const wxString & FormatX(double v) { static wxString t; wxDateTime d; d.Set(vi+2400000.5); t=d.Format(wxT("%H:%M")); return t; };
        //virtual const wxString & FormatX(double v) { static wxString t; t=wxString::Format(wxT("%.1f"),v); return t; };
        virtual const wxString & FormatY(double v) { static wxString t; t=wxString::Format(wxT("%.1f"),v); return t; };

        gXAxis *Xaxis;
};

class FlagData:public gPointData
{
public:
    FlagData(MachineCode _code,double _value=0,int _field=-1,int _offset=-1);
    virtual ~FlagData();
    virtual void Reload(Day *day=NULL);
protected:
    MachineCode code;
    double value;
    int field;
    int offset;
};

class TAPData:public gPointData
{
public:
    TAPData(MachineCode _code);
    virtual ~TAPData();
    virtual void Reload(Day *day=NULL);

    static const int max_slots=256;
    wxTimeSpan pTime[max_slots];
    MachineCode code;

};

class WaveData:public gPointData
{
public:
    WaveData(MachineCode _code,int _size=250000);
    virtual ~WaveData();
    virtual void Reload(Day *day=NULL);
protected:
    MachineCode code;
};

class EventData:public gPointData
{
public:
    EventData(MachineCode _code,int _field=0,int _size=4096,bool _skipzero=false);
    virtual ~EventData();
    virtual void Reload(Day *day=NULL);
protected:
    MachineCode code;
    int field;
    bool skipzero;
};


class AHIData:public gPointData
{
public:
    AHIData();
    virtual ~AHIData();
    virtual void Reload(Day *day=NULL);
};

class HistoryData:public gPointData
{
public:
    HistoryData(Profile * _profile);
    virtual ~HistoryData();

    void SetProfile(Profile *_profile) { profile=_profile; Reload(); };
    Profile * GetProfile() { return profile; };
    double GetAverage();

    virtual double Calc(Day *day);
    virtual void Reload(Day *day=NULL);
    virtual void ResetDateRange();
    virtual void SetDateRange(wxDateTime start,wxDateTime end);
  //  virtual void Reload(Machine *machine=NULL);
protected:
    Profile * profile;
};

class HistoryCodeData:public HistoryData
{
public:
    HistoryCodeData(Profile *_profile,MachineCode _code);
    virtual ~HistoryCodeData();
    virtual double Calc(Day *day);
protected:
    MachineCode code;
};


enum T_UHD { UHD_Bedtime, UHD_Waketime, UHD_Hours };
class UsageHistoryData:public HistoryData
{
public:
    UsageHistoryData(Profile *_profile,T_UHD _uhd);
    virtual ~UsageHistoryData();
    virtual double Calc(Day *day);
protected:
    T_UHD uhd;
};

void GraphInit();
extern void GraphDone();

#endif // GRAPH_H
