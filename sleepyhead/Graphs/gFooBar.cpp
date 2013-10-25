/*
 gFooBar Implementation
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/
#include <cmath>
#include "gFooBar.h"
#include "gYAxis.h"

gShadowArea::gShadowArea(QColor shadow_color,QColor line_color)
:Layer(NoChannel),m_shadow_color(shadow_color),m_line_color(line_color)
{
    addVertexBuffer(quads=new gVertexBuffer(20,GL_QUADS));
    addVertexBuffer(lines=new gVertexBuffer(20,GL_LINES));
    quads->forceAntiAlias(true);
    lines->setAntiAlias(true);
    lines->setSize(2);
}
gShadowArea::~gShadowArea()
{
}
void gShadowArea::paint(gGraph & w,int left, int top, int width, int height)
{
    if (!m_visible) return;
    double xx=w.max_x-w.min_x;

    if (xx==0)
        return;

    int start_px=left-1;
    int end_px=left+width;

    //float h=top;

    double rmx=w.rmax_x-w.rmin_x;
    double px=((1.0/rmx)*(w.min_x-w.rmin_x))*width;
    double py=((1.0/rmx)*(w.max_x-w.rmin_x))*width;

    quads->add(start_px,top,start_px,top+height,start_px+px, top+height, start_px+px, top,m_shadow_color.rgba());
    quads->add(start_px+py, top, start_px+py, top+height,end_px, top+height, end_px, top,m_shadow_color.rgba());

    lines->add(start_px+px, top, start_px+py, top,m_line_color.rgba());
    lines->add(start_px+px, top+height+1, start_px+py, top+height+1,m_line_color.rgba());
}

gFooBar::gFooBar(int offset,QColor handle_color,QColor line_color)
:Layer(NoChannel),m_offset(offset),m_handle_color(handle_color),m_line_color(line_color)
{
}
gFooBar::~gFooBar()
{
}
void gFooBar::paint(gGraph & w,int left, int top, int width, int height)
{
    Q_UNUSED(top);
    Q_UNUSED(left);
    Q_UNUSED(width);
    Q_UNUSED(height);
    if (!m_visible) return;

    double xx=w.max_x-w.min_x;

    if (xx==0)
        return;

    //int start_px=left;
    //int end_px=left+width;

    //float h=top;

  /*  glLineWidth(1);
    glBegin(GL_LINES);
    w.qglColor(m_line_color);
    glVertex2f(start_px, h);
    glVertex2f(start_px+width, h);
    glEnd();

    double rmx=w.rmax_x-w.rmin_x;
    double px=((1/rmx)*(w.min_x-w.rmin_x))*width;
    double py=((1/rmx)*(w.max_x-w.rmin_x))*width;

    int extra=0;
    if (fabs(px-py)<2) extra=2;

    int hh=25;
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glBegin(GL_QUADS);

    w.qglColor(m_handle_color);
    glVertex2f(start_px+px-extra,top-hh);
    glVertex2f(start_px+py+extra,top-hh);
    //glColor4ub(255,255,255,128);
    glColor4ub(255,255,255,128);
    glVertex2f(start_px+py+extra,top-hh/2.0);
    glVertex2f(start_px+px-extra,top-hh/2.0);
//    glColor4ub(255,255,255,128);
    glColor4ub(255,255,255,128);
    glVertex2f(start_px+px-extra,top-hh/2.0);
    glVertex2f(start_px+py+extra,top-hh/2.0);
    w.qglColor(m_handle_color);
//    glColor4ub(192,192,192,128);
    glVertex2f(start_px+py+extra,h);
    glVertex2f(start_px+px-extra,h);
    glEnd();
    glDisable(GL_BLEND);

    w.qglColor(m_handle_color);
    glBegin(GL_LINE_LOOP);
    glVertex2f(start_px+px-extra,top-hh);
    glVertex2f(start_px+py+extra,top-hh);
    glVertex2f(start_px+py+extra,h);
    glVertex2f(start_px+px-extra,h);
    glEnd();

    glLineWidth(3);
    glBegin(GL_LINES);
    w.qglColor(m_handle_color);
    glVertex2f(start_px+px-extra,h);
    glVertex2f(start_px+py+extra,h);
    glEnd();

    glLineWidth(1);  */

}

