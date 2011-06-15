/*
gGraph Graphing System Implementation

Author: Mark Watkins <jedimark64@users.sourceforge.net>
License: LGPL
*/

#include "freetype-gl/font-manager.h"
#include "freetype-gl/texture-font.h"

#include "graph.h"

//#include <wx/glcanvas.h>

#include <wx/settings.h>
#include <wx/graphics.h>
#include <wx/image.h>
#include <wx/bitmap.h>
#include <wx/dcbuffer.h>

#include <wx/log.h>
#include <math.h>

#include "sleeplib/profiles.h"

#include "freesans.c"

//#include <wx/dcbuffer.h>

#if !wxUSE_GLCANVAS
    #error "OpenGL required: set wxUSE_GLCANVAS to 1 and rebuild the wx library"
#endif

extern pBuffer *buffer;

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

bool gfont_init=false;

FontManager *font_manager;
TextureFont *zfont=NULL;
VertexBuffer *vbuffer=NULL;
TextMarkup *markup=NULL;

// Must be called from a thread inside the application.
void GraphInit()
{
    #if defined(__WXMSW__)
    glewInit(); // Dont forget this nasty little sucker.. :)
    #endif

    if (!gfont_init) {
        font_manager=new FontManager();
        vbuffer=new VertexBuffer((char *)"v3i:t2f:c4f");
        zfont=font_manager->GetFromFilename(pref.Get("{home}{sep}FreeSans.ttf"),14);
        markup=new TextMarkup();

        glBindTexture( GL_TEXTURE_2D, font_manager->m_atlas->m_texid );

        wxString fontfile=pref.Get("{home}{sep}FreeSans.ttf");
        if (!wxFileExists(fontfile)) {
            wxFFile f;
            f.Open(fontfile,wxT("wb"));
            long size=sizeof(FreeSans_ttf);
            if (!f.Write(FreeSans_ttf,size)) {
                wxLogError(wxT("Couldn't Write Font file.. Sorry.. need it to run"));
                return;
            }
            f.Close();
        }
        gfont_init=true;
    }
}
void GraphDone()
{
    if (gfont_init) {
        delete font_manager;
        delete vbuffer;
        delete markup;
        gfont_init=false;
    }
    if (shared_context) {
        delete shared_context;
        shared_context=NULL;
    }
}

void GetTextExtent(wxString text, float & width, float & height, TextureFont *font=zfont)
{
    TextureGlyph *glyph;
    height=width=0;

    for (unsigned i=0;i<text.Length();i++) {
        glyph=font->GetGlyph((wchar_t)text[i]);
        if (!height) height=glyph->m_height; // > height) height=glyph->m_height;
        width+=glyph->m_advance_x;
    }
}
void DrawText2(wxString text, float x, float y,TextureFont *font)
{
    Pen pen;
    pen.x=x;
    pen.y=y;

    TextureGlyph *glyph;
    glyph=font->GetGlyph((wchar_t)text[0]);
    if (!glyph) return;
    assert(vbuffer!=NULL);

    vbuffer->Clear();
    glyph->AddToVertexBuffer(vbuffer, markup, &pen);
    for (unsigned j=1; j<text.Length(); ++j) {
        glyph=font->GetGlyph(text[j]);
        pen.x += glyph->GetKerning(text[j-1]);
        glyph->AddToVertexBuffer(vbuffer, markup, &pen);
    }
    //glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glEnable( GL_TEXTURE_2D );
    glColor4f(1,1,1,1);
    vbuffer->Render(GL_TRIANGLES, (char *)"vtc" );
    glDisable(GL_BLEND);

}

void DrawText(wxString text, float x, float y, float angle=0, const wxColor & color=*wxBLACK,TextureFont *font=zfont)
{
    if (!font) {
        wxLogError(wxT("Font Problem. Forgot to call GraphInit() ?"));
        abort();
        return;
    }
    if (angle==0) {
        DrawText2(text,x,y,font);
        return;
    }


    float w,h;
    GetTextExtent(text, w, h, font);

    //glColor4ub(color.Red(),color.Green(),color.Blue(),color.Alpha());

    glPushMatrix();
    glTranslatef(x,y,0);
    glRotatef(angle, 0.0f, 0.0f, 1.0f);
    DrawText2(text,-w/2.0,-h/2.0,font);
    glTranslatef(-x,-y,0);
    glPopMatrix();

}
void RoundedRectangle(int x,int y,int w,int h,int radius,const wxColor & color)
{
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glColor4ub(color.Red(),color.Green(),color.Blue(),color.Alpha());

	glBegin(GL_POLYGON);
		glVertex2i(x+radius,y);
		glVertex2i(x+w-radius,y);
		for(float i=(float)M_PI*1.5f;i<M_PI*2;i+=0.1f)
			glVertex2f(x+w-radius+cos(i)*radius,y+radius+sin(i)*radius);
		glVertex2i(x+w,y+radius);
		glVertex2i(x+w,y+h-radius);
		for(float i=0;i<(float)M_PI*0.5f;i+=0.1f)
			glVertex2f(x+w-radius+cos(i)*radius,y+h-radius+sin(i)*radius);
		glVertex2i(x+w-radius,y+h);
		glVertex2i(x+radius,y+h);
		for(float i=(float)M_PI*0.5f;i<M_PI;i+=0.1f)
			glVertex2f(x+radius+cos(i)*radius,y+h-radius+sin(i)*radius);
		glVertex2i(x,y+h-radius);
		glVertex2i(x,y+radius);
		for(float i=(float)M_PI;i<M_PI*1.5f;i+=0.1f)
			glVertex2f(x+radius+cos(i)*radius,y+radius+sin(i)*radius);
	glEnd();

	glEnable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
}

void LinedRoundedRectangle(int x,int y,int w,int h,int radius,int lw,wxColor & color)
{
	glDisable(GL_TEXTURE_2D);
	glShadeModel(GL_SMOOTH);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glColor4ub(color.Red(),color.Green(),color.Blue(),color.Alpha());
	glLineWidth((GLfloat)lw);

	glBegin(GL_LINE_STRIP);
		for(float i=(float)M_PI;i<=1.5f*M_PI;i+=0.1f)
			glVertex2f(radius*cos(i)+x+radius,radius*sin(i)+y+radius);
		for(float i=1.5f*(float)M_PI;i<=2*M_PI; i+=0.1f)
			glVertex2f(radius*cos(i)+x+w-radius,radius*sin(i)+y+radius);
		for(float i=0;i<=0.5f*M_PI; i+=0.1f)
			glVertex2f(radius*cos(i)+x+w-radius,radius*sin(i)+y+h-radius);
		for(float i=0.5f*(float)M_PI;i<=M_PI;i+=0.1f)
			glVertex2f(radius*cos(i)+x+radius,radius*sin(i)+y+h-radius);
		glVertex2i(x,y+radius);
	glEnd();

	glEnable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
}



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
    for (vector<wxPoint2DDouble *>::iterator i=point.begin();i!=point.end();i++)
        delete [] (*i);
}
void gPointData::AddSegment(int max_points)
{
    maxsize.push_back(max_points);
    np.push_back(0);
    wxPoint2DDouble *p=new wxPoint2DDouble [max_points];
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
    color.push_back(*wxRED);
    color.push_back(*wxGREEN);
}
gLayer::~gLayer()
{

}

