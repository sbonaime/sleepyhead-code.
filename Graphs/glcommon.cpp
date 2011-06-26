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

//FontManager * font_manager;
//VertexBuffer *vbuffer=NULL;
//TextMarkup *markup=NULL;
QFont * defaultfont;
QFont * mediumfont;
QFont * bigfont;

/*GLText::GLText(const QString & text, int x, int y, float angle, QColor color, QFont * font)
{
}
void GLText::Draw()
{
} */

// Must be called from a thread inside the application.
void InitFonts()
{
    if (!_font_init) {
    /*#if defined(__WXMSW__)
        static bool glewinit_called=false;
        if (!glewinit_called) {
            glewInit(); // Dont forget this nasty little sucker.. :)
            glewinit_called=true;
        }
    #endif */
        //wxString glvendor=wxString((char *)glGetString(GL_VENDOR),wxConvUTF8);
        //wxString glrenderer=wxString((char *)glGetString(GL_RENDERER),wxConvUTF8);
        //wxString glversion=wxString((char *)glGetString(GL_VERSION),wxConvUTF8);
        //wxLogDebug(wxT("GLInfo: ")+glvendor+wxT(" ")+glrenderer+wxT(" ")+glversion);

        // Despite the stupid warning, this does NOT always evaluate as true. Too lazy to put it in #ifdefs
        /*if (!glGenBuffers) {
            wxMessageBox(wxT("Sorry, your computers graphics card drivers are too old to run this program.\n")+glvendor+wxT(" may have an update.\n")+glrenderer+wxT("\n")+glversion,_("Welcome to..."),wxOK,NULL);
            exit(-1);
        } */

        //font_manager=new FontManager();
        //vbuffer=new VertexBuffer((char *)"v3i:t2f:c4f");
        //defaultfont=font_manager->GetFromFilename(pref.Get("{home}{sep}FreeSans.ttf"),12);
        //bigfont=font_manager->GetFromFilename(pref.Get("{home}{sep}FreeSans.ttf"),32);
        //markup=new TextMarkup();
        //markup->SetForegroundColor(QColor("light green"));

        //glBindTexture( GL_TEXTURE_2D, font_manager->m_atlas->m_texid );

/*        QString fontfile=pref.Get("{home}{sep}FreeSans.ttf");
        QFile f(fontfile);
        if (!f.exists(fontfile)) {
            f.open(QIODevice::WriteOnly);
            if (!f.write((char *)FreeSans_ttf,FreeSans_length)) {
                qWarning("Couldn't Write Font file.. Sorry.. need it to run");
                return;
            }
            f.close();
        } */

        defaultfont=new QFont("Helvetica",10);//new QFont(QApplication::font());
        bigfont=new QFont("Helvetica",35);
        mediumfont=new QFont("Helvetica",18);

        _font_init=true;
    }
}
void DoneFonts()
{
    if (_font_init) {
        delete bigfont;
        delete mediumfont;
        //delete font_manager;
      //  delete vbuffer;
        //delete markup;
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
/*{

    TextureGlyph *glyph;
    height=width=0;

    for (unsigned i=0;i<text.length();i++) {
        glyph=font->GetGlyph((wchar_t)text[i].unicode());
        if (glyph->m_height > height) height=glyph->m_height;
        width+=glyph->m_advance_x;
    }
}*/
 // The actual raw font drawing routine..
/*void DrawText2(QString text, float x, float y,QFont *font)
{
    Pen pen;
    pen.x=x;
    pen.y=y;

    TextureGlyph *glyph;
    glyph=font->GetGlyph((wchar_t)text[0].unicode());
    if (!glyph) return;
    //assert(vbuffer!=NULL);

    //vbuffer->Clear();

    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glEnable( GL_TEXTURE_2D );
    glColor4f(0,0,0,1);

    glyph->Render(markup,&pen);
//    glyph->AddToVertexBuffer(vbuffer, markup, &pen);
    for (unsigned j=1; j<text.length(); ++j) {
        glyph=font->GetGlyph(text[j].unicode());
        pen.x += glyph->GetKerning(text[j-1].unicode());
        glyph->Render(markup,&pen);

  //      glyph->AddToVertexBuffer(vbuffer, markup, &pen);
    }
    //glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    //vbuffer->Render(GL_TRIANGLES, (char *)"vtc" );
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);

} */

void DrawText(gGraphWindow & wid, QString text, float x, float y, float angle, QColor color,QFont *font)
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
    glColor4ub(color.red(),color.green(),color.blue(),color.alpha());
    if (angle==0) {
        wid.renderText(x,y,text,*font);
       // DrawText2(text,x,y,font);
        return;
    }
    QPainter painter(&wid);

    //painter.setMatrixEnabled(true);
    //painter.setWindow(0,0,wid.GetScrX(),wid.GetScrY());
    //painter.begin();
    //QMatrix matrix;
    //matrix.rotate(angle);
    //painter.setMatrix(matrix,true);
    //float w,h;
    GetTextExtent(text, w, h, font);

    painter.translate(floor(x),floor(y));
    painter.rotate(-90);
    painter.setFont(*font);
    painter.setPen(QPen(color));
    painter.drawText(floor(-w/2.0),floor(-h/2.0),text);
    painter.translate(floor(-x),floor(-y));
    painter.end();
/*    glPushMatrix();
    glEnable(GL_DEPTH_TEST);
    //glEnable(GL_TEXTURE_2D);
    glTranslatef(floor(x),floor(y),0);
    glRotatef(angle, 0.0f, 0.0f, 1.0f);

    wid.renderText(floor(-w/2.0),floor(-h/2.0),text,*font);
    //DrawText2(text,floor(-w/2.0),floor(-h/2.0),font);
    glTranslatef(floor(-x),floor(-y),0);
    //glDisable(GL_TEXTURE_2D);
    glPopMatrix(); */
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);

}


void RoundedRectangle(int x,int y,int w,int h,int radius,const QColor color)
{
    //glDisable(GL_TEXTURE_2D);

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

