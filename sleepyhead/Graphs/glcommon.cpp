/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 *
 * glcommon GL code & font stuff
 *
 * Copyright (c) 2011-2014 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#include <cmath>
#include "glcommon.h"

#ifdef BUILD_WITH_MSVC
double round(double number)
{
    return number < 0.0 ? ceil(number - 0.5) : floor(number+0.5);
}
#endif


void RoundedRectangle(int x,int y,int w,int h,int radius,const QColor color)
{

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glColor4ub(color.red(),color.green(),color.blue(),color.alpha());

    glBegin(GL_POLYGON);
        glVertex2i(x+radius,y);
        glVertex2i(x+w-radius,y);
        for(float i=(float)M_PI*1.5f;i<M_PI*2;i+=0.1f)
            glVertex2f(x+w-radius+cos(i)*radius,y+radius+sin(i)*radius);
        glVertex2i(x+w,y+radius);
        glVertex2i(x+w,y+h-radius);
        for(float i=0;i<(float)M_PI*0.5f;i+=0.1f)
            glVertex2f(x+w-radius+cos(i)*radius,y+h-radius+sin(i)*radius);
        glVertex2i(x+w-radius,y+h);
        glVertex2i(x+radius,y+h);
        for(float i=(float)M_PI*0.5f;i<M_PI;i+=0.1f)
            glVertex2f(x+radius+cos(i)*radius,y+h-radius+sin(i)*radius);
        glVertex2i(x,y+h-radius);
        glVertex2i(x,y+radius);
        for(float i=(float)M_PI;i<M_PI*1.5f;i+=0.1f)
            glVertex2f(x+radius+cos(i)*radius,y+radius+sin(i)*radius);
    glEnd();

    glDisable(GL_BLEND);
}

void LinedRoundedRectangle(int x,int y,int w,int h,int radius,int lw,QColor color)
{
    //glDisable(GL_TEXTURE_2D);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glColor4ub(color.red(),color.green(),color.blue(),color.alpha());
    glLineWidth((GLfloat)lw);

    glBegin(GL_LINE_STRIP);
        for(float i=(float)M_PI;i<=1.5f*M_PI;i+=0.1f)
            glVertex2f(radius*cos(i)+x+radius,radius*sin(i)+y+radius);
        for(float i=1.5f*(float)M_PI;i<=2*M_PI; i+=0.1f)
            glVertex2f(radius*cos(i)+x+w-radius,radius*sin(i)+y+radius);
        for(float i=0;i<=0.5f*M_PI; i+=0.1f)
            glVertex2f(radius*cos(i)+x+w-radius,radius*sin(i)+y+h-radius);
        for(float i=0.5f*(float)M_PI;i<=M_PI;i+=0.1f)
            glVertex2f(radius*cos(i)+x+radius,radius*sin(i)+y+h-radius);
        glVertex2i(x,y+radius);
    glEnd();

    //glEnable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
}