void gLayer::Plot(gGraphWindow & w,float scrx,float scry)
{
}



IMPLEMENT_DYNAMIC_CLASS(gGraphWindow, wxGLCanvas)

BEGIN_EVENT_TABLE(gGraphWindow, wxWindow)
    EVT_PAINT       (gGraphWindow::OnPaint)
    EVT_SIZE        (gGraphWindow::OnSize)
    EVT_MOTION      (gGraphWindow::OnMouseMove )
    EVT_LEFT_DOWN   (gGraphWindow::OnMouseLeftDown)
    EVT_LEFT_DCLICK (gGraphWindow::OnMouseLeftDClick)
    EVT_LEFT_UP     (gGraphWindow::OnMouseLeftRelease)
    EVT_RIGHT_DOWN  (gGraphWindow::OnMouseRightDown)
    EVT_RIGHT_UP    (gGraphWindow::OnMouseRightRelease)
    //EVT_MOUSEWHEEL (gGraphWindow::OnMouseWheel )
    //EVT_MIDDLE_DOWN (gGraphWindow::OnMouseRightDown)
    //EVT_MIDDLE_UP   (gGraphWindow::OnMouShowPopupMenu)


END_EVENT_TABLE()

 	//wxGLCanvas (wxWindow *parent, wxWindowID id=wxID_ANY, const int *attribList=NULL, const wxPoint &pos=wxDefaultPosition, const wxSize &size=wxDefaultSize, long style=0, const wxString &name="GLCanvas", const wxPalette &palette=wxNullPalette)


static int wx_gl_attribs[] = {WX_GL_RGBA, WX_GL_DOUBLEBUFFER, WX_GL_DEPTH_SIZE, 24, 0};

gGraphWindow::gGraphWindow()
: wxGLCanvas( (wxWindow *)NULL,shared_context,wxID_ANY) //,wxDefaultPosition,wxDefaultSize,wxT("GLContext"),(int *)wx_gl_attribs,wxNullPalette)
{
}


gGraphWindow::gGraphWindow(wxWindow *parent, wxWindowID id,const wxString & title,const wxPoint &pos,const wxSize &size,long flags)
: wxGLCanvas( parent, shared_context, id, pos, size, flags, title, (int *)wx_gl_attribs, wxNullPalette )
{
    m_scrX   = m_scrY   = 64;
    m_title=title;
    m_mouseRDown=m_mouseLDown=false;
    SetBackgroundColour( *wxWHITE );
    m_bgColour = *wxWHITE;
    m_fgColour = *wxBLACK;
    SetMargins(5, 15, 0, 0);
    m_block_move=false;
    m_block_zoom=false;
    m_drag_foobar=false;
    m_foobar_pos=0;
    m_foobar_moved=0;
    gtitle=foobar=xaxis=yaxis=NULL;

    if (!shared_context) {

#if defined(__DARWIN__) && !wxCHECK_VERSION(2,9,0)
        // Screw you apple..
        int *attribList = (int*) NULL;
        AGLPixelFormat aglpf=aglChoosePixelFormat(NULL,0,attribList);
        shared_context=new wxGLContext(aglpf,this,wxNullPalette,NULL);

        // Mmmmm.. Platform incosistency with wx..
#else
        // Darwin joins the rest of the platforms as of wx2.9
        shared_context=new wxGLContext(this,NULL);
#endif

    }

#if defined(__WXMSW__)
    shared_context->SetCurrent(*this); // Windows needs this done now or it borks..
#endif

#if !defined(__WXMAC__) && defined (__UNIX__)
    real_shared_context = glXGetCurrentContext();  // Unix likes this, but can't really have it..
#endif

    GraphInit(); // Font
    //texfont=::texfont;
    if (!title.IsEmpty()) {
        AddLayer(new gGraphTitle(title,wxVERTICAL));
    }

}
gGraphWindow::~gGraphWindow()
{
    for (list<gLayer *>::iterator l=layers.begin();l!=layers.end();l++) delete (*l);
    layers.clear();

}


