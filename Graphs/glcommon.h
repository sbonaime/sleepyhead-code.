/*
 glcommon GL code & font stuff Header
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

#ifndef GLCOMMON_H
#define GLCOMMON_H

#include <QtOpenGL/qgl.h>
#include <QColor>

void LinedRoundedRectangle(int x,int y,int w,int h,int radius,int lw,QColor color);
void RoundedRectangle(int x,int y,int w,int h,int radius,const QColor color);

#endif // GLCOMMON_H
