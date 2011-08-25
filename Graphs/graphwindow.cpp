// This file is Scheduled for destruction..

/*
 gGraphWindow Implementation
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

#include <math.h>
#include <QDebug>
#include <QLabel>
#include <QMouseEvent>

#ifdef __APPLE__
#include <AGL/agl.h>
#endif

#include "SleepLib/profiles.h"
#include "graphwindow.h"
#include "gTitle.h"
#include "gXAxis.h"
#include "gYAxis.h"
#include "gFooBar.h"

extern QLabel *qstatus2;

gGraphWindow::gGraphWindow(QWidget *parent, const QString & title, QGLWidget * shared,Qt::WindowFlags f)
: QGLWidget(parent,shared, f )
{
    splitter=NULL;
    m_scrX   = m_scrY   = 100;
    m_title=title;
    m_mouseRDown=m_mouseLDown=false;
    m_block_zoom=false;
    m_drag_foobar=false;
    m_dragGraph=false;

    m_gradient_background=true;
    m_foobar_pos=0;
    m_foobar_moved=0;
    SetMargins(10, 15, 0, 0);
    lastlayer=NULL;
    InitGraphs();
    ti=QDateTime::currentDateTime();
    gtitle=foobar=xaxis=yaxis=NULL;
    if (!title.isEmpty()) {
        AddLayer(new gTitle(title,Qt::black,*mediumfont));
    }
    //setAcceptDrops(true);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    rmin_x=rmax_x=0;
    min_x=max_x=0;
    rmin_y=rmax_y=0;
    min_y=max_y=0;
}

/*gGraphWindow::gGraphWindow(QWidget *parent, const QString & title, QGLContext * context,Qt::WindowFlags f)
: QGLWidget((QGLContext *)context, parent, 0, f )
{
    gl_context=context;
    m_scrX   = m_scrY   = 100;
    m_title=title;
    m_mouseRDown=m_mouseLDown=false;
    SetMargins(10, 15, 0, 0);
    m_block_zoom=false;
    m_drag_foobar=false;
    m_dragGraph=false;
    m_gradient_background=false;
    m_foobar_pos=0;
    m_foobar_moved=0;
    lastlayer=NULL;
    ti=QDateTime::currentDateTime();
    gtitle=foobar=xaxis=yaxis=NULL;

    if (!title.isEmpty()) {
        AddLayer(new gTitle(title));
    }
    //setAcceptDrops(true);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
}*/

gGraphWindow::~gGraphWindow()
{
    for (QList<gLayer *>::iterator l=layers.begin();l!=layers.end();l++) delete (*l);
    layers.clear();
}

bool gGraphWindow::isEmpty()
{
    bool empty=true;
    for (QList<gLayer *>::iterator l=layers.begin();l!=layers.end();l++) {
        if (!(*l)->isEmpty()) {
            empty=false;
            break;
        }
    }
    return empty;
}
/*void gGraphWindow::resizeEvent(QResizeEvent *e)
{
    QGLWidget::resizeEvent(e);
}*/


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
        //l->NotifyGraphWindow(this);
        layers.push_back(l);
    }
};


// Sets a new Min & Max X clipping, refreshing the graph and all it's layers.
void gGraphWindow::SetXBounds(qint64 minx, qint64 maxx)
{
    min_x=minx;
    max_x=maxx;

    updateGL();
}
void gGraphWindow::ResetBounds()
{
    min_x=MinX();
    max_x=MaxX();
    min_y=MinY();
    max_y=MaxY();
}