void gGraphWindow::AddLayer(gLayer *l) {
    if (l) {
        if (dynamic_cast<gXAxis *>(l)) {
            if (xaxis) {
                wxLogError(wxT("Can only have one gXAxis per graph"));
                return;
            }
            if (m_marginBottom<gXAxis::Margin) m_marginBottom+=gXAxis::Margin;
            xaxis=l;
        }
        if (dynamic_cast<gFooBar *>(l)) {
            if (foobar) {
                wxLogError(wxT("Can only have one gFooBar per graph"));
                return;
            }
            if (m_marginBottom<gFooBar::Margin) m_marginBottom+=gFooBar::Margin;
            foobar=l;
        }
        if (dynamic_cast<gYAxis *>(l)) {
            if (yaxis) {
                wxLogError(wxT("Can only have one gYAxis per graph"));
                return;
            }
            if (m_marginLeft<gYAxis::Margin) m_marginLeft+=gYAxis::Margin;
            yaxis=l;
        }
        if (dynamic_cast<gGraphTitle *>(l)) {
            if (gtitle) {
                wxLogError(wxT("Can only have one gGraphTitle per graph"));
                return;
            }
            if (m_marginLeft<gGraphTitle::Margin) m_marginLeft+=gGraphTitle::Margin;
            gtitle=l;
        }
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

/*    for (list<gGraphWindow *>::iterator g=link_zoom.begin();g!=link_zoom.end();g++) {
        (*g)->SetXBounds(min,max);
    } */
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
gGraphWindow *LastGraphLDown=NULL;

void gGraphWindow::OnMouseMove(wxMouseEvent &event)
{
//    static bool first=true;
    static wxRect last;
    if (LastGraphLDown && (LastGraphLDown!=this)) {
        LastGraphLDown->OnMouseMove(event);
        return;
    }

    if (foobar && m_drag_foobar) {
        int y=event.GetY();
        int x=event.GetX();
        if (x<GetLeftMargin()) return;
        int x1=x-GetLeftMargin();

        int width=m_scrX-GetRightMargin()-GetLeftMargin();
        //int height=m_scrY-GetBottomMargin()-GetTopMargin();

        if (x>m_scrX-GetRightMargin()) return;

        double mx=double(x1)/double(width);

        double rminx=RealMinX();
        double rmaxx=RealMaxX();
        double rx=rmaxx-rminx;

        double qx=rx*mx;
        qx+=rminx;

        // qx is centerpoint of new zoom area.

        double minx=MinX();
        double dx=MaxX()-minx; // zoom rect width;

        // Could smarten this up by remembering where the mouse was clicked on the foobar

        double gx=dx*m_foobar_pos;

        qx-=gx;
        if (qx<rminx) qx=rminx;

        double ex=qx+dx;

        if ((qx+dx)>rmaxx) {
            ex=rmaxx;
            qx=ex-dx;
        }
        m_foobar_moved+=fabs((qx-minx))+fabs((m_mouseLClick.x-x))+fabs((m_mouseLClick.y-y));
        SetXBounds(qx,ex);
        if (pref["LinkGraphMovement"]) {
            for (list<gGraphWindow *>::iterator g=link_zoom.begin();g!=link_zoom.end();g++) {
                (*g)->SetXBounds(qx,ex);
            }
        }
    } else
    if (event.m_rightDown) {
        MoveX(event.GetX() - m_mouseRClick.x);
        m_mouseRClick.x=event.GetX();
        double min=MinX();
        double max=MaxX();
        if (pref["LinkGraphMovement"]) {
            for (list<gGraphWindow *>::iterator g=link_zoom.begin();g!=link_zoom.end();g++) {
                (*g)->SetXBounds(min,max);
            }
        }
    } else
    if (event.m_leftDown) {

        int x1=m_mouseLClick.x;
        int x2=event.GetX();
        int t1=MIN(x1,x2);
        int t2=MAX(x1,x2);
        if (t1<=m_marginLeft) t1=m_marginLeft+1;
        if (t2>(m_scrX-m_marginRight)) t2=m_scrX-m_marginRight;

        wxRect r(t1, m_marginBottom, t2-t1, m_scrY-m_marginBottom-m_marginTop);

        m_mouseRBlast=m_mouseRBrect;
        m_mouseRBrect=r;
        RefreshRect(r.Union(m_mouseRBlast),true);

    }
    event.Skip();
}
void gGraphWindow::OnMouseRightDown(wxMouseEvent &event)
{
    if (event.GetY()<GetTopMargin()) // before top margin
        return;
    //else if (event.GetY()>m_scrY-GetBottomMargin()) {  // after top margin
    //    return;
    //}


    // inside the margin area..
    m_mouseRClick.x = event.GetX();
    m_mouseRClick.y = event.GetY();

    m_mouseRClick_start=m_mouseRClick;
    m_mouseRDown=true;

    event.Skip();
}

void gGraphWindow::OnMouseRightRelease(wxMouseEvent &event)
{
    // Do this properly with real hotspots later..


    double zoom_fact=2;
    if (event.GetY()<GetTopMargin())
        return;
    else if (event.GetY()>m_scrY-GetBottomMargin()) {
        if (!foobar) return;
    }

    if (event.ControlDown()) zoom_fact=5.0;
    if (abs(event.GetX()-m_mouseRClick_start.x)<3 && abs(event.GetY()-m_mouseRClick_start.y)<3) {
        for (list<gGraphWindow *>::iterator g=link_zoom.begin();g!=link_zoom.end();g++) {
            (*g)->ZoomX(zoom_fact,0);
        }
        if (!m_block_zoom) {
            ZoomX(zoom_fact,0); //event.GetX()); // adds origin to zoom out.. Doesn't look that cool.
        }
    } else {
        double min=MinX();
        double max=MaxX();
        for (list<gGraphWindow *>::iterator g=link_zoom.begin();g!=link_zoom.end();g++) {
            (*g)->SetXBounds(min,max);
        }
    }

    m_mouseRDown=false;

    event.Skip();
}

void gGraphWindow::OnMouseLeftDClick(wxMouseEvent &event)
{
   //wxLogMessage(wxT("WTF?? STUPID MOUSEz"));
   OnMouseLeftDown(event);

}

void gGraphWindow::OnMouseLeftDown(wxMouseEvent &event)
{
    int y=event.GetY();
    int x=event.GetX();
    int width=m_scrX-GetRightMargin()-GetLeftMargin();
    int height=m_scrY-GetBottomMargin()-GetTopMargin();
    wxRect hot1(GetLeftMargin(),GetTopMargin(),width,height); // Graph data area.
    m_mouseLClick.x = x;
    m_mouseLClick.y = y;

    if (foobar && (y>(m_scrY-GetBottomMargin())) && (y<(m_scrY-GetBottomMargin())+20) ) {
        double rx=RealMaxX()-RealMinX();
        double qx=double(width)/rx;
        double minx=MinX()-RealMinX();
        double maxx=MaxX()-RealMinX();;

        int x1=(qx*minx);
        int x2=(qx*maxx);  // length in pixels
        int xw=x2-x1;

        x1+=GetLeftMargin();
        x2+=GetLeftMargin();
        if ((x>x1) && (x<x2)) {
            double xj=x-x1;
            m_foobar_pos=(1.0/double(xw))*xj; // where along the foobar the user clicked.
            m_foobar_moved=0;
            m_drag_foobar=true;
         //   wxLogMessage("Foobar Area Pushed");
        } else m_drag_foobar=false;
    } else if (hot1.Contains(x,y)) {
        m_mouseLDown=true;
    }
    LastGraphLDown=this;

    event.Skip();

}
void gGraphWindow::OnMouseLeftRelease(wxMouseEvent &event)
{
    if (LastGraphLDown && (LastGraphLDown!=this)) { // Same graph that initiated the click??
        LastGraphLDown->OnMouseLeftRelease(event);  // Nope.. Give it the event.
        return;
    }

    int y=event.GetY();
    int x=event.GetX();
    int width=m_scrX-GetRightMargin()-GetLeftMargin();
    int height=m_scrY-GetBottomMargin()-GetTopMargin();
    wxRect hot1(GetLeftMargin(),GetTopMargin(),width,height); // Graph data area.

    bool zoom_in=false;
    bool did_draw=false;

    // Finished Dragging the FooBar?
    if (foobar && m_drag_foobar) {
        if (m_foobar_moved<5) {
            double zoom_fact=0.5;
            if (event.ControlDown()) zoom_fact=0.25;
            if (!m_block_zoom) {
                ZoomX(zoom_fact,0);
            }
            m_foobar_moved=0;
            did_draw=true;
        }
    }

    if (!did_draw && !zoom_in && m_mouseLDown) {
        wxPoint release(event.GetX(), m_scrY-m_marginBottom);
        wxPoint press(m_mouseLClick.x, m_marginTop);
        int x1=m_mouseRBrect.x;
        int x2=x1+m_mouseRBrect.width;
        int t1=MIN(x1,x2);
        int t2=MAX(x1,x2);

        if ((t2-t1)>4) {
            // Range Selected
            ZoomXPixels(t1,t2);
            did_draw=true;
        }

    }


    if (!did_draw && hot1.Contains(x,y) && !m_drag_foobar && m_mouseLDown) {
        int xp=x;
        if (zoom_in) xp=0;
        double zoom_fact=0.5;
        if (event.ControlDown()) zoom_fact=0.25;
        //for (list<gGraphWindow *>::iterator g=link_zoom.begin();g!=link_zoom.end();g++) {
        //    (*g)->ZoomX(zoom_fact,xp);
        //}
        if (!m_block_zoom) {
            ZoomX(zoom_fact,xp); //event.GetX()); // adds origin to zoom in.. Doesn't look that cool.
        }
        did_draw=true;
    }

    m_drag_foobar=false;
    wxRect r=m_mouseRBrect;
    m_mouseRBrect=wxRect(0, 0, 0, 0);
    m_mouseLDown=false;
    m_drag_foobar=false;
    if (!did_draw) { // Should never happen.
        if (r!=m_mouseRBrect)
            Refresh();
    //} else { // Update any linked graphs..
    }
        double min=MinX();
        double max=MaxX();
        for (list<gGraphWindow *>::iterator g=link_zoom.begin();g!=link_zoom.end();g++) {
            (*g)->SetXBounds(min,max);
        }
    //}
    LastGraphLDown=NULL;
    event.Skip();
}


/*void gGraphWindow::Update()
{
    Refresh();
} */

void gGraphWindow::SetMargins(float top, float right, float bottom, float left)
{
    m_marginTop=top;
    m_marginBottom=bottom;
    m_marginLeft=left;
    m_marginRight=right;
}

wxGLContext *shared_context=NULL;

pBuffer *pbuffer=NULL;

wxBitmap * gGraphWindow::RenderBitmap(int width,int height)
{
    if (!pbuffer) {
        try {
#if defined(__WXMSW__)
            pbuffer=new pBufferWGL(width,height);
#elif defined(__WXMAC__) || defined(__WXDARWIN__)
            pbuffer=new pBufferAGL(width,height);
#elif defined(__UNIX__)
            pbuffer=new pBufferGLX(width,height);
#endif
        } catch(GLException e) {
            // Should log already if failed..
            pbuffer=NULL;
            return NULL;
        }
    }

    if (pbuffer) {
        pbuffer->UseBuffer(true);
    }
    // Move this bitmap code to pBuffer

    // glClearColor(1,1,0,1);
    glClear(GL_COLOR_BUFFER_BIT);

    // Can't use font's in multiple contexts
    Render(width,height);

    void* pixels = malloc(3 * width * height); // must use malloc
    glPixelStorei(GL_PACK_ALIGNMENT, 1);

    //glDrawBuffer(GL_BACK_LEFT);
    //glReadBuffer(GL_FRONT);
    glReadBuffer( GL_BACK_LEFT );

    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels);

    // Put the image into a wxImage
    wxImage image(width, height, true);
    image.SetData((unsigned char*) pixels);
    image = image.Mirror(false);
    glFlush();

    wxBitmap *bmp=new wxBitmap(image);


    if (pbuffer) {
        //delete pbuffer;
    }

    return bmp;
}

void gGraphWindow::Render(float scrX, float scrY)
{
    glViewport(0, 0, scrX, scrY);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glOrtho(0, scrX, 0, scrY, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    //glDisable(GL_LIGHTING);

    glBegin(GL_QUADS);
    glColor3f(1.0,1.0,1.0); // Gradient start
    glVertex2f(0, scrY);
    glVertex2f(0, 0);

    glColor3f(0.8,0.8,1.0); // Gradient End
    glVertex2f(scrX, 0);
    glVertex2f(scrX, scrY);
    glEnd();

    glEnable(GL_TEXTURE_2D);

    for (list<gLayer *>::iterator l=layers.begin();l!=layers.end();l++) {
      (*l)->Plot(*this,scrX,scrY);
    }

    glEnable(GL_DEPTH_TEST);
}

void gGraphWindow::OnPaint(wxPaintEvent& event)
{
    // Shouldn't need this anymore as opengl double buffers anyway.

//#if defined(__WXMSW__)
//    wxAutoBufferedPaintDC dc(this);
//#else
    wxPaintDC dc(this);
//#endif

//#if wxCHECK_VERSION(2,9,0)
    //SetCurrent(*shared_context);
//#else

#if defined(__DARWIN__) && !wxCHECK_VERSION(2,9,0)
    shared_context->SetCurrent();
#else
    shared_context->SetCurrent(*this);
#endif

//#endif

#if !defined(__WXMAC__) && defined (__UNIX__)
    real_shared_context = glXGetCurrentContext();
#endif

    GetClientSize(&m_scrX, &m_scrY);

    Render(m_scrX,m_scrY);

    if (m_mouseLDown) {
        if (m_mouseRBrect.width>0)
            glDisable(GL_DEPTH_TEST);
            RoundedRectangle(m_mouseRBrect.x,m_mouseRBrect.y,m_mouseRBrect.width-1,m_mouseRBrect.height,5,*wxDARK_GREY);
            glEnable(GL_DEPTH_TEST);
    }


    SwapBuffers(); // Dump to screen.

    event.Skip();
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
    double val=0,tmp;
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
    double val=0,tmp;
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
    double val=0,tmp;
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
    double val=0,tmp;
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
    double val=0,tmp;
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
    double val=0,tmp;
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
    double val=0,tmp;
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
    double val=0,tmp;
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
        color.push_back(*col);
    }
}
gXAxis::~gXAxis()
{
}
void gXAxis::Plot(gGraphWindow & w,float scrx,float scry)
{
    float px,py;

    //int start_px=w.GetLeftMargin();
    //int start_py=w.GetTopMargin();
    float width=scrx-(w.GetLeftMargin()+w.GetRightMargin());
//    float height=scry-(w.GetTopMargin()+w.GetBottomMargin());

    double xx=w.max_x-w.min_x;

    if (xx==0) return;

    wxString fd;
    if (xx<1.5) {
        fd=wxT("00:00:00:0000");
    } else {
        fd=wxT("XX XXX");
    }
    float x,y;
    GetTextExtent(fd,x,y);
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

    //.Clip(start_px-10,start_py+height,width+20,w.GetBottomMargin());
    double st3=st;
    while (st3>w.min_x) {
        st3-=min_tick/10.0;
    }
    st3+=min_tick/10.0;

    py=w.GetBottomMargin();

    glLineWidth(0.25);
    glColor3f(0,0,0);
    for (double i=st3; i<=w.max_x; i+=min_tick/10.0) {
        if (i<w.min_x) continue;
        //px=x2p(w,i);
        px=w.x2p(i); //w.GetLeftMargin()+((i - w.min_x) * xmult);
        glBegin(GL_LINES);
        glVertex2f(px,py);
        glVertex2f(px,py-4);
        glEnd();
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
        glColor3f(0,0,0);
        glBegin(GL_LINES);
        glVertex2f(px,py);
        glVertex2f(px,py-6);
        glEnd();

        GetTextExtent(fd,x,y);

        // There is a wx2.8 bug in wxMSW that screws up calculating x properly.
        const int offset=0;

        if (!show_time) {
            DrawText(fd, px-(y/2)-2, py-(x/2)-14+offset, 90.0,*wxBLACK);

        } else {
            DrawText(fd, px-(x/2), py-14-y);
        }

    }

}


gYAxis::gYAxis(const wxColor * col)
:gLayer(NULL)
{
    if (col) {
        color.clear();
        color.push_back(*col);
    }
    m_show_major_lines=true;
    m_show_minor_lines=true;
}
gYAxis::~gYAxis()
{
}
void gYAxis::Plot(gGraphWindow &w,float scrx,float scry)
{
    static wxColor wxDARK_GREY(0xA0,0xA0,0xA0,0xA0);
    static wxPen pen1(*wxLIGHT_GREY, 1, wxDOT);
    static wxPen pen2(wxDARK_GREY, 1, wxDOT);
    float x,y;
    int labelW=0;

    double miny=w.min_y;
    double maxy=w.max_y;
    if (maxy==miny)
        return;
    if ((w.max_x-w.min_x)==0) return;

    int start_px=w.GetLeftMargin();
    int start_py=w.GetBottomMargin();
    int width=scrx-(w.GetLeftMargin()+w.GetRightMargin());
    int height=scry-(w.GetTopMargin()+w.GetBottomMargin());


    const wxColor & linecol1=*wxLIGHT_GREY;
    const wxColor & linecol2=wxDARK_GREY;

    wxString fd=wxT("0");
    GetTextExtent(fd,x,y);
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
    glColor3f(0,0,0);
    glLineWidth(0.25);
    for (float i=w.min_y; i<w.max_y; i+=min_ytick/2) {
		ty=(i - w.min_y) * ymult;
        h=(start_py+height)-ty;
        glBegin(GL_LINES);
        glVertex2f(start_px-4, h);
        glVertex2f(start_px, h);
        glEnd();
    }

    glColor4ub(linecol1.Red(),linecol1.Green(),linecol1.Blue(),linecol1.Alpha());
    for (double i=w.min_y; i<w.max_y; i+=min_ytick/2) {
		ty=(i - w.min_y) * ymult;
        h=start_py+ty;
        if (m_show_minor_lines && (i > w.min_y)) {
            glBegin(GL_LINES);
            glVertex2f(start_px+1, h);
            glVertex2f(start_px+width, h);
            glEnd();
        }
    }

    for (double i=w.min_y; i<=w.max_y; i+=min_ytick) {
		ty=(i - w.min_y) * ymult;
        fd=Format(i); // Override this as a function.
        GetTextExtent(fd,x,y);
        if (x>labelW) labelW=x;
        h=start_py+ty;
        DrawText(fd,start_px-8-x,h - (y / 2));

        glColor3f(0,0,0);
        glBegin(GL_LINES);
        glVertex2f(start_px-6, h);
        glVertex2f(start_px, h);
        glEnd();

        if (m_show_major_lines && (i > w.min_y)) {
            glColor4ub(linecol1.Red(),linecol1.Green(),linecol1.Blue(),linecol1.Alpha());

            glBegin(GL_LINES);
            glVertex2f(start_px+1, h);
            glVertex2f(start_px+width, h);
            glEnd();
        }
	}
}

gGraphTitle::gGraphTitle(const wxString & _title,wxOrientation o, const wxColor * color)
:gLayer(NULL),m_title(_title),m_orientation(o),m_color((wxColor *)color) //m_font((wxFont*)font),
{
    m_textheight=m_textwidth=0;
}
gGraphTitle::~gGraphTitle()
{
}
void gGraphTitle::Plot(gGraphWindow & w,float scrx,float scry)
{
    if (!m_visible) return;

    wxAlignment m_alignment=wxALIGN_LEFT;

    float width,height;
    if (m_orientation==wxHORIZONTAL) {
        GetTextExtent(m_title,width,height);
        DrawText(m_title,4,scrx-height,0);
    } else {
        GetTextExtent(m_title,width,height);
        int xp=(height/2)+5;
        if (m_alignment==wxALIGN_RIGHT) xp=scrx-4-height;
        DrawText(m_title,xp,w.GetBottomMargin()+((scry-w.GetBottomMargin())/2.0)+(height/2),90.0,*wxBLACK);
    }

}


gFooBar::gFooBar(const wxColor * col1,const wxColor * col2)
:gLayer(NULL)
{
    if (col1 && col2) {
        color.clear();
        color.push_back(*col2);
        color.push_back(*col1);
    }
}
gFooBar::~gFooBar()
{
}
void gFooBar::Plot(gGraphWindow & w,float scrx,float scry)
{
    if (!m_visible) return;

    double xx=w.max_x-w.min_x;

    if (xx==0)
        return;

    int start_px=w.GetLeftMargin();
    int width=scrx - (w.GetLeftMargin() + w.GetRightMargin());
   // int height=scry - (w.GetTopMargin() + w.GetBottomMargin());

    wxColor & col1=color[0];
    wxColor & col2=color[1];

    float h=w.GetBottomMargin()-10;
    glColor4ub(col1.Red(),col1.Green(),col1.Blue(),col1.Alpha());

    glLineWidth(1);
    glBegin(GL_LINES);
    glVertex2f(start_px, h);
    glVertex2f(start_px+width, h);
    glEnd();

    double rmx=w.rmax_x-w.rmin_x;
    double px=((1/rmx)*(w.min_x-w.rmin_x))*width;
    double py=((1/rmx)*(w.max_x-w.rmin_x))*width;

    glColor4ub(col2.Red(),col2.Green(),col2.Blue(),col2.Alpha());
    glLineWidth(4);
    glBegin(GL_LINES);
    glVertex2f(start_px+px,h);
    glVertex2f(start_px+py,h);
    glEnd();
}

gCandleStick::gCandleStick(gPointData *d,wxOrientation o)
:gLayer(d)
{
    m_direction=o;
}
gCandleStick::~gCandleStick()
{
}
void gCandleStick::Plot(gGraphWindow & w,float scrx,float scry)
{
    if (!m_visible) return;
    if (!data) return;
    if (!data->IsReady()) return;

    int start_px=w.GetLeftMargin();
    int start_py=w.GetBottomMargin();
    int width=scrx-(w.GetLeftMargin()+w.GetRightMargin())-1;
    int height=scry-(w.GetTopMargin()+w.GetBottomMargin())-1;

    float sum=0;
    for (int i=0;i<data->np[0];i++)
        sum+=data->point[0][i].m_y;

    float pxr;
    float px;
    if (m_direction==wxVERTICAL) {
        pxr=height/sum;
        px=start_py;
    } else {
        pxr=width/sum;
        px=start_px;
    }

    float x,y;

    float t1,t2;
    int barwidth;
    if (m_direction==wxVERTICAL) {
        barwidth=width;
    } else {
        barwidth=height;
    }

    wxString str;
    for (int i=0;i<data->np[0];i++) {
        t1=floor(px);
        t2=data->point[0][i].m_y*pxr;
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

        const wxColor & col1=color[i % color.size()];
        const wxColor & col2=*wxLIGHT_GREY;

        glBegin(GL_QUADS);
        glColor4ub(col1.Red(),col1.Green(),col1.Blue(),col1.Alpha());
        glVertex2f(rect.x, rect.y+height);
        glVertex2f(rect.x+rect.width, rect.y+height);

        glColor4ub(col2.Red(),col2.Green(),col2.Blue(),col2.Alpha());
        glVertex2f(rect.x+rect.width, rect.y);
        glVertex2f(rect.x, rect.y);
        glEnd();

        wxColor c(0,0,0,255);
        LinedRoundedRectangle(rect.x,rect.y,rect.width,rect.height,0,1,c);


        str=wxT("");
        if ((int)m_names.size()>i) {
            str=m_names[i]+wxT(" ");
        }
        str+=wxString::Format(wxT("%0.1f"),data->point[0][i].m_x);
        GetTextExtent(str, x, y);
        x+=5;
        if (t2>x) {
            int j=t1+((t2/2)-(x/2));
            if (m_direction==wxVERTICAL) {
                DrawText(str,start_px+barwidth+2+y,j,270.0,*wxBLACK);
            } else {
                DrawText(str,j,start_py+(barwidth/2)-(y/2)+1);
            }
        }

    }

}

gBarChart::gBarChart(gPointData *d,const wxColor *col,wxOrientation o)
:gLayer(d),m_direction(o)
{
    if (col) {
        color.clear();
        color.push_back(*col);
    }
    Xaxis=new gXAxis(wxBLACK);
}
gBarChart::~gBarChart()
{
    delete Xaxis;
}

void gBarChart::Plot(gGraphWindow & w,float scrx,float scry)
{
    if (!m_visible) return;
    if (!data) return;
    if (!data->IsReady()) return;

    int start_px=w.GetLeftMargin();
    int start_py=w.GetBottomMargin();
    int width=scrx-(w.GetLeftMargin()+w.GetRightMargin());
    int height=scry-(w.GetTopMargin()+w.GetBottomMargin());

    double xx=w.max_x - w.min_x;
    double days=int(xx);
    //days=data->np[0];

    days=0;
    for (int i=0;i<data->np[0];i++) {
       if ((data->point[0][i].m_x >= w.min_x) && (data->point[0][i].m_x<w.max_x)) days+=1;
    }
    if (days==0) return;

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
    float textX, textY;

    wxString str;
    bool draw_xticks_instead=false;

    for (int i=0;i<data->np[0];i++) {
        if (data->point[0][i].m_x < w.min_x) continue;
        if (data->point[0][i].m_x >= w.max_x) break;
        t1=px;
        px+=barwidth+1;
        t2=px-t1-1;

        wxRect rect;
        wxDirection dir;

        u2=data->point[0][i].m_y*pxr;
        u1=start_py;

        if (m_direction==wxVERTICAL) {
            rect=wxRect(start_px,t1,u2,t2);
        } else {
            rect=wxRect(t1,u1,t2,u2);
        }
        dir=wxEAST;
        //RoundedRectangle(rect.x,rect.y,rect.width,rect.height,1,color[0]); //,*wxLIGHT_GREY,dir);

        // TODO: Put this in a function..
        wxColor & col1=color[0];
        wxColor & col2=*((wxColor *)wxLIGHT_GREY);
        glBegin(GL_QUADS);
        //red color
        glColor4ub(col1.Red(),col1.Green(),col1.Blue(),col1.Alpha());
        glVertex2f(rect.x, rect.y+rect.height);
        glVertex2f(rect.x, rect.y);
        //blue color
        glColor4ub(col2.Red(),col2.Green(),col2.Blue(),col2.Alpha());
        glVertex2f(rect.x+rect.width,rect.y);
        glVertex2f(rect.x+rect.width, rect.y+rect.height);
        glEnd();


        wxColor c(0,0,0,255);
        LinedRoundedRectangle(rect.x,rect.y,rect.width,rect.height,0,1,c);

        str=FormatX(data->point[0][i].m_x);

        GetTextExtent(str, textX, textY);
        if (t2>textY) {
            int j=t1+((t2/2)-(textY/2));
            if (m_direction==wxVERTICAL) {
                DrawText(str,start_px-textX-8,j);
            } else {
                DrawText(str,j,start_py-16-(textX/2),90,*wxBLACK);
            }
        } else draw_xticks_instead=true;

    }
    if (draw_xticks_instead)
        Xaxis->Plot(w,scrx,scry);

    glColor3f (0.1F, 0.1F, 0.1F);
    glLineWidth (1);
    glBegin (GL_LINES);
    glVertex2f (start_px, start_py);
    glVertex2f (start_px, start_py+height);
    glVertex2f (start_px,start_py);
    glVertex2f (start_px+width, start_py);
    glEnd ();

}

gLineChart::gLineChart(gPointData *d,const wxColor * col,int dlsize,bool _accelerate,bool _hide_axes,bool _square_plot)
:gLayer(d),m_accelerate(_accelerate),m_drawlist_size(dlsize),m_hide_axes(_hide_axes),m_square_plot(_square_plot)
{
    m_drawlist=new wxRealPoint [dlsize];
    color.clear();
    color.push_back(*col);
    m_report_empty=false;
}
gLineChart::~gLineChart()
{
    delete [] m_drawlist;
}

// Time Domain Line Chart
void gLineChart::Plot(gGraphWindow & w,float scrx,float scry)
{

    if (!m_visible) return;
    if (!data) return;
    if (!data->IsReady()) return;

    int start_px=w.GetLeftMargin(), start_py=w.GetBottomMargin();

    int width=scrx-(w.GetLeftMargin()+w.GetRightMargin());
    int height=scry-(w.GetTopMargin()+w.GetBottomMargin());

    double minx=w.min_x, miny=w.min_y, maxx=w.max_x, maxy=w.max_y;
    double xx=maxx-minx, yy=maxy-miny;
    float xmult=width/xx, ymult=height/yy;   // time to pixel conversion multiplier

    // Return on screwy min/max conditions
    if ((xx<0) || (yy<0)) return;
    if ((yy==0) && (miny==0)) return;

    int num_points=0;
    for (int z=0;z<data->VC();z++) num_points+=data->np[z]; // how many points all up?

    // Draw bounding box if something else will be drawn.
    if (!(!m_report_empty && !num_points)) {
        glColor3f (0.1F, 0.1F, 0.1F);
        glLineWidth (1);
        glBegin (GL_LINE_LOOP);
        glVertex2f (start_px, start_py);
        glVertex2f (start_px, start_py+height);
        glVertex2f (start_px+width,start_py+height);
        glVertex2f (start_px+width, start_py);
        glEnd ();
    }

    width--;
    if (!num_points) { // No Data?
        if (m_report_empty) {
            wxString msg=_("No Waveform Available");
            float x,y; //,descent,leading;
            GetTextExtent(msg,x,y); //,largefont);//,&descent,&leading);
            DrawText(msg,start_px+(width/2.0)-(x/2.0),start_py+(height/2.0)-(y/2.0),0,*wxDARK_GREY); //,largefont);
        }
        return;
    }

    bool accel=m_accelerate;
    double sfit,sam,sr;
    int dp;

    wxColor & col=color[0];
    // Selected the plot line color
    glColor4ub(col.Red(),col.Green(),col.Blue(),col.Alpha());

    // Crop to inside the margins.
    glScissor(w.GetLeftMargin(),w.GetBottomMargin(),width,height);
    glEnable(GL_SCISSOR_TEST);

    glLineWidth (.25);
    //glEnable(GL_LINE_SMOOTH);
    //glHint(GL_LINE_SMOOTH_HINT,  GL_NICEST);
    glBegin (GL_LINES); //_LOOP);

    float lastpx,lastpy;
    float px,py;
    for (int n=0;n<data->VC();n++) {
        dp=0;
        int & siz=data->np[n];
        // Don't bother drawing 1 point or less.
        if (siz<=1) continue;

        bool done=false;
        bool first=true;
        wxPoint2DDouble * point=data->point[n];

        // Calculate the number of points to skip when too much data.
        if (accel) {
            sr=point[1].m_x-point[0].m_x; // Time distance between points
            sfit=xx/sr;
            sam=sfit/width;
            if (sam<=8) { // Don't accelerate if threshold less than this.
                accel=false;
                sam=1;
            } else {
                sam/=25;   // lower this number the more data is skipped over (and the faster things run)
                if (sam<=1) {
                    sam=1;
                    accel=false;
                }
            }

        } else sam=1;

        // Prepare the min max y values if we still are accelerating this plot
        if (accel) {
            for (int i=0;i<width;i++) {
                m_drawlist[i].x=height;
                m_drawlist[i].y=0;
            }
        }
        int minz=width,maxz=0;

        // Technically shouldn't never ever get fed reverse data.
        assert(point[0].m_x < point[siz-1].m_x);

        bool firstpx=true;
        for (int i=0;i<siz;i+=sam) {

                if (point[i].m_x < minx) continue; // Skip stuff before the start of our data window

                if (first) {
                    first=false;
                    if (i>=sam)  i-=sam; // Start with the previous sample (which will be in clipping area)
                }

                if (point[i].m_x > maxx) done=true; // Let this iteration finish.. (This point will be in far clipping)

            px=1+((point[i].m_x - minx) * xmult);   // Scale the time scale X to pixel scale X
            py=1+((point[i].m_y - miny) * ymult);   // Same for Y scale


            if (!accel) {
                if (firstpx) {
                    firstpx=false;
                } else {
                    if (m_square_plot) {
                        glVertex2f(lastpx,lastpy);
                        glVertex2f(start_px+px,lastpy);
                        glVertex2f(start_px+px,lastpy);
                    } else {
                        glVertex2f(lastpx,lastpy);
                    }
                    glVertex2f(start_px+px,start_py+py);
                }
                lastpx=start_px+px;
                lastpy=start_py+py;
            } else {
                // Just clip ugly in accel mode.. Too darn complicated otherwise
                if (px<0) {
                    px=0;
                }
                if (px>width) {
                    px=width;
                }
                // In accel mode, each pixel has a min/max Y value.
                // m_drawlist's index is the pixel index for the X pixel axis.

                int z=round(px);
                if (z<minz) minz=z;  // minz=First pixel
                if (z>maxz) maxz=z;  // maxz=Last pixel

                // Update the Y pixel bounds.
                if (py<m_drawlist[z].x) m_drawlist[z].x=py;
                if (py>m_drawlist[z].y) m_drawlist[z].y=py;

            }

            if (done) break;
        }

        if (accel) {
            dp=0;
            // Plot compressed accelerated vertex list
            for (int i=minz;i<maxz;i++) {
                glVertex2f(start_px+i+1,start_py+m_drawlist[i].x);
                glVertex2f(start_px+i+1,start_py+m_drawlist[i].y);
            }
        }
    }
    glEnd ();
    //glDisable(GL_LINE_SMOOTH);
    glDisable(GL_SCISSOR_TEST);
}

gLineOverlayBar::gLineOverlayBar(gPointData *d,const wxColor * col,wxString _label,LO_Type _lot)
:gLayer(d),label(_label),lo_type(_lot)
{
    color.clear();
    color.push_back(*col);
}
gLineOverlayBar::~gLineOverlayBar()
{
}
//nov = number of vertex
//r = radius
void Dot(int nov, float r)
{
	if (nov < 4) nov = 4;
	if (nov > 360) nov = 360;
	float angle = (360/nov)*(3.142159/180);
	float x[360] = {0};
	float y[360] = {0};
	for (int i = 0; i < nov; i++){
		x[i] = cosf(r*cosf(i*angle));
		y[i] = sinf(r*sinf(i*angle));
		glBegin(GL_POLYGON);
		for (int i = 0; i < nov; i++){
			glVertex2f(x[i],y[i]);
		}
        glEnd();
	}
	//render to texture and map to GL_POINT_SPRITE
}
void gLineOverlayBar::Plot(gGraphWindow & w,float scrx,float scry)
{
    if (!m_visible) return;
    if (!data) return;
    if (!data->IsReady()) return;

    //int start_px=w.GetLeftMargin();
    int start_py=w.GetBottomMargin();
    int width=scrx-(w.GetLeftMargin()+w.GetRightMargin());
    int height=scry-(w.GetTopMargin()+w.GetBottomMargin());

    double xx=w.max_x-w.min_x;
    if (xx<=0) return;

    float x1,x2;

    float x,y;//,descent,leading;

    // Crop to inside the margins.
    glScissor(w.GetLeftMargin(),w.GetBottomMargin(),width,height);
    glEnable(GL_SCISSOR_TEST);

    wxColor & col=color[0];
    for (int n=0;n<data->VC();n++) {

       // bool done=false;
        bool first=true;
        int fi=0,li;
        for (li=0;li<data->np[n];li++) {
            // m_x & m_y are both x axis's here.. (m_y is only used for spans)

            if (data->point[n][li].m_y < w.min_x) continue;
            if (data->point[n][li].m_x > w.max_x) break;
            if (first) {
                first=false;
                if (li>0)  li--;
                fi=li;
            }
        }
        glColor4ub(col.Red(),col.Green(),col.Blue(),col.Alpha());
        if (lo_type==LOT_Bar) {
            glLineWidth (0.25);
            glBegin(GL_LINES);
            float bottom=start_py+25, top=start_py+height-25;

            // Draw any lines
            for (int i=fi;i<li;i++) {
                wxPoint2DDouble & rp=data->point[n][i];
                if (rp.m_x==rp.m_y) {
                    x1=w.x2p(rp.m_x);
                    glVertex2f(x1,top);
                    glVertex2f(x1,bottom);
                }
            }
            glEnd();

            //glEnable(GL_POINT_SPRITE);
            // Draw the dots above
            //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            Dot(100,50);
            glPointSize(5);
            glEnable(GL_POINT_SMOOTH);
            glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
            glBegin(GL_POINTS);
            for (int i=fi;i<li;i++) {
                wxPoint2DDouble & rp=data->point[n][i];
                if (rp.m_x==rp.m_y) {
                    x1=w.x2p(rp.m_x);
                    glVertex2f(x1,top);
                }
            }
            glEnd();
            glDisable(GL_POINT_SMOOTH);
            //glDisable(GL_POINT_SPRITE);

            // Text Labels & Spans
            for (int i=fi;i<li;i++) {
                wxPoint2DDouble & rp=data->point[n][i];
                x1=w.x2p(rp.m_x);
                x2=w.x2p(rp.m_y);
                if (rp.m_x==rp.m_y) {
                    if (xx<(1800.0/86400)) {
                        GetTextExtent(label,x,y);
                        DrawText(label,x1-(x/2),start_py+height-30+y);
                   }
                } else {
                    float w1=x2-x1;
                    RoundedRectangle(x1,start_py,w1,height,2,col);
                }
            }
        } else if (lo_type==LOT_Dot) {
            //glEnable(GL_BLEND);
            glEnable(GL_POINT_SMOOTH);
            glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
            glPointSize(4);
            glBegin(GL_POINTS);
            for (int i=fi;i<li;i++) {
                wxPoint2DDouble & rp=data->point[n][i];
                x1=w.x2p(rp.m_x);

                glVertex2f(x1,start_py+(height/2)+14);
            }
            glEnd();
            glDisable(GL_POINT_SMOOTH);
            //glDisable(GL_BLEND);
        }
    }
    glDisable(GL_SCISSOR_TEST);
}

gFlagsLine::gFlagsLine(gPointData *d,const wxColor * col,wxString _label,int _line_num,int _total_lines)
:gLayer(d),label(_label),line_num(_line_num),total_lines(_total_lines)
{
    color.clear();
    color.push_back(*col);

}
gFlagsLine::~gFlagsLine()
{
}
void gFlagsLine::Plot(gGraphWindow & w,float scrx,float scry)
{
    if (!m_visible) return;
    if (!data) return;
    if (!data->IsReady()) return;

    double xx=w.max_x-w.min_x;
    if (xx<=0) return;

    int start_px=w.GetLeftMargin();
    int start_py=w.GetBottomMargin();
    int width=scrx-(w.GetLeftMargin()+w.GetRightMargin());
    int height=scry-(w.GetTopMargin()+w.GetBottomMargin());

    static wxColor col1=wxColor(0xd0,0xff,0xd0,0xff);
    static wxColor col2=wxColor(0xff,0xff,0xff,0xff);


    float line_h=float(height-2)/float(total_lines);
    line_h=line_h;
    float line_top=(start_py+height-line_h)-line_num*line_h;


    if ((line_num==total_lines-1)) { // last lines responsibility to draw the title.

        glColor3f (0.1F, 0.1F, 0.1F);
        glLineWidth (1);
        glBegin (GL_LINE_LOOP);
        glVertex2f (start_px-1, start_py);
        glVertex2f (start_px-1, start_py+height);
        glVertex2f (start_px+width,start_py+height);
        glVertex2f (start_px+width, start_py);
        glEnd ();


    }

    // Alternating box color
    wxColor *barcol=&col2;
    if (line_num & 1) {
        barcol=&col1;
    }

    // Draw box
    RoundedRectangle(start_px,line_top,width-1,line_h+1,0,*barcol);

    // Draw text label
    float x,y;
    GetTextExtent(label,x,y);
    DrawText(label,start_px-x-6,line_top+(line_h/2)-(y/2));

    float x1,x2,w1;

    wxColor & col=color[0];
    glColor4ub(col.Red(),col.Green(),col.Blue(),col.Alpha());
    int fi=0,li;
    glScissor(w.GetLeftMargin(),w.GetBottomMargin(),width,height);
    glEnable(GL_SCISSOR_TEST);

    for (int n=0;n<data->VC();n++) {

        //bool done=false;
        bool first=true;
        for (li=0;li<data->np[n];li++) { //,done==false
            if (data->point[n][li].m_y < w.min_x) continue;
            if (data->point[n][li].m_x > w.max_x) break;
            if (first) {
                fi=li;
                if (li>0)  li--;
                first=false;
            }
        }
        glLineWidth (1);
        glBegin(GL_LINES);
        float top=floor(line_top)+2;
        float bottom=top+floor(line_h)-3;
        for (int i=fi;i<li;i++) {
            wxPoint2DDouble & rp=data->point[n][i];
            if (rp.m_x==rp.m_y) {
                x1=w.x2p(rp.m_x);
                glVertex2f(x1,top);
                glVertex2f(x1,bottom);
            }
        }
        glEnd();

        bottom=floor(line_h)-4; // just the height

        for (int i=fi;i<li;i++) {
            wxPoint2DDouble & rp=data->point[n][i];
            if (rp.m_x!=rp.m_y) {
                x1=w.x2p(rp.m_x);
                x2=w.x2p(rp.m_y);
                w1=x2-x1;
                RoundedRectangle(x1,top,w1,bottom,0,color[0]);
            }
        }
    }
    glDisable(GL_SCISSOR_TEST);
}


WaveData::WaveData(MachineCode _code, int _size)
:gPointData(_size),code(_code)
{
}
WaveData::~WaveData()
{
}
void WaveData::Reload(Day *day)
{
    vc=0;
    if (!day) {
        m_ready=false;
        return;
    }
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
                wxPoint2DDouble r(st,(*w)[i]);
                st+=rate;
                point[vc][t++]=r;
                assert(t<max_points);
                if (first) {
                    min_y=r.m_y;
                    first=false;
                } else {
                    if (r.m_y<min_y) min_y=r.m_y;
                }
                if (r.m_y>max_y) max_y=r.m_y;
            }
            np[vc]=t;
            vc++;
        }
    }
    min_y=floor(min_y);
    max_y=ceil(max_y);

    //double t1=MAX(fabs(min_y),fabs(max_y));
    // Get clever here..
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


