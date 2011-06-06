/*
gGraph Graphing System Implementation

Author: Mark Watkins <jedimark64@users.sourceforge.net>
License: LGPL
*/

#include <wx/settings.h>
#include <wx/dcbuffer.h>
#include <wx/log.h>
#include "graph.h"
#include "sleeplib/profiles.h"

#if !wxCHECK_VERSION(2,9,0)
wxColor zwxYELLOW=wxColor(0xb0,0xb0,0x40,0xff);
wxColor *wxYELLOW=&zwxYELLOW;
#endif
wxColor zwxAQUA=wxColor(0x00,0xaf,0xbf,0xff);
wxColor * wxAQUA=&zwxAQUA;
wxColor zwxPURPLE=wxColor(0xff,0x40,0xff,0xff);
wxColor * wxPURPLE=&zwxPURPLE;
wxColor zwxGREEN2=wxColor(0x40,0xff,0x40,0x5f);
wxColor * wxGREEN2=&zwxGREEN2;
wxColor zwxLIGHT_YELLOW(228,228,168,255);
wxColor *wxLIGHT_YELLOW=&zwxLIGHT_YELLOW;
wxColor zwxDARK_GREEN=wxColor(20,128,20,255);
wxColor *wxDARK_GREEN=&zwxDARK_GREEN;
wxColor zwxDARK_GREY(0xA0,0xA0,0xA0,0xA0);
wxColor *wxDARK_GREY=&zwxDARK_GREY;


const wxColor *gradient_start_color=wxWHITE, *gradient_end_color=wxLIGHT_YELLOW;
wxDirection gradient_direction=wxEAST;
const wxColor *selection_color=wxBLUE; //GREEN2;

gGraphData::gGraphData(int mp,gDataType t)
:vc(0),type(t),max_points(mp)
{
    m_ready=false;
    force_min_y=force_max_y=0;
}
gGraphData::~gGraphData()
{
}
void gGraphData::Update(Day *day)
{
    Reload(day);

    for (list<gLayer *>::iterator i=notify_layers.begin();i!=notify_layers.end();i++) {
        gGraphData *g=this;
        if (!day) g=NULL;
        (*i)->DataChanged(g);
    }
}

gPointData::gPointData(int mp)
:gGraphData(mp,gDT_Point)
{
}
gPointData::~gPointData()
{
    for (vector<wxRealPoint *>::iterator i=point.begin();i!=point.end();i++)
        delete [] (*i);
}
void gPointData::AddSegment(int max_points)
{
    maxsize.push_back(max_points);
    np.push_back(0);
    wxRealPoint *p=new wxRealPoint [max_points];
    point.push_back(p);
}

gPoint3DData::gPoint3DData(int mp)
:gGraphData(mp,gDT_Point3D)
{
}
gPoint3DData::~gPoint3DData()
{
    for (vector<Point3D *>::iterator i=point.begin();i!=point.end();i++)
        delete [] (*i);
}
void gPoint3DData::AddSegment(int mp)
{
    maxsize.push_back(mp);
    np.push_back(0);
    Point3D *p=new Point3D [mp];
    point.push_back(p);
}


gLayer::gLayer(gPointData *d,wxString title)
:m_title(title),data(d)
{
    if (data) {
        data->AddLayer(this);
    }
    m_visible = true;
    m_movable = false;
    color.push_back(wxRED);
    color.push_back(wxGREEN);
}
gLayer::~gLayer()
{

}

void gLayer::Plot(wxDC & dc, gGraphWindow & w)
{
}



IMPLEMENT_DYNAMIC_CLASS(gGraphWindow, wxWindow)

BEGIN_EVENT_TABLE(gGraphWindow, wxWindow)
    EVT_PAINT       (gGraphWindow::OnPaint)
    EVT_SIZE        (gGraphWindow::OnSize)
    EVT_MOTION      (gGraphWindow::OnMouseMove )
    EVT_LEFT_DOWN   (gGraphWindow::OnMouseLeftDown)
    EVT_LEFT_UP     (gGraphWindow::OnMouseLeftRelease)
    EVT_RIGHT_DOWN  (gGraphWindow::OnMouseRightDown)
    EVT_RIGHT_UP    (gGraphWindow::OnMouseRightRelease)
    //EVT_MOUSEWHEEL (gGraphWindow::OnMouseWheel )
    //EVT_MIDDLE_DOWN (gGraphWindow::OnMouseRightDown)
    //EVT_MIDDLE_UP   (gGraphWindow::OnMouShowPopupMenu)


END_EVENT_TABLE()

gGraphWindow::gGraphWindow(wxWindow *parent, wxWindowID id,const wxString & title,const wxPoint &pos,const wxSize &size,long flags)
: wxWindow( parent, id, pos, size, flags, title )
{
    m_scrX   = m_scrY   = 64;
    m_title=title;
    m_mouseRDown=m_mouseLDown=false;
    SetBackgroundColour( *wxWHITE );
    m_bgColour = *wxWHITE;
    m_fgColour = *wxBLACK;
    SetMargins(10, 15, 46, 80);
    m_block_move=false;
    m_block_zoom=false;


}
gGraphWindow::~gGraphWindow()
{
    for (list<gLayer *>::iterator l=layers.begin();l!=layers.end();l++) delete (*l);
    layers.clear();
}


void gGraphWindow::AddLayer(gLayer *l) {
    if (l) {
        l->NotifyGraphWindow(this);
        layers.push_back(l);
    }
};


// Sets a new Min & Max X clipping, refreshing the graph and all it's layers.
void gGraphWindow::SetXBounds(double minx, double maxx)
{
    //min_x=minx;
    //max_x=maxx;
    SetMinX(minx);
    SetMaxX(maxx);
    Refresh(false);
}
void gGraphWindow::ResetXBounds()
{
    //min_x=minx;
    //max_x=maxx;
    SetMinX(RealMinX());
    SetMaxX(RealMaxX());
    Refresh(false);
}

void gGraphWindow::ZoomXPixels(int x1, int x2)
{
    double rx1=0,rx2=0;
    ZoomXPixels(x1,x2,rx1,rx2);
    for (list<gGraphWindow *>::iterator g=link_zoom.begin();g!=link_zoom.end();g++) {
        (*g)->SetXBounds(rx1,rx2);
    }

    if (m_block_zoom) {
        RefreshRect(m_mouseRBrect,false);
    } else {
        SetXBounds(rx1,rx2);
    }
}
void gGraphWindow::ZoomXPixels(int x1,int x2,double &rx1,double &rx2)
{
    x1-=GetLeftMargin();
    x2-=GetLeftMargin();
    if (x1<0) x1=0;
    if (x2<0) x2=0;
    if (x1>Width()) x1=Width();
    if (x2>Width()) x2=Width();

    double min=min_x;
    double max=max_x;
    double q=max-min;
    rx1=min+(double(x1)/Width()) * q;
    rx2=min+(double(x2)/Width()) * q;
}

// Move x-axis by the amount of space represented by integer i Pixels (negative values moves backwards)
void gGraphWindow::MoveX(int i,double &min, double & max)
{
    //if (i==0) return;
    min=min_x;
    max=max_x;
    double q=max-min;
    double rx1=(double(i)/Width()) * q;
    min-=rx1;
    max-=rx1;

    // Keep bounds when hitting hard edges
    if (min<rmin_x) { //(t=rRealMinX())) {
        min=rmin_x;
        max=min+q;
    }
    if (max>rmax_x) {
        max=rmax_x;
        min=max-q;
    }
}

