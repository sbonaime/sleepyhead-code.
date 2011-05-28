/*
gGraph Graphing System Implementation

Author: Mark Watkins <jedimark64@users.sourceforge.net>
License: LGPL
*/

#include <wx/settings.h>
#include <wx/dcbuffer.h>
#include <wx/log.h>
#include "graph.h"

#if !wxCHECK_VERSION(2,9,0)
wxColor zwxYELLOW=wxColor(0xc0,0xc0,0x40,0xff);
wxColor *wxYELLOW=&zwxYELLOW;
#endif
wxColor zwxAQUA=wxColor(0x00,0xff,0xff,0xff);
wxColor * wxAQUA=&zwxAQUA;
wxColor zwxPURPLE=wxColor(0xff,0x40,0xff,0xff);
wxColor * wxPURPLE=&zwxPURPLE;
wxColor zwxGREEN2=wxColor(0x40,0xff,0x40,0x5f);
wxColor * wxGREEN2=&zwxGREEN2;
wxColor zwxLIGHT_YELLOW(228,228,168,255);
wxColor *wxLIGHT_YELLOW=&zwxLIGHT_YELLOW;
wxColor zwxDARK_GREEN=wxColor(20,128,20,255);
wxColor *wxDARK_GREEN=&zwxDARK_GREEN;


const wxColor *gradient_start_color=wxWHITE, *gradient_end_color=wxLIGHT_YELLOW;
wxDirection gradient_direction=wxEAST;
const wxColor *selection_color=wxBLUE; //GREEN2;
wxColor wxDARK_GREY(0xA0,0xA0,0xA0,0xA0);

gGraphData::gGraphData(int mp,gDataType t)
:vc(0),type(t),max_points(mp)
{
    m_ready=false;

}
gGraphData::~gGraphData()
{
}
void gGraphData::Update(Day *day)
{
    Reload(day);
    for (auto i=notify_layers.begin();i!=notify_layers.end();i++) {
        (*i)->DataChanged(this);
    }
}

