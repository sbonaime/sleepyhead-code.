/*
 gSegmentChart Implementation
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

#include <cmath>
#include "gSegmentChart.h"


gSegmentChart::gSegmentChart(GraphSegmentType type,QColor gradient_color,QColor outline_color)
:Layer(NoChannel),m_graph_type(type),m_gradient_color(gradient_color),m_outline_color(outline_color)
{
    m_empty=true;
    addGLBuf(poly=new GLFloatBuffer(4000,GL_POLYGON));
    addGLBuf(lines=new GLFloatBuffer(4000,GL_LINE_LOOP));
    lines->setSize(1);
    poly->forceAntiAlias(false);
    lines->forceAntiAlias(true);
    lines->setAntiAlias(true);
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
    Layer::SetDay(d);
    m_total=0;
    if (!m_day) return;
    for (int c=0;c<m_codes.size();c++) {
        m_values[c]=0;
        for (QList<Session *>::iterator s=m_day->begin();s!=m_day->end();++s) {
            if (!(*s)->enabled())  continue;

            int cnt=(*s)->count(m_codes[c]);
            m_values[c]+=cnt;
            m_total+=cnt;
        }
    }
    m_empty=true;
    for (int i=0;i<m_codes.size();i++) {
        if (m_day->count(m_codes[i])>0) {
            m_empty=false;
            break;
        }
    }

}
bool gSegmentChart::isEmpty()
{
    return m_empty;
}

void gSegmentChart::paint(gGraph & w,int left, int top, int width, int height)
{
    if (!m_visible) return;
    if (!m_day) return;
    int start_px=left;
    int start_py=top;

    width--;
    float diameter=MIN(width,height);
    diameter-=8;
    float radius=diameter/2.0;

    float j=0.0;
    float sum=0.0;
    float step=1.0/720.0;
    float px,py;
    float q;

    float xmult=float(width)/float(m_total);
    float ymult=float(height)/float(m_total);

    float xp=left;

    int xoffset=width/2;
    int yoffset=height/2;
    if (m_total==0) {
        QColor col=Qt::green;
        QString a=":-)";
        int x,y;
        GetTextExtent(a,x,y,bigfont);

        w.renderText(a,start_px+xoffset-x/2, (start_py+yoffset+y/2),0,col,bigfont);
        return;
    }

    int data;
    unsigned size=m_values.size();
    float line_step=float(width)/float(size-1);
    bool line_first=true;
    int line_last;

    gVertexBuffer *quads=w.quads();
    gVertexBuffer *lines2=w.lines();
    for (unsigned m=0;m<size;m++) {
        data=m_values[m];

        if (data==0) continue;
/////////////////////////////////////////////////////////////////////////////////////
// Pie Chart
/////////////////////////////////////////////////////////////////////////////////////
        if (m_graph_type==GST_Pie) {
            QColor & col=schema::channel[m_codes[m % m_colors.size()]].defaultColor();
            j=float(data)/float(m_total); // ratio of this pie slice

            // Draw Filling
            poly->add(start_px+xoffset, start_py+height-yoffset,m_gradient_color);
            for (q=sum;q<sum+j;q+=step) {
                px=start_px+xoffset+sin(q*2*M_PI)*radius;
                py=start_py+height-(yoffset+cos(q*2*M_PI)*radius);
                poly->add(px,py,col);
            }
            q=sum+j;
            px=start_px+xoffset+sin(q*2*M_PI)*radius;
            py=start_py+height-(yoffset+cos(q*2*M_PI)*radius);
            poly->add(px,py,col);

            if (m_total!=data) {
                // Draw the center point first
                lines->add(start_px+xoffset, start_py+height-yoffset,m_outline_color);
            }
            for (q=sum;q<sum+j;q+=step) {
                px=start_px+xoffset+sin(q*2*M_PI)*radius;
                py=start_py+height-(yoffset+cos(q*2*M_PI)*radius);
                lines->add(px,py,m_outline_color);
            }
            double tpx=start_px+xoffset+sin((sum+(j/2.0))*2*M_PI)*(radius/1.7);
            double tpy=start_py+height-(yoffset+cos((sum+(j/2.0))*2*M_PI)*(radius/1.7));
            q=sum+j;
            px=start_px+xoffset+sin(q*2*M_PI)*radius;
            py=start_py+height-(yoffset+cos(q*2*M_PI)*radius);
            lines->add(px,py,m_outline_color);

            if (j>.09) {
                QString a=m_names[m]; //QString::number(floor(100.0/m_total*data),'f',0)+"%";
                int x,y;
                GetTextExtent(a,x,y);
                w.renderText(a,tpx-(x/2.0),(tpy+y/2.0),0,Qt::black,defaultfont,false); // antialiasing looks like crap here..
            }

            sum=q;

/////////////////////////////////////////////////////////////////////////////////////
// CandleStick Chart
/////////////////////////////////////////////////////////////////////////////////////
        } else if (m_graph_type==GST_CandleStick) {
            QColor & col=m_colors[m % m_colors.size()];
            float bw=xmult*float(data);

            quads->add(xp,start_py,xp+bw,start_py,m_gradient_color.rgba());
            quads->add(xp+bw,start_py+height,xp,start_py+height,col.rgba());

            lines2->add(xp,start_py,xp+bw,start_py,m_outline_color.rgba());
            lines2->add(xp+bw,start_py+height,xp,start_py+height,m_outline_color.rgba());

            if (!m_names[m].isEmpty()) {
                int px,py;
                GetTextExtent(m_names[m],px,py);
                if (px+5<bw) {
                    w.renderText(m_names[m],(xp+bw/2)-(px/2),top+((height/2)-(py/2)),0,Qt::black);
                }
            }

            xp+=bw;
/////////////////////////////////////////////////////////////////////////////////////
// Line Chart
/////////////////////////////////////////////////////////////////////////////////////
        } else if (m_graph_type==GST_Line) {
            QColor col=Qt::black; //m_colors[m % m_colors.size()];
            float h=(top+height)-(float(data)*ymult);
            if (line_first) {
                line_first=false;
            } else {
                lines->add(xp,line_last,xp+line_step,h,col);
                xp+=line_step;
            }
            line_last=h;
        }
    }
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
    Layer::SetDay(d);
    m_total=0;
    if (!m_day) return;
    QMap<EventStoreType,qint64> tap;

    EventStoreType data=0,lastval=0;
    qint64 time=0,lasttime=0;
    //bool first;
    bool rfirst=true;
    //bool changed;
    EventDataType gain=1,offset=0;
    QHash<ChannelID,QVector<EventList *> >::iterator ei;
    for (QList<Session *>::iterator s=m_day->begin();s!=m_day->end();++s) {
        if (!(*s)->enabled())  continue;

        if ((ei=(*s)->eventlist.find(m_code))==(*s)->eventlist.end()) continue;
        for (int q=0;q<ei.value().size();q++) {
            EventList &el=*(ei.value()[q]);
            lasttime=el.time(0);
            lastval=el.raw(0);
            if (rfirst) {
                gain=el.gain();
                offset=el.offset();
                rfirst=false;
            }
            //first=true;
            //changed=false;
            for (quint32 i=1;i<el.count();i++) {
                data=el.raw(i);
                time=el.time(i);
                if (lastval!=data) {
                    qint64 v=(time-lasttime);
                    if (tap.find(lastval)!=tap.end()) {
                        tap[lastval]+=v;
                    } else {
                        tap[lastval]=v;
                    }
                    //changed=true;
                    lasttime=time;
                    lastval=data;
                }
            }
            if (time!=lasttime) {
                qint64 v=(time-lasttime);
                if (tap.find(lastval)!=tap.end()) {
                    tap[data]+=v;
                } else {
                    tap[data]=v;
                }
            }
        }
    }
    m_values.clear();
    m_names.clear();
    m_total=0;
    EventDataType val;

    for (QMap<EventStoreType,qint64>::iterator i=tap.begin();i!=tap.end();i++) {
        val=float(i.key())*gain+offset;

        m_values.push_back(i.value()/1000L);
        m_total+=i.value()/1000L;
        m_names.push_back(QString::number(val,'f',2));
    }
    m_empty=m_values.size()==0;
}
