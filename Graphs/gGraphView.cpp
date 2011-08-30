#include <cmath>
#include <QFontMetrics>
#include "gGraphView.h"
#include "SleepLib/profiles.h"

bool _graph_init=false;

QFont * defaultfont=NULL;
QFont * mediumfont=NULL;
QFont * bigfont=NULL;

bool evil_intel_graphics_chip=false;

// Must be called from a thread inside the application.
void InitGraphs()
{
    if (!_graph_init) {
        defaultfont=new QFont("Sans Serif",10);
        mediumfont=new QFont("Sans Serif",11);
        bigfont=new QFont("Serif",35);

        defaultfont->setStyleHint(QFont::SansSerif,QFont::OpenGLCompatible);
        mediumfont->setStyleHint(QFont::SansSerif,QFont::OpenGLCompatible);
        bigfont->setStyleHint(QFont::Serif ,QFont::OpenGLCompatible);

        _graph_init=true;
    }
}
void DoneGraphs()
{
    if (_graph_init) {
        delete defaultfont;
        delete bigfont;
        delete mediumfont;
        _graph_init=false;
    }
}

void GetTextExtent(QString text, float & width, float & height, QFont *font)
{
    QFontMetrics fm(*font);
    //QRect r=fm.tightBoundingRect(text);
    width=fm.width(text); //fm.width(text);
    height=fm.xHeight()+2; //fm.ascent();
}

GLBuffer::GLBuffer(QColor color,int max,int type)
    :m_color(color), m_max(max), m_type(type)
{
    m_scissor=false;
    buffer=new GLshort [max+8];
    m_cnt=0;
    m_size=1;
}
GLBuffer::~GLBuffer()
{
    delete [] buffer;
}
void GLBuffer::add(GLshort s)
{
    if (m_cnt<m_max) {
        buffer[m_cnt++]=s;
    } else {
        qDebug() << "GLBuffer overflow";
    }
}
void GLBuffer::add(GLshort x, GLshort y)
{
    if (m_cnt<m_max+2) {
        buffer[m_cnt++]=x;
        buffer[m_cnt++]=y;
    } else {
        qDebug() << "GLBuffer overflow";
    }
}
void GLBuffer::add(GLshort x1, GLshort y1, GLshort x2, GLshort y2)
{
    if (m_cnt<m_max+4) {
        buffer[m_cnt++]=x1;
        buffer[m_cnt++]=y1;
        buffer[m_cnt++]=x2;
        buffer[m_cnt++]=y2;
    } else {
        qDebug() << "GLBuffer overflow";
    }
}

void GLBuffer::draw()
{
    if (m_cnt>0) {
        bool antialias=pref["UseAntiAliasing"].toBool() && m_antialias;
        float size=m_size;
        if (antialias) {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            if (m_type==GL_LINES) {
                glEnable(GL_LINE_SMOOTH);
                glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
                size+=0.5;
            }
        }
        if (m_type==GL_LINES) {
            glLineWidth(size);
        } else if (m_type==GL_POINTS) {
            glPointSize(size);
        }
        if (m_scissor) {
            glScissor(s1,s2,s3,s4);
            glEnable(GL_SCISSOR_TEST);
        }
        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(2, GL_SHORT, 0, buffer);
        glColor4ub(m_color.red(),m_color.green(),m_color.blue(),m_color.alpha());
        glDrawArrays(m_type, 0, m_cnt >> 1);
        glDisableClientState(GL_VERTEX_ARRAY);
        //qDebug() << "I Drawed" << m_cnt << "vertices";
        m_cnt=0;
        if (m_scissor) {
            glDisable(GL_SCISSOR_TEST);
            m_scissor=false;
        }
        if (antialias) {
            if (m_type==GL_LINES) {
                glDisable(GL_LINE_SMOOTH);
            }
            glDisable(GL_BLEND);
        }
    }
}

Layer::Layer(ChannelID code)
{
    m_code = code;
    m_visible = true;
    m_movable = false;

    m_day=NULL;
    m_miny=m_maxy=0;
    m_minx=m_maxx=0;
    m_order=0;
    m_width=m_height=0;
    m_X=m_Y=0;
    m_position=LayerCenter;
}

Layer::~Layer()
{
}

void Layer::SetDay(Day * d)
{
    m_day=d;
    if (!d) return;

    m_minx=d->first(m_code);
    m_maxx=d->last(m_code);
    m_miny=d->min(m_code);
    m_maxy=d->max(m_code);
}

bool Layer::isEmpty()
{
    if (m_day && (m_day->count(m_code)!=0))
        return false;
    return true;
}
void Layer::setLayout(LayerPosition position, short width, short height, short order)
{
    m_position=position;
    m_width=width;
    m_height=height;
    m_order=order;
}

LayerGroup::LayerGroup() :
    Layer(EmptyChannel)
{
}
LayerGroup::~LayerGroup()
{
}
bool LayerGroup::isEmpty()
{
    if (!m_day)
        return true;
    bool empty=true;
    for (int i=0;i<layers.size();i++) {
        if (layers[i]->isEmpty()) {
            empty=false;
            break;
        }
    }
    return empty;
}
void LayerGroup::SetDay(Day * d)
{
    for (int i=0;i<layers.size();i++) {
         layers[i]->SetDay(d);
    }
    m_day=d;
}

void LayerGroup::AddLayer(Layer *l)
{
    layers.push_back(l);
}

