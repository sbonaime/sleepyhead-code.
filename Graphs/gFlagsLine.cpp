/********************************************************************
 gFlagsLine Implementation
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*********************************************************************/

#include <cmath>
#include <vector>
#include "SleepLib/profiles.h"
#include "gFlagsLine.h"

gFlagsGroup::gFlagsGroup()
{
}
gFlagsGroup::~gFlagsGroup()
{
}

void gFlagsGroup::Plot(gGraphWindow &w, float scrx, float scry)
{
    if (!m_visible) return;
    int start_px=w.GetLeftMargin();
    int start_py=w.GetBottomMargin();
    int width=scrx-(w.GetLeftMargin()+w.GetRightMargin())-1;
    int height=scry-(w.GetTopMargin()+w.GetBottomMargin());

    glColor3f (0.1F, 0.1F, 0.1F);
    glLineWidth (1);
    glBegin (GL_LINE_LOOP);
    glVertex2f (start_px-1, start_py);
    glVertex2f (start_px-1, start_py+height);
    glVertex2f (start_px+width,start_py+height);
    glVertex2f (start_px+width, start_py);
    glEnd ();

    vector<gFlagsLine *> visible;
    for (unsigned i=0;i<layers.size();i++) {
        gFlagsLine *f=dynamic_cast<gFlagsLine *>(layers[i]);
        if (!f) continue;

        if (!f->isEmpty() || f->isAlwaysVisible()) {
            visible.push_back(f);
        }
    }
    int vis=visible.size();
    for (unsigned i=0;i<visible.size();i++) {
        visible[i]->line_num=i;
        visible[i]->total_lines=vis;
        visible[i]->Plot(w,scrx,scry);
    }
}


gFlagsLine::gFlagsLine(gPointData *d,QColor col,QString _label,bool always_visible, int _line_num,int _total_lines)
:gLayer(d),label(_label),m_always_visible(always_visible),line_num(_line_num),total_lines(_total_lines)
{
    color.clear();
    color.push_back(col);

}
gFlagsLine::~gFlagsLine()
{
}
void gFlagsLine::Plot(gGraphWindow & w,float scrx,float scry)
{
    if (!m_visible) return;
    if (!data) return;
    if (!data->IsReady()) return;

    double minx;
    double maxx;

    if (w.BlockZoom()) {
        minx=w.rmin_x;
        maxx=w.rmax_x;
    } else {
        minx=w.min_x;
        maxx=w.max_x;
    }

    double xx=maxx-minx;
    if (xx<=0) return;

    int start_px=w.GetLeftMargin();
    int start_py=w.GetBottomMargin();
    int width=scrx-(w.GetLeftMargin()+w.GetRightMargin())-1;
    int height=scry-(w.GetTopMargin()+w.GetBottomMargin());

    double xmult=width/xx;

    static QColor col1=QColor(0xd0,0xff,0xd0,0xff);
    static QColor col2=QColor(0xff,0xff,0xff,0xff);


    float line_h=float(height-2)/float(total_lines);
    line_h=line_h;
    float line_top=(start_py+height-line_h)-line_num*line_h;

    // Alternating box color
    QColor *barcol=&col2;
    if (line_num & 1)
        barcol=&col1;


    // Filled rectangle
    glColor4ub(barcol->red(),barcol->green(),barcol->blue(),barcol->alpha());
    glBegin(GL_QUADS);
    glVertex2f(start_px+1, line_top);
    glVertex2f(start_px+1, line_top+line_h);
    glVertex2f(start_px+width-1, line_top+line_h);
    glVertex2f(start_px+width-1, line_top);
    glEnd();

    qint32 vertcnt=0;
    GLshort * vertarray=vertex_array[0];
    qint32 quadcnt=0;
    GLshort * quadarray=vertex_array[1];
    assert(vertarray!=NULL);
    assert(quadarray!=NULL);

    // Draw text label
    float x,y;
    GetTextExtent(label,x,y);
    DrawText(label,start_px-x-10,(scry-line_top)-(line_h/2)+(y/2));
    float x1,x2;

    QColor & col=color[0];

    float top=floor(line_top)+2;
    float bottom=top+floor(line_h)-3;

    for (int n=0;n<data->VC();n++) {
        if (!data->np[n]) continue;

        for (int i=0;i<data->np[n];i++) {
            QPointD & rp=data->point[n][i];
            if (rp.y() < minx) continue;
            if (rp.x() > maxx) break;
            x1=(rp.x() - minx) * xmult + w.GetLeftMargin();
            if (rp.x()==rp.y()) {
                vertarray[vertcnt++]=x1;
                vertarray[vertcnt++]=top;
                vertarray[vertcnt++]=x1;
                vertarray[vertcnt++]=bottom;
            } else {
                x2=(rp.y()-minx)*xmult+w.GetLeftMargin();
                //w1=x2-x1;
                quadarray[quadcnt++]=x1;
                quadarray[quadcnt++]=top;
                quadarray[quadcnt++]=x1;
                quadarray[quadcnt++]=bottom;
                quadarray[quadcnt++]=x2;
                quadarray[quadcnt++]=bottom;
                quadarray[quadcnt++]=x2;
                quadarray[quadcnt++]=top;
            }
        }
    }
    glLineWidth (1);

    glColor4ub(col.red(),col.green(),col.blue(),col.alpha());
    glScissor(w.GetLeftMargin(),w.GetBottomMargin(),width,height);
    glEnable(GL_SCISSOR_TEST);

    glLineWidth (1);
    bool antialias=pref["UseAntiAliasing"].toBool();
    if (antialias) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); //_MINUS_SRC_ALPHA);
        glEnable(GL_LINE_SMOOTH);
        glHint(GL_LINE_SMOOTH_HINT,  GL_NICEST);

    }

    if (quadcnt>0) {
        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(2, GL_SHORT, 0, quadarray);
        glDrawArrays(GL_QUADS, 0, quadcnt>>1);
        glDisableClientState(GL_VERTEX_ARRAY);
    }
    if (vertcnt>0) {
        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(2, GL_SHORT, 0, vertarray);
        glDrawArrays(GL_LINES, 0, vertcnt>>1);
        glDisableClientState(GL_VERTEX_ARRAY);
    }
    if (antialias) {
        glDisable(GL_LINE_SMOOTH);
        glDisable(GL_BLEND);
    }

    glDisable(GL_SCISSOR_TEST);
}
