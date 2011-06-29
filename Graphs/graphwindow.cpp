/********************************************************************
 gGraphWindow Implementation
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*********************************************************************/

#include <math.h>
#include <QMouseEvent>
#include "SleepLib/profiles.h"
#include "graphwindow.h"
#include "Graphs/gTitle.h"
gGraphWindow::gGraphWindow(QWidget *parent, const QString & title, QGLWidget * shared,Qt::WindowFlags f)
: QGLWidget(parent,shared, f )
{
    m_scrX   = m_scrY   = 100;
    m_title=title;
    m_mouseRDown=m_mouseLDown=false;
    m_block_zoom=false;
    m_drag_foobar=false;
    m_gradient_background=true;
    m_foobar_pos=0;
    m_foobar_moved=0;
    SetMargins(10, 15, 0, 0);
    lastlayer=NULL;
    ti=QDateTime::currentDateTime();
    gtitle=foobar=xaxis=yaxis=NULL;
    if (!title.isEmpty()) {
        AddLayer(new gTitle(title));
    }

}

gGraphWindow::gGraphWindow(QWidget *parent, const QString & title, QGLContext * context,Qt::WindowFlags f)
: QGLWidget((QGLContext *)context, parent, 0, f )
{
    gl_context=context;
    m_scrX   = m_scrY   = 100;
    m_title=title;
    m_mouseRDown=m_mouseLDown=false;
    SetMargins(10, 15, 0, 0);
    m_block_zoom=false;
    m_drag_foobar=false;
    m_gradient_background=false;
    m_foobar_pos=0;
    m_foobar_moved=0;
    lastlayer=NULL;
    ti=QDateTime::currentDateTime();
    gtitle=foobar=xaxis=yaxis=NULL;

    if (!title.isEmpty()) {
        AddLayer(new gTitle(title));
    }
}

gGraphWindow::~gGraphWindow()
{
    for (list<gLayer *>::iterator l=layers.begin();l!=layers.end();l++) delete (*l);
    layers.clear();
}

