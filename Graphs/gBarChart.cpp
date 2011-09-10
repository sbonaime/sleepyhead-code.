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
SummaryChart::SummaryChart(Profile *p,QString label,GraphType type)
:Layer(EmptyChannel),m_profile(p),m_label(label),m_graphtype(type)
{
    QColor color=Qt::black;
    addGLBuf(quads=new GLBuffer(color,20000,GL_QUADS));
    addGLBuf(lines=new GLBuffer(color,20000,GL_LINES));
    quads->forceAntiAlias(true);
    lines->setSize(2);
    lines->forceAntiAlias(false);
    m_empty=true;
    hl_day=-1;
}
SummaryChart::~SummaryChart()
{
}
void SummaryChart::SetDay(Day * nullday)
{
    if (!m_profile) {
        qWarning() << "Forgot to set profile for gBarChart dummy!";
        m_day=NULL;
        return;
    }
    Day * day=nullday;
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
    m_empty=true;

    SummaryType type;
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
            type=m_type[j];
            for (int i=0;i<d.value().size();i++) {
                day=d.value()[i];
                if (type==ST_HOURS || day->channelExists(code)) { // too many lookups happening here.. stop the crap..
                    switch(m_type[j]) {
                        case ST_AVG: tmp=day->avg(code); break;
                        case ST_SUM: tmp=day->sum(code); break;
                        case ST_WAVG: tmp=day->wavg(code); break;
                        case ST_90P: tmp=day->p90(code); break;
                        case ST_MIN: tmp=day->min(code); break;
                        case ST_MAX: tmp=day->max(code); break;
                        case ST_CNT: tmp=day->count(code); break;
                        case ST_CPH: tmp=day->cph(code); break;
                        case ST_SPH: tmp=day->sph(code); break;
                        case ST_HOURS: tmp=day->hours(); break;
                    default:    break;
                    }
                    //if (tmp>0) {
                        fnd=true;
                        total+=tmp;
                        m_values[dn][j+1]=tmp;
                        if (tmp<m_miny) m_miny=tmp;
                        if (tmp>m_maxy) m_maxy=tmp;
                        break;
                   // }

                }
            }
        }
        if (fnd) {
            if (!m_fday) m_fday=dn;
            m_values[dn][0]=total;
            if (m_graphtype==GT_BAR) {
                if (total<m_miny) m_miny=total;
                if (total>m_maxy) m_maxy=total;
            }
            m_empty=false;
       }
    }
    if (m_graphtype==GT_BAR) {
        m_miny=0;
    }
   // m_minx=qint64(QDateTime(m_profile->FirstDay(),QTime(0,0,0),Qt::UTC).toTime_t())*1000L;
    m_maxx=qint64(QDateTime(m_profile->LastDay().addDays(1),QTime(0,0,0),Qt::UTC).toTime_t())*1000L;

}

QColor brighten(QColor color)
{
    int cr,cg,cb;

    cr=color.red();
    cg=color.green();
    cb=color.blue();

    if (cr<64) cr=64;
    if (cg<64) cg=64;
    if (cb<64) cb=64;

    cr*=2;
    cg*=2;
    cb*=2;

    if (cr>255) cr=255;
    if (cg>255) cg=255;
    if (cb>255) cb=255;

    return QColor(cr,cg,cb,255);

}

