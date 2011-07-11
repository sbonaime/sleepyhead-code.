/********************************************************************
 glcommon GL code & font stuff
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*********************************************************************/

#include <QApplication>
#include <QFontMetrics>
#include <QDebug>
#include <math.h>
#include "glcommon.h"
#include "SleepLib/profiles.h"
#include <QFile>

bool _graph_init=false;

QFont * defaultfont=NULL;
QFont * mediumfont=NULL;
QFont * bigfont=NULL;

GLshort *vertex_array[num_vert_arrays]={NULL};
bool evil_intel_graphics_chip=false;

// Must be called from a thread inside the application.
void InitGraphs()
{
    if (!_graph_init) {

        defaultfont=new QFont("FreeSans",10);
        bigfont=new QFont("FreeSans",35);
        mediumfont=new QFont("FreeSans",18);
        for (int i=0;i<num_vert_arrays;i++) {
            vertex_array[i]=new GLshort[maxverts];
        }
        _graph_init=true;
    }
}
void DoneGraphs()
{
    if (_graph_init) {
        delete defaultfont;
        delete bigfont;
        delete mediumfont;
        for (int i=0;i<num_vert_arrays;i++) {
            delete [] vertex_array[i];
        }
        _graph_init=false;
    }
}

void GetTextExtent(QString text, float & width, float & height, QFont *font)
{
    QFontMetrics fm(*font);
    //QRect r=fm.tightBoundingRect(text);
    width=fm.width(text); //fm.width(text);
    height=fm.xHeight()+2; //fm.ascent();
}
void RDrawText(gGraphWindow & wid, QString text, int x, int  y, float angle, QColor color,QFont *font)
{
    //QFontMetrics fm(*font);
    float w,h;
    //GetTextExtent(text,w,h,font);
    //int a=fm.overlinePos(); //ascent();
    //LinedRoundedRectangle(x,wid.GetScrY()-y,w,h,0,1,QColor("black"));
    if (!font) {
        qDebug() << "Font Problem. Forgot to call GraphInit() ?";
        abort();
        return;
    }

//    glEnable(GL_TEXTURE_2D);
//    glDisable(GL_DEPTH_TEST);
    glFinish();
    QPainter painter(&wid);
    painter.setFont(*font);
    color=Qt::black;
    painter.setPen(color);
    painter.setBrush(QBrush(color));
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
struct TextBuffer
{
    gGraphWindow *wid;
    QString text;
    int x,y;
    float angle;
    QColor *color;
    QFont *font;
    TextBuffer(gGraphWindow * _wid, QString _text, int _x, int  _y, float _angle, QColor *_color,QFont *_font) {
        wid=_wid; text=_text; x=_x; y=_y; angle=_angle; color=_color; font=_font;
    }
};
vector<TextBuffer *> TextQue;

void DrawTextQueue()
{
    for (unsigned i=0;i<TextQue.size();i++) {
        TextBuffer * t=TextQue[i];
        RDrawText(*t->wid,t->text,t->x,t->y,t->angle,*t->color,t->font);
        delete TextQue[i];
    }
    TextQue.clear();
}
void DrawText(gGraphWindow & wid, QString text, int x, int  y, float angle, QColor color,QFont *font)
{
    TextQue.push_back(new TextBuffer(&wid,text,x,y,angle,&color,font));
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

