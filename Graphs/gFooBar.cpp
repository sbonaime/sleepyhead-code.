/*
 gFooBar Implementation
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

#include "gFooBar.h"

gFooBar::gFooBar(int offset,QColor handle_color,QColor line_color,bool shadow,QColor shadow_color)
:gLayer(EmptyChannel),m_offset(offset),m_handle_color(handle_color),m_line_color(line_color),m_shadow(shadow),m_shadow_color(shadow_color)
{
}
gFooBar::~gFooBar()
{
}
void gFooBar::Plot(gGraphWindow & w,float scrx,float scry)
{
    if (!m_visible) return;

    double xx=w.max_x-w.min_x;

    if (xx==0)
        return;

    int start_px=w.GetLeftMargin()-1;
    int width=scrx - (w.GetLeftMargin() + w.GetRightMargin());
    int height=scry - (w.GetTopMargin() + w.GetBottomMargin());
    int end_px=scrx-w.GetRightMargin();

    glDisable(GL_DEPTH_TEST);
    float h=m_offset;

    glLineWidth(1);
    glBegin(GL_LINES);
    w.qglColor(m_line_color);
    glVertex2f(start_px, h);
    glVertex2f(start_px+width, h);
    glEnd();

    double rmx=w.rmax_x-w.rmin_x;
    double px=((1/rmx)*(w.min_x-w.rmin_x))*width;
    double py=((1/rmx)*(w.max_x-w.rmin_x))*width;

    glLineWidth(4);
    glBegin(GL_LINES);
    w.qglColor(m_handle_color);
    glVertex2f(start_px+px-4,h);
    glVertex2f(start_px+py+4,h);
    glEnd();

    glLineWidth(1);

    if ((m_shadow)) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

        glBegin(GL_QUADS);
        w.qglColor(m_shadow_color);

        glVertex2f(start_px, w.GetBottomMargin());
        glVertex2f(start_px, w.GetBottomMargin()+height);
        glVertex2f(start_px+px, w.GetBottomMargin()+height);
        glVertex2f(start_px+px, w.GetBottomMargin());

        glVertex2f(start_px+py, w.GetBottomMargin());
        glVertex2f(start_px+py, w.GetBottomMargin()+height);
        glVertex2f(end_px, w.GetBottomMargin()+height);
        glVertex2f(end_px, w.GetBottomMargin());
        glEnd();
        glDisable(GL_BLEND);
    }
}