void gGraphWindow::MoveX(int i)
{
    double min,max;
    MoveX(i,min,max);

    for (list<gGraphWindow *>::iterator g=link_zoom.begin();g!=link_zoom.end();g++) {
        (*g)->SetXBounds(min,max);
    }
    if (!m_block_zoom) SetXBounds(min,max);
}
void gGraphWindow::ZoomX(double mult,int origin_px)
{
    if (origin_px==0) origin_px=(Width()/2); else origin_px-=GetLeftMargin();

    if (origin_px<0) origin_px=0;
    if (origin_px>Width()) origin_px=Width();

    double min=min_x;
    double max=max_x;

    double hardspan=rmax_x-rmin_x;
    double span=max-min;
    double origin=double(origin_px) / Width() * span;

    double q=span*mult;
    if (q>hardspan) q=hardspan;

    min=min+(origin-(q/2.0));
    max=min+q;

    if (min<rmin_x) {
        min=rmin_x;
        max=min+q;
    }
    if (max>rmax_x) {
        max=rmax_x;
        min=max-q;
    }
    SetXBounds(min,max);
}

void gGraphWindow::OnMouseMove(wxMouseEvent &event)
{
//    static bool first=true;
    static wxRect last;
    if (event.m_rightDown) {
        MoveX(event.GetX() - m_mouseRClick.x);
        m_mouseRClick.x=event.GetX();
    } else
    if (event.m_leftDown) {

        int x1=m_mouseLClick.x;
        int x2=event.GetX();
        int t1=MIN(x1,x2);
        int t2=MAX(x1,x2);
        if (t1<=m_marginLeft) t1=m_marginLeft+1;
        if (t2>(m_scrX-m_marginRight)) t2=m_scrX-m_marginRight;

        wxRect r(t1, m_marginTop, t2-t1, m_scrY-m_marginBottom-m_marginTop);

        m_mouseRBlast=m_mouseRBrect;
        m_mouseRBrect=r;

        RefreshRect(r.Union(m_mouseRBlast),false);

    }
    event.Skip();
}
void gGraphWindow::OnMouseRightDown(wxMouseEvent &event)
{
    m_mouseRClick.x = event.GetX();
    m_mouseRClick.y = event.GetY();

    m_mouseRClick_start=m_mouseRClick;
    m_mouseRDown=true;

    event.Skip();
}
//voiid ZoomX
void gGraphWindow::OnMouseRightRelease(wxMouseEvent &event)
{
    double zoom_fact=2;
    if (event.ControlDown()) zoom_fact=5.0;
    if (abs(event.GetX()-m_mouseRClick_start.x)<3 && abs(event.GetY()-m_mouseRClick_start.y)<3) {
        for (list<gGraphWindow *>::iterator g=link_zoom.begin();g!=link_zoom.end();g++) {
            (*g)->ZoomX(zoom_fact,0);
        }
        if (!m_block_zoom) {
            ZoomX(zoom_fact,0); //event.GetX()); // adds origin to zoom out.. Doesn't look that cool.
        }
    }
    m_mouseRDown=false;

    event.Skip();
}
void gGraphWindow::OnMouseLeftDown(wxMouseEvent &event)
{
    m_mouseLClick.x = event.GetX();
    m_mouseLClick.y = event.GetY();

    m_mouseLDown=true;
    event.Skip();
}
void gGraphWindow::OnMouseLeftRelease(wxMouseEvent &event)
{
    wxPoint release(event.GetX(), m_scrY-m_marginBottom);
    wxPoint press(m_mouseLClick.x, m_marginTop);
    m_mouseLDown=false;
    wxDateTime a,b;
    int x1=m_mouseRBrect.x;
    int x2=x1+m_mouseRBrect.width;
    int t1=MIN(x1,x2);
    int t2=MAX(x1,x2);

    wxRect r;
    if ((t2-t1)>3) {
        ZoomXPixels(t1,t2);
    } else {
        double zoom_fact=0.5;
        if (event.ControlDown()) zoom_fact=0.25;
        for (list<gGraphWindow *>::iterator g=link_zoom.begin();g!=link_zoom.end();g++) {
            (*g)->ZoomX(zoom_fact,event.GetX());
        }
        if (!m_block_zoom) {
            ZoomX(zoom_fact,event.GetX()); //event.GetX()); // adds origin to zoom out.. Doesn't look that cool.
        }
    }

    r=wxRect(0, 0, 0, 0);


    m_mouseRBrect=r;
    event.Skip();
}


void gGraphWindow::Update()
{
    Refresh();
}
void gGraphWindow::SetMargins(int top, int right, int bottom, int left)
{
    m_marginTop=top;
    m_marginBottom=bottom;
    m_marginLeft=left;
    m_marginRight=right;
}


void gGraphWindow::OnPaint(wxPaintEvent& WXUNUSED(event))
{
    #if defined(__WXMSW__)
    wxAutoBufferedPaintDC dc(this);
    #else
    wxPaintDC dc(this);
    #endif
    GetClientSize(&m_scrX, &m_scrY);

    dc.SetPen( *wxTRANSPARENT_PEN );

    wxBrush brush( GetBackgroundColour() );
    dc.SetBrush( brush );

	dc.SetTextForeground(m_fgColour);

    wxRect r=wxRect(0,0,m_scrX,m_scrY);

	dc.GradientFillLinear(r,*gradient_start_color,*gradient_end_color,gradient_direction);
	//dc.DrawRectangle(0,0,m_scrX,m_scrY);

    //wxLogMessage(wxT("Paint"));
    //dc.DrawText(m_title,m_marginLeft,3);
    for (list<gLayer *>::iterator l=layers.begin();l!=layers.end();l++) {
        (*l)->Plot(dc,*this);
    }

    static wxPen pen(*wxBLACK, 1, wxSOLID);
    static wxBrush brush2(*selection_color,wxFDIAGONAL_HATCH);

    if (m_mouseLDown) {
        dc.SetPen(pen);
        //dc.SetBrush(brush);
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        if (m_mouseRBrect.width>0)
            dc.DrawRectangle(m_mouseRBrect);
    }

}
void gGraphWindow::OnSize(wxSizeEvent& event)
{
    GetClientSize( &m_scrX,&m_scrY);
    Refresh();
}

double gGraphWindow::MinX()
{
    //static bool f=true; //need a bool for each one, and reset when a layer reports data change.
    //if (!f) return min_x;
    //f=false;

    bool first=true;
    double val,tmp;
    for (list<gLayer *>::iterator l=layers.begin();l!=layers.end();l++) {
        if (first) {
            val=(*l)->MinX();
            if (!((val==(*l)->MaxX()) && (val==0)))
                first=false;
        } else {
            tmp=(*l)->MinX();
            if (!((tmp==(*l)->MaxX()) && (tmp==0))) {
                if (tmp < val) val = tmp;
            }
        }
    }

    return min_x=val;
}
double gGraphWindow::MaxX()
{
    //static bool f=true;
    //if (!f) return max_x;
    //f=false;

    bool first=true;
    double val,tmp;
    for (list<gLayer *>::iterator l=layers.begin();l!=layers.end();l++) {
        if (first) {
            val=(*l)->MaxX();
            if (!((val==(*l)->MinX()) && (val==0)))
                first=false;

        } else {
            tmp=(*l)->MaxX();
            if (!((tmp==(*l)->MinX()) && (tmp==0))) {
                if (tmp > val) val = tmp;
            }
        }
    }
    return max_x=val;
}
double gGraphWindow::MinY()
{
    //static bool f=true;
    //if (!f) return min_y;
    //f=false;

    bool first=true;
    double val,tmp;
    for (list<gLayer *>::iterator l=layers.begin();l!=layers.end();l++) {
        if (first) {
            val=(*l)->MinY();
            if (!((val==(*l)->MaxY()) && (val==0)))
                first=false;
        } else {
            tmp=(*l)->MinY();
            if (!((tmp==(*l)->MaxY()) && (tmp==0))) { // Ignore this layer if both are 0
                if (tmp < val) val = tmp;
            }
        }
    }
    return min_y=val;
}
double gGraphWindow::MaxY()
{
    //static bool f=true;
    //if (!f) return max_y;
    //f=false;

    bool first=true;
    double val,tmp;
    for (list<gLayer *>::iterator l=layers.begin();l!=layers.end();l++) {
        if (first) {
            val=(*l)->MaxY();
            if (!((val==(*l)->MinY()) && (val==0)))
                first=false;
        } else {
            tmp=(*l)->MaxY();
            if (!((tmp==(*l)->MinY()) && (tmp==0))) { // Ignore this layer if both are 0
                if (tmp > val) val = tmp;
            }
        }
    }
    return max_y=val;
}

