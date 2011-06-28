#include <math.h>
#include "gpiechart.h"

gPieChart::gPieChart(gPointData *d,QColor col)
:gLayer(d)
{
    color.clear();
    color.push_back(col);
}
gPieChart::~gPieChart()
{
}

void gPieChart::Plot(gGraphWindow & w,float scrx,float scry)
{
    if (!m_visible) return;
    if (!data) return;
    if (!data->IsReady()) return;

    int start_px=w.GetLeftMargin();
    int start_py=w.GetBottomMargin();
    int width=scrx-(w.GetLeftMargin()+w.GetRightMargin());
    int height=scry-(w.GetTopMargin()+w.GetBottomMargin());

    float diameter=MIN(width,height);
    diameter-=8;
    float radius=diameter/2.0;

    double total=0;
    for (int i=0;i<data->np[0];i++)
        total+=data->point[0][i].y();


    double j=0.0;
    double sum=0.0;
    double step=1.0/45.0;
    float px,py;
    //glEnable(GL_TEXTURE_2D);
    //glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    //glEnable(GL_POLYGON_SMOOTH);
    glEnable(GL_LINE_SMOOTH);
    glLineWidth(1.5);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //glBlendFunc( GL_SRC_ALPHA_SATURATE, GL_ONE );
    //glHint(GL_POLYGON_SMOOTH_HINT,  GL_NICEST);

    for (int i=0;i<data->np[0];i++) {
        j=(data->point[0][i].y()/total); // ratio of this pie slice
        QColor col1=color[i % color.size()];
        w.qglColor(col1);
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
        w.qglColor(Qt::black);
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
    glDisable(GL_POLYGON_SMOOTH);
    glDisable(GL_BLEND);
    //glDisable(GL_DEPTH_TEST);
    //glDisable(GL_TEXTURE_2D);
}