void gGraphWindow::ZoomXPixels(int x1, int x2)
{
    qint64 rx1=0,rx2=0;
    ZoomXPixels(x1,x2,rx1,rx2);
    if (pref["LinkGraphMovement"].toBool()) {
        for (QList<gGraphWindow *>::iterator g=link_zoom.begin();g!=link_zoom.end();g++) {
            (*g)->SetXBounds(rx1,rx2);
        }
    }

    SetXBounds(rx1,rx2);
}
void gGraphWindow::ZoomXPixels(int x1,int x2,qint64 &rx1,qint64 &rx2)
{
    x1-=GetLeftMargin();
    x2-=GetLeftMargin();
    if (x1<0) x1=0;
    if (x2<0) x2=0;
    if (x1>Width()) x1=Width();
    if (x2>Width()) x2=Width();

    qint64 min;
    qint64 max;
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
void gGraphWindow::MoveX(int i,qint64 &min, qint64 & max)
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
    qint64 min,max;
    MoveX(i,min,max);

/*    for (QList<gGraphWindow *>::iterator g=link_zoom.begin();g!=link_zoom.end();g++) {
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


    // Okay, I want it to zoom in centered on the mouse click area..
    // Find X graph position of mouse click
    // find current zoom width
    // apply zoom
    // center on point found in step 1.

    qint64 min=min_x;
    qint64 max=max_x;

    double hardspan=rmax_x-rmin_x;
    double span=max-min;
    double ww=double(origin_px) / double(Width());
    double origin=ww * span;
    //double center=0.5*span;
    //double dist=(origin-center);

    double q=span*mult;
    if (q>hardspan) q=hardspan;
    if (q<hardspan/400.0) q=hardspan/400.0;

    min=min+origin-(q*ww);
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
    updateSelectionTime(max-min);
}

void gGraphWindow::updateSelectionTime(qint64 span) // in milliseconds
{
    qint64 time=span % 86400000;
    int days,hours,minutes,seconds,milli;
    milli=time % 1000;
    time/=1000;
    seconds=time%60;
    time/=60;
    minutes=time%60;
    time/=60;
    hours=time;
    time/=24;
    days=time;
    QString s;
    qint64 z=(max_x-min_x)/1000;
    if (z>86400) {
        s.sprintf("%i days",days);
    } else if (z>3600) {
        s.sprintf("%02i:%02i:%02i",hours,minutes,seconds);
    } else {
        s.sprintf("%02i:%02i:%02i:%04i",hours,minutes,seconds,milli);
    }

    qstatus2->setText(s);
}

gGraphWindow *LastGraphLDown=NULL;
gGraphWindow *LastGraphRDown=NULL;
gGraphWindow *currentWidget=NULL;
void gGraphWindow::mouseMoveEvent(QMouseEvent * event)
{
//    static bool first=true;
    static QRect last;
    // grabbed
    if (m_dragGraph) {
        if (LastGraphLDown!=this)
            currentWidget=this;
        if (splitter) {
            if (event->y()>m_scrY) {
                //qDebug() << "Swap Down";
                int i=splitter->indexOf(this);
                if (i<splitter->count()-2) {
                    splitter->insertWidget(i+1,this);
                    splitter->setStretchFactor(this,1);
                    splitter->layout();
                }

            } else if (event->y()<0) {
                //qDebug() << "Swap up";
                int i=splitter->indexOf(this);
                if (i>0) {
                    splitter->insertWidget(i-1,this);
                    splitter->setStretchFactor(this,1);
                    splitter->layout();
                }
            }
        }

        return;
    }
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

        qint64 rminx=rmin_x;
        qint64 rmaxx=rmax_x;
        double rx=rmaxx-rminx;

        double qx=rx*mx;
        qx+=rminx;

        // qx is centerpoint of new zoom area.

        qint64 minx=min_x;
        double dx=max_x-minx; // zoom rect width;

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
            for (QList<gGraphWindow *>::iterator g=link_zoom.begin();g!=link_zoom.end();g++) {
                if (*g==this) {
                    qWarning() << "mouseMoveEvent *g should not equal this";
                    continue;
                }
                (*g)->SetXBounds(qx,ex);
            }
        }
    } else
    if (event->buttons() & Qt::RightButton) {
        MoveX(x - m_mouseRClick.x());
        m_mouseRClick.setX(x);
        if (pref["LinkGraphMovement"].toBool()) {
            for (QList<gGraphWindow *>::iterator g=link_zoom.begin();g!=link_zoom.end();g++) {
                (*g)->SetXBounds(min_x,max_x);
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

        double z;
        if (m_block_zoom) {
            z=rmax_x-rmin_x;
        } else {
            z=max_x-min_x;
        }
        double q=double(t2-t1)/Width();
        this->updateSelectionTime(q*z);
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
void gGraphWindow::keyPressEvent(QKeyEvent * event)
{
    bool moved=false;
    int accel=1;
    if (event->key()==Qt::Key_Left) {
        if (event->modifiers() & Qt::ControlModifier) accel=4;
        MoveX(40*accel);
        moved=true;
    } else if (event->key()==Qt::Key_Right) {
        if (event->modifiers() & Qt::ControlModifier) accel=4;
        MoveX(-40*accel);
        moved=true;
    } else if (event->key()==Qt::Key_Up) {
        double zoom_fact=2;
        if (event->modifiers() & Qt::ControlModifier) zoom_fact=5;
        ZoomX(zoom_fact,0);
        moved=true;
    } else if (event->key()==Qt::Key_Down) {
        double zoom_fact=.5;
        if (event->modifiers() & Qt::ControlModifier) zoom_fact=.2;
        ZoomX(zoom_fact,0);
        moved=true;
    }

    if (moved) {
        if (pref["LinkGraphMovement"].toBool()) {
            for (QList<gGraphWindow *>::iterator g=link_zoom.begin();g!=link_zoom.end();g++) {
                (*g)->SetXBounds(min_x,max_x);
            }
        }
        event->accept();
    } else {
        event->ignore();
    }
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
        double rx=rmax_x-rmin_x;
        double qx=double(width)/rx;
        qint64 minx=min_x-rmin_x;
        qint64 maxx=max_x-rmin_x;

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
                    ZoomX(zoom_fact,x);
                    did_draw=true;
                //}
                m_foobar_moved=0;
            }
        }
        m_drag_foobar=false;

        if (did_draw) {
            if (pref["LinkGraphMovement"].toBool()) {
                for (QList<gGraphWindow *>::iterator g=link_zoom.begin();g!=link_zoom.end();g++) {
                    (*g)->SetXBounds(min_x,max_x);
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
      //  for (QList<gGraphWindow *>::iterator g=link_zoom.begin();g!=link_zoom.end();g++) {
            //(*g)->ZoomX(zoom_fact,0);
        //}
        //if (!m_block_zoom) {
            ZoomX(zoom_fact,x); //event.GetX()); // adds origin to zoom out.. Doesn't look that cool.

            if (pref["LinkGraphMovement"].toBool()) {
                for (QList<gGraphWindow *>::iterator g=link_zoom.begin();g!=link_zoom.end();g++) {
                    (*g)->SetXBounds(min_x,max_x);
                }
            }
        //}

    }/* else {
        double min=MinX();
        double max=MaxX();
        for (QList<gGraphWindow *>::iterator g=link_zoom.begin();g!=link_zoom.end();g++) {
            (*g)->SetXBounds(min,max);
        }
    }*/

    m_mouseRDown=false;

}

