/*
 gBarChart Implementation
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

#include <math.h>
#include <QDateTime>
#include "gBarChart.h"

gBarChart::gBarChart(ChannelID code,QColor color,Qt::Orientation o)
:Layer(code),m_orientation(o)
{
    //Xaxis=new gXAxis();
    addGLBuf(quads=new GLBuffer(color,20000,GL_QUADS));
    //addGLBuf(lines=new GLBuffer(col,20,GL_LINES));
    quads->forceAntiAlias(true);
    //lines->setAntiAlias(true);
    //lines->setSize(2);
    m_empty=true;
}
gBarChart::~gBarChart()
{
    //delete Xaxis;
}
void gBarChart::SetDay(Day * day)
{

    if (!m_profile) {
        qWarning() << "Forgot to set profile for gBarChart dummy!";
        m_day=NULL;
        return;
    }
    Layer::SetDay(day);
    //m_empty=true;
   // if (!day) return;
    m_empty=false;
    /*for (int i=0;i<m_codes.size();i++) {
        if (day->count(m_codes[i])>0) {
            m_empty=false;
            break;
        }
    } */

    m_values.clear();
    m_miny=9999999;
    m_maxy=-9999999;
    m_minx=0;
    m_maxx=0;

    int dn;
    EventDataType tmp,total;
    ChannelID code;

    m_fday=0;
    qint64 tt;

    for (QMap<QDate,QVector<Day *> >::iterator d=m_profile->daylist.begin();d!=m_profile->daylist.end();d++) {
        tt=QDateTime(d.key(),QTime(0,0,0),Qt::UTC).toTime_t();
        dn=tt/86400;
        tt*=1000L;
        if (!m_minx || tt<m_minx) m_minx=tt;
        if (!m_maxx || tt>m_maxx) m_maxx=tt;


        total=0;
        bool fnd=false;
        for (int j=0;j<m_codes.size();j++) {
            code=m_codes[j];
            for (int i=0;i<d.value().size();i++) {
                Day *day=d.value()[i];
                if (day->channelExists(code)) { // too many lookups happening here.. stop the crap..
                    tmp=day->count(code)/day->hours();
                    if (tmp>0) {
                        fnd=true;
                        total+=tmp;
                        m_values[dn][code]=tmp;
                        break;
                    }
                }
            }
        }
        if (fnd) {
            if (!m_fday) m_fday=dn;
            m_values[dn][EmptyChannel]=total;
            if (total<m_miny) m_miny=total;
            if (total>m_maxy) m_maxy=total;
       }
    }
    m_maxy=ceil(m_maxy);
    //m_minx-=86400000L;

   // m_minx=qint64(QDateTime(m_profile->FirstDay(),QTime(0,0,0),Qt::UTC).toTime_t())*1000L;
    //m_maxx=qint64(QDateTime(m_profile->LastDay().addDays(1),QTime(0,0,0),Qt::UTC).toTime_t())*1000L;
    int i=0;

    //set miny & maxy here.. how?
}