void SummaryChart::paint(gGraph & w,int left, int top, int width, int height)
{
    if (!m_visible) return;

    rtop=top;
    GLBuffer *outlines=w.lines();
    QColor blk=Qt::black;
    outlines->add(left, top, left, top+height, blk);
    outlines->add(left, top+height, left+width,top+height, blk);
    outlines->add(left+width,top+height, left+width, top, blk);
    outlines->add(left+width, top, left, top, blk);

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
    EventDataType h,tmp;


    l_offset=(minx) % 86400000L;
    offset=float(l_offset)/86400000.0;

    offset*=barw;
    px=left-offset;
    l_minx=minx;
    l_maxx=maxx+86400000L;

    //QHash<short, EventDataType> lastvalues;
    int total_days=0;
    double total_val=0;
    qint64 lastQ=0;
    bool lastdaygood=false;
    QVector<int> totalcounts;
    QVector<EventDataType> totalvalues;
    QVector<EventDataType> lastvalues;
    QVector<float> lastX;
    QVector<short> lastY;
    int numcodes=m_codes.size();
    totalcounts.resize(numcodes);
    totalvalues.resize(numcodes);
    lastvalues.resize(numcodes);
    lastX.resize(numcodes);
    lastY.resize(numcodes);
    int zd=minx/86400000L;
    zd--;
    QHash<int,QHash<short,EventDataType> >::iterator d=m_values.find(zd);
//    if (d==m_values.end()) {
//        d=m_values.find(zd--);
 //   }
    lastdaygood=true;
    for (int i=0;i<numcodes;i++) {
        totalcounts[i]=0;
        totalvalues[i]=0;
        lastvalues[i]=0;
        lastX[i]=px;
        if (d!=m_values.end()) {
            tmp=d.value()[i+1];
            h=tmp*ymult;
        } else {
            lastdaygood=false;
            h=0;
        }
        lastY[i]=top+height-1-h;
    }

    for (qint64 Q=minx;Q<=maxx+86400000L;Q+=86400000L) {
        zd=Q/86400000L;
        d=m_values.find(zd);
        qint64 extra=86400000;
        if (Q<minx) continue;
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
                j--;
                QColor col=m_colors[j];
                if (zd==hl_day) {
                    col=QColor("gold");
                }

                tmp=g.value();
                totalvalues[j]+=tmp;
                totalcounts[j]++;
                h=tmp*ymult; // height in pixels

                if (m_graphtype==GT_BAR) {
                    QColor col2=brighten(col);

                    quads->add(x1,py,col);
                    quads->add(x1,py-h,col);
                    quads->add(x2,py-h,col2);
                    quads->add(x2,py,col2);
                    if (barw>2) {
                        outlines->add(x1,py,x1,py-h,blk);
                        outlines->add(x1,py-h,x2,py-h,blk);
                        outlines->add(x1,py,x2,py,blk);
                        outlines->add(x2,py,x2,py-h,blk);
                    } // if (bar
                    py-=h;
                } else if (m_graphtype==GT_LINE) { // if (m_graphtype==GT_BAR
                    col.setAlpha(128);
                    short px2=px+barw;
                    short py2=top+height-1-h;
                    if (lastdaygood) {
                        lines->add(lastX[j],lastY[j],px,py2,m_colors[j]);
                        lines->add(px,py2,px2,py2,col);
                    } else {
                        lines->add(x1,py2,x2,py2,col);
                    }
                    lastX[j]=px2;
                    lastY[j]=py2;

                    //}
                }
            }  // for(QHash<short
            lastdaygood=true;
            if (Q>maxx+extra) break;
        } else lastdaygood=false;
        px+=barw;
        daynum++;
        lastQ=Q;
    }

    lines->scissor(left,w.flipY(top+height+2),width+1,height+1);

    // Draw Ledgend
    px=left+width;
    py=top+10;
    QString a;
    int x,y;
    for (int j=0;j<m_codes.size();j++) {
        if (totalvalues[j]==0) continue;
        a=channel[m_codes[j]].label();
        a+=" ";
        switch(m_type[j]) {
                case ST_WAVG: a+="Avg"; break;
                case ST_AVG:  a+="Avg"; break;
                case ST_90P:  a+="90%"; break;
                case ST_MIN:  a+="Min"; break;
                case ST_MAX:  a+="Max"; break;
                case ST_CPH:  a+=""; break;
                case ST_SPH:  a+="%"; break;
                case ST_HOURS: a+="Hours"; break;
                default:
                    break;
        }
        GetTextExtent(a,x,y);
        px-=30+x;
        w.renderText(a,px+24,py+5);
        lines->add(px,py,px+20,py,m_colors[j]);
        lines->add(px,py+1,px+20,py+1,m_colors[j]);
    }

    QString z=m_label;
    if (m_graphtype==GT_LINE) {
        if (totalcounts[0]>0) {
            float val=totalvalues[0]/float(totalcounts[0]);
            z+="="+QString::number(val,'f',2)+" days="+QString::number(totalcounts[0],'f',0);
        }
       // val = AHI for selected area.
    } else { // Bar chart works in total mode
        if (total_days>0) {
            float val=total_val/float(total_days);
            z+="="+QString::number(val,'f',2)+" days="+QString::number(total_days,'f',0);
        }

    }
    w.renderText(z,left,top-1);
}
bool SummaryChart::mouseMoveEvent(QMouseEvent *event)
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
   // if (m_graphtype==GT_LINE) mx+=86400000L;
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

            EventDataType val;
            if (m_graphtype==GT_BAR) {
                val=d.value()[0];
            } else {
                val=d.value()[1];
            }
            QString z=dt.date().toString(Qt::SystemLocaleShortDate)+"\n"+m_label+"="+QString::number(val,'f',2);;
            graph->ToolTip(z,x,y,1500);
            return true;
        }
        //graph->redraw();
    }
    //qDebug() << l_left <<  x << hl_day << y << offset << barw;
    return false;
}

bool SummaryChart::mousePressEvent(QMouseEvent * event)
{
    return false;
}

bool SummaryChart::mouseReleaseEvent(QMouseEvent * event)
{
    hl_day=-1;
    graph->timedRedraw(2000);
    return false;
}

/*
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

    int days=0;
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
            days++;
        }
    }
    if (!days) m_empty=true; else m_empty=false;
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

void AHIChart::SetDay(Day * nullday)
{
}




AvgChart::AvgChart(Profile *profile)
    :gBarChart()
{
    m_label="Avg";
    m_profile=profile;
}

void AvgChart::SetDay(Day * day)
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
                    tmp=day->wavg(code);
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
    m_miny=0;

   // m_minx=qint64(QDateTime(m_profile->FirstDay(),QTime(0,0,0),Qt::UTC).toTime_t())*1000L;
    m_maxx=qint64(QDateTime(m_profile->LastDay().addDays(1),QTime(0,0,0),Qt::UTC).toTime_t())*1000L;

    m_empty=m_values.size()==0;
}

*/