void gGraphWindow::OnMouseLeftDown(QMouseEvent * event)
{
    int y=event->y();
    int x=event->x();

    if (x<GetLeftMargin()) {
        LastGraphLDown=this;
        m_dragGraph=true;
        return;
    }
    int width=m_scrX-GetRightMargin()-GetLeftMargin();
    int height=m_scrY-GetBottomMargin()-GetTopMargin();
    QRect hot1(GetLeftMargin(),GetTopMargin(),width,height); // Graph data area.
    m_mouseLClick.setX(x);
    m_mouseLClick.setY(y);

    if (foobar && ((y>(m_scrY-GetBottomMargin())) && (y<(m_scrY)))) { //-GetBottomMargin())+20))) {
        double rx=rmax_x-rmin_x;
        double qx=double(width)/rx;
        double minx=min_x-rmin_x;
        double maxx=max_x-rmin_x;

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
    if (m_dragGraph) {
        // Graph Reorder Magic
        if (splitter && currentWidget && LastGraphLDown) {
            if (LastGraphLDown!=currentWidget) {
                //int newidx=splitter->indexOf(currentWidget);
                //if (qobject_cast<gGraphWindow *>(splitter->widget(newidx))) {
                 //   splitter->insertWidget(newidx,LastGraphLDown);
                //}
                return;

            }
        }
        m_dragGraph=false;
        return;
    }

    if (LastGraphLDown && (LastGraphLDown!=this)) { // Same graph that initiated the click??
        LastGraphLDown->OnMouseLeftRelease(event);  // Nope.. Give it the event.
        return;
    }

    int y=event->y();
    int x=event->x();
    int width=m_scrX-GetRightMargin()-GetLeftMargin();
    int height=m_scrY-GetBottomMargin()-GetTopMargin();
    QRect hot1(GetLeftMargin(),GetTopMargin(),width,height); // Graph data area.

    //bool was_dragging_foo=false;
    bool did_draw=false;

    // Finished Dragging the FooBar?
    if (foobar && m_drag_foobar) {
        m_drag_foobar=false;
        //was_dragging_foo=true;
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
            double qx=(rx/width)*double(x);
            if (mx>=rx) {
                mx=300000;//1.0/(24.0*15.0);
            }
            qx+=rmin_x;
            qx-=mx/2.0;
            if (qx<rmin_x) {
                qx=rmin_x;
            }
            if (qx+mx>rmax_x) {
                qx=rmax_x-mx;
            }
            glFlush();
            glFinish();
            SetXBounds(qx,qx+mx);
            did_draw=true;
        } else {
            int xp=x;
            //xp=0;
            double zoom_fact=0.5;
            if (event->modifiers() & Qt::ControlModifier) zoom_fact=0.25;
            ZoomX(zoom_fact,xp);
            did_draw=true;
        }
    }

    m_drag_foobar=false;
    QRect r=m_mouseRBrect;
    m_mouseRBrect=QRect(0, 0, 0, 0);
    m_mouseLDown=false;
    m_drag_foobar=false;
    if (!did_draw) {
        if (r!=m_mouseRBrect) {
            updateGL();
        }
    } else {
        if (pref["LinkGraphMovement"].toBool()) {
            for (QList<gGraphWindow *>::iterator g=link_zoom.begin();g!=link_zoom.end();g++) {
                (*g)->SetXBounds(min_x,max_x);
            }
            glFinish();
        }
    }
    LastGraphLDown=NULL;
}

