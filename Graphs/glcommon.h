/********************************************************************
 glcommon GL code & font stuff Header
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*********************************************************************/

#ifndef GLCOMMON_H
#define GLCOMMON_H

#include <QFont>
#include <QColor>
#include <QtOpenGL/qgl.h>
#include "Graphs/graphwindow.h"

void InitFonts();
void DoneFonts();

extern QFont * defaultfont;
extern QFont * mediumfont;
extern QFont * bigfont;


class gGraphWindow;
void GetTextExtent(QString text, float & width, float & height, QFont *font=defaultfont);
void DrawText(gGraphWindow & wid, QString text, int x, int y, float angle=0, QColor color=QColor("black"),QFont *font=defaultfont);

void LinedRoundedRectangle(int x,int y,int w,int h,int radius,int lw,QColor color);
void RoundedRectangle(int x,int y,int w,int h,int radius,const QColor color);

#endif // GLCOMMON_H