qint64 LayerGroup::Minx()
{
    bool first=true;
    qint64 m=0,t;
    for (int i=0;i<layers.size();i++)  {
        t=layers[i]->Minx();
        if (!t) continue;
        if (first) {
            m=t;
            first=false;
        } else
        if (m>t) m=t;
    }
    return m;
}
qint64 LayerGroup::Maxx()
{
    bool first=true;
    qint64 m=0,t;
    for (int i=0;i<layers.size();i++)  {
        t=layers[i]->Maxx();
        if (!t) continue;
        if (first) {
            m=t;
            first=false;
        } else
        if (m<t) m=t;
    }
    return m;
}
EventDataType LayerGroup::Miny()
{
    bool first=true;
    EventDataType m=0,t;
    for (int i=0;i<layers.size();i++)  {
        t=layers[i]->Miny();
        if (t==layers[i]->Minx()) continue;
        if (first) {
            m=t;
            first=false;
        } else {
            if (m>t) m=t;
        }
    }
    return m;
}
EventDataType LayerGroup::Maxy()
{
    bool first=true;
    EventDataType m=0,t;
    for (int i=0;i<layers.size();i++)  {
        t=layers[i]->Maxy();
        if (t==layers[i]->Miny()) continue;
        if (first) {
            m=t;
            first=false;
        } else
        if (m<t) m=t;
    }
    return m;
}


const double zoom_hard_limit=500.0;

gGraph::gGraph(gGraphView *graphview,QString title,int height,short group) :
    m_graphview(graphview),
    m_title(title),
    m_height(height),
    m_visible(true)
{
    m_min_height=60;
    m_layers.clear();

    if (graphview) {
        graphview->AddGraph(this,group);
    } else {
        qWarning() << "gGraph created without a gGraphView container.. Naughty programmer!! Bad!!!";
    }
    m_margintop=10;
    m_marginbottom=5;
    m_marginleft=5;
    m_marginright=10;
    m_selecting_area=m_blockzoom=false;
    m_lastx23=0;
}
gGraph::~gGraph()
{
}
bool gGraph::isEmpty()
{
    bool empty=true;
    for (QVector<Layer *>::iterator l=m_layers.begin();l!=m_layers.end();l++) {
        if (!(*l)->isEmpty()) {
            empty=false;
            break;
        }
    }
    return empty;
}
/*void gGraph::invalidate()
{ // this may not be necessary, as scrollbar & resize issues a full redraw..

    //m_lastbounds.setWidth(m_graphview->width());
    m_lastbounds.setY(m_graphview->findTop(this));
    m_lastbounds.setX(gGraphView::titleWidth);
    m_lastbounds.setHeight(m_height * m_graphview->scaleY());
    m_lastbounds.setWidth(m_graphview->width()-gGraphView::titleWidth);
    int i=0;
    //m_lastbounds.setHeight(0);
}

void gGraph::repaint()
{
    if (m_lastbounds.height()>0) {
        //glScissor(0,m_lastbounds.y(),m_lastbounds.width(),m_lastbounds.height());
//        m_graphview->swapBuffers(); // how fast is this??
        //glEnable(GL_SCISSOR_BOX);

        glBegin(GL_QUADS);
        glColor4f(1,1,1,1.0); // Gradient End
        glVertex2i(0,m_lastbounds.y());
        glVertex2i(gGraphView::titleWidth,m_lastbounds.y());
        glVertex2i(gGraphView::titleWidth,m_lastbounds.y()+height());
        glVertex2i(0,m_lastbounds.y()+height());
        glEnd();

        paint(m_lastbounds.x(),m_lastbounds.y(),m_lastbounds.width(),m_lastbounds.height());
        m_graphview->swapBuffers();
        //glDisable(GL_SCISSOR_BOX);
    } else {
        qDebug() << "Wanted to redraw graph" << m_title << "but previous bounds were invalid.. Issuing a slower full redraw instead. Todo: Find out why.";
        m_graphview->updateGL();
    }
}
*/
void gGraph::qglColor(QColor col)
{
    m_graphview->qglColor(col);
}
void gGraph::renderText(QString text, int x,int y, float angle, QColor color, QFont *font)
{
    m_graphview->AddTextQue(text,x,y,angle,color,font);
}


