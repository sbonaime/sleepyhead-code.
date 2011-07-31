/*
 gSegmentChart Implementation
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

#include <cmath>
#include "gSegmentChart.h"


gSegmentChart::gSegmentChart(GraphSegmentType type,QColor gradient_color,QColor outline_color)
:gLayer(EmptyChannel),m_graph_type(type),m_gradient_color(gradient_color),m_outline_color(outline_color)
{
   // m_gradient_color=QColor(200,200,200);
}
gSegmentChart::~gSegmentChart()
{
}
void gSegmentChart::AddSlice(ChannelID code,QColor color,QString name)
{
    m_codes.push_back(code);
    m_values.push_back(0);
    m_colors.push_back(color);
    m_names.push_back(name);
    m_total=0;
}
void gSegmentChart::SetDay(Day *d)
{
    gLayer::SetDay(d);
    m_total=0;
    if (!m_day) return;
    for (int c=0;c<m_codes.size();c++) {
        m_values[c]=0;
        for (QVector<Session *>::iterator s=m_day->begin();s!=m_day->end();s++) {
            int cnt=(*s)->count(m_codes[c]);
            m_values[c]+=cnt;
            m_total+=cnt;
        }
    }

}

void gSegmentChart::Plot(gGraphWindow & w,float scrx,float scry)
{
    if (!m_visible) return;
    if (!m_day) return;
    if (!m_total) return;
    int start_px=w.GetLeftMargin();
    int start_py=w.GetBottomMargin();
    int width=scrx-(w.GetLeftMargin()+w.GetRightMargin());
    int height=scry-(w.GetTopMargin()+w.GetBottomMargin());
    width--;
    float diameter=MIN(width,height);
    diameter-=8;
    float radius=diameter/2.0;

    float j=0.0;
    float sum=0.0;
    float step=1.0/45.0;
    float px,py;
    float q;
    glEnable(GL_BLEND);
    glEnable(GL_LINE_SMOOTH);
    glLineWidth(1.5);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float xmult=float(width)/float(m_total);
    float ymult=float(height)/float(m_total);

    float xp=w.GetLeftMargin();

    int xoffset=width/2;
    int yoffset=height/2;

    int data;
    unsigned size=m_values.size();
    float line_step=float(width)/float(size-1);
    bool line_first=true;
    int line_last;

    if (m_graph_type==GST_Line) {
        w.qglColor(m_outline_color);
        glBegin(GL_LINES);
    }

    for (unsigned m=0;m<size;m++) {
        data=m_values[m];

/////////////////////////////////////////////////////////////////////////////////////
// Pie Chart
/////////////////////////////////////////////////////////////////////////////////////
        if (m_graph_type==GST_Pie) {
            j=float(data)/float(m_total); // ratio of this pie slice

            // Draw Filling
            glPolygonMode(GL_BACK,GL_FILL);
            glBegin(GL_POLYGON);
            w.qglColor(m_gradient_color);
            glVertex2f(start_px+xoffset, start_py+yoffset);
            w.qglColor(m_colors[m % m_colors.size()]);
            for (q=sum;q<sum+j;q+=step) {
                px=start_px+xoffset+sin(q*2*M_PI)*radius;
                py=start_py+yoffset+cos(q*2*M_PI)*radius;
                glVertex2f(px,py);
            }
            q=sum+j;
            px=start_px+xoffset+sin(q*2*M_PI)*radius;
            py=start_py+yoffset+cos(q*2*M_PI)*radius;
            glVertex2f(px,py);
            glEnd();

            // Draw Outline
            w.qglColor(m_outline_color);
            if (m_total>data) { // Draw the center point first
                glPolygonMode(GL_BACK,GL_LINE);
                glBegin(GL_POLYGON);
                glVertex2f(start_px+xoffset, start_py+yoffset);
            } else { // Only one entry, so just draw the circle
                glBegin(GL_LINE_LOOP);
            }
            for (q=sum;q<sum+j;q+=step) {
                px=start_px+xoffset+sin(q*2*M_PI)*radius;
                py=start_py+yoffset+cos(q*2*M_PI)*radius;
                glVertex2f(px,py);
            }
            q=sum+j;
            px=start_px+xoffset+sin(q*2*M_PI)*radius;
            py=start_py+yoffset+cos(q*2*M_PI)*radius;
            glVertex2f(px,py);
            glEnd();
            sum=q;

/////////////////////////////////////////////////////////////////////////////////////
// CandleStick Chart
/////////////////////////////////////////////////////////////////////////////////////
        } else if (m_graph_type==GST_CandleStick) {
            float bw=xmult*float(data);

            glBegin(GL_QUADS);
            w.qglColor(m_gradient_color);
            glVertex2f(xp,start_py);
            glVertex2f(xp+bw,start_py);
            w.qglColor(m_colors[m % m_colors.size()]);
            glVertex2f(xp+bw,start_py+height);
            glVertex2f(xp,start_py+height);
            glEnd();

            w.qglColor(m_outline_color);

            glBegin(GL_LINE_LOOP);
            glVertex2f(xp,start_py);
            glVertex2f(xp+bw,start_py);
            glVertex2f(xp+bw,start_py+height);
            glVertex2f(xp,start_py+height);
            glEnd();

            if (!m_names[m].isEmpty()) {
                GetTextExtent(m_names[m],px,py);
                if (px+5<bw) {
                    DrawText(w,m_names[m],(xp+bw/2)-(px/2),scry-((height/2)-(py/2)),0,Qt::black);
                }
            }

            xp+=bw;
        } else if (m_graph_type==GST_Line) {
            float h=float(data)*ymult;
            if (line_first) {
                line_first=false;
            } else {
                glVertex2f(xp,line_last);
                xp+=line_step;
                glVertex2f(xp,h);
            }
            line_last=h;
        }
    }
    if (m_graph_type==GST_Line) {
        glEnd();
    }
    glDisable(GL_BLEND);
}


gTAPGraph::gTAPGraph(ChannelID code,GraphSegmentType gt, QColor gradient_color,QColor outline_color)
:gSegmentChart(gt,gradient_color,outline_color),m_code(code)
{
    m_colors.push_back(Qt::red);
    m_colors.push_back(Qt::green);
}
gTAPGraph::~gTAPGraph()
{
}
void gTAPGraph::SetDay(Day *d)
{
    gLayer::SetDay(d);
    m_total=0;
    if (!m_day) return;
    QVector<qint64> tap;
    const int max_value=2600;
    tap.resize(max_value);

    EventStoreType data=0,lastval=0;
    qint64 time=0,lasttime=0,lastlasttime=0;
    bool first=true;
    bool rfirst=true;
    EventDataType gain=1,offset=0;
    for (QVector<Session *>::iterator s=m_day->begin();s!=m_day->end();s++) {
        if ((*s)->eventlist.find(m_code)==(*s)->eventlist.end()) continue;
        for (int q=0;q<(*s)->eventlist[m_code].size();q++) {
            EventList &el=*(*s)->eventlist[m_code][q];
            lasttime=el.time(0);
            first=true;

            for (int i=0;i<el.count();i++) {
                data=el.raw(i);
                if (data>max_value) {
                    qWarning() << "max_value is too small in gTAPGraph::SetDay()";
                    break;
                }
                time=el.time(i);
                if (rfirst) {
                    gain=el.gain();
                    offset=el.offset();
                    rfirst=false;
                }
                if (first) {
                    first=false;
                } else {
                    if (lastval!=data) {
                        int v=(time-lasttime)/1000;
                        tap[lastval]+=v;
                    }
                }
                lastlasttime=lasttime;
                lasttime=time;
                lastval=data;
            }
            if (lastval!=data){
                int v=(time-lastlasttime)/1000L;
                tap[data]+=v;
            }
        }
    }
    m_values.clear();
    m_names.clear();
    m_total=0;
    EventDataType val;
    for (int i=0;i<max_value;i++) {
        if (tap[i]>0) {
            val=float(i)*gain+offset;
            m_values.push_back(tap[i]);
            m_total+=tap[i];

            m_names.push_back(QString::number(val,'f',2));
        }
    }
}
