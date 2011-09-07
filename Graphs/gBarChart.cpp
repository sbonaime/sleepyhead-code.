/*
 gBarChart Implementation
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

#include <math.h>
#include <QLabel>
#include <QDateTime>
#include "gYAxis.h"
#include "gBarChart.h"

extern QLabel * qstatus2;
gBarChart::gBarChart(ChannelID code,QColor color,Qt::Orientation o)
:Layer(code),m_orientation(o)
{
    addGLBuf(quads=new GLBuffer(color,20000,GL_QUADS));
    quads->forceAntiAlias(true);
    m_empty=true;
    hl_day=-1;
}
gBarChart::~gBarChart()
{
    //delete Xaxis;
}
void gBarChart::SetDay(Day * day)
{
    m_maxx=m_minx=0;
    m_maxy=m_miny=0;
    m_empty=true;
}

void gBarChart::paint(gGraph & w,int left, int top, int width, int height)
{
    if (!m_visible) return;
    //if (!m_day) return;

    rtop=top;
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

    EventDataType maxy=m_maxy;
    EventDataType miny=m_miny;

    w.roundY(miny,maxy);

    EventDataType yy=maxy-miny;
    EventDataType ymult=float(height-2)/yy;

    barw=(float(width)/float(days));

    qint64 ts;

    graph=&w;
    float px=left;
    l_left=w.m_marginleft+gYAxis::Margin;
    l_top=w.m_margintop;
    l_width=width;
    l_height=height;
    float py;
    EventDataType total;

    int daynum=0;
    float h,tmp;


    l_offset=(minx) % 86400000L;
    offset=float(l_offset)/86400000.0;

    offset*=barw;
    px=left-offset;
    l_minx=minx;
    l_maxx=maxx+86400000L;

    int total_days=0;
    double total_val=0;
    for (qint64 Q=minx;Q<=maxx+86400000L;Q+=86400000L) {
        int zd=Q/86400000L;
        QHash<int,QHash<short,EventDataType> >::iterator d=m_values.find(zd);
        if (Q<minx) continue;
        if (Q>maxx+86400000) continue;  // break; // out of order if I end up using a hash instead.??
        if (d!=m_values.end()) {
            int x1=px,x2=px+barw;

            if (x1<left) x1=left;
            if (x2>left+width) x2=left+width;
            if (x2<x1) continue;
            ChannelID code;
            total=d.value()[0];


            if (total>0) {
                total_val+=total;
                total_days++;
            }
            py=top+height;
            for (QHash<short,EventDataType>::iterator g=d.value().begin();g!=d.value().end();g++) {
                short j=g.key();
                if (!j) continue;
                QColor col=m_colors[j-1];

                int cr,cg,cb;

                cr=col.red();
                cg=col.green();
                cb=col.blue();

                if (cr<64) cr=64;
                if (cg<64) cg=64;
                if (cb<64) cb=64;

                cr*=2;
                cg*=2;
                cb*=2;

                if (cr>255) cr=255;
                if (cg>255) cg=255;
                if (cb>255) cb=255;

                if (zd==hl_day) {
                    col=QColor("gold");
                }
                QColor col2=QColor(cr,cg,cb,255);
                //col2=QColor(220,220,220,255);

                tmp=g.value(); //(g.value()/float(total));
                h=tmp*ymult; //(float(total)*ymult); // height of chunk
                quads->add(x1,py,col);
                quads->add(x1,py-h,col);
                quads->add(x2,py-h,col2);
                quads->add(x2,py,col2);
                if (barw>2) {
                    lines->add(x1,py,x1,py-h,blk);
                    lines->add(x1,py-h,x2,py-h,blk);
                    lines->add(x1,py,x2,py,blk);
                    lines->add(x2,py,x2,py-h,blk);
                }
                py-=h;
            }
        }
        px+=barw;
        daynum++;
    }
    if (total_days>0) {
        float val=total_val/float(total_days);
        QString z=m_label+"="+QString::number(val,'f',2)+" days="+QString::number(total_days,'f',0);
        w.renderText(z,left,top-1);
        // val = AHI for selected area.
    }
}
bool gBarChart::mouseMoveEvent(QMouseEvent *event)
{
    int x=event->x()-l_left;
    int y=event->y()-l_top;
    if ((x<0 || y<0 || x>l_width || y>l_height)) {
        hl_day=-1;
        //graph->timedRedraw(2000);
        return false;
    }

    double xx=l_maxx-l_minx;
    double xmult=xx/double(l_width+barw);

    qint64 mx=ceil(xmult*double(x-offset));
    mx+=l_minx;
    mx=mx+l_offset;//-86400000L;
    int zd=mx/86400000L;
    //if (hl_day!=zd)
    {
        hl_day=zd;
        QHash<int,QHash<short,EventDataType> >::iterator d=m_values.find(hl_day);
        if (d!=m_values.end()) {

            //int yy=y;
            //int x=event->x()+graph->left+gGraphView::titleWidth;
                    ;
            //int x=event->x()+gYAxis::Margin-gGraphView::titleWidth;
            //if (x>l_width-45) x=l_width-45;
            x+=gYAxis::Margin+gGraphView::titleWidth; //graph->m_marginleft+
            int y=event->y()+rtop-10;
            /*int w=90;
            int h=32;
            if (x<41+w/2) x=41+w/2;
            if (y<1) y=1;
            if (y>l_height-h+1) y=l_height-h+1; */


            //y+=rtop;
            //TODO: Convert this to a ToolTip class


            QDateTime dt=QDateTime::fromTime_t(hl_day*86400);
            QString z=dt.date().toString(Qt::SystemLocaleShortDate)+"\n"+m_label+"="+QString::number(d.value()[0],'f',2);;
            graph->ToolTip(z,x,y,1500);
            return true;
        }
        //graph->redraw();
    }
    //qDebug() << l_left <<  x << hl_day << y << offset << barw;
    return false;
}