double gGraphWindow::RealMinX()
{
    //static bool f=true;
    //if (!f) return rmin_x;
    //f=false;

    bool first=true;
    double val,tmp;
    for (list<gLayer *>::iterator l=layers.begin();l!=layers.end();l++) {
        if (first) {
            val=(*l)->RealMinX();
            if (!((val==(*l)->RealMaxX()) && (val==0)))
                first=false;
        } else {
            tmp=(*l)->RealMinX();
            if (!((tmp==(*l)->RealMaxX()) && (tmp==0))) { // Ignore this layer if both are 0
                if (tmp < val) val = tmp;
            }
        }
    }
    return rmin_x=val;
}
double gGraphWindow::RealMaxX()
{
    //static bool f=true;
    //if (!f) return rmax_x;
    //f=false;

    bool first=true;
    double val,tmp;
    for (list<gLayer *>::iterator l=layers.begin();l!=layers.end();l++) {
        if (first) {
            val=(*l)->RealMaxX();
            if (!((val==(*l)->RealMinX()) && (val==0)))
                first=false;
        } else {
            tmp=(*l)->RealMaxX();
            if (!((tmp==(*l)->RealMinX()) && (tmp==0))) { // Ignore this layer if both are 0
                if (tmp > val) val = tmp;
            }
        }
    }
    return rmax_x=val;
}
double gGraphWindow::RealMinY()
{
    //static bool f=true;
    //if (!f) return rmin_y;
    //f=false;

    bool first=true;
    double val,tmp;
    for (list<gLayer *>::iterator l=layers.begin();l!=layers.end();l++) {
        if (first) {
            val=(*l)->RealMinY();
            if (!((val==(*l)->RealMaxY()) && (val==0)))
                first=false;
        } else {
            tmp=(*l)->RealMinY();
            if (!((tmp==(*l)->RealMaxY()) && (tmp==0))) { // Ignore this if both are 0
                if (tmp < val) val = tmp;
            }
        }
    }
    return rmin_y=val;
}
double gGraphWindow::RealMaxY()
{
    //static bool f=true;
    //if (!f) return rmax_y;
    //f=false;

    bool first=true;
    double val,tmp;
    for (list<gLayer *>::iterator l=layers.begin();l!=layers.end();l++) {
        if (first) {
            val=(*l)->RealMaxY();
            if (!((val==(*l)->RealMinY()) && (val==0))) // Does this create a loop??
                first=false;
        } else {
            tmp=(*l)->RealMaxY();
            if (!((tmp==(*l)->RealMinY()) && (tmp==0))) { // Ignore this if both are 0
                if (tmp > val) val = tmp;
            }
        }
    }
    return rmax_y=val;
}

void gGraphWindow::SetMinX(double v)
{
    min_x=v;
    for (list<gLayer *>::iterator l=layers.begin();l!=layers.end();l++) {
        (*l)->SetMinX(v);
    }
}
void gGraphWindow::SetMaxX(double v)
{
    max_x=v;
    for (list<gLayer *>::iterator l=layers.begin();l!=layers.end();l++) {
        (*l)->SetMaxX(v);
    }
}
void gGraphWindow::SetMinY(double v)
{
    min_y=v;
    for (list<gLayer *>::iterator l=layers.begin();l!=layers.end();l++) {
        (*l)->SetMinY(v);
    }
}
void gGraphWindow::SetMaxY(double v)
{
    max_y=v;
    for (list<gLayer *>::iterator l=layers.begin();l!=layers.end();l++) {
        (*l)->SetMaxY(v);
    }
}

void gGraphWindow::DataChanged(gLayer *layer)
{
    // This is possibly evil.. It needs to push one refresh event for all layers

    // Assmption currently is Refresh que does skip
    if (layer) {
        MinX(); MinY(); MaxX(); MaxY();
        RealMinX(); RealMinY(); RealMaxX(); RealMaxY();
    } else {
        max_x=min_x=0;
    }

    Refresh(false);
}


gXAxis::gXAxis(const wxColor * col)
:gLayer(NULL)
{
    if (col) {
        color.clear();
        color.push_back(col);
    }
}
gXAxis::~gXAxis()
{
}
void gXAxis::Plot(wxDC & dc, gGraphWindow & w)
{
    float px,py;
    wxCoord x,y;

    int scrx = w.GetScrX();
    int scry = w.GetScrY();
    //wxString label=wxString::Format(wxT("%i %i"),scrx,scry);
    //dc.DrawText(label,0,0);

    int start_px=w.GetLeftMargin();
    int start_py=w.GetTopMargin();
    int width=scrx-(start_px+w.GetRightMargin());
    int height=scry-(start_py+w.GetBottomMargin());

    wxPen pen=wxPen(*wxBLACK,1,wxSOLID);  //color[0]
    dc.SetPen(pen);
    dc.SetTextForeground(*wxBLACK);

    double xx=w.max_x-w.min_x;

    if (xx==0) return;
    //wxDateTime d;
    wxString fd;
    if (xx<1.5) {
        fd=wxT("00:00:00:0000");
    } else {
        fd=wxT("XX XXX");
    }
    dc.GetTextExtent(fd,&x,&y);
    double max_ticks=(x+25.0)/width; // y+50 for rotated text
    double jj=1/max_ticks;
    double minor_tick=max_ticks*xx;
    double st2=w.min_x; //double(int(frac*1440.0))/1440.0;
    double st,q;

    bool show_seconds=false;
    bool show_milliseconds=false;
    bool show_time=true;

    double min_tick;
    if (xx<1.5) {
        int rounding[16]={12,24,48,72,96,144,288,720,1440,2880,5760,8640,17280,86400,172800,345600}; // time rounding

        int ri;
        for (ri=0;ri<16;ri++) {
            st=round(st2*rounding[ri])/rounding[ri];
            min_tick=round(minor_tick*rounding[ri])/rounding[ri];
            q=xx/min_tick;  // number of ticks that fits in range
            if (q<=jj) break; // compared to number of ticks that fit on screen.
        }
        if (ri>8) show_seconds=true;
        if (ri>=14) show_milliseconds=true;

        if (min_tick<=0.25/86400.0)
            min_tick=0.25/86400;
    } else { // Day ticks..
        show_time=false;
        st=st2;
        min_tick=1.0;
        double mtiks=(x+20.0)/width;
        double mt=mtiks*xx;
        min_tick=mt;
        //if (min_tick<1.0) min_tick=1.0;
        //if (min_tick>10) min_tick=10;
    }

   // dc.SetPen(*wxBLACK_PEN);


    dc.SetClippingRegion(start_px-10,start_py+height,width+20,w.GetBottomMargin());
    double st3=st;
    while (st3>w.min_x) {
        st3-=min_tick/10.0;
    }
    st3+=min_tick/10.0;

    py=start_py+height;

    for (double i=st3; i<=w.max_x; i+=min_tick/10.0) {
        if (i<w.min_x) continue;
        //px=x2p(w,i);
        px=w.x2p(i); //w.GetLeftMargin()+((i - w.min_x) * xmult);
		dc.DrawLine(px,py,px,py+4);
    }

    //st=st3;

    while (st<w.min_x) {
        st+=min_tick; //10.0;  // mucking with this changes the scrollyness of the ticker.
    }

    int hour,minute,second,millisecond;
    wxDateTime d;

    for (double i=st; i<=w.max_x; i+=min_tick) { //600.0/86400.0) {
        d.Set(i+2400000.5+.00000001+1); // JDN vs MJD vs Rounding errors

        if (show_time) {
            minute=d.GetMinute();
            hour=d.GetHour();
            second=d.GetSecond();
            millisecond=d.GetMillisecond();

            if (show_milliseconds) {
                fd=wxString::Format(wxT("%02i:%02i:%02i:%04i"),hour,minute,second,millisecond);
            } else if (show_seconds) {
                fd=wxString::Format(wxT("%02i:%02i:%02i"),hour,minute,second);
            } else {
                fd=wxString::Format(wxT("%02i:%02i"),hour,minute);
            }
        } else {
            fd=d.Format(wxT("%d %b"));
        }

        px=w.x2p(i);
		dc.DrawLine(px,py,px,py+6);
		//dc.DrawLine(px+1,py,px+1,py+6);
        y=x=0;
        dc.GetTextExtent(fd,&x,&y); // This doesn't work properly on windows.

        // There is a wx2.8 bug in wxMSW that screws up calculating x properly.
        const int offset=0;

        if (!show_time) {
            dc.DrawRotatedText(fd, px-(y/2)+2, py+x+16+offset, 90.0);

        } else {
            dc.DrawText(fd, px-(x/2), py+y);
        }

    }
    dc.DestroyClippingRegion();

}


