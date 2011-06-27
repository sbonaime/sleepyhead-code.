/********************************************************************
 gFooBar Implementation
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*********************************************************************/

#include "gFooBar.h"

gFooBar::gFooBar(int offset,QColor col1,QColor col2,bool funkbar)
:gLayer(NULL),m_funkbar(funkbar),m_offset(offset)
{
    color.clear();
    color.push_back(col2);
    color.push_back(col1);
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

    QColor & col1=color[0];
    QColor & col2=color[1];

    float h=m_offset;
    glColor4ub(col1.red(),col1.green(),col1.blue(),col1.alpha());

    glLineWidth(1);
    glBegin(GL_LINES);
    glVertex2f(start_px, h);
    glVertex2f(start_px+width, h);
    glEnd();

    double rmx=w.rmax_x-w.rmin_x;
    double px=((1/rmx)*(w.min_x-w.rmin_x))*width;
    double py=((1/rmx)*(w.max_x-w.rmin_x))*width;

    glColor4ub(col2.red(),col2.green(),col2.blue(),col2.alpha());
    glLineWidth(4);
    glBegin(GL_LINES);
    glVertex2f(start_px+px-4,h);
    glVertex2f(start_px+py+4,h);
    glEnd();

    glLineWidth(1);

    if ((m_funkbar)) { // && ((w.min_x>w.rmin_x) || (w.max_x<w.rmax_x))) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

        glColor4f(.2,.2,.2,.4);
        glBegin(GL_QUADS);
        glVertex2f(start_px+px, w.GetBottomMargin());
        glVertex2f(start_px+px, w.GetBottomMargin()+height);
        glVertex2f(start_px+py, w.GetBottomMargin()+height);
        glVertex2f(start_px+py, w.GetBottomMargin());
        glEnd();
        glDisable(GL_BLEND);
    }
}