void gGraph::paint(int originX, int originY, int width, int height)
{
    m_lastbounds=QRect(originX,originY,width,height);

    /*glEnable(GL_BLEND);
    glBegin(GL_QUADS);
    glColor4f(1,1,1,0.4); // Gradient End
    glVertex2i(originX,originY);
    glVertex2i(originX+width,originY);
    glColor4f(1,1,1.0,.7); // Gradient End
    glVertex2i(originX+width,originY+height);
    glVertex2i(originX,originY+height);
    glEnd();
    glDisable(GL_BLEND);
    */
    glColor4f(0,0,0,1);
    renderText(title(),20,originY+height/2,90);

    left=0,right=0,top=0,bottom=0;

    int tmp;

    originX+=m_marginleft;
    originY+=m_margintop;
    width-=m_marginleft+m_marginright;
    height-=m_margintop+m_marginbottom;

    for (int i=0;i<m_layers.size();i++) {
        Layer *ll=m_layers[i];
        tmp=ll->Height();
        if (ll->position()==LayerTop) top+=tmp;
        if (ll->position()==LayerBottom) bottom+=tmp;
    }

    for (int i=0;i<m_layers.size();i++) {
        Layer *ll=m_layers[i];
        tmp=ll->Width();
        if (ll->position()==LayerLeft) {
            ll->paint(*this,originX+left,originY+top,tmp,height-top-bottom);
            left+=tmp;
        }
        if (ll->position()==LayerRight) {
            right+=tmp;
            ll->paint(*this,originX+width-right,originY+top,tmp,height-top-bottom);
        }
    }

    bottom=0; top=0;
    for (int i=0;i<m_layers.size();i++) {
        Layer *ll=m_layers[i];
        tmp=ll->Height();
        if (ll->position()==LayerTop) {
            ll->paint(*this,originX+left,originY+top,width-left-right,tmp);
            top+=tmp;
        }
        if (ll->position()==LayerBottom) {
            bottom+=tmp;
            ll->paint(*this,originX+left,originY+height-bottom,width-left-right,tmp);
        }
    }

    for (int i=0;i<m_layers.size();i++) {
        Layer *ll=m_layers[i];
        if (ll->position()==LayerCenter) {
            ll->paint(*this,originX+left,originY+top,width-left-right,height-top-bottom);
        }
    }

    if (m_selection.width()>0 && m_selecting_area) {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_BLEND);
        glBegin(GL_QUADS);
        glColor4ub(128,128,128,128);
        glVertex2i(originX+m_selection.x(),originY+top);
        glVertex2i(originX+m_selection.x()+m_selection.width(),originY+top);
        glColor4ub(128,128,255,128);
        glVertex2i(originX+m_selection.x()+m_selection.width(),originY+height-top-bottom);
        glVertex2i(originX+m_selection.x(),originY+height-top-bottom);
        glEnd();
        glDisable(GL_BLEND);
    }

}

void gGraph::AddLayer(Layer * l,LayerPosition position, short width, short height, short order, bool movable, short x, short y)
{
    l->setLayout(position,width,height,order);
    l->setMovable(movable);
    l->setPos(x,y);
    m_layers.push_back(l);
}
void gGraph::mouseMoveEvent(QMouseEvent * event)
{
   // qDebug() << m_title << "Move" << event->pos() << m_graphview->pointClicked();
    int y=event->pos().y();
    int x=event->pos().x();
    int x2=m_graphview->pointClicked().x(),y2=m_graphview->pointClicked().y();
    int w=m_lastbounds.width()-(right+m_marginright);
    int h=m_lastbounds.height()-(bottom+m_marginbottom);
    double xx=max_x-min_x;
    double xmult=xx/w;
    m_selecting_area=false;

    if (m_graphview->m_selected_graph==this) {
        if (event->buttons() & Qt::LeftButton) {
            //qDebug() << m_title << "Moved" << x << y << left << right << top << bottom << m_width << h;
            int a1=MIN(x,x2);
            int a2=MAX(x,x2);
            if (a1<m_marginleft+left) a1=m_marginleft+left;
            if (a2>w) a2=w;
            m_selecting_area=true;
            m_selection=QRect(a1-m_marginleft-1,0,a2-a1,m_lastbounds.height());
            m_graphview->updateGL();
        } else if (event->buttons() & Qt::RightButton) {
            m_graphview->setPointClicked(event->pos());
            x-=left+m_marginleft;
            x2-=left+m_marginleft;
            //int a1=MIN(x,x2);
            //int a2=MAX(x,x2);
            //if (a1<m_marginleft+left) a1=m_marginleft+left;
            //if (a2>w) a2=w;
            if (!m_blockzoom) {
                xx=max_x-min_x;
                w-=m_marginleft+left;
                xmult=xx/double(w);
                qint64 j1=xmult*x;
                qint64 j2=xmult*x2;
                qint64 jj=j2-j1;
                min_x+=jj;
                max_x+=jj;
                if (min_x<rmin_x) {
                    min_x=rmin_x;
                    max_x=rmin_x+xx;
                }
                if (max_x>rmax_x) {
                    max_x=rmax_x;
                    min_x=rmax_x-xx;
                }
                //if (a2>rmax_x) a2=rmax_x;
                m_graphview->SetXBounds(min_x,max_x,m_group);
            } else {
                qint64 qq=rmax_x-rmin_x;
                xx=max_x-min_x;
                if (xx==qq) xx=1800000;
                w-=m_marginleft+left;
                xmult=qq/double(w);
                qint64 j1=(xmult*x);
                min_x=rmin_x+j1-(xx/2);
                max_x=min_x+xx;
                if (min_x<rmin_x) {
                    min_x=rmin_x;
                    max_x=rmin_x+xx;
                }
                if (max_x>rmax_x) {
                    max_x=rmax_x;
                    min_x=rmax_x-xx;
                }
                m_graphview->SetXBounds(min_x,max_x,m_group);

            }
        }
    }

    if (x>left+m_marginleft && x<m_lastbounds.width()-(right+m_marginright) && y>top+m_margintop && y<m_lastbounds.height()-(bottom+m_marginbottom)) { // main area
        x-=left+m_marginleft;
        y-=top+m_margintop;
        //qDebug() << m_title << "Moved" << x << y << left << right << top << bottom << m_width << m_height;
    }
}
void gGraph::mousePressEvent(QMouseEvent * event)
{
    int y=event->pos().y();
    int x=event->pos().x();
    int w=m_lastbounds.width()-(right+m_marginright);
    int h=m_lastbounds.height()-(bottom+m_marginbottom);
    int x2,y2;
    double xx=max_x-min_x;
    double xmult=xx/w;
    if (x>left+m_marginleft && x<m_lastbounds.width()-(right+m_marginright) && y>top+m_margintop && y<m_lastbounds.height()-(bottom+m_marginbottom)) { // main area
        x-=left+m_marginleft;
        y-=top+m_margintop;
    }
    //qDebug() << m_title << "Clicked" << x << y << left << right << top << bottom << m_width << m_height;
}