gYAxis::gYAxis(const wxColor * col)
:gLayer(NULL)
{
    if (col) {
        color.clear();
        color.push_back(col);
    }
    m_show_major_lines=true;
    m_show_minor_lines=true;
}
gYAxis::~gYAxis()
{
}
void gYAxis::Plot(wxDC & dc,gGraphWindow &w)
{
    static wxColor wxDARK_GREY(0xA0,0xA0,0xA0,0xA0);
    static wxPen pen1(*wxLIGHT_GREY, 1, wxDOT);
    static wxPen pen2(wxDARK_GREY, 1, wxDOT);
    wxCoord x,y,labelW=0;

    int scrx = w.GetScrX();
    int scry = w.GetScrY();
    //wxString label=wxString::Format(wxT("%i %i"),scrx,scry);
    //dc.DrawText(label,0,0);

    double miny=w.min_y;
    double maxy=w.max_y;
    if (maxy==miny)
        return;
    if ((w.max_x-w.min_x)==0) return;

    int start_px=w.GetLeftMargin();
    int start_py=w.GetTopMargin();
    int width=scrx-(w.GetLeftMargin()+w.GetRightMargin());
    int height=scry-(w.GetTopMargin()+w.GetBottomMargin());

    dc.SetPen(*wxBLACK_PEN);

    wxString fd=wxT("0");
    dc.GetTextExtent(fd,&x,&y);
    double max_yticksdiv=(y+15.0)/(height); // y+50 for rotated text
    double max_yticks=1/max_yticksdiv;
    double yy=w.max_y-w.min_y;
    double ymult=height/yy;
    double major_ytick=max_yticksdiv*yy;

    double min_ytick,q;

    if (w.min_y>=0) {
        int yrounding[9]={1,2,5,10,20,30,40,50,100}; // time rounding
        int ry;
        for (ry=0;ry<9;ry++) {
       // st=round(st2*rounding[ry])/rounding[ry];
            min_ytick=round(major_ytick*yrounding[ry])/yrounding[ry];
            q=yy/min_ytick;  // number of ticks that fits in range
            if (q<=max_yticks) break; // compared to number of ticks that fit on screen.
        }
    } else {
        min_ytick=60;
    }
    if (min_ytick<=0.25)
        min_ytick=0.25;

    int ty,h;
    for (float i=w.min_y; i<w.max_y; i+=min_ytick/2) {
		ty=(i - w.min_y) * ymult;
        h=(start_py+height)-ty;
    	dc.DrawLine(start_px-4, h, start_px, h);
    }
    dc.SetPen(pen1);
    for (double i=w.min_y; i<w.max_y; i+=min_ytick/2) {
		ty=(i - w.min_y) * ymult;
        h=(start_py+height)-ty;
        if (m_show_minor_lines && (i > w.min_y))
            dc.DrawLine(start_px+1,h,start_px+width,h);
    }

    for (double i=w.min_y; i<=w.max_y; i+=min_ytick) {
		ty=(i - w.min_y) * ymult;
        fd=Format(i); // Override this as a function.
        dc.GetTextExtent(fd,&x,&y);
        if (x>labelW) labelW=x;
        h=(start_py+height)-ty;
        dc.DrawText(fd,start_px-8-x,h - (y / 2));
        dc.SetPen(*wxBLACK_PEN);
		dc.DrawLine(start_px-6,h,start_px,h);
        dc.SetPen(pen2);
        if (m_show_major_lines && (i > w.min_y))
            dc.DrawLine(start_px+1,h,start_px+width,h);
	}
	dc.GetTextExtent(w.Title(),&x,&y);
    dc.DrawRotatedText(w.Title(), start_px-8-labelW - y, start_py+((height + x)>>1), 90);
}

gFooBar::gFooBar(const wxColor * col,const wxColor * col2)
:gLayer(NULL)
{
    if (col && col2) {
        color.clear();
        color.push_back(col);
        color.push_back(col2);
    }
}
gFooBar::~gFooBar()
{
}
void gFooBar::Plot(wxDC & dc, gGraphWindow & w)
{
    if (!m_visible) return;

    double xx=w.max_x-w.min_x;
    if (xx==0)
        return;

    int scrx = w.GetScrX();
    int scry = w.GetScrY();

    int start_px=w.GetLeftMargin();
    int start_py=w.GetTopMargin();
    int width=scrx-(w.GetLeftMargin()+w.GetRightMargin());
    int height=scry-(w.GetTopMargin()+w.GetBottomMargin());

    wxPen pen2(*color[0], 1, wxDOT);
    wxPen pen3(*color[1], 2, wxSOLID);

    dc.SetPen( pen2 );
    dc.DrawLine(start_px, start_py+height+10, start_px+width, start_py+height+10);
    double rmx=w.rmax_x-w.rmin_x;
    double px=((1/rmx)*(w.min_x-w.rmin_x))*width;
    double py=((1/rmx)*(w.max_x-w.rmin_x))*width;
    dc.SetPen(pen3);
    dc.DrawLine(start_px+px, start_py+height+10, start_px+py, start_py+height+10);
    dc.DrawLine(start_px+px, start_py+height+8, start_px+px, start_py+height+12);
    dc.DrawLine(start_px+py, start_py+height+8, start_px+py, start_py+height+12);
}