EventData::EventData(MachineCode _code,int _field,int _size,bool _skipzero)
:gPointData(_size),code(_code),field(_field),skipzero(_skipzero)
{
}
EventData::~EventData()
{
}
void EventData::Reload(Day *day)
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
    EventDataType lastp=0;
    for (vector<Session *>::iterator s=day->begin();s!=day->end(); s++) {
        if ((*s)->events.find(code)==(*s)->events.end()) continue;
        if (vc>=(int)point.size()) {
            AddSegment(max_points);
        }

        int t=0;
        EventDataType p;
        for (vector<Event *>::iterator ev=(*s)->events[code].begin(); ev!=(*s)->events[code].end(); ev++) {
            p=(*(*ev))[field];
            if (((p!=0) && skipzero) || !skipzero) {
                wxPoint2DDouble r((*ev)->time().GetMJD(),p);
                point[vc][t++]=r;
                assert(t<max_points);
                if (first) {
                    max_y=min_y=r.m_y;
                    //lastp=p;
                    first=false;
                } else {
                    if (r.m_y<min_y) min_y=r.m_y;
                    if (r.m_y>max_y) max_y=r.m_y;
                }
            } else {
                if ((p!=lastp) && (t>0)) { // There really should not be consecutive zeros.. just in case..
                    np[vc]=t;
                    tt+=t;
                    t=0;
                    vc++;
                    if (vc>=(int)point.size()) {
                        AddSegment(max_points);
                    }
                }
            }
            lastp=p;
        }
        np[vc]=t;
        if (t>0) {
            tt+=t;
            vc++;
        }

    }
    if (tt>0) {
        min_y=floor(min_y);
        max_y=ceil(max_y+1);
        if (min_y>1) min_y-=1;
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
    int lastval=0,val;

    int field=0;

    for (vector<Session *>::iterator s=day->begin();s!=day->end();s++) {
        if ((*s)->events.find(code)==(*s)->events.end()) continue;
        first=true;
        for (vector<Event *>::iterator e=(*s)->events[code].begin(); e!=(*s)->events[code].end(); e++) {
            Event & ev =(*(*e));
            val=ev[field]*10.0;
            if (field > ev.fields()) throw BoundsError();
            if (first) {
                first=false; // only bother setting lastval (below) this time.
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
            point[0][jj].m_x=i/10.0;
            point[0][jj].m_y=(100.0/seconds)*pTime[i].GetSeconds().GetLo();
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
    point[0][0].m_y=day->count(CPAP_Hypopnea)/day->hours();
    point[0][0].m_x=point[0][0].m_y;
    point[0][1].m_y=day->count(CPAP_Obstructive)/day->hours();
    point[0][1].m_x=point[0][1].m_y;
    point[0][2].m_y=day->count(CPAP_ClearAirway)/day->hours();
    point[0][2].m_x=point[0][2].m_y;
    point[0][3].m_y=day->count(CPAP_RERA)/day->hours();
    point[0][3].m_x=point[0][3].m_y;
    point[0][4].m_y=day->count(CPAP_FlowLimit)/day->hours();
    point[0][4].m_x=point[0][4].m_y;
    point[0][5].m_y=(100.0/day->hours())*(day->sum(CPAP_CSR)/3600.0);
    point[0][5].m_x=point[0][5].m_y;
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
            point[vc][c].m_x=v1;
            point[vc][c].m_y=v2;
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
            if (d->machine_type()==MT_CPAP) {
                y=Calc(d);
                z++;
            }
        }
        if (!z) continue;
        if (z>1) y /= z;
        if (first) {
          //  max_x=min_x=x;
            lasty=max_y=min_y=y;
            first=false;
        }
        point[vc][i].m_x=x;
        point[vc][i].m_y=y;
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
        x=point[0][i].m_x;
        if ((x<min_x) || (x>max_x)) continue;
        val+=point[0][i].m_y;
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
