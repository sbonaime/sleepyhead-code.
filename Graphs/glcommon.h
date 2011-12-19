/*
 glcommon GL code & font stuff Header
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

#ifndef GLCOMMON_H
#define GLCOMMON_H

#include <QtOpenGL/qgl.h>
#include <QColor>

/*! \brief Draw an outline of a rounded rectangle
    \param radius Radius of corner rounding
    \param lw Line Width
    \param color Color of drawn lines
    */
void LinedRoundedRectangle(int x,int y,int w,int h,int radius,int lw,QColor color);

/*! \brief Draws a filled rounded rectangle
    \param radius Radius of corner rounding
    \param color Color of entire rectangle
    */
void RoundedRectangle(int x,int y,int w,int h,int radius,const QColor color);

#ifdef BUILD_WITH_MSVC
// Visual C++ doesn't have either of these in it's maths header.. I'm not surprised at Microsofts maths abilities..
const double M_PI=3.141592653589793;

double round(double number);
#endif

#endif // GLCOMMON_H
