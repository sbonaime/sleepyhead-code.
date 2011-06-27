/********************************************************************
 glcommon GL code & font stuff
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*********************************************************************/

#include <QApplication>
#include <QFontMetrics>
#include <math.h>
#include "glcommon.h"
#include "SleepLib/profiles.h"
#include <QFile>
bool _font_init=false;

QFont * defaultfont;
QFont * mediumfont;
QFont * bigfont;

// Must be called from a thread inside the application.
void InitFonts()
{
    if (!_font_init) {

        defaultfont=new QFont("FreeSans",10);
        bigfont=new QFont("FreeSans",35);
        mediumfont=new QFont("FreeSans",18);

        _font_init=true;
    }
}
void DoneFonts()
{
    if (_font_init) {
        delete bigfont;
        delete mediumfont;
        _font_init=false;
    }
}

void GetTextExtent(QString text, float & width, float & height, QFont *font)
{
    QFontMetrics fm(*font);
    //QRect r=fm.tightBoundingRect(text);
    width=fm.width(text); //fm.width(text);
    height=fm.xHeight()+2; //fm.ascent();
}
void DrawText(gGraphWindow & wid, QString text, int x, int  y, float angle, QColor color,QFont *font)
{
    //QFontMetrics fm(*font);
    float w,h;
    //GetTextExtent(text,w,h,font);
    //int a=fm.overlinePos(); //ascent();
    //LinedRoundedRectangle(x,wid.GetScrY()-y,w,h,0,1,QColor("black"));
    if (!font) {
        qDebug("Font Problem. Forgot to call GraphInit() ?");
        abort();
        return;
    }

//    glEnable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    glFlush();
    QPainter painter(&wid);
    painter.setFont(*font);
    painter.setPen(Qt::black);
    painter.setOpacity(1);
 //   painter.setCompositionMode(QPainter::CompositionMode_);
    if (angle==0) {
        painter.drawText(x,y,text);
    } else {
        GetTextExtent(text, w, h, font);
        painter.translate(floor(x),floor(y));
        painter.rotate(-90);
        painter.drawText(floor(-w/2.0),floor(-h/2.0),text);
        painter.translate(floor(-x),floor(-y));

    }

    painter.end();

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);

}


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
    glDisable(GL_TEXTURE_2D);
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

    glEnable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
}