gCandleStick::gCandleStick(gPointData *d,wxOrientation o)
:gLayer(d)
{
    m_direction=o;
}
gCandleStick::~gCandleStick()
{
}
void gCandleStick::Plot(wxDC & dc, gGraphWindow & w)
{
    if (!m_visible) return;
    if (!data) return;
    if (!data->IsReady()) return;

    int scrx = w.GetScrX();
    int scry = w.GetScrY();
    dc.SetPen( *wxBLACK_PEN );
    //wxString label=wxString::Format(wxT("%i %i"),scrx,scry);
    //dc.DrawText(label,0,0);

    int start_px=w.GetLeftMargin();
    int start_py=w.GetTopMargin();
    int width=scrx-(w.GetLeftMargin()+w.GetRightMargin());
    int height=scry-(w.GetTopMargin()+w.GetBottomMargin());

    float sum=0;
    for (int i=0;i<data->np[0];i++)
        sum+=data->point[0][i].y;

    float pxr;
    float px;
    if (m_direction==wxVERTICAL) {
        pxr=height/sum;
        px=start_py;
    } else {
        pxr=width/sum;
        px=start_px;
    }

    int x,y;
    dc.GetTextExtent(w.Title(),&x,&y);
    dc.DrawText(w.Title(),start_px,0);

    double t1,t2;
    int barwidth=25;
    int textX, textY;

    wxString str;
    for (int i=0;i<data->np[0];i++) {
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        t1=floor(px);
        t2=data->point[0][i].y*pxr;
        px+=t2;
        t2=ceil(t2)+1;
        wxRect rect;
        wxDirection dir;
        if (m_direction==wxVERTICAL) {
            rect=wxRect(start_px,t1,barwidth,t2);
            dir=wxEAST;
        } else {
            rect=wxRect(t1,start_py,t2,barwidth);
            dir=wxSOUTH;
        }
        dc.SetTextForeground(*wxBLACK);
        dc.SetPen(*wxBLACK_PEN);
        dc.GradientFillLinear(rect,*color[i % color.size()],*wxLIGHT_GREY,dir);
        dc.DrawRectangle(rect);
        str=wxT("");
        if ((int)m_names.size()>i) {
            str=m_names[i]+wxT(" ");
        }
        str+=wxString::Format(wxT("%0.2f"),data->point[0][i].x);
        dc.GetTextExtent(str, &textX, &textY);
        if (t2>textX+5) {
            int j=t1+((t2/2)-(textX/2));
            if (m_direction==wxVERTICAL) {
                dc.DrawRotatedText(str,start_px+barwidth+2+textY,j,270);
            } else {
                dc.DrawText(str,j,start_py+(barwidth/2)-(textY/2));
            }
        }

    }
    dc.SetTextForeground(*wxBLACK); //WHITE_PEN);

}

gBarChart::gBarChart(gPointData *d,const wxColor *col,wxOrientation o)
:gLayer(d),m_direction(o)
{
    if (col) {
        color.clear();
        color.push_back(col);
    }
    Xaxis=new gXAxis(wxBLACK);
    Yaxis=new gYAxis(wxBLACK);
    foobar=new gFooBar();

}
gBarChart::~gBarChart()
{
    delete foobar;
    delete Yaxis;
    delete Xaxis;
}

void gBarChart::Plot(wxDC & dc, gGraphWindow & w)
{
    if (!m_visible) return;
    if (!data) return;
    if (!data->IsReady()) return;

    int scrx = w.GetScrX();
    int scry = w.GetScrY();

    int start_px=w.GetLeftMargin();
    int start_py=w.GetTopMargin();
    int width=scrx-(w.GetLeftMargin()+w.GetRightMargin());
    int height=scry-(w.GetTopMargin()+w.GetBottomMargin());

    double xx=w.max_x - w.min_x;
    double days=int(xx);
    //days=data->np[0];

    days=0;
    for (int i=0;i<data->np[0];i++) {
       if ((data->point[0][i].x >= w.min_x) && (data->point[0][i].x<w.max_x)) days+=1;
    }
    if (days==0) return;

    Yaxis->Plot(dc,w);
    foobar->Plot(dc,w);

    dc.SetPen( *wxBLACK_PEN );

    float barwidth,pxr;
    float px;//,py;

    if (m_direction==wxVERTICAL) {
        barwidth=(height-days)/float(days);
        pxr=width/w.max_y;
        px=start_py;
    } else {
        barwidth=(width-days)/float(days);
        pxr=height/w.max_y;
        px=start_px;
    }
    px+=1;
    int t1,t2;
    int u1,u2;
    int textX, textY;


    wxString str;
    bool draw_xticks_instead=false;

    for (int i=0;i<data->np[0];i++) {
        if (data->point[0][i].x < w.min_x) continue;
        if (data->point[0][i].x >= w.max_x) break;
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        t1=px;
        px+=barwidth+1;
        t2=px-t1-1;

        wxRect rect;
        wxDirection dir;

        u2=data->point[0][i].y*pxr;
        u1=(start_py+height)-u2;

        if (m_direction==wxVERTICAL) {
            rect=wxRect(start_px,t1,u2,t2);
        } else {
            rect=wxRect(t1,u1,t2,u2);
        }
        dir=wxEAST;
        dc.GradientFillLinear(rect,*color[0],*wxLIGHT_GREY,dir);
        dc.DrawRectangle(rect);

        str=FormatX(data->point[0][i].x);
        textX=textY=0;
        dc.GetTextExtent(str, &textX, &textY);
        if (t2>textY) {
            int j=t1+((t2/2)-(textY/2));
            if (m_direction==wxVERTICAL) {
                dc.DrawRotatedText(str,start_px-textX-8,j,0);
            } else {
                dc.DrawRotatedText(str,j,start_py+height+16+textX,90);
            }
        } else draw_xticks_instead=true;

    }
    if (draw_xticks_instead)
        Xaxis->Plot(dc,w);

    dc.DrawLine(start_px,start_py,start_px,start_py+height);

    dc.DrawLine(start_px,start_py+height,start_px+width,start_py+height);
}

gLineChart::gLineChart(gPointData *d,const wxColor * col,int dlsize,bool _accelerate,bool _hide_axes,bool _square_plot)
:gLayer(d),m_accelerate(_accelerate),m_drawlist_size(dlsize),m_hide_axes(_hide_axes),m_square_plot(_square_plot)
{
    m_drawlist=new wxPoint [dlsize];
    color.clear();
    color.push_back(col);
    foobar=new gFooBar();
    Yaxis=new gYAxis(wxBLACK);
    Yaxis->SetShowMajorLines(true);
    Yaxis->SetShowMinorLines(true);
}
gLineChart::~gLineChart()
{
    delete Yaxis;
    delete foobar;
    delete [] m_drawlist;
}