void gGraphWindow::SetMargins(int top, int right, int bottom, int left)
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
    glDisable(GL_TEXTURE_2D);
    m_scrX=width();
    m_scrY=height();

    /*QGLFormat glFormat(QGL::SampleBuffers);
    glFormat.setAlpha(true);
    glFormat.setDirectRendering(true);
    glFormat.setSwapInterval(1);
    setFormat(glFormat);*/

}

bool first_draw_event=true;
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

void gGraphWindow::Render(int w, int h)
{
    if (m_gradient_background) {
        glClearColor(255,255,255,255);
        //glClearDepth(1);
        glClear(GL_COLOR_BUFFER_BIT);// | GL_DEPTH_BUFFER_BIT);

        //glEnable(GL_BLEND);
        //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBegin(GL_QUADS);
        glColor4f(1.0,1.0,1.0,1.0); // Gradient start
        glVertex2f(0, h);
        glVertex2f(0, 0);

        glColor4f(0.8,0.8,1.0,1.0); // Gradient End
        glVertex2f(w, 0);
        glVertex2f(w, h);

        /*glColor4f(1.0,1.0,1.0,0.5); // Gradient start
        glVertex2f(GetLeftMargin(), h-GetTopMargin());
        glVertex2f(GetLeftMargin(), GetBottomMargin());
        glVertex2f(w-GetRightMargin(), GetBottomMargin());
        glVertex2f(w-GetRightMargin(), h-GetTopMargin()); */
        glEnd();
        //glDisable(GL_BLEND);
    } else {
        glClearColor(255,255,255,255);
        glClearDepth(1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }


    for (QList<gLayer *>::iterator l=layers.begin();l!=layers.end();l++) {
        (*l)->Plot(*this,w,h);
    }

    DrawTextQueue(*this);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
}

void gGraphWindow::paintGL()
{
    if (m_scrX<=0) m_scrX=width();
    if (m_scrY<=0) m_scrY=height();
    if (m_scrX<=0) return;
    if (m_scrY<=0) return;
#ifdef __APPLE__
    AGLContext aglContext;
    aglContext=aglGetCurrentContext();
    GLint swapInt=1;
    aglSetInteger(aglContext, AGL_SWAP_INTERVAL, &swapInt);
#endif

    //glDisable(GL_DEPTH_TEST);
    Render(m_scrX,m_scrY);

    if (m_mouseLDown) {
        if (m_mouseRBrect.width()>0)
            //glDisable(GL_DEPTH_TEST);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            glBegin(GL_QUADS);
            glColor4ub(140,140,140,64);
            glVertex2f(m_mouseRBrect.x(),m_mouseRBrect.y());
            glVertex2f(m_mouseRBrect.x()+m_mouseRBrect.width(),m_mouseRBrect.y());
            glColor4ub(50,50,200,128);
            glVertex2f(m_mouseRBrect.x()+m_mouseRBrect.width(),m_mouseRBrect.y()+m_mouseRBrect.height());
            glVertex2f(m_mouseRBrect.x(),m_mouseRBrect.y()+m_mouseRBrect.height());
            glEnd();
            glDisable(GL_BLEND);
            //glFinish();
            //RoundedRectangle(m_mouseRBrect.x(),m_mouseRBrect.y(),m_mouseRBrect.width(),m_mouseRBrect.height(),5,QColor(50,50,200,64));
            //glEnable(GL_DEPTH_TEST);
    }
    //glEnable(GL_DEPTH_TEST);


    swapBuffers(); // Dump to screen.
}

qint64 gGraphWindow::MinX()
{
    bool first=true;
    qint64 val=0,tmp;
    for (QList<gLayer *>::iterator l=layers.begin();l!=layers.end();l++) {
        if ((*l)->isEmpty()) continue;
        tmp=(*l)->Minx();
        if (!tmp) continue;
        if (first) {
            val=tmp;
            first=false;
        } else {
            if (tmp < val) val = tmp;
        }
    }
    if (val) rmin_x=val;
    return val;
}
qint64 gGraphWindow::MaxX()
{
    bool first=true;
    qint64 val=0,tmp;
    for (QList<gLayer *>::iterator l=layers.begin();l!=layers.end();l++) {
        if ((*l)->isEmpty()) continue;
        tmp=(*l)->Maxx();
        if (!tmp) continue;
        if (first) {
            val=tmp;
            first=false;
        } else {
            if (tmp > val) val = tmp;
        }
    }
    if (val) rmax_x=val;
    return val;
}
EventDataType gGraphWindow::MinY()
{
    bool first=true;
    EventDataType val=0,tmp;
    for (QList<gLayer *>::iterator l=layers.begin();l!=layers.end();l++) {
        if ((*l)->isEmpty()) continue;
        tmp=(*l)->Miny();
        if (tmp==0 && tmp==(*l)->Maxy()) continue;
        if (first) {
            val=tmp;
            first=false;
        } else {
            if (tmp < val) val = tmp;
        }
    }
    return rmin_y=val;
}
EventDataType gGraphWindow::MaxY()
{
    bool first=true;
    EventDataType val=0,tmp;
    for (QList<gLayer *>::iterator l=layers.begin();l!=layers.end();l++) {
        if ((*l)->isEmpty()) continue;
        tmp=(*l)->Maxy();
        if (tmp==0 && tmp==(*l)->Miny()) continue;
        if (first) {
            val=tmp;
            first=false;
        } else {
            if (tmp > val) val = tmp;
        }
    }
    return rmax_y=val;
}

void gGraphWindow::SetMinX(qint64 v)
{
    min_x=v;
}
void gGraphWindow::SetMaxX(qint64 v)
{
    max_x=v;
}
void gGraphWindow::SetMinY(EventDataType v)
{
    min_y=v;
}
void gGraphWindow::SetMaxY(EventDataType v)
{
    max_y=v;
}