bool gBarChart::mousePressEvent(QMouseEvent * event)
{
    return false;
}

bool gBarChart::mouseReleaseEvent(QMouseEvent * event)
{
    hl_day=-1;
    graph->timedRedraw(2000);
    return false;
}


UsageChart::UsageChart(Profile *profile)
    :gBarChart()
{
    m_profile=profile;
    m_colors.push_back(Qt::green);
    m_label="Hours";
}

void UsageChart::SetDay(Day * day)
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
        // there could possibly may be a bug by doing this.. if charts don't match up.. come back here and enable the m_minx right down the bottom of this function.
        if (!m_minx || tt<m_minx) m_minx=tt;
        if (!m_maxx || tt>m_maxx) m_maxx=tt;

        Day *day=m_profile->GetDay(d.key(),MT_CPAP);
        if (day) {
            total=day->hours();

            m_values[dn][0]=total;
            m_values[dn][1]=total;
            if (total<m_miny) m_miny=total;
            if (total>m_maxy) m_maxy=total;
        }
    }
    //m_maxy=ceil(m_maxy);
    //m_miny=floor(m_miny);
    m_miny=0;

   // m_minx=qint64(QDateTime(m_profile->FirstDay(),QTime(0,0,0),Qt::UTC).toTime_t())*1000L;
    m_maxx=qint64(QDateTime(m_profile->LastDay().addDays(1),QTime(0,0,0),Qt::UTC).toTime_t())*1000L;
}


AHIChart::AHIChart(Profile *profile)
    :gBarChart()
{
    m_label="AHI";
    m_profile=profile;
}

void AHIChart::SetDay(Day * day)
{
    if (!m_profile) {
        qWarning() << "Forgot to set profile for gBarChart dummy!";
        m_day=NULL;
        return;
    }
    Layer::SetDay(day);

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
        //tt=QDateTime(d.key(),QTime(12,0,0)).toTime_t();
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
                        m_values[dn][j+1]=tmp;
                        break;
                    }
                }
            }
        }
        if (fnd) {
            if (!m_fday) m_fday=dn;
            m_values[dn][0]=total;
            if (total<m_miny) m_miny=total;
            if (total>m_maxy) m_maxy=total;
       }
    }
    //m_maxy=ceil(m_maxy);
    //m_miny=floor(m_miny);
    m_miny=0;

   // m_minx=qint64(QDateTime(m_profile->FirstDay(),QTime(0,0,0),Qt::UTC).toTime_t())*1000L;
    m_maxx=qint64(QDateTime(m_profile->LastDay().addDays(1),QTime(0,0,0),Qt::UTC).toTime_t())*1000L;

    m_empty=m_values.size()==0;
}