// Time Domain Line Chart
void gLineChart::Plot(wxDC & dc, gGraphWindow & w)
{
    int dp;
    double px,py;


    if (!m_visible) return;
    if (!data) return;
    if (!data->IsReady()) return;
    //if (!data->NP()) return;

    int scrx = w.GetScrX();
    int scry = w.GetScrY();
    //wxString label=wxString::Format(wxT("%i %i"),scrx,scry);
    //dc.DrawText(label,0,0);

    int start_px=w.GetLeftMargin();
    int start_py=w.GetTopMargin();
    int width=scrx-(w.GetLeftMargin()+w.GetRightMargin())-1;
    int height=scry-(w.GetTopMargin()+w.GetBottomMargin())-1;

    double minx=w.min_x,miny=w.min_y;
    double maxx=w.max_x,maxy=w.max_y;
    double xx=maxx-minx;
    double yy=maxy-miny;
    double xmult=width/xx;
    double ymult=height/yy;
    if ((xx<0) || (yy<0)) return;
    if ((yy==0) && (w.min_y==0)) return;
   // assert(xx>=0);

    static wxPen pen1(*wxLIGHT_GREY, 1, wxDOT);

    dc.SetPen( *wxBLACK_PEN );
    dc.DrawLine(start_px,start_py,start_px,start_py+height+1);
    dc.DrawLine(start_px,start_py+height+1,start_px+width+1,start_py+height+1);
   // dc.DrawLine(start_px,start_py,start_px+width,start_py);
    dc.DrawLine(start_px+width+1,start_py,start_px+width+1,start_py+height+1);


    foobar->Plot(dc,w);

    if (!m_hide_axes) {
        Yaxis->Plot(dc,w);
    }

    wxPen pen(*color[0], 1, wxSOLID);
    dc.SetPen(pen);
    bool accel=m_accelerate;

    double s1,s2,sr;
    double sfit,sam;
    for (int n=0;n<data->VC();n++) {
        dp=0;
        bool done=false;
        bool first=true;
        wxRealPoint *point=data->point[n];
        if (accel) {
            s1=point[0].x;
            s2=point[1].x;
            sr=s2-s1;
            sfit=xx/sr;
            sam=sfit/width;
            if (sam<=8) { // Don't accelerate if threshold less than this.
                accel=false;
                sam=1;
            } else {
                sam/=25;   // lower this number the more data is skipped over (and the faster things run)
                if (sam<=1) {
                    sam=1;
                 //accel=false;
                }
            }

        } else sam=1;

        int siz=data->np[n];
        if (accel) {
            for (int i=0;i<width;i++) {
                m_drawlist[i].x=height;
                m_drawlist[i].y=0;
            }
        }
        int minz=width,maxz=0;

        bool reverse=false;
        if (point[0].x>point[siz-1].x) reverse=true;

        for (int i=0;i<siz;i+=sam) { //,done==false
            if (!reverse) {
                if (point[i].x < minx) continue;
                if (first) {
                    first=false;
                    if (i>=sam)  i-=sam;
                }

                if (point[i].x > maxx) done=true;
            } else {
                if (point[i].x > maxx) continue;
                if (first) {
                    first=false;
                    if (i>=sam)  i-=sam;
                }
                if (point[i].x < minx) done=true;

            }

            px=(point[i].x - minx) * xmult;
            py=height - ((point[i].y - miny) * ymult);

            // Can't avoid this.. SetClippingRegion does not work without crashing.. need to find a workaround:(
            if (px<0) {
                px=0;
            }
            if (px>width) {
                px=width;
            }

            if (accel) {
                int z=round(px);
                if (z<minz) minz=z;
                if (z>maxz) maxz=z;
                if (py<m_drawlist[z].x) m_drawlist[z].x=py;
                if (py>m_drawlist[z].y) m_drawlist[z].y=py;
            } else {
                if (m_square_plot && (dp>0)) { // twice as many points needed
                    m_drawlist[dp].x=start_px+px;
                    m_drawlist[dp].y=m_drawlist[dp-1].y;
                    if (++dp>=m_drawlist_size) {
                        wxLogWarning(wxT("gLineChart: m_drawlist is too small"));
                        break;
                    }
                }
                m_drawlist[dp].x=start_px+px;
                m_drawlist[dp].y=start_py+py;
                if (++dp>=m_drawlist_size) {
                    wxLogWarning(wxT("gLineChart: m_drawlist is too small"));
                    break;
                }
            }
            if (done) break;
        }

        dc.SetClippingRegion(start_px+1,start_py-1,width,height+2);
        if (accel) {
            // dc.DrawLine(1, 1, 1, height);
            dp=0;
            for (int i=minz;i<=maxz;i++) {
                int y1=m_drawlist[i].x;
                int y2=m_drawlist[i].y;
                screen[dp].x=start_px+i;
                screen[dp].y=start_py+y1;
                dp++;
                screen[dp].x=start_px+i;
                screen[dp].y=start_py+y2;
                dp++;
                //dc.DrawLine(start_px+i, start_py+, start_px+i, start_py+m_drawlist[i].y);
            }
            if (dp) dc.DrawLines(dp,screen);

        } else {
            if (dp) dc.DrawLines(dp,m_drawlist);
        }
    }
    dc.DestroyClippingRegion();
    //dc.SetClippingRegion(start_px-1,start_py+height,width+1,w.GetBottomMargin());
    //dc.DestroyClippingRegion();
}

gLineOverlayBar::gLineOverlayBar(gPointData *d,const wxColor * col,wxString _label,LO_Type _lot)
:gLayer(d),label(_label),lo_type(_lot)
{
    color.clear();
    color.push_back(col);
}
gLineOverlayBar::~gLineOverlayBar()
{
}

void gLineOverlayBar::Plot(wxDC & dc, gGraphWindow & w)
{
    double x1,x2;


    if (!m_visible) return;
    if (!data) return;
    if (!data->IsReady()) return;
    //if (!data->NP()) return;

    int scrx = w.GetScrX();
    int scry = w.GetScrY();
    //wxString label=wxString::Format(wxT("%i %i"),scrx,scry);
    //dc.DrawText(label,0,0);

    int start_px=w.GetLeftMargin();
    int start_py=w.GetTopMargin();
    int width=scrx-(w.GetLeftMargin()+w.GetRightMargin());
    int height=scry-(w.GetTopMargin()+w.GetBottomMargin());


    double xx=w.max_x-w.min_x;
    if (xx<=0) return;
    wxPen sfp3(*color[0], 4, wxSOLID);
    wxPen sfp2(*color[0], 5, wxSOLID);
    wxPen sfp1(*color[0], 1, wxSOLID);

    wxBrush brush(*color[0],wxFDIAGONAL_HATCH);
    dc.SetBrush(brush);

    for (int n=0;n<data->VC();n++) {

        bool done=false;
        bool first=true;
        for (int i=0;i<data->np[n];i++) { //,done==false
            if (data->point[n][i].y < w.min_x) continue;
            if (first) {
                first=false;
                if (i>0)  i--;
            }

            wxRealPoint & rp=data->point[n][i];
            x1=w.x2p(rp.x);
            x2=w.x2p(rp.y);

            // point z is marker code

            if (x1<=start_px) x1=start_px+1;
            if (x2<=start_px) continue;
            if (x1>start_px+width) {
                //done=true;
                break;
            }
            if (x2>=start_px+width+1) x2=start_px+width+1;
            double w1=x2-x1;
            dc.SetPen(sfp1);
            int x,y;
            if (lo_type==LOT_Bar) {
                if (rp.x==rp.y) {
                    if (xx<(1800.0/86400)) {
                        //dc.SetTextForeground(*color[0]);
                        dc.GetTextExtent(label,&x,&y);
                        dc.DrawText(label,x1-(x/2),start_py+20-y);
                    }
                    dc.DrawLine(x1,start_py+25,x1,start_py+height-25);
                    dc.SetPen(sfp2);
                    dc.DrawLine(x1,start_py+25,x1,start_py+25);
                } else {
            //       if ((x1>w.GetLeftMargin()) && (x1<w.GetLeftMargin()+w.Width()))
                    dc.DrawRectangle(x1,start_py,w1,height);
                }
            } else if (lo_type==LOT_Dot) {
                dc.SetPen(sfp3);
                dc.DrawLine(x1,start_py+(height/2)-10,x1,start_py+(height/2)-10);
            }


            if (done) break;

        }

        if (done) break;
    }
}

gFlagsLine::gFlagsLine(gPointData *d,const wxColor * col,wxString _label,int _line_num,int _total_lines)
:gLayer(d),label(_label),line_num(_line_num),total_lines(_total_lines)
{
    color.clear();
    color.push_back(col);

}
gFlagsLine::~gFlagsLine()
{
}
void gFlagsLine::Plot(wxDC & dc, gGraphWindow & w)
{
    if (!m_visible) return;
    if (!data) return;
    if (!data->IsReady()) return;

    double xx=w.max_x-w.min_x;
    if (xx<=0) return;
    //if (!data->NP()) return;

    int scrx = w.GetScrX();
    int scry = w.GetScrY();
    //wxString label=wxString::Format(wxT("%i %i"),scrx,scry);
    //dc.DrawText(label,0,0);

    int start_px=w.GetLeftMargin();
    int start_py=w.GetTopMargin();
    int width=scrx-(w.GetLeftMargin()+w.GetRightMargin());
    int height=scry-(w.GetTopMargin()+w.GetBottomMargin());

    static wxColor col1=wxColor(0xff,0xf0,0xd0,0x7f);
    static wxColor col2=wxColor(0xe0,0xff,0xd0,0x7f);
    static wxBrush linebr1(col1, wxSOLID);
    static wxBrush linebr2(col2, wxSOLID);

    wxPen sfp1(*color[0], 1, wxSOLID);
    wxBrush brush(*color[0],wxSOLID); //FDIAGONAL_HATCH);


    double line_h=(height+1)/double(total_lines);
    //int r=int(height) % total_lines;

    double line_top=start_py+round(line_num*line_h)-1;
    //double line_bottom=line_top+line_h;

    dc.SetPen(*wxBLACK);
    if (line_num & 1) {
        dc.SetBrush(linebr1);
    } else {
        dc.SetBrush(linebr2);
    }
    dc.DrawRectangle(start_px,line_top,width+1,round(line_h+1));
    int x,y;
    dc.GetTextExtent(label,&x,&y);
    dc.DrawText(label,start_px-x-6,line_top+(line_h/2)-(y/2));

    if (line_num==0) {  // first lines responsibility to draw the title.
        int lw=x;
        dc.GetTextExtent(w.Title(),&x,&y);
        dc.DrawRotatedText(w.Title(), start_px-8-lw - y, start_py+((height + x)>>1), 90);
    }
    dc.SetBrush(brush);
    int x1,x2;
    for (int n=0;n<data->VC();n++) {

        bool done=false;
        bool first=true;
        for (int i=0;i<data->np[n];i++) { //,done==false
            if (data->point[n][i].y < w.min_x) continue;
            if (first) {
                first=false;
                if (i>0)  i--;
            }

            wxRealPoint & rp=data->point[n][i];
            x1=w.x2p(rp.x);
            x2=w.x2p(rp.y);

            // point z is marker code

            if (x1<=start_px) x1=start_px+1;
            if (x2<=start_px) continue;
            if (x1>start_px+width) {
                //done=true;
                break;
            }
            if (x2>=start_px+width+1) x2=start_px+width+1;
            double w1=x2-x1;
            dc.SetPen(sfp1);
            if (rp.x==rp.y) {
                dc.DrawLine(x1,line_top+4,x1,line_top+line_h-3);
                //dc.SetPen(sfp2);
                //dc.DrawLine(x1,w.GetTopMargin()+25,x1,w.GetTopMargin()+25);
            } else {
         //       if ((x1>w.GetLeftMargin()) && (x1<w.GetLeftMargin()+w.Width()))
                dc.DrawRectangle(x1,line_top+4,w1,line_h-6);
            }


            if (done) break;

        }

        if (done) break;
    }


}