void gGraph::mouseReleaseEvent(QMouseEvent * event)
{
    int y=event->pos().y();
    int x=event->pos().x();
    int w=m_lastbounds.width()-(m_marginleft+left+right+m_marginright);
    int h=m_lastbounds.height()-(bottom+m_marginbottom);
    int x2=m_graphview->pointClicked().x(),y2=m_graphview->pointClicked().y();


    //qDebug() << m_title << "Released" << min_x << max_x << x << y << x2 << y2 << left << right << top << bottom << m_width << m_height;
    if (m_selecting_area) {
        m_selecting_area=false;
        m_selection.setWidth(0);

        if (m_graphview->horizTravel()>4) {
            x-=left+m_marginleft;
            y-=top+m_margintop;
            x2-=left+m_marginleft;
            y2-=top+m_margintop;
            if (x<0) x=0;
            if (x2<0) x2=0;
            if (x>w) x=w;
            if (x2>w) x2=w;
            if (!m_blockzoom) {
                double xx=max_x-min_x;
                double xmult=xx/double(w);
                qint64 j1=min_x+xmult*x;
                qint64 j2=min_x+xmult*x2;
                qint64 a1=MIN(j1,j2)
                qint64 a2=MAX(j1,j2)
                //if (a1<rmin_x) a1=rmin_x;
                if (a2>rmax_x) a2=rmax_x;
                m_graphview->SetXBounds(a1,a2,m_group);
            } else {
                double xx=rmax_x-rmin_x;
                double xmult=xx/double(w);
                qint64 j1=rmin_x+xmult*x;
                qint64 j2=rmin_x+xmult*x2;
                qint64 a1=MIN(j1,j2)
                qint64 a2=MAX(j1,j2)
                //if (a1<rmin_x) a1=rmin_x;
                if (a2>rmax_x) a2=rmax_x;
                m_graphview->SetXBounds(a1,a2,m_group);
            }
            return;
        } else m_graphview->updateGL();
    }

    if ((m_graphview->horizTravel()<4) && (x>left+m_marginleft && x<w+m_marginleft+left && y>top+m_margintop && y<h)) { // normal click in main area
        if (!m_blockzoom) {
            double zoom;
            if (event->button() & Qt::RightButton) {
                zoom=1.33;
                if (event->modifiers() & Qt::ControlModifier) zoom*=1.5;
                ZoomX(zoom,x);  // Zoom out
                return;
            } else if (event->button() & Qt::LeftButton) {
                zoom=0.75;
                if (event->modifiers() & Qt::ControlModifier) zoom/=1.5;
                ZoomX(zoom,x); // zoom in.
                return;
            }
        } else {
            x-=left+m_marginleft;
            y-=top+m_margintop;
            //w-=m_marginleft+left;
            double qq=rmax_x-rmin_x;
            double xmult;

            double xx=max_x-min_x;
            //if (xx==qq) xx=1800000;

            xmult=qq/double(w);
            if ((xx==qq) || (x==m_lastx23)) {
                double zoom;
                if (event->button() & Qt::RightButton) {
                    zoom=1.33;
                    if (event->modifiers() & Qt::ControlModifier) zoom*=1.5;
                } else if (event->button() & Qt::LeftButton) {
                    zoom=0.75;
                    if (event->modifiers() & Qt::ControlModifier) zoom/=1.5;
                }
                xx*=zoom;
                if (xx<qq/zoom_hard_limit) xx=qq/zoom_hard_limit;
                if (xx>qq) xx=qq;
            }
            double j1=xmult*x;
            min_x=rmin_x+j1-(xx/2.0);
            max_x=min_x+xx;
            if (min_x<rmin_x) {
                min_x=rmin_x;
                max_x=rmin_x+xx;
            } else
            if (max_x>rmax_x) {
                max_x=rmax_x;
                min_x=rmax_x-xx;
            }
            m_graphview->SetXBounds(min_x,max_x,m_group);
            m_lastx23=x;
        }
    }
    //m_graphview->updateGL();
}


void gGraph::wheelEvent(QWheelEvent * event)
{
    //qDebug() << m_title << "Wheel" << event->x() << event->y() << event->delta();
    //int y=event->pos().y();
    int x=event->pos().x()-m_graphview->titleWidth;//(left+m_marginleft);
    if (event->delta()>0) {
        ZoomX(0.75,x);
    } else {
        ZoomX(1.5,x);
    }
}
void gGraph::mouseDoubleClickEvent(QMouseEvent * event)
{
    //mousePressEvent(event);
    //mouseReleaseEvent(event);
    int y=event->pos().y();
    int x=event->pos().x();
    int w=m_lastbounds.width()-(m_marginleft+left+right+m_marginright);
    int h=m_lastbounds.height()-(bottom+m_marginbottom);
    int x2=m_graphview->pointClicked().x(),y2=m_graphview->pointClicked().y();
    if ((m_graphview->horizTravel()<4) && (x>left+m_marginleft && x<w+m_marginleft+left && y>top+m_margintop && y<h)) { // normal click in main area
        if (event->button() & Qt::RightButton) {
            ZoomX(1.66,x);  // Zoon out
            return;
        } else if (event->button() & Qt::LeftButton) {
            ZoomX(0.75/2.0,x); // zoom in.
            return;
        }
    }
    //mousePressEvent(event);
    //mouseReleaseEvent(event);
    //qDebug() << m_title << "Double Clicked" << event->x() << event->y();
}
void gGraph::keyPressEvent(QKeyEvent * event)
{
    qDebug() << m_title << "Key Pressed.. implement me" << event->key();
}

