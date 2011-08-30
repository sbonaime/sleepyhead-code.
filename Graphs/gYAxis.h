/*
 gYAxis Header
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

#ifndef GYAXIS_H
#define GYAXIS_H

#include "gGraphView.h"

class gYSpacer:public Layer
{
    public:
        gYSpacer(int spacer=20);
        virtual void paint(gGraph & w,int left,int top, int width, int height) {w=w; left=left; top=top; width=width; height=height; }

};

class gXGrid:public Layer
{
public:
    gXGrid(QColor col=QColor("black"));
    virtual ~gXGrid();
    virtual void paint(gGraph & w,int left,int top, int width, int height);

    void setShowMinorLines(bool b) { m_show_minor_lines=b; }
    void setShowMajorLines(bool b) { m_show_major_lines=b; }
    bool showMinorLines() { return m_show_minor_lines; }
    bool showMajorLines() { return m_show_major_lines; }
protected:
    bool m_show_major_lines;
    bool m_show_minor_lines;
    QColor m_major_color;
    QColor m_minor_color;
    GLBuffer * minorvert, * majorvert;
};

class gYAxis:public Layer
{
    public:
        gYAxis(QColor col=QColor("black"));
        virtual ~gYAxis();
        virtual void paint(gGraph & w,int left,int top, int width, int height);
        void SetShowMinorLines(bool b) { m_show_minor_lines=b; }
        void SetShowMajorLines(bool b) { m_show_major_lines=b; }
        bool ShowMinorLines() { return m_show_minor_lines; }
        bool ShowMajorLines() { return m_show_major_lines; }
        void SetShowMinorTicks(bool b) { m_show_minor_ticks=b; }
        void SetShowMajorTicks(bool b) { m_show_major_ticks=b; }
        bool ShowMinorTicks() { return m_show_minor_ticks; }
        bool ShowMajorTicks() { return m_show_major_ticks; }
        virtual const QString & Format(double v) { static QString t; t.sprintf("%.1f",v); return t; }
        static const int Margin=50; // Left margin space

        void SetScale(float f) { m_yaxis_scale=f; } // Scale yaxis ticker values (only what's displayed)
        float Scale() { return m_yaxis_scale; }
    protected:
        bool m_show_major_lines;
        bool m_show_minor_lines;
        bool m_show_minor_ticks;
        bool m_show_major_ticks;
        float m_yaxis_scale;

        QColor m_line_color;
        QColor m_text_color;
        GLBuffer * vertarray;
};

#endif // GYAXIS_H