FlowData::FlowData()
:gPointData(250000)
{
}
FlowData::~FlowData()
{
}
void FlowData::Reload(Day *day)
{
    vc=0;
    if (!day) {
        m_ready=false;
        return;
    }
    //wxRealPoint *rpl;
    MachineCode code=CPAP_FlowRate;
    min_x=day->first().GetMJD();
    max_x=day->last().GetMJD();
    max_y=0;
    bool first=true;
    for (vector<Session *>::iterator s=day->begin();s!=day->end(); s++) {
        if ((*s)->waveforms.find(code)==(*s)->waveforms.end()) continue;
        for (vector<Waveform *>::iterator l=(*s)->waveforms[code].begin();l!=(*s)->waveforms[code].end();l++) {
            int ps=point.size();
            if (vc>=ps) {
                AddSegment(max_points); // TODO: Add size limit capabilities.
            }
            int t=0;

            Waveform *w=(*l);
            double st=w->start().GetMJD();
            double rate=(w->duration()/w->samples())/86400.0;
            for (int i=0;i<w->samples();i++) {
                wxRealPoint r(st,(*w)[i]);
                st+=rate;
                point[vc][t++]=r;
                assert(t<max_points);
                if (first) {
                    min_y=r.y;
                    first=false;
                } else {
                    if (r.y<min_y) min_y=r.y;
                }
                if (r.y>max_y) max_y=r.y;
            }
            np[vc]=t;
            vc++;
        }
    }
    min_y=floor(min_y);
    max_y=ceil(max_y);

    //double t1=MAX(fabs(min_y),fabs(max_y));

    if (max_y>128) {
    } else if (max_y>90) {
        max_y=120;
        min_y=-120;
    } else if (max_y>60) {
        min_y=-90;
        max_y=90;
    } else  {
        min_y=-60;
        max_y=60;
    }

    if (force_min_y!=force_max_y) {
        min_y=force_min_y;
        max_y=force_max_y;
    }

    real_min_x=min_x;
    real_min_y=min_y;
    real_max_x=max_x;
    real_max_y=max_y;
    m_ready=true;
    //graph->Refresh(false);
}

PressureData::PressureData(MachineCode _code,int _field,int _size)
:gPointData(_size),code(_code),field(_field)
{
}
PressureData::~PressureData()
{
}
void PressureData::Reload(Day *day)
{
    vc=0;
    if (!day) {
        m_ready=false;
        return;
    }

    min_x=day->first().GetMJD();
    max_x=day->last().GetMJD();
    assert(min_x<max_x);
    min_y=max_y=0;
    int tt=0;
    bool first=true;
    for (vector<Session *>::iterator s=day->begin();s!=day->end(); s++) {
        if ((*s)->events.find(code)==(*s)->events.end()) continue;
        if (vc>=(int)point.size()) {
            AddSegment(max_points);
        }

        int t=0;
        EventDataType p; //,lastp=-1;
        for (vector<Event *>::iterator ev=(*s)->events[code].begin(); ev!=(*s)->events[code].end(); ev++) {
            p=(*(*ev))[field];
            /*if (lastp>=0) {
                wxRealPoint r2((*ev)->time().GetMJD(),lastp);
                point[vc][t++]=r2;
                assert(t<max_points);
            } */
            wxRealPoint r((*ev)->time().GetMJD(),p);
            point[vc][t++]=r;
            assert(t<max_points);
            if (first) {
                max_y=min_y=r.y;
                first=false;
            } else {
                if (r.y<min_y) min_y=r.y;
                if (r.y>max_y) max_y=r.y;
            }

            //lastp=p;
        }
        np[vc]=t;
        tt+=t;
        vc++;

    }
    /*if ((code==CPAP_Pressure) || (code==CPAP_EAP) || (code==CPAP_IAP)) {
        if (day->summary_max(CPAP_Mode)==MODE_CPAP) {
            min_y=4;
            max_y=ceil(max_y+1);
        } else {
            if (min_y>day->summary_min(CPAP_PressureMin)) min_y=day->summary_min(CPAP_PressureMin);
            if (max_y<day->summary_max(CPAP_PressureMax)) max_y=day->summary_min(CPAP_PressureMax);
        //max_y=ceil(day->summary_max(CPAP_PressureMax));
            min_y=floor(min_y);
            max_y=ceil(max_y);
        }
    } else { */
    if (tt>0) {
        min_y=floor(min_y);
        max_y=ceil(max_y+1);
        if (min_y>1) min_y-=1;
    }

    //}
    if (force_min_y!=force_max_y) {
        min_y=force_min_y;
        max_y=force_max_y;
    }


    real_min_x=min_x;
    real_min_y=min_y;
    real_max_x=max_x;
    real_max_y=max_y;
    m_ready=true;

    //max_y=25;
    //max_y=max_y/25)*25;
    //graph->Refresh(false);
}

TAPData::TAPData(MachineCode _code)
:gPointData(256),code(_code)
{
    AddSegment(max_points);
}
TAPData::~TAPData()
{
}
void TAPData::Reload(Day *day)
{
    if (!day) {
        m_ready=false;
        return;
    }


    for (int i=0;i<max_slots;i++) pTime[i]=wxTimeSpan::Seconds(0);

    int cnt=0;

    bool first;
    wxDateTime last;
    int lastval,val;

    int field=0;

    for (vector<Session *>::iterator s=day->begin();s!=day->end();s++) {
        if ((*s)->events.find(code)==(*s)->events.end()) continue;
        first=true;
        for (vector<Event *>::iterator e=(*s)->events[code].begin(); e!=(*s)->events[code].end(); e++) {
            Event & ev =(*(*e));
            val=ev[field]*10.0;
            if (field > ev.fields()) throw BoundsError();
            if (first) {
                first=false;
            } else {
                wxTimeSpan d=ev.time()-last;
                if (lastval>max_slots) throw BoundsError();
                pTime[lastval]+=d;
            }
            cnt++;
            last=ev.time();
            lastval=val;
        }
    }
    wxTimeSpan TotalTime(0);
    for (int i=0; i<max_slots; i++) {
        TotalTime+=pTime[i];
    }

    int jj=0;
    int seconds=TotalTime.GetSeconds().GetLo();
    for (int i=0; i<max_slots; i++) {
        if (pTime[i]>wxTimeSpan::Seconds(0)) {
            point[0][jj].x=i/10.0;
            point[0][jj].y=(100.0/seconds)*pTime[i].GetSeconds().GetLo();
            jj++;
        }
    }
    np[0]=jj;
    //graph->Refresh();
    m_ready=true;
}