void gBarChart::paint(gGraph & w,int left, int top, int width, int height)
{
    if (!m_visible) return;
    //if (!m_day) return;

    GLBuffer *lines=w.lines();
    QColor blk=Qt::black;
    lines->add(left, top, left, top+height, blk);
    lines->add(left, top+height, left+width,top+height, blk);
    lines->add(left+width,top+height, left+width, top, blk);
    lines->add(left+width, top, left, top, blk);

    qint64 minx=w.min_x, maxx=w.max_x;
    //qint64 minx=m_minx, maxx=m_maxx;
    qint64 xx=maxx - minx;
    float days=double(xx)/86400000.0;

    EventDataType yy=m_maxy-m_miny;
    EventDataType ymult=float(height)/yy;

    float barw=(float(width)/float(days));

    qint64 ts;

    float px=left;
    float py;
    EventDataType total;

    int daynum=0;
    float h,tmp;

    qint64 offs=minx% 86400000L;
    //zz*=86400000L;
    float offset=offs/86400000.0;

    offset*=barw;
    px=left-offset;

    for (qint64 Q=minx;Q<=maxx+86400000L;Q+=86400000L) {
        int zd=Q/86400000L;
        QHash<int,QMap<ChannelID,EventDataType> >::iterator d=m_values.find(zd);
        if (Q<minx) continue;
        if (Q>maxx+86400000) continue;  // break; // out of order if I end up using a hash instead.??
        if (d!=m_values.end()) {
            ChannelID code;
            total=d.value()[EmptyChannel];
            py=top+height;
            for (int j=0;j<m_codes.size();j++) {
                code=m_codes[j];
                QMap<ChannelID,EventDataType>::iterator g=d.value().find(code);
                if (g!=d.value().end()) {
                    if (code==EmptyChannel) continue;
                    //look up it's color key
                    QColor col=m_colors[j];
                    QColor col2=Qt::white;

                    tmp=(g.value()/float(total));
                    h=tmp*(float(total)*ymult); // height of chunk
                    int x1=px,x2=px+barw;

                    if (x1<left) x1=left;
                    if (x2>left+width) x2=left+width;
                    if (x2>x1) {
                        quads->add(x1,py,col);
                        quads->add(x1,py-h,col);
                        quads->add(x2,py-h,col2);
                        quads->add(x2,py,col2);
                        lines->add(x1,py,x1,py-h,blk);
                        lines->add(x1,py-h,x2,py-h,blk);
                        lines->add(x1,py,x2,py,blk);
                        lines->add(x2,py,x2,py-h,blk);
                    }
                    py-=h;
                }
            }
        }
        px+=barw;
        daynum++;
    }



   // if (!data) return;
   //if (!data->IsReady()) return;

    //int start_px=left;
    //int start_py=top;

    //days=data->np[0];

    //days=0;
/*    for (int i=0;i<data->np[0];i++) {
       if ((data->point[0][i].x() >= w.min_x) && (data->point[0][i].x()<w.max_x)) days+=1;
    }
    if (days==0) return;

    float barwidth,pxr;
    float px,zpx;//,py;

    if (m_orientation==Qt::Vertical) {
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

    QString str;
    bool draw_xticks_instead=false;
    bool antialias=pref["UseAntiAliasing"].toBool();

    if (antialias) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); //_MINUS_SRC_ALPHA);
        glEnable(GL_LINE_SMOOTH);
        glHint(GL_LINE_SMOOTH_HINT,  GL_NICEST);
    }
    zpx=px;
    int i,idx=-1;

    for (i=0;i<data->np[0];i++) {
        if (data->point[0][i].x() < w.min_x) continue;
        if (data->point[0][i].x() >= w.max_x) break;
        if (idx<0) idx=i;
        t1=px;
        px+=barwidth+1;
        t2=px-t1-1;

        QRect rect;
        //Qt:wxDirection dir;

        u2=data->point[0][i].y()*pxr;
        u1=start_py;
        if (antialias) {
            u1++;
            u2++;
        }

        if (m_orientation==Qt::Vertical) {
            rect=QRect(start_px,t1,u2,t2);
        } else {
            rect=QRect(t1,u1,t2,u2);
        }
        //dir=wxEAST;
        //RoundedRectangle(rect.x,rect.y,rect.width,rect.height,1,color[0]); //,*wxLIGHT_GREY,dir);

        // TODO: Put this in a function..
        QColor & col1=color[0];
        QColor col2("light grey");

        glBegin(GL_QUADS);
        //red color
        glColor4ub(col1.red(),col1.green(),col1.blue(),col1.alpha());
        glVertex2f(rect.x(), rect.y()+rect.height());
        glVertex2f(rect.x(), rect.y());
        //blue color
        glColor4ub(col2.red(),col2.green(),col2.blue(),col2.alpha());
        glVertex2f(rect.x()+rect.width(),rect.y());
        glVertex2f(rect.x()+rect.width(), rect.y()+rect.height());
        glEnd();


        glColor4ub(0,0,0,255);
        glLineWidth (1);
        glBegin(GL_LINE_LOOP);
        glVertex2f(rect.x(), rect.y()+rect.height()+.5);
        glVertex2f(rect.x(), rect.y());
        glVertex2f(rect.x()+rect.width(),rect.y());
        glVertex2f(rect.x()+rect.width(), rect.y()+rect.height()+.5);
        //glVertex2f(rect.x(), rect.y()+rect.height());
        glEnd();

        if (!draw_xticks_instead) {
            str=FormatX(data->point[0][i].x());

            GetTextExtent(str, textX, textY);
            if (t2<textY+6)
                draw_xticks_instead=true;
        }
    }
    if (antialias) {
        glDisable(GL_LINE_SMOOTH);
        glDisable(GL_BLEND);
    }

    if (draw_xticks_instead) {
        // turn off the minor ticks..
        Xaxis->SetShowMinorTicks(false);
        Xaxis->Plot(w,scrx,scry);
    } else {
        px=zpx;
        for (i=idx;i<data->np[0];i++) {
            if (data->point[0][i].x() < w.min_x) continue;
            if (data->point[0][i].x() >= w.max_x) break;
            t1=px;
            px+=barwidth+1;
            t2=px-t1-1;
            str=FormatX(data->point[0][i].x());
            GetTextExtent(str, textX, textY);
            float j=t1+((t2/2.0)+(textY/2.0)+5);
            if (m_orientation==Qt::Vertical) {
                DrawText(str,start_px-textX-8,scry-j);
            } else {
                DrawText(str,j,scry-(start_py-3-(textX/2)),90);
            }
        }
    }

    glColor3f (0.1F, 0.1F, 0.1F);
    glLineWidth(1);
    glBegin (GL_LINES);
    glVertex2f (start_px, start_py);
    glVertex2f (start_px, start_py+height+1);
    //glVertex2f (start_px,start_py);
    //glVertex2f (start_px+width, start_py);
    glEnd ();
*/
}