gPointData::gPointData(int mp)
:gGraphData(mp,gDT_Point)
{
}
gPointData::~gPointData()
{
    for (auto i=point.begin();i!=point.end();i++)
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
    for (auto i=point.begin();i!=point.end();i++)
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
void gGraphWindow::ZoomXPixels(int x1, int x2)
{
    double rx1=0,rx2=0;
    ZoomXPixels(x1,x2,rx1,rx2);
    for (auto g=link_zoom.begin();g!=link_zoom.end();g++) {
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

    for (auto g=link_zoom.begin();g!=link_zoom.end();g++) {
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
    if (abs(event.GetX()-m_mouseRClick_start.x)<3 && abs(event.GetY()-m_mouseRClick_start.y)<3) {
        for (auto g=link_zoom.begin();g!=link_zoom.end();g++) {
            (*g)->ZoomX(2,0);
        }
        if (m_block_zoom) {
        } else {
            ZoomX(2,0); //event.GetX()); // adds origin to zoom out.. Doesn't look that cool.
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
    if (t1 != t2) {
        ZoomXPixels(t1,t2);
    }
    r=wxRect(0, 0, 0, 0);


    m_mouseRBrect=r;
    event.Skip();
}


gGraphWindow::gGraphWindow(wxWindow *parent, wxWindowID id,const wxString & title,const wxPoint &pos,const wxSize &size,long flags)
: wxWindow( parent, id, pos, size, flags, title )
{
    m_scrX   = m_scrY   = 64;
    m_title=title;
    m_mouseRDown=m_mouseLDown=false;
    SetBackgroundColour( *wxWHITE );
    m_bgColour = *wxWHITE;
    m_fgColour = *wxBLACK;
    SetMargins(10, 15, 30, 80);
    m_block_move=false;
    m_block_zoom=false;


}
gGraphWindow::~gGraphWindow()
{
    for (auto l=layers.begin();l!=layers.end();l++) delete (*l);
    layers.clear();
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
    for (auto l=layers.begin();l!=layers.end();l++) {
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
    double val;
    for (auto l=layers.begin();l!=layers.end();l++) {
        if (first) {
            val=(*l)->MinX();
            first=false;
        } else {
            if (val > (*l)->MinX()) val = (*l)->MinX();
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
    double val;
    for (auto l=layers.begin();l!=layers.end();l++) {
        if (first) {
            val=(*l)->MaxX();
            first=false;
        } else {
            if (val < (*l)->MinX()) val = (*l)->MaxX();
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
    double val;
    for (auto l=layers.begin();l!=layers.end();l++) {
        if (first) {
            val=(*l)->MinY();
            first=false;
        } else {
            if (val > (*l)->MinX()) val = (*l)->MinY();
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
    double val;
    for (auto l=layers.begin();l!=layers.end();l++) {
        if (first) {
            val=(*l)->MaxY();
            first=false;
        } else {
            if (val > (*l)->MinX()) val = (*l)->MaxY();
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
    double val;
    for (auto l=layers.begin();l!=layers.end();l++) {
        if (first) {
            val=(*l)->RealMinX();
            first=false;
        } else {
            if (val > (*l)->MinX()) val = (*l)->RealMinX();
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
    double val;
    for (auto l=layers.begin();l!=layers.end();l++) {
        if (first) {
            val=(*l)->RealMaxX();
            first=false;
        } else {
            if (val < (*l)->MinX()) val = (*l)->RealMaxX();
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
    double val;
    for (auto l=layers.begin();l!=layers.end();l++) {
        if (first) {
            val=(*l)->RealMinY();
            first=false;
        } else {
            if (val > (*l)->MinX()) val = (*l)->RealMinY();
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
    double val;
    for (auto l=layers.begin();l!=layers.end();l++) {
        if (first) {
            val=(*l)->RealMaxY();
            first=false;
        } else {
            if (val > (*l)->MinX()) val = (*l)->RealMaxY();
        }
    }
    return rmax_y=val;
}

void gGraphWindow::SetMinX(double v)
{
    min_x=v;
    for (auto l=layers.begin();l!=layers.end();l++) {
        (*l)->SetMinX(v);
    }
}
void gGraphWindow::SetMaxX(double v)
{
    max_x=v;
    for (auto l=layers.begin();l!=layers.end();l++) {
        (*l)->SetMaxX(v);
    }
}
void gGraphWindow::SetMinY(double v)
{
    min_y=v;
    for (auto l=layers.begin();l!=layers.end();l++) {
        (*l)->SetMinY(v);
    }
}
void gGraphWindow::SetMaxY(double v)
{
    max_y=v;
    for (auto l=layers.begin();l!=layers.end();l++) {
        (*l)->SetMaxY(v);
    }
}

void gGraphWindow::DataChanged(gLayer *layer)
{
    // This is possibly evil.. It needs to push one refresh event for all layers

    // Assmption currently is Refresh que does skip
    MinX(); MinY(); MaxX(); MaxY();
    RealMinX(); RealMinY(); RealMaxX(); RealMaxY();

    Refresh(false);
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
        if (m_names.size()>i) {
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
    m_yminor_ticks=2;
    m_ymajor_ticks=10;
    m_show_grid=true;
    m_show_minor_grid=true;
    if (col) {
        color.clear();
        color.push_back(col);
    }
}
gBarChart::~gBarChart()
{
}
void gBarChart::DrawYTicks(wxDC & dc,gGraphWindow &w)
{
    static wxColor wxDARK_GREY(0xA0,0xA0,0xA0,0xA0);
    static wxPen pen1(*wxLIGHT_GREY, 1, wxDOT);
    static wxPen pen2(wxDARK_GREY, 1, wxDOT);
    wxCoord x,y,labelW=0;

    int scrx = w.GetScrX();
    int scry = w.GetScrY();
    //wxString label=wxString::Format(wxT("%i %i"),scrx,scry);
    //dc.DrawText(label,0,0);

    int start_px=w.GetLeftMargin();
    int start_py=w.GetTopMargin();
    int width=scrx-(w.GetLeftMargin()+w.GetRightMargin());
    int height=scry-(w.GetTopMargin()+w.GetBottomMargin());

    dc.SetPen(*wxBLACK_PEN);

    wxString fd=wxT("0");
    dc.GetTextExtent(fd,&x,&y);
    double max_yticksdiv=(y+15.0)/(height); // y+50 for rotated text
    double max_yticks=1/max_yticksdiv;
    double miny=w.min_y;
    double maxy=w.max_y;
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
        if (m_show_minor_grid && (i > w.min_y))
            dc.DrawLine(start_px+1,h,start_px+width,h);
    }

    for (double i=w.min_y; i<=w.max_y; i+=min_ytick) {
		ty=(i - w.min_y) * ymult;
        fd=FormatY(i); // Override this as a function.
        dc.GetTextExtent(fd,&x,&y);
        if (x>labelW) labelW=x;
        h=(start_py+height)-ty;
        dc.DrawText(fd,start_px-8-x,h - (y / 2));
        dc.SetPen(*wxBLACK_PEN);
		dc.DrawLine(start_px-6,h,start_px,h);
        dc.SetPen(pen2);
        if (m_show_grid && (i > w.min_y))
            dc.DrawLine(start_px+1,h,start_px+width,h);
	}
	dc.GetTextExtent(w.Title(),&x,&y);
    dc.DrawRotatedText(w.Title(), start_px-8-labelW - y, start_py+((height + x)>>1), 90);
}

void gBarChart::Plot(wxDC & dc, gGraphWindow & w)
{
    if (!m_visible) return;
    if (!data) return;
    if (!data->IsReady()) return;

    int scrx = w.GetScrX();
    int scry = w.GetScrY();
    //if (m_direction==wxVERTICAL) {
    //} else {
        DrawYTicks(dc,w);
    //}
    dc.SetPen( *wxBLACK_PEN );
    //wxString label=wxString::Format(wxT("%i %i"),scrx,scry);
    //dc.DrawText(label,0,0);

    int start_px=w.GetLeftMargin();
    int start_py=w.GetTopMargin();
    int width=scrx-(w.GetLeftMargin()+w.GetRightMargin());
    int height=scry-(w.GetTopMargin()+w.GetBottomMargin());

    double xx=w.max_x - w.min_x;
    double days=int(xx);
    days=data->np[0];

    //for (int i=0;i<data->np[0];i++) {
     //   if ((data->point[0][i].x > w.min_x) && (data->point[0][i].x<w.max_x)) days++;
    //}
    // == max_y

    float barwidth,pxr;
    int px,py;

    if (m_direction==wxVERTICAL) {
        barwidth=(height-days*2)/days;
        pxr=width/w.max_y;
        px=start_py;
    } else {
        barwidth=(width-days*2)/days;
        pxr=height/w.max_y;
        px=start_px;
    }
    px+=1;
    int t1,t2;
    int u1,u2;
    int textX, textY;


    wxString str;
    //const wxColor *colors[6]={wxRED, wxBLUE, wxGREEN, wxCYAN , wxBLACK, wxLIGHT_GREY };

    int cnt=0;
    for (int i=0;i<data->np[0];i++) {
        //if (data->point[0][i].x < w.min_x) continue;
        //if (data->point[0][i].x > w.max_x) break;
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        t1=px+1;
        px+=barwidth+2;
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
        dc.GetTextExtent(str, &textX, &textY);
        if (t2>textY) {
            int j=t1+((t2/2)-(textY/2));
            if (m_direction==wxVERTICAL) {
                dc.DrawRotatedText(str,start_px-textX-8,j,0);
            } else {
                dc.DrawRotatedText(str,j,start_py+height+4+textX,90);
            }
        }

    }

    dc.DrawLine(start_px,start_py,start_px,start_py+height);

    dc.DrawLine(start_px,start_py+height,start_px+width,start_py+height);
}

/*gBarChart::gBarChart(gGraphData *d,wxOrientation o)
:gLayer(d),m_direction(o)
{
    m_yminor_ticks=2;
    m_ymajor_ticks=10;
}
gBarChart::~gBarChart()
{
}
void gBarChart::Plot(wxDC & dc, gGraphWindow & w)
{
    if (!m_visible) return;
    if (!data) return;
    if (!data->NP(0)) return;

    int scrx = w.GetScrX();
    int scry = w.GetScrY();
    dc.SetPen( *wxBLACK_PEN );
    //wxString label=wxString::Format(wxT("%i %i"),scrx,scry);
    //dc.DrawText(label,0,0);

    int start_px=w.GetLeftMargin();
    int start_py=w.GetTopMargin();
    int width=scrx-(w.GetLeftMargin()+w.GetRightMargin());
    int height=scry-(w.GetTopMargin()+w.GetBottomMargin());

    double max=0;

    for (int i=0;i<data->NP(0);i++) {
        if (data->vpoint[0][i].y>max) max=data->vpoint[0][i].y;
    }

    float barwidth,pxr;
    int px,py;

    if (m_direction==wxVERTICAL) {
        barwidth=(height-data->NP(0)*2)/data->NP(0);
        pxr=width/max;
        px=start_py;
    } else {
        barwidth=(width-data->NP(0)*2)/data->NP(0);
        pxr=(height)/max;
        px=start_px;
    }
    px+=1;
    int t1,t2;
    int u1,u2;
    int textX, textY;

    wxString str;
    const wxColor *colors[6]={wxRED, wxBLUE, wxGREEN, wxCYAN , wxBLACK, wxLIGHT_GREY };

    for (int i=0;i<data->NP(0);i++) {
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        t1=px+1;
        px+=barwidth+2;
        t2=px-t1-1;

        wxRect rect;
        wxDirection dir;

        if (data->Type()==gDT_Stacked) {
            py=start_py+height;
            for (size_t j=0;j<data->yaxis[i].size();j++) {
                u1=(data->yaxis[i][j]*pxr); // height of section in pixels
                u2=height-u1;

                //if (m_direction==wxVERTICAL) {
                //    rect=wxRect(start_px+u1,t1,u2,t2);
                //    dir=wxSOUTH;
                //} else {
                rect=wxRect(t1,py-u1,t2,u1);
                dir=wxEAST;
                dc.GradientFillLinear(rect,*colors[j%6],*wxLIGHT_GREY,dir);
                dc.DrawRectangle(rect);
                py-=u1;
            }
        } else if (data->Type()==gDT_Point) {
            u2=data->vpoint[0][i].y*pxr;
            u1=(start_py+height)-u2;
            rect=wxRect(t1,u1,t2,u2);
            dir=wxEAST;
            dc.GradientFillLinear(rect,*colors[0],*wxLIGHT_GREY,dir);
            dc.DrawRectangle(rect);
        }

        str=FormatX(data->vpoint[0][i].x);
        dc.GetTextExtent(str, &textX, &textY);
        if (t2>textY) {
            int j=t1+((t2/2)-(textY/2));
            if (m_direction==wxVERTICAL) {
                dc.DrawRotatedText(str,start_px+barwidth+2+textY,j,270);
            } else {
                dc.DrawRotatedText(str,j,start_py+height+8+textX,90);
            }
        }

    }

    dc.DrawLine(start_px,start_py,start_px,start_py+height);

    dc.DrawLine(start_px,start_py+height,start_px+width,start_py+height);


	wxCoord x,y;
	wxString S;
	wxCoord labelW=0;
	for (float i=0;i<=max;i+=1) {
		int s=4;
		if (i/m_yminor_ticks!=0) continue;
		if (i/m_ymajor_ticks==0) {
			s=6;
			S=FormatY(i); // Override this as a function.
			dc.GetTextExtent(S,&x,&y);
			if (x>labelW) labelW=x;
			dc.DrawText(S,start_px-8-x,start_py+height-(i*pxr)-y/2);
		}
		dc.DrawLine(start_px-s,start_py+height-(i*pxr),start_px,start_py+height-(i*pxr));
	}
	dc.GetTextExtent(w.Title(),&x,&y);
    dc.DrawRotatedText(w.Title(), start_px-8-labelW - y, start_py+((height + x)>>1), 90);

}
*/

gLineChart::gLineChart(gPointData *d,const wxColor * col,int dlsize,bool a)
:gLayer(d),m_accelerate(a),m_drawlist_size(dlsize)
{
    m_yminor_ticks=0.5;
    m_ymajor_ticks=1;
    m_drawlist=new wxPoint [dlsize];
    color.clear();
    color.push_back(col);
    m_show_grid=true;
    m_show_minor_grid=true;

}
gLineChart::~gLineChart()
{
    delete [] m_drawlist;
}
void gLineChart::DrawYTicks(wxDC & dc,gGraphWindow &w)
{
    static wxColor wxDARK_GREY(0xA0,0xA0,0xA0,0xA0);
    static wxPen pen1(*wxLIGHT_GREY, 1, wxDOT);
    static wxPen pen2(wxDARK_GREY, 1, wxDOT);
    wxCoord x,y,labelW=0;

    int scrx = w.GetScrX();
    int scry = w.GetScrY();
    //wxString label=wxString::Format(wxT("%i %i"),scrx,scry);
    //dc.DrawText(label,0,0);

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
        int yrounding[9]={1,2,5,10,20,30,40,60,120}; // time rounding
        int ry;
        for (ry=0;ry<3;ry++) {
       // st=round(st2*rounding[ry])/rounding[ry];
            min_ytick=round(major_ytick*yrounding[ry])/yrounding[ry];
            q=yy/min_ytick;  // number of ticks that fits in range
            if (q<=max_yticks) break; // compared to number of ticks that fit on screen.
        }
    } else {
        min_ytick=60;
    }
    if (min_ytick<=1)
        min_ytick=1;

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
        if (m_show_minor_grid && (i > w.min_y))
            dc.DrawLine(start_px+1,h,start_px+width,h);
    }

    for (double i=w.min_y; i<=w.max_y; i+=min_ytick) {
		ty=(i - w.min_y) * ymult;
        fd=FormatY(i); // Override this as a function.
        dc.GetTextExtent(fd,&x,&y);
        if (x>labelW) labelW=x;
        h=(start_py+height)-ty;
        dc.DrawText(fd,start_px-8-x,h - (y / 2));
        dc.SetPen(*wxBLACK_PEN);
		dc.DrawLine(start_px-6,h,start_px,h);
        dc.SetPen(pen2);
        if (m_show_grid && (i > w.min_y))
            dc.DrawLine(start_px+1,h,start_px+width,h);
	}
	dc.GetTextExtent(w.Title(),&x,&y);
    dc.DrawRotatedText(w.Title(), start_px-8-labelW - y, start_py+((height + x)>>1), 90);
}

void gLineChart::DrawXTicks(wxDC & dc,gGraphWindow &w)
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

    dc.SetPen(*wxBLACK_PEN);

    //wxDateTime d;
    wxString fd=wxT("00:00:00:0000");
    dc.GetTextExtent(fd,&x,&y);
    double max_ticks=(x+25.0)/width; // y+50 for rotated text
    double jj=1/max_ticks;
    double xx=w.max_x-w.min_x;
    double minor_tick=max_ticks*xx;
    double st2=w.min_x; //double(int(frac*1440.0))/1440.0;
    double st,q;

    bool show_seconds=false;
    bool show_milliseconds=false;
    int rounding[16]={12,24,48,72,96,144,288,720,1440,2880,5760,8640,17280,86400,172800,345600}; // time rounding
    double min_tick;
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

    dc.SetPen(*wxBLACK_PEN);

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


    while (st<w.min_x) {
        st+=min_tick;
    }

    int hour,minute,second,millisecond;
    wxDateTime d;
    for (double i=st; i<=w.max_x; i+=min_tick) { //600.0/86400.0) {
        d.Set(i+2400000.5+.000001); // JDN vs MJD vs Rounding errors
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
        dc.GetTextExtent(fd,&x,&y);

//        px=x2p(w,i);
        px=w.x2p(i); //w.GetLeftMargin()+((i - w.min_x) * xmult);
		dc.DrawLine(px,py,px,py+6);

        dc.GetTextExtent(fd,&x,&y);
        //dc.DrawRotatedText(fd, px-(y/2)-20, py+x, 60);
        dc.DrawText(fd, px-(x/2), py+y);

    }

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
    if (xx<=0)
        return;
   // assert(xx>=0);
    static wxPoint screen[4096]; // max screen size

    static wxPen pen1(*wxLIGHT_GREY, 1, wxDOT);

    dc.SetPen( *wxBLACK_PEN );
    dc.DrawLine(start_px,start_py,start_px,start_py+height+1);
    dc.DrawLine(start_px,start_py+height+1,start_px+width+1,start_py+height+1);
   // dc.DrawLine(start_px,start_py,start_px+width,start_py);
    dc.DrawLine(start_px+width+1,start_py,start_px+width+1,start_py+height+1);

    static wxPen pen2(wxDARK_GREY, 1, wxDOT);
    static wxPen pen3(*wxGREEN, 2, wxSOLID);

    dc.SetPen( pen2 );
    dc.DrawLine(start_px,start_py+height+10,start_px+width,start_py+height+10);
    double rmx=w.rmax_x-w.rmin_x;
    px=((1/rmx)*(w.min_x-w.rmin_x))*width;
    py=((1/rmx)*(w.max_x-w.rmin_x))*width;
    dc.SetPen(pen3);
    dc.DrawLine(start_px+px,start_py+height+10,start_px+py,start_py+height+10);
    dc.DrawLine(start_px+px,start_py+height+8,start_px+px,start_py+height+12);
    dc.DrawLine(start_px+py,start_py+height+8,start_px+py,start_py+height+12);

    DrawYTicks(dc,w);

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

        for (int i=0;i<siz;i+=sam) { //,done==false
            if (point[i].x < minx) continue;
            if (first) {
                first=false;
                if (i>=sam)  i-=sam;
            }

            if (point[i].x > maxx) done=true;


            px=(point[i].x - minx) * xmult;
            if (px<0) px=0;
            if (px>width) px=width;
            py=height - ((point[i].y - miny) * ymult);

            if (accel) {
                int z=round(px);
                if (z<minz) minz=z;
                if (z>maxz) maxz=z;
                if (py<m_drawlist[z].x) m_drawlist[z].x=py;
                if (py>m_drawlist[z].y) m_drawlist[z].y=py;
            } else {
                m_drawlist[dp].x=start_px+px;
                m_drawlist[dp].y=start_py+py;
                if (++dp>=m_drawlist_size) {
                    wxLogWarning(wxT("gLineChart: m_drawlist is too small"));
                    break;
                }
            }
            if (done) break;
        }

        if (accel) {
             dc.DrawLine(1, 1, 1, height);
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
    DrawXTicks(dc,w);
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

    static wxColor col1=wxColor(0xff,0xf0,0xd0,0x7f);
    static wxColor col2=wxColor(0xe0,0xff,0xd0,0x7f);
    static wxBrush linebr1(col1, wxSOLID);
    static wxBrush linebr2(col2, wxSOLID);


    static wxPen pen2(wxDARK_GREY, 1, wxDOT);
    static wxPen pen3(*wxGREEN, 2, wxSOLID);

    dc.SetPen( pen2 );
    dc.DrawLine(start_px,start_py+height+10,start_px+width,start_py+height+10);
    double rmx=w.rmax_x-w.rmin_x;
    double px=((1/rmx)*(w.min_x-w.rmin_x))*width;
    double py=((1/rmx)*(w.max_x-w.rmin_x))*width;
    dc.SetPen(pen3);
    dc.DrawLine(start_px+px,start_py+height+10,start_px+py,start_py+height+10);
    dc.DrawLine(start_px+px,start_py+height+8,start_px+px,start_py+height+12);
    dc.DrawLine(start_px+py,start_py+height+8,start_px+py,start_py+height+12);

    wxPen sfp1(*color[0], 1, wxSOLID);

    wxBrush brush(*color[0],wxSOLID); //FDIAGONAL_HATCH);


    double line_h=((height)/total_lines);
    int r=int(height) % total_lines;

    double line_top=1+(start_py+(line_num*line_h)-r/2.0);
    //double line_bottom=line_top+line_h;

    dc.SetPen(*wxBLACK);
    if (line_num & 1) {
        dc.SetBrush(linebr1);
    } else {
        dc.SetBrush(linebr2);
    }
    dc.DrawRectangle(start_px,line_top,width+1,line_h+1);
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
    min_x=day->first(CPAP_Pressure).GetMJD();
    max_x=day->last(CPAP_Pressure).GetMJD();
    max_y=0;
    bool first=true;
    for (auto s=day->begin();s!=day->end(); s++) {
        if ((*s)->waveforms.find(code)==(*s)->waveforms.end()) continue;
        for (auto l=(*s)->waveforms[code].begin();l!=(*s)->waveforms[code].end();l++) {
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

    double t1=MAX(fabs(min_y),fabs(max_y));

    max_y=t1;
    min_y=-t1;
    min_y=-120;
    max_y=120;

    real_min_x=min_x;
    real_min_y=min_y;
    real_max_x=max_x;
    real_max_y=max_y;
    m_ready=true;
    //graph->Refresh(false);
}

PressureData::PressureData(MachineCode _code,int _field)
:gPointData(1024),code(_code),field(_field)
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
    if (min_x>max_x) {
        max_x=max_x;
    }
    max_y=0;
    bool first=true;
    for (auto s=day->begin();s!=day->end(); s++) {
        if ((*s)->events.find(code)==(*s)->events.end()) continue;
        if (vc>=(int)point.size()) {
            AddSegment(max_points);
        }

        int t=0;
        EventDataType p,lastp=-1;
        for (auto ev=(*s)->events[code].begin(); ev!=(*s)->events[code].end(); ev++) {
            p=(*(*ev))[field];
            if (lastp>=0) {
                wxRealPoint r2((*ev)->time().GetMJD(),lastp);
                point[vc][t++]=r2;
                assert(t<max_points);
            }
            wxRealPoint r((*ev)->time().GetMJD(),p);
            point[vc][t++]=r;
            assert(t<max_points);
            if (first) {
                min_y=r.y;
                first=false;
            } else {
                if (r.y<min_y) min_y=r.y;
            }
            if (r.y>max_y) max_y=r.y;

            lastp=p;
        }
        np[vc]=t;
        vc++;

    }
    if ((code==CPAP_Pressure) && (day->summary_max(CPAP_Mode)==MODE_CPAP)) {
        min_y=4;
        max_y=ceil(max_y+1);
    } else {
        if (min_y>day->summary_min(CPAP_PressureMin)) min_y=day->summary_min(CPAP_PressureMin);
        if (max_y<day->summary_max(CPAP_PressureMax)) max_y=day->summary_min(CPAP_PressureMax);
        //max_y=ceil(day->summary_max(CPAP_PressureMax));
        min_y=floor(min_y);
        max_y=ceil(max_y);
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

TAPData::TAPData()
:gPointData(256)
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

    const int max_slots=256;
    static wxTimeSpan pTime[max_slots];

    for (int i=0;i<max_slots;i++) pTime[i]=wxTimeSpan::Seconds(0);

    int cnt=0;

    bool first;
    wxDateTime last;
    int lastval,val;

    int field=0;
    MachineCode code=CPAP_Pressure;

    for (auto s=day->begin();s!=day->end();s++) {
        if ((*s)->events.find(code)==(*s)->events.end()) continue;
        first=true;
        for (auto e=(*s)->events[code].begin(); e!=(*s)->events[code].end(); e++) {
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

    for (auto s=day->begin();s!=day->end();s++) {
        if ((*s)->events.find(code)==(*s)->events.end()) continue;
        first=true;
        for (auto e=(*s)->events[code].begin(); e!=(*s)->events[code].end(); e++) {
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
    min_y=0;
    max_y=value;
    np[vc]=c;
    vc++;
    real_min_x=min_x;
    real_min_y=min_y;
    real_max_x=max_x;
    real_max_y=max_y;
    m_ready=true;
}

HistoryData::HistoryData(Machine *_machine,int _days)
:gPointData(_days*2),machine(_machine),days(_days)
{
    AddSegment(max_points);
}
HistoryData::~HistoryData()
{
}
double HistoryData::Calc(Day *day)
{
    return (day->count(CPAP_Obstructive)+day->count(CPAP_Hypopnea)+day->count(CPAP_ClearAirway))/day->hours();
}

void HistoryData::Reload(Day *day)
{
    if (!machine) return;

    auto d=machine->day.rbegin();
    int i=0;
    vc=0;
    bool first=true;
    double x,y;
    max_y=0;
    while (d!=machine->day.rend() && (i<days)) {
        y=Calc(d->second);
        x=d->first.GetMJD()+1;
        if (first) {
            max_x=x;
            min_x=x;
            min_y=y;
            first=false;
        }
        if (y>max_y) max_y=y;
        if (y<min_y) min_y=y;
        if (x<min_x) min_x=x;
        if (x>max_x) max_x=x;

        point[vc][i].x=x;
        point[vc][i].y=y;
        i++;
        d++;
    }
    np[vc]=i;
    vc++;
    min_y=0;
    max_y=ceil(max_y);
    real_min_x=min_x;
    real_max_x=max_x;
    real_min_y=min_y;
    real_max_y=max_y;
    m_ready=true;
}
double HistoryData::GetAverage()
{
    double val=0;
    for (int i=0;i<np[0];i++) {
        val+=point[0][i].y;
    }
    val/=np[0];
    return val;
}


HistoryCodeData::HistoryCodeData(Machine *_machine,MachineCode _code,int _days)
:HistoryData(_machine,_days),code(_code)
{
}
HistoryCodeData::~HistoryCodeData()
{
}
double HistoryCodeData::Calc(Day *day)
{
    return day->summary_avg(code);
}

UsageHistoryData::UsageHistoryData(Machine *_machine,int _days,T_UHD _uhd)
:HistoryData(_machine,_days),uhd(_uhd)
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
