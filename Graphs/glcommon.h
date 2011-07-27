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

void InitGraphs();
void DoneGraphs();

extern QFont * defaultfont;
extern QFont * mediumfont;
extern QFont * bigfont;

const int num_vert_arrays=4;
const qint32 maxverts=65536*4; // Resolution dependant..
extern GLshort *vertex_array[num_vert_arrays];


class gGraphWindow;
void GetTextExtent(QString text, float & width, float & height, QFont *font=defaultfont);
void DrawText(gGraphWindow &wid, QString text, int x, int y, float angle=0, QColor color=Qt::black,QFont *font=defaultfont);
void DrawTextQueue(gGraphWindow & wid);

void LinedRoundedRectangle(int x,int y,int w,int h,int radius,int lw,QColor color);
void RoundedRectangle(int x,int y,int w,int h,int radius,const QColor color);

#endif // GLCOMMON_H
