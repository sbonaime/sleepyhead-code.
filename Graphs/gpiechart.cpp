#include <math.h>
#include "gpiechart.h"

gPieChart::gPieChart(QColor outline_color)
:gLayer(MC_UNKNOWN),m_outline_color(outline_color)
{
}
gPieChart::~gPieChart()
{
}
void gPieChart::AddSlice(MachineCode code,QColor color,QString name)
{
    m_counts[code]=code;
    m_colors[code]=color;
    m_names[code]=name;
    m_total=0;
}
void gPieChart::SetDay(Day *d)
{
    gLayer::SetDay(d);
    m_total=0;
    if (!m_day) return;
    for (map<MachineCode,int>::iterator c=m_counts.begin();c!=m_counts.end();c++) {
        c->second=0;
        for (vector<Session *>::iterator s=m_day->begin();s!=m_day->end();s++) {
            int cnt=(*s)->count(c->first);
            c->second+=cnt;
            m_total+=cnt;
        }
    }

}

void gPieChart::Plot(gGraphWindow & w,float scrx,float scry)
{
    if (!m_visible) return;
    if (!m_day) return;
    if (!m_total) return;
    int start_px=w.GetLeftMargin();
    int start_py=w.GetBottomMargin();
    int width=scrx-(w.GetLeftMargin()+w.GetRightMargin());
    int height=scry-(w.GetTopMargin()+w.GetBottomMargin());

    float diameter=MIN(width,height);
    diameter-=8;
    float radius=diameter/2.0;

    double j=0.0;
    double sum=0.0;
    double step=1.0/45.0;
    float px,py;
    glEnable(GL_BLEND);
    glEnable(GL_LINE_SMOOTH);
    glLineWidth(1.5);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    for (map<MachineCode,int>::iterator m=m_counts.begin();m!=m_counts.end();m++) {
        if (!m->second) continue;
        j=float(m->second)/float(m_total); // ratio of this pie slice
        w.qglColor(m_colors[m->first]);
        glPolygonMode(GL_BACK,GL_FILL);
        glBegin(GL_POLYGON);
        glVertex2f(start_px+radius+4, start_py+radius+4);
        double q;
        for (q=sum;q<sum+j;q+=step) {
            px=start_px+radius+4+sin(q*2*M_PI)*radius;
            py=start_py+radius+4+cos(q*2*M_PI)*radius;
            glVertex2f(px,py);
        }
        q=sum+j;
        px=start_px+radius+4+sin(q*2*M_PI)*radius;
        py=start_py+radius+4+cos(q*2*M_PI)*radius;
        glVertex2f(px,py);
        glEnd();

        glPolygonMode(GL_BACK,GL_LINE);
        w.qglColor(m_outline_color);
        glBegin(GL_POLYGON);
        glVertex2f(start_px+radius+4, start_py+radius+4);
        for (q=sum;q<sum+j;q+=step) {
            px=start_px+radius+4+sin(q*2*M_PI)*radius;
            py=start_py+radius+4+cos(q*2*M_PI)*radius;
            glVertex2f(px,py);
        }
        q=sum+j;
        px=start_px+radius+4+sin(q*2*M_PI)*radius;
        py=start_py+radius+4+cos(q*2*M_PI)*radius;
        glVertex2f(px,py);
        glEnd();

        sum=q;
    }
    glDisable(GL_BLEND);
}
