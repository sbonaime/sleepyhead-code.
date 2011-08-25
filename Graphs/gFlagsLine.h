/*
 gFlagsLine Header
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

#ifndef GFLAGSLINE_H
#define GFLAGSLINE_H

#include "gGraphView.h"

class gFlagsGroup;

class gFlagsLine:public Layer
{
    friend class gFlagsGroup;
    public:
        gFlagsLine(ChannelID code,QColor col=Qt::black,QString label="",bool always_visible=false,FlagType flt=FT_Bar);
        virtual ~gFlagsLine();

        virtual void paint(gGraph & w,int left, int top, int width, int height);
        bool isAlwaysVisible() { return m_always_visible; }
        void setAlwaysVisible(bool b) { m_always_visible=b; }
        QString label() { return m_label; }
        void setLabel(QString s) { m_label=s; }
        void setTotalLines(int i) { total_lines=i; }
        void setLineNum(int i) { line_num=i; }
    protected:
        QString m_label;
        bool m_always_visible;
        int total_lines,line_num;
        FlagType m_flt;
        QColor m_flag_color;
};

class gFlagsGroup:public LayerGroup
{
public:
    gFlagsGroup();
    virtual ~gFlagsGroup();

    virtual void paint(gGraph & w,int left, int top, int width, int height);
    virtual qint64 Minx();
    virtual qint64 Maxx();
};

#endif // GFLAGSLINE_H