void gGraph::ZoomX(double mult,int origin_px)
{

    int width=m_lastbounds.width()-(m_marginleft+left+right+m_marginright);
    if (origin_px==0) origin_px=(width/2); else origin_px-=m_marginleft+left;

    if (origin_px<0) origin_px=0;
    if (origin_px>width) origin_px=width;


    // Okay, I want it to zoom in centered on the mouse click area..
    // Find X graph position of mouse click
    // find current zoom width
    // apply zoom
    // center on point found in step 1.

    qint64 min=min_x;
    qint64 max=max_x;

    double hardspan=rmax_x-rmin_x;
    double span=max-min;
    double ww=double(origin_px) / double(width);
    double origin=ww * span;
    //double center=0.5*span;
    //double dist=(origin-center);

    double q=span*mult;
    if (q>hardspan) q=hardspan;
    if (q<hardspan/zoom_hard_limit) q=hardspan/zoom_hard_limit;

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
    m_graphview->SetXBounds(min,max,m_group);
    //updateSelectionTime(max-min);
}

void gGraph::DrawTextQue()
{
    m_graphview->DrawTextQue();
}

// margin recalcs..
void gGraph::resize(int width, int height)
{
    //m_height=height;
    //m_width=width;
}

qint64 gGraph::MinX()
{
    bool first=true;
    qint64 val=0,tmp;
    for (QVector<Layer *>::iterator l=m_layers.begin();l!=m_layers.end();l++) {
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
qint64 gGraph::MaxX()
{
    bool first=true;
    qint64 val=0,tmp;
    for (QVector<Layer *>::iterator l=m_layers.begin();l!=m_layers.end();l++) {
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
EventDataType gGraph::MinY()
{
    bool first=true;
    EventDataType val=0,tmp;
    for (QVector<Layer *>::iterator l=m_layers.begin();l!=m_layers.end();l++) {
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
EventDataType gGraph::MaxY()
{
    bool first=true;
    EventDataType val=0,tmp;
    for (QVector<Layer *>::iterator l=m_layers.begin();l!=m_layers.end();l++) {
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

void gGraph::SetMinX(qint64 v)
{
    min_x=v;
}
void gGraph::SetMaxX(qint64 v)
{
    max_x=v;
}
void gGraph::SetMinY(EventDataType v)
{
    min_y=v;
}
void gGraph::SetMaxY(EventDataType v)
{
    max_y=v;
}
void gGraph::DrawStaticText(QStaticText & text, short x, short y)
{
    m_graphview->DrawStaticText(text,x,y);
}

// Sets a new Min & Max X clipping, refreshing the graph and all it's layers.
void gGraph::SetXBounds(qint64 minx, qint64 maxx)
{
    min_x=minx;
    max_x=maxx;

    //repaint();
    //m_graphview->updateGL();
}
int gGraph::flipY(int y)
{
    return m_graphview->height()-y;
}

void gGraph::ResetBounds()
{
    min_x=MinX();
    max_x=MaxX();
    min_y=MinY();
    max_y=MaxY();
}


gGraphView::gGraphView(QWidget *parent, gGraphView * shared) :
    QGLWidget(parent,shared),
    m_offsetY(0),m_offsetX(0),m_scaleY(1.0),m_scrollbar(NULL)
{
    m_shared=shared;
    m_sizer_index=m_graph_index=0;
    m_textque_items=0;
    m_button_down=m_graph_dragging=m_sizer_dragging=false;
    m_lastypos=m_lastxpos=0;
    m_horiz_travel=0;
    this->setMouseTracking(true);
    m_emptytext=QObject::tr("No Data");
    InitGraphs();
}
gGraphView::~gGraphView()
{
    for (int i=0;i<m_graphs.size();i++) {
        delete m_graphs[i];
    }
    m_graphs.clear();
    if (m_scrollbar) {
        this->disconnect(SIGNAL(sliderMoved(int)),this);
    }
}
void gGraphView::DrawTextQue()
{
    glPushAttrib(GL_COLOR_BUFFER_BIT);
    glFlush();
    //glEnable(GL_BLEND);
    QPainter painter(this);
    for (int i=0;i<m_textque_items;i++) {
        // GL Font drawing is ass in Qt.. :(
        TextQue & q=m_textque[i];

        QBrush b(q.color);
        painter.setBrush(b);
        painter.setFont(*q.font);
        if (q.angle==0) {
            painter.drawText(q.x, q.y, q.text);
        } else {
            float w,h;
            GetTextExtent(q.text, w, h, q.font);

            painter.translate(q.x, q.y);
            painter.rotate(-q.angle);
            painter.drawText(floor(-w/2.0), floor(-h/2.0), q.text);
            painter.rotate(+q.angle);
            painter.translate(-q.x, -q.y);
        }
        q.text.clear();
        //q.text.squeeze();
    }
    painter.end();
    glPopAttrib();
    //qDebug() << "rendered" << m_textque_items << "text items";
    m_textque_items=0;
}

void gGraphView::AddTextQue(QString & text, short x, short y, float angle, QColor & color, QFont * font)
{
    text_mutex.lock();
    if (m_textque_items>=textque_max) {
        DrawTextQue();
    }
    TextQue & q=m_textque[m_textque_items];
    q.text=text;
    q.x=x;
    q.y=y;
    q.angle=angle;
    q.color=color;
    q.font=font;
    m_textque_items++;
    text_mutex.unlock();
}
void gGraphView::DrawStaticText(QStaticText & text, short x, short y)
{
    // don't use this for multithread
    QPainter painter(this);
    painter.drawStaticText(x,y,text);
    painter.end();
}

void gGraphView::AddGraph(gGraph *g,short group)
{
    if (!m_graphs.contains(g)) {
        g->setGroup(group);
        m_graphs.push_back(g);

        updateScrollBar();
    }
}
float gGraphView::totalHeight()
{
    float th=0;
    for (int i=0;i<m_graphs.size();i++) {
        if (m_graphs[i]->isEmpty() || (!m_graphs[i]->visible())) continue;
        th += m_graphs[i]->height() + graphSpacer;
    }
    return ceil(th);
}
float gGraphView::findTop(gGraph * graph)
{
    float th=-m_offsetY;
    for (int i=0;i<m_graphs.size();i++) {
        if (m_graphs[i]==graph) break;
        if (m_graphs[i]->isEmpty() || (!m_graphs[i]->visible())) continue;
        th += m_graphs[i]->height()*m_scaleY + graphSpacer;
    }
    //th-=m_offsetY;
    return ceil(th);
}
float gGraphView::scaleHeight()
{
    float th=0;
    for (int i=0;i<m_graphs.size();i++) {
        if (m_graphs[i]->isEmpty() || (!m_graphs[i]->visible())) continue;
        th += m_graphs[i]->height() * m_scaleY + graphSpacer;
    }
    return ceil(th);
}
void gGraphView::resizeEvent(QResizeEvent *e)
{
    QGLWidget::resizeEvent(e); // This ques a redraw event..

    updateScale();
    for (int i=0;i<m_graphs.size();i++) {
        m_graphs[i]->resize(e->size().width(),m_graphs[i]->height()*m_scaleY);
    }
}
void gGraphView::scrollbarValueChanged(int val)
{
    //qDebug() << "Scrollbar Changed" << val;
    if (m_offsetY!=val) {
        m_offsetY=val;
        updateGL(); // do this on a timer?
    }
}
void gGraphView::ResetBounds() //short group)
{
    for (int i=0;i<m_graphs.size();i++) {
        //if (m_graphs[i]->group()==group)
            m_graphs[i]->ResetBounds();
    }
    updateScale();
}

void gGraphView::SetXBounds(qint64 minx, qint64 maxx,short group)
{
    for (int i=0;i<m_graphs.size();i++) {
        if (m_graphs[i]->group()==group)
            m_graphs[i]->SetXBounds(minx,maxx);
    }
    updateGL();
}
void gGraphView::updateScale()
{
    float th=totalHeight(); // height of all graphs
    float h=height();       // height of main widget

    if (th < h) {
        m_scaleY=h / th;    // less graphs than fits on screen, so scale to fit
    } else {
        m_scaleY=1.0;
    }
    updateScrollBar();
}

void gGraphView::updateScrollBar()
{
    if (!m_scrollbar) return;

    if (!m_graphs.size()) return;

    float th=scaleHeight(); // height of all graphs
    float h=height();       // height of main widget

    float vis=0;
    for (int i=0;i<m_graphs.size();i++) vis+=m_graphs[i]->isEmpty()  || (!m_graphs[i]->visible()) ? 0 : 1;

    if (th<h) { // less graphs than fits on screen

        m_scrollbar->setMaximum(0); // turn scrollbar off.

    } else {  // more graphs than fit on screen
        //m_scaleY=1.0;
        float avgheight=th/vis;
        m_scrollbar->setPageStep(avgheight);
        m_scrollbar->setSingleStep(avgheight/8.0);
        m_scrollbar->setMaximum(th-height());
        if (m_offsetY>th-height()) {
            m_offsetY=th-height();
        }
    }
}

void gGraphView::setScrollBar(MyScrollBar *sb)
{
    m_scrollbar=sb;
    m_scrollbar->setMinimum(0);
    updateScrollBar();
    this->connect(m_scrollbar,SIGNAL(valueChanged(int)),SLOT(scrollbarValueChanged(int)));
}
void gGraphView::initializeGL()
{
    setAutoFillBackground(false);
    setAutoBufferSwap(false);
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);
}

void gGraphView::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glOrtho(0, w, h, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void gGraphView::paintGL()
{

    if (width()<=0) return;
    if (height()<=0) return;

    QTime time;
    time.start();

    glClearColor(255,255,255,255);
    //glClearDepth(1);
    glClear(GL_COLOR_BUFFER_BIT);// | GL_DEPTH_BUFFER_BIT);

    //glEnable(GL_BLEND);

    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    /*glBegin(GL_QUADS);
    glColor4f(1.0,1.0,1.0,1.0); // Gradient start
    glVertex2f(0, height());
    glVertex2f(0, 0);

    //glColor4f(0.9,0.9,0.9,1.0); // Gradient End
    glVertex2f(width(), 0);
    glVertex2f(width(), height());

    glEnd();*/

    float px=titleWidth-m_offsetX;
    float py=-m_offsetY;
    int numgraphs=0;
    float h,w;
    //ax=px;//-m_offsetX;

    for (int i=0;i<m_graphs.size();i++) {
        if (m_graphs[i]->isEmpty() || !m_graphs[i]->visible()) continue;
        numgraphs++;
        h=m_graphs[i]->height() * m_scaleY;

        // set clipping?

        if (py > height())
            break; // we are done.. can't draw anymore

        if ((py + h + graphSpacer) >= 0) {
            w=width();
            m_graphs[i]->paint(px,py,width()-titleWidth,h);
            glColor4f(0,0,0,1);
            //if (i<numgraphs-1) {

            // draw the splitter handle
                glBegin(GL_QUADS);
                glColor4f(.5,.5,.5,1.0);
                glVertex2f(0,py+h);
                glVertex2f(w,py+h);
                glColor4f(.7,.7,.7,1.0);
                glVertex2f(w,py+h+graphSpacer/2.0);
                glVertex2f(0,py+h+graphSpacer/2.0);
                glColor4f(1,1,1,1.0);
                glVertex2f(0,py+h+graphSpacer/2.0);
                glVertex2f(w,py+h+graphSpacer/2.0);
                glColor4f(.3,.3,.3,1.0);
                glVertex2f(w,py+h+graphSpacer);
                glVertex2f(0,py+h+graphSpacer);
                glEnd();
            //}
        }
        py+=h;
        //if (i<numgraphs-1)
        py+=graphSpacer;
        py=ceil(py);
    }
    if (!numgraphs) {
        QColor col=Qt::black;
        float x,y;
        GetTextExtent(m_emptytext,x,y,bigfont);
        AddTextQue(m_emptytext,(width()/2)-x/2,(height()/2)+y/2,0.0,col,bigfont);
    }
    DrawTextQue();
    //glDisable(GL_TEXTURE_2D);
    //glDisable(GL_DEPTH_TEST);

    swapBuffers(); // Dump to screen.
    qDebug() << "Graph Draw" << time.elapsed() << "ms";
}

// For manual scrolling
void gGraphView::setOffsetY(int offsetY)
{
    if (m_offsetY!=offsetY) {
        m_offsetY=offsetY;
        updateGL(); //issue full redraw..
    }
}

// For manual X scrolling (not really needed)
void gGraphView::setOffsetX(int offsetX)
{
    if (m_offsetX!=offsetX) {
        m_offsetX=offsetX;
        updateGL(); //issue redraw
    }
}

void gGraphView::mouseMoveEvent(QMouseEvent * event)
{
    int x=event->x();
    int y=event->y();

    if (m_sizer_dragging) { // Resize handle being dragged
        float my=y - m_sizer_point.y();
        //qDebug() << "Sizer moved vertically" << m_sizer_index << my*m_scaleY;
        float h=m_graphs[m_sizer_index]->height();
        h+=my / m_scaleY;
        if (h > m_graphs[m_sizer_index]->minHeight()) {
            m_graphs[m_sizer_index]->setHeight(h);
            m_sizer_point.setX(x);
            m_sizer_point.setY(y);
            updateScrollBar();
            updateGL();
        }
        return;
    }

    if (m_graph_dragging) { // Title bar being dragged to reorder
        gGraph *p;
        int yy=m_sizer_point.y();
        bool empty;
        if (y < yy) {

            for (int i=m_graph_index-1;i>=0;i--) {
                empty=m_graphs[i]->isEmpty() || (!m_graphs[i]->visible());
                // swapping upwards.
                int yy2=yy-graphSpacer-m_graphs[i]->height()*m_scaleY;
                yy2+=m_graphs[m_graph_index]->height()*m_scaleY;
                if (y<yy2) {
                    //qDebug() << "Graph Reorder" << m_graph_index;
                    p=m_graphs[m_graph_index];
                    m_graphs[m_graph_index]=m_graphs[i];
                    m_graphs[i]=p;
                    if (!empty) {
                        m_sizer_point.setY(yy-graphSpacer-m_graphs[m_graph_index]->height()*m_scaleY);
                        updateGL();
                    }
                    m_graph_index--;
                }
                if (!empty) break;

            }
        } else if (y > yy+graphSpacer+m_graphs[m_graph_index]->height()*m_scaleY) {
            // swapping downwards
            //qDebug() << "Graph Reorder" << m_graph_index;
            for (int i=m_graph_index+1;i<m_graphs.size();i++) {
                empty=m_graphs[i]->isEmpty() || (!m_graphs[i]->visible());
                p=m_graphs[m_graph_index];
                m_graphs[m_graph_index]=m_graphs[i];
                m_graphs[i]=p;
                if (!empty) {
                    m_sizer_point.setY(yy+graphSpacer+m_graphs[m_graph_index]->height()*m_scaleY);
                    updateGL();
                }
                m_graph_index++;
                if (!empty) break;
            }
        }
        return;
    }

    float py = -m_offsetY;
    float h;

    for (int i=0; i < m_graphs.size(); i++) {

        if (m_graphs[i]->isEmpty() || (!m_graphs[i]->visible())) continue;

        h=m_graphs[i]->height() * m_scaleY;
        if (py > height())
            break; // we are done.. can't draw anymore

        if (m_button_down || ((py + h + graphSpacer) >= 0)) {
            if (m_button_down || ((y >= py) && (y < py + h))) {
                if (m_button_down || (x >= titleWidth)) {
                    this->setCursor(Qt::ArrowCursor);
                    m_horiz_travel+=abs(x-m_lastxpos)+abs(y-m_lastypos);
                    m_lastxpos=x;
                    m_lastypos=y;
                    QPoint p(x-titleWidth,y-py);
                    QMouseEvent e(event->type(),p,event->button(),event->buttons(),event->modifiers());

                    m_graphs[i]->mouseMoveEvent(&e);
                } else {
                    this->setCursor(Qt::OpenHandCursor);
                }
            } else if ((y >= py + h) && (y <= py + h + graphSpacer + 1)) {
                this->setCursor(Qt::SplitVCursor);
            }

        }
        py+=h;
        py+=graphSpacer; // do we want the extra spacer down the bottom?
    }

}

void gGraphView::mousePressEvent(QMouseEvent * event)
{
    int x=event->x();
    int y=event->y();

    float py=-m_offsetY;
    float h;

    for (int i=0;i<m_graphs.size();i++) {

        if (m_graphs[i]->isEmpty() || (!m_graphs[i]->visible())) continue;

        h=m_graphs[i]->height()*m_scaleY;
        if (py>height())
            break; // we are done.. can't draw anymore

        if ((py + h + graphSpacer) >= 0) {
            if ((y >= py) && (y < py + h)) {
                //qDebug() << "Clicked" << i;
                if (x < titleWidth) { // clicked on title to drag graph..
                    m_graph_dragging=true;
                    m_graph_index=i;
                    m_sizer_point.setX(x);
                    m_sizer_point.setY(py); // point at top of graph..
                    this->setCursor(Qt::ClosedHandCursor);
                } else { // send event to graph..
                    m_global_point_clicked=QPoint(x,y);
                    m_point_clicked=QPoint (x-titleWidth,y-py);

                    m_selected_graph=m_graphs[i];

                    QMouseEvent e(event->type(),m_point_clicked,event->button(),event->buttons(),event->modifiers());
                    m_graph_index=i;
                    m_button_down=true;
                    m_horiz_travel=0;
                    m_graphs[i]->mousePressEvent(&e);
                }
            } else if ((y >= py + h) && (y <= py + h + graphSpacer + 1)) {
                this->setCursor(Qt::SplitVCursor);
                m_sizer_dragging=true;
                m_sizer_index=i;
                m_sizer_point.setX(x);
                m_sizer_point.setY(y);
                //qDebug() << "Sizer clicked" << i;
            }

        }
        py+=h;
        py+=graphSpacer; // do we want the extra spacer down the bottom?

    }
}

void gGraphView::mouseReleaseEvent(QMouseEvent * event)
{
    this->setCursor(Qt::ArrowCursor);

    if (m_sizer_dragging) {
        m_sizer_dragging=false;
        return;
    }
    if (m_graph_dragging) {
        m_graph_dragging=false;
        return;
    }
    if (m_button_down) {
        m_button_down=false;
        int x1=m_global_point_clicked.x()-event->x();
        int y1=m_global_point_clicked.y()-event->y();

        QPoint p(m_point_clicked.x()-x1,m_point_clicked.y()-y1);
        QMouseEvent e(event->type(),p,event->button(),event->buttons(),event->modifiers());
        m_graphs[m_graph_index]->mouseReleaseEvent(&e);
    }
    int x=event->x();
    int y=event->y();
}

void gGraphView::mouseDoubleClickEvent(QMouseEvent * event)
{
    mousePressEvent(event);
    return;

/*    int x=event->x();
    int y=event->y();

    float py=-m_offsetY;
    float h;

    for (int i=0;i<m_graphs.size();i++) {

        if (m_graphs[i]->isEmpty()) continue;

        h=m_graphs[i]->height()*m_scaleY;
        if (py>height())
            break;

        if ((py + h + graphSpacer) >= 0) {
            if ((y >= py) && (y <= py + h)) {
                if (x < titleWidth) {
                    // What to do when double clicked on the graph title ??
                } else {
                    // send event to graph..
                    m_graphs[i]->mouseDoubleClickEvent(event);
                }
            } else if ((y >= py + h) && (y <= py + h + graphSpacer + 1)) {
                // What to do when double clicked on the resize handle?
            }

        }
        py+=h;
        py+=graphSpacer; // do we want the extra spacer down the bottom?
    } */
}
void gGraphView::wheelEvent(QWheelEvent * event)
{
    // Hmm.. I could optionalize this to change mousewheel behaviour without affecting the scrollbar now..

    if ((event->modifiers() & Qt::ControlModifier)) {
        int x=event->x();
        int y=event->y();

        float py=-m_offsetY;
        float h;

        for (int i=0;i<m_graphs.size();i++) {

            if (m_graphs[i]->isEmpty() || (!m_graphs[i]->visible())) continue;

            h=m_graphs[i]->height()*m_scaleY;
            if (py>height())
                break;

            if ((py + h + graphSpacer) >= 0) {
                if ((y >= py) && (y <= py + h)) {
                    if (x < titleWidth) {
                        // What to do when ctrl+wheel is used on the graph title ??
                    } else {
                        // send event to graph..
                        m_graphs[i]->wheelEvent(event);
                    }
                } else if ((y >= py + h) && (y <= py + h + graphSpacer + 1)) {
                    // What to do when the wheel is used on the resize handle?
                }

            }
            py+=h;
            py+=graphSpacer; // do we want the extra spacer down the bottom?
        }
    } else {
        m_scrollbar->SendWheelEvent(event); // Just forwarding the event to scrollbar for now..
    }
}

void gGraphView::keyPressEvent(QKeyEvent * event)
{

}

MyScrollBar::MyScrollBar(QWidget * parent)
    :QScrollBar(parent)
{
}
void MyScrollBar::SendWheelEvent(QWheelEvent * e)
{
    wheelEvent(e);
}