AHIData::AHIData()
:gPointData(256)
{
    AddSegment(max_points);
}
AHIData::~AHIData()
{
}
void AHIData::Reload(Day *day)
{
    if (!day) {
        m_ready=false;
        return;
    }
    point[0][0].y=day->count(CPAP_Hypopnea)/day->hours();
    point[0][0].x=point[0][0].y;
    point[0][1].y=day->count(CPAP_Obstructive)/day->hours();
    point[0][1].x=point[0][1].y;
    point[0][2].y=day->count(CPAP_ClearAirway)/day->hours();
    point[0][2].x=point[0][2].y;
    point[0][3].y=day->count(CPAP_RERA)/day->hours();
    point[0][3].x=point[0][3].y;
    point[0][4].y=day->count(CPAP_FlowLimit)/day->hours();
    point[0][4].x=point[0][4].y;
    point[0][5].y=(100.0/day->hours())*(day->sum(CPAP_CSR)/3600.0);
    point[0][5].x=point[0][5].y;
    np[0]=6;
    m_ready=true;
    //REFRESH??
}

FlagData::FlagData(MachineCode _code,double _value,int _field,int _offset)
:gPointData(1024),code(_code),value(_value),field(_field),offset(_offset)
{
    AddSegment(max_points);
}
FlagData::~FlagData()
{
}
void FlagData::Reload(Day *day)
{
    if (!day) {
        m_ready=false;
        return;
    }
    int c=0;
    vc=0;
    double v1,v2;
    bool first;
    min_x=day->first().GetMJD();
    max_x=day->last().GetMJD();

    for (vector<Session *>::iterator  s=day->begin();s!=day->end();s++) {
        if ((*s)->events.find(code)==(*s)->events.end()) continue;
        first=true;
        for (vector<Event *>::iterator e=(*s)->events[code].begin(); e!=(*s)->events[code].end(); e++) {
            Event & ev =(*(*e));
            v2=v1=ev.time().GetMJD();
            if (offset>=0)
                v1-=ev[offset]/86400.0;
            point[vc][c].x=v1;
            point[vc][c].y=v2;
            //point[vc][c].z=value;
            c++;
            assert(c<max_points);
            /*if (first) {
                min_y=v1;
                first=false;
            } else {

       if (v1<min_x) min_x=v1;
            }
            if (v2>max_x) max_x=v2; */
        }
    }
    min_y=-value;
    max_y=value;
    np[vc]=c;
    vc++;
    real_min_x=min_x;
    real_min_y=min_y;
    real_max_x=max_x;
    real_max_y=max_y;
    m_ready=true;
}

////////////////////////////////////////////////////////////////////////////////////////////
// HistoryData Implementation
////////////////////////////////////////////////////////////////////////////////////////////

HistoryData::HistoryData(Profile * _profile)
:gPointData(1024),profile(_profile)
{
    AddSegment(max_points);
    if (profile->LastDay().IsValid()) {
        real_min_x=profile->FirstDay().GetMJD();
        real_max_x=profile->LastDay().GetMJD()+1;
    }
    real_min_y=real_max_y=0;
}
HistoryData::~HistoryData()
{
}
void HistoryData::ResetDateRange()
{
    if (profile->LastDay().IsValid()) {
        real_min_x=profile->FirstDay().GetMJD();
        real_max_x=profile->LastDay().GetMJD()+1;
    }
 //   Reload(NULL);
}
double HistoryData::Calc(Day *day)
{
    return (day->summary_sum(CPAP_Obstructive) + day->summary_sum(CPAP_Hypopnea) + day->summary_sum(CPAP_ClearAirway)) / day->hours();
}

void HistoryData::Reload(Day *day)
{
    wxDateTime date;
    vc=0;
    int i=0;
    bool first=true;
    bool done=false;
    double y,lasty=0;
    min_y=max_y=0;
    min_x=max_x=0;
    for (int x=real_min_x;x<=real_max_x;x++) {
        date.Set(x+2400000.5);
        date.ResetTime();
        if (profile->daylist.find(date)==profile->daylist.end()) continue;

        y=0;
        int z=0;
        vector<Day *> & daylist=profile->daylist[date];
        for (vector<Day *>::iterator dd=daylist.begin(); dd!=daylist.end(); dd++) { // average any multiple data sets
            Day *d=(*dd);
            y=Calc(d);
            z++;
        }
        if (!z) continue;
        if (z>1) y /= z;
        if (first) {
          //  max_x=min_x=x;
            lasty=max_y=min_y=y;
            first=false;
        }
        point[vc][i].x=x;
        point[vc][i].y=y;
        if (y>max_y) max_y=y;
        if (y<min_y) min_y=y;
        //if (x<min_x) min_x=x;
        //if (x>max_x) max_x=x;

        i++;
        if (i>max_points) {
            wxLogError(wxT("max_points is not enough in HistoryData"));
            done=true;
        }
        if (done) break;
        lasty=y;
    }
    np[vc]=i;
    vc++;
    min_x=real_min_x;
    max_x=real_max_x;

   // max_x+=1;
    //real_min_x=min_x;
    //real_max_x=max_x;
    if (force_min_y!=force_max_y) {
        min_y=force_min_y;
        max_y=force_max_y;
    } else {
        if (!((min_y==max_y) && (min_y==0))) {
            if (min_y>1) min_y-=1;
            max_y++;
        }
    }
    real_min_y=min_y;
    real_max_y=max_y;
    m_ready=true;
}
double HistoryData::GetAverage()
{
    double x,val=0;
    int cnt=0;
    for (int i=0;i<np[0];i++) {
        x=point[0][i].x;
        if ((x<min_x) || (x>max_x)) continue;
        val+=point[0][i].y;
        cnt++;
    }
    if (!cnt) return 0;
    val/=cnt;
    return val;
}
void HistoryData::SetDateRange(wxDateTime start,wxDateTime end)
{
    double x1=start.GetMJD()-0.5;
    double x2=end.GetMJD();
    if (x1 < real_min_x) x1=real_min_x;
    if (x2 > (real_max_x)) x2=(real_max_x);
    min_x=x1;
    max_x=x2;
    for (list<gLayer *>::iterator i=notify_layers.begin();i!=notify_layers.end();i++) {
        (*i)->DataChanged(this);
    }    // Do nothing else.. Callers responsibility to Refresh window.
}


HistoryCodeData::HistoryCodeData(Profile *_profile,MachineCode _code)
:HistoryData(_profile),code(_code)
{
}
HistoryCodeData::~HistoryCodeData()
{
}
double HistoryCodeData::Calc(Day *day)
{
    return day->summary_avg(code);
}

UsageHistoryData::UsageHistoryData(Profile *_profile,T_UHD _uhd)
:HistoryData(_profile),uhd(_uhd)
{
}
UsageHistoryData::~UsageHistoryData()
{
}
double UsageHistoryData::Calc(Day *day)
{
    double d;
    if (uhd==UHD_Bedtime) {
        d=day->first().GetHour();
        if (d<12) d+=24;
        d+=(day->first().GetMinute()/60.0);
        d+=(day->first().GetSecond()/3600.0);
        return d;
    }
    else if (uhd==UHD_Waketime) {
        d=day->last().GetHour();
        d+=(day->last().GetMinute()/60.0);
        d+=(day->last().GetSecond()/3600.0);
        return d;
    }
    else if (uhd==UHD_Hours) return day->hours();
    else
        return 0;
}