#include "gXAxis.h"
#include "gYAxis.h"
#include "gFooBar.h"
void gGraphWindow::AddLayer(gLayer *l) {
    if (l) {
        if (dynamic_cast<gXAxis *>(l)) {
            if (xaxis) {
                qWarning("Can only have one gXAxis per graph");
                return;
            }
            //if (m_marginBottom<gXAxis::Margin)
            m_marginBottom+=gXAxis::Margin;
            xaxis=l;
        }
        if (dynamic_cast<gFooBar *>(l)) {
            if (foobar) {
                qWarning("Can only have one gFooBar per graph");
                return;
            }
            //if (m_marginBottom<gFooBar::Margin)
            m_marginBottom+=gFooBar::Margin;
            foobar=l;
        }
        if (dynamic_cast<gYAxis *>(l)) {
            if (yaxis) {
                qWarning("Can only have one gYAxis per graph");
                return;
            }
            //if (m_marginLeft<gYAxis::Margin)
            m_marginLeft+=gYAxis::Margin;
            yaxis=l;
        }
        if (dynamic_cast<gTitle *>(l)) {
            if (gtitle) {
                qWarning("Can only have one gGraphTitle per graph");
                return;
            }
            //if (m_marginLeft<gTitle::Margin)
            m_marginLeft+=gTitle::Margin;
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

    updateGL();
}
void gGraphWindow::ResetXBounds()
{
    //min_x=minx;
    //max_x=maxx;
    SetMinX(RealMinX());
    SetMaxX(RealMaxX());
  //  updateGL();
}

void gGraphWindow::ZoomXPixels(int x1, int x2)
{
    double rx1=0,rx2=0;
    ZoomXPixels(x1,x2,rx1,rx2);
    if (pref["LinkGraphMovement"].toBool()) {
        for (list<gGraphWindow *>::iterator g=link_zoom.begin();g!=link_zoom.end();g++) {
            (*g)->SetXBounds(rx1,rx2);
        }
    }

    SetXBounds(rx1,rx2);
}
void gGraphWindow::ZoomXPixels(int x1,int x2,double &rx1,double &rx2)
{
    x1-=GetLeftMargin();
    x2-=GetLeftMargin();
    if (x1<0) x1=0;
    if (x2<0) x2=0;
    if (x1>Width()) x1=Width();
    if (x2>Width()) x2=Width();

    double min;
    double max;
    if (!m_block_zoom) {
        min=min_x;
        max=max_x;
    } else {
        min=rmin_x;
        max=rmax_x;
    }

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
    //if (!m_block_zoom) {
        SetXBounds(min,max);
    //}
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
    if (q<hardspan/400) q=hardspan/400;

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
gGraphWindow *LastGraphRDown=NULL;

void gGraphWindow::mouseMoveEvent(QMouseEvent * event)
{
//    static bool first=true;
    static QRect last;

    // grabbed
    if (m_mouseLDown && LastGraphLDown && (LastGraphLDown!=this)) {
        LastGraphLDown->mouseMoveEvent(event);
        return;
    }
    if (m_mouseRDown && LastGraphRDown && (LastGraphRDown!=this)) {
        LastGraphRDown->mouseMoveEvent(event);
        return;
    }

    int y=event->y();
    int x=event->x();
    if (foobar && m_drag_foobar) {
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
        if (m_mouseRDown)
            m_foobar_moved+=fabs((qx-minx))+fabs((m_mouseRClick.x()-x))+fabs((m_mouseRClick.y()-y));
        if (m_mouseLDown)
            m_foobar_moved+=fabs((qx-minx))+fabs((m_mouseLClick.x()-x))+fabs((m_mouseLClick.y()-y));
        //else
        SetXBounds(qx,ex);
        if (pref["LinkGraphMovement"].toBool()) {
            for (list<gGraphWindow *>::iterator g=link_zoom.begin();g!=link_zoom.end();g++) {
                assert((*g)!=this);
                (*g)->SetXBounds(qx,ex);
            }
        }
    } else
    if (event->buttons() & Qt::RightButton) {
        MoveX(x - m_mouseRClick.x());
        m_mouseRClick.setX(x);
        double min=MinX();
        double max=MaxX();
        if (pref["LinkGraphMovement"].toBool()) {
            for (list<gGraphWindow *>::iterator g=link_zoom.begin();g!=link_zoom.end();g++) {
                (*g)->SetXBounds(min,max);
            }
        }
    } else
    if (event->buttons() & Qt::LeftButton) {

        int x1=m_mouseLClick.x();
        int x2=x;
        int t1=MIN(x1,x2);
        int t2=MAX(x1,x2);
        if (t1<m_marginLeft) t1=m_marginLeft;
        if (t2<m_marginLeft) t2=m_marginLeft;
        if (t2>(m_scrX-m_marginRight)) t2=m_scrX-m_marginRight;

        QRect r(t1-1, m_marginBottom, t2-t1, m_scrY-m_marginBottom-m_marginTop);

        m_mouseRBlast=m_mouseRBrect;
        m_mouseRBrect=r;

        // TODO: Only the rect needs clearing, however OpenGL &  wx have reversed coordinate systems.
        updateGL(); //r.Union(m_mouseRBlast),true);
    }
}
void gGraphWindow::mouseDoubleClickEvent(QMouseEvent * event)
{
    // TODO: Retest.. QT might not be so retarded
    if (event->buttons() & Qt::LeftButton) OnMouseLeftDown(event);
    else if (event->buttons() & Qt::RightButton) OnMouseRightDown(event);
}

void gGraphWindow::mousePressEvent(QMouseEvent * event)
{
    if (event->button()==Qt::LeftButton)
        OnMouseLeftDown(event);
    else if (event->button()==Qt::RightButton)
        OnMouseRightDown(event);
}
void gGraphWindow::mouseReleaseEvent(QMouseEvent * event)
{
    if (event->button()==Qt::LeftButton)
        OnMouseLeftRelease(event);
    else if (event->button()==Qt::RightButton)
        OnMouseRightRelease(event);

}

void gGraphWindow::OnMouseRightDown(QMouseEvent * event)
{
    int y=event->y();
    int x=event->x();
    if (y<GetTopMargin()) // before top margin
        return;
    //else if (event.GetY()>m_scrY-GetBottomMargin()) {  // after top margin
    //    return;
    //}


    // inside the margin area..

    int width=m_scrX-GetRightMargin()-GetLeftMargin();
    int height=m_scrY-GetBottomMargin()-GetTopMargin();
    QRect hot1(GetLeftMargin(),GetTopMargin(),width,height); // Graph data area.
    m_mouseRClick.setX(x);
    m_mouseRClick.setY(y);

    m_mouseRClick_start=m_mouseRClick;
    m_mouseRDown=true;

   // m_mouseRDown=true;
    if (foobar && m_block_zoom && (((y>GetTopMargin())) && (y<m_scrY))) {
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
            LastGraphRDown=this;
            return;
            //   wxLogMessage("Foobar Area Pushed");
        } else {
            m_drag_foobar=false;
            m_mouseRDown=false;
            x1=(rx/width)*(x-GetLeftMargin())+rmin_x;
            double z=(max_x-min_x); // width of selected area
            x1-=z/2.0;

            if (x1<rmin_x) x1=rmin_x;
            x2=x1+z;
            if (x2>rmax_x) {
                x2=rmax_x;
                x1=x2-z;
            }
           // SetXBounds(x1,x2);
            LastGraphRDown=this;
        }
    }

}

void gGraphWindow::OnMouseRightRelease(QMouseEvent * event)
{
    if (LastGraphRDown && (LastGraphRDown!=this)) { // Same graph that initiated the click??
        LastGraphRDown->OnMouseLeftRelease(event);  // Nope.. Give it the event.
        return;
    }
    LastGraphRDown=NULL;
    if (!m_mouseRDown) return;
    // Do this properly with real hotspots later..
    int y=event->y();
    int x=event->x();
    int width=m_scrX-GetRightMargin()-GetLeftMargin();
    int height=m_scrY-GetBottomMargin()-GetTopMargin();
    QRect hot1(GetLeftMargin(),GetTopMargin(),width,height); // Graph data area.

    if (m_block_zoom) { // && hot1.Contains(x,y)) {


        //bool zoom_in=false;
        bool did_draw=false;

        // Finished Dragging the FooBar?
        if (foobar && m_drag_foobar) {
            if (m_foobar_moved<5) {
                double zoom_fact=2;
                if (event->modifiers() & Qt::ControlModifier) zoom_fact=5;
                //if (!m_block_zoom) {
                    ZoomX(zoom_fact,0);
                    did_draw=true;
                //}
                m_foobar_moved=0;
            }
        }
        m_drag_foobar=false;

        if (did_draw) {
            if (pref["LinkGraphMovement"].toBool()) {
                double min=MinX();
                double max=MaxX();
                for (list<gGraphWindow *>::iterator g=link_zoom.begin();g!=link_zoom.end();g++) {
                    (*g)->SetXBounds(min,max);
                }
            }
        }

        return;
    }

    double zoom_fact=2;
    if (y<GetTopMargin()) {
        return;
    } else if (y>m_scrY-GetBottomMargin()) {
        if (!foobar) return;
    }

    if (event->modifiers() & Qt::ControlModifier) zoom_fact=5.0;
    if (abs(x-m_mouseRClick_start.x())<3 && abs(y-m_mouseRClick_start.y())<3) {
      //  for (list<gGraphWindow *>::iterator g=link_zoom.begin();g!=link_zoom.end();g++) {
            //(*g)->ZoomX(zoom_fact,0);
        //}
        //if (!m_block_zoom) {
            ZoomX(zoom_fact,0); //event.GetX()); // adds origin to zoom out.. Doesn't look that cool.

            if (pref["LinkGraphMovement"].toBool()) {
                double min=MinX();
                double max=MaxX();

                for (list<gGraphWindow *>::iterator g=link_zoom.begin();g!=link_zoom.end();g++) {
                    (*g)->SetXBounds(min,max);
                }
            }
        //}

    }/* else {
        double min=MinX();
        double max=MaxX();
        for (list<gGraphWindow *>::iterator g=link_zoom.begin();g!=link_zoom.end();g++) {
            (*g)->SetXBounds(min,max);
        }
    }*/

    m_mouseRDown=false;

}

void gGraphWindow::OnMouseLeftDown(QMouseEvent * event)
{
    int y=event->y();
    int x=event->x();
    int width=m_scrX-GetRightMargin()-GetLeftMargin();
    int height=m_scrY-GetBottomMargin()-GetTopMargin();
    QRect hot1(GetLeftMargin(),GetTopMargin(),width,height); // Graph data area.
    m_mouseLClick.setX(x);
    m_mouseLClick.setY(y);

    if (foobar && ((y>(m_scrY-GetBottomMargin())) && (y<(m_scrY)))) { //-GetBottomMargin())+20))) {
        double rx=RealMaxX()-RealMinX();
        double qx=double(width)/rx;
        double minx=MinX()-RealMinX();
        double maxx=MaxX()-RealMinX();;

        int x1=(qx*minx);
        int x2=(qx*maxx);  // length in pixels
        int xw=x2-x1;

        x1+=GetLeftMargin();
        x2+=GetLeftMargin();
        if ((x>x1-4) && (x<x2+4)) {
            double xj=x-x1;
            m_foobar_pos=(1.0/double(xw))*xj; // where along the foobar the user clicked.
            m_foobar_moved=0;
            m_drag_foobar=true;
         //   wxLogMessage("Foobar Area Pushed");
        } else m_drag_foobar=false;
        m_mouseLDown=true;
        LastGraphLDown=this;
    } else if (hot1.contains(x,y)) {
        m_mouseLDown=true;
        LastGraphLDown=this;
    }
}
void gGraphWindow::OnMouseLeftRelease(QMouseEvent * event)
{
    if (LastGraphLDown && (LastGraphLDown!=this)) { // Same graph that initiated the click??
        LastGraphLDown->OnMouseLeftRelease(event);  // Nope.. Give it the event.
        return;
    }

    int y=event->y();
    int x=event->x();
    int width=m_scrX-GetRightMargin()-GetLeftMargin();
    int height=m_scrY-GetBottomMargin()-GetTopMargin();
    QRect hot1(GetLeftMargin(),GetTopMargin(),width,height); // Graph data area.

    bool was_dragging_foo=false;
    bool did_draw=false;

    // Finished Dragging the FooBar?
    if (foobar && m_drag_foobar) {
        m_drag_foobar=false;
        was_dragging_foo=true;
        if (m_foobar_moved<5) {
            double zoom_fact=0.5;
            if (event->modifiers() & Qt::ControlModifier) zoom_fact=0.25;
            //if (!m_block_zoom) {
            ZoomX(zoom_fact,0);
            did_draw=true;
            //}
            m_foobar_moved=0;
        } else m_mouseLDown=false;


    }

    // The user selected a range with the left mouse button
    if (!did_draw && m_mouseLDown) {
        //QPoint release(x, m_scrY-m_marginBottom);
        //QPoint press(m_mouseLClick.x(), m_marginTop);

        int x1=m_mouseRBrect.x();
        int x2=x1+m_mouseRBrect.width();
        int t1=MIN(x1,x2);
        int t2=MAX(x1,x2);

        if ((t2-t1)>4) { // Range Selected less than threshold
            m_mouseLDown=false;
            ZoomXPixels(t1,t2);

            did_draw=true;
        }

    }


    if (!did_draw && hot1.contains(x,y) && !m_drag_foobar && m_mouseLDown) {
        if (m_block_zoom) {
            x-=GetLeftMargin();
            double rx=rmax_x-rmin_x;
            double mx=max_x-min_x;
            if (mx<rx) {
                double qx=(rx/width)*double(x);
                qx+=rmin_x;
                qx-=mx/2.0;
                if (qx<rmin_x) {
                    qx=rmin_x;
                }
                if (qx+mx>rmax_x) {
                    qx=rmax_x-mx;
                }
                SetXBounds(qx,qx+mx);
                did_draw=true;
            }
        } else {
            int xp=x;
            xp=0;
            double zoom_fact=0.5;
            if (event->modifiers() & Qt::ControlModifier) zoom_fact=0.25;
            ZoomX(zoom_fact,xp); //event.GetX()); // adds origin to zoom in.. Doesn't look that cool.
            did_draw=true;
        }
        //}
    }

    m_drag_foobar=false;
    QRect r=m_mouseRBrect;
    m_mouseRBrect=QRect(0, 0, 0, 0);
    m_mouseLDown=false;
    m_drag_foobar=false;
    if (!did_draw) {
        if (r!=m_mouseRBrect)
            updateGL();
    } else {
        if (pref["LinkGraphMovement"].toBool()) {
            double min=MinX();
            double max=MaxX();
            for (list<gGraphWindow *>::iterator g=link_zoom.begin();g!=link_zoom.end();g++) {
                (*g)->SetXBounds(min,max);
            }
        }
    }
    //}
    LastGraphLDown=NULL;
}

void gGraphWindow::SetMargins(float top, float right, float bottom, float left)
{
    m_marginTop=top;
    m_marginBottom=bottom;
    m_marginLeft=left;
    m_marginRight=right;
}

void gGraphWindow::initializeGL()
{
    setAutoFillBackground(false);
    setAutoBufferSwap(false);
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
    m_scrX=width();
    m_scrY=height();

}

void gGraphWindow::resizeGL(int w, int h)
{
    m_scrX=w;
    m_scrY=h;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glOrtho(0, w, 0, h, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Only needs doing on a mac
    updateGL();

}

void gGraphWindow::Render(float w, float h)
{
    /*glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glOrtho(0, width, 0, height, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();*/

    if (m_gradient_background) {
        glBegin(GL_QUADS);
        glColor4f(1.0,1.0,1.0,.5); // Gradient start
        glVertex2f(0, h);
        glVertex2f(0, 0);

        glColor4f(0.8,0.8,1.0,.5); // Gradient End
        glVertex2f(w, 0);
        glVertex2f(w, h);
        glEnd();
    } else {

        glClearColor(255,255,255,0);
        glClear(GL_COLOR_BUFFER_BIT ); //| GL_DEPTH_BUFFER_BIT
    //    glClear(GL_COLOR_BUFFER_BIT);
    }


    for (list<gLayer *>::iterator l=layers.begin();l!=layers.end();l++) {
        (*l)->Plot(*this,w,h);
    }
}

void gGraphWindow::paintGL()
{

    if (m_scrX<=0) m_scrX=width();
    if (m_scrY<=0) m_scrY=height();
    if (m_scrX<=0) return;
    if (m_scrY<=0) return;

    InitGraphs();
    glDisable(GL_DEPTH_TEST);
    Render(m_scrX,m_scrY);

    if (m_mouseLDown) {
        if (m_mouseRBrect.width()>0)
            glDisable(GL_DEPTH_TEST);
            RoundedRectangle(m_mouseRBrect.x(),m_mouseRBrect.y(),m_mouseRBrect.width(),m_mouseRBrect.height(),5,QColor(50,50,50,128));
            glEnable(GL_DEPTH_TEST);
    }
    glEnable(GL_DEPTH_TEST);


    swapBuffers(); // Dump to screen.
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
    QDateTime n=QDateTime::currentDateTime();
    double t=ti.msecsTo(n);
    ti=n;

    if (layer) {
        MinX(); MinY(); MaxX(); MaxY();
        RealMinX(); RealMinY(); RealMaxX(); RealMaxY();
    } else {
        max_x=min_x=0;
    }

    //long l=t.GetMilliseconds().GetLo();
    //wxLogMessage(wxString::Format(wxT("%li"),l));
    if ((t<2)  && (layer!=lastlayer)) {
        lastlayer=layer;
        return;
    }

    lastlayer=layer;
    // This is possibly evil.. It needs to push one refresh event for all layers

    // Assmption currently is Refresh que does skip

  //  updateGL();
}


