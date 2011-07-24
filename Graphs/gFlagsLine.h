/********************************************************************
 gFlagsLine Header
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*********************************************************************/

#ifndef GFLAGSLINE_H
#define GFLAGSLINE_H

#include "graphlayer.h"

class gFlagsGroup;

class gFlagsLine:public gLayer
{
    friend class gFlagsGroup;
    public:
        gFlagsLine(gPointData *d=NULL,QColor col=Qt::black,QString _label="",bool always_visible=false);
        virtual ~gFlagsLine();

        virtual void Plot(gGraphWindow & w,float scrx,float scry);
        bool isAlwaysVisible() { return m_always_visible; }
        void setAlwaysVisible(bool b) { m_always_visible=b; }
        QString Label() { return label; }
        void Label(QString s) { label=s; }
        void setTotalLines(int i) { total_lines=i; };
        void setLineNum(int i) { line_num=i; };
    protected:
        QString label;
        bool m_always_visible;
        int total_lines,line_num;
};

class gFlagsGroup:public gLayerGroup
{
public:
    gFlagsGroup();
    virtual ~gFlagsGroup();

    virtual void Plot(gGraphWindow &w, float scrx, float scry);
};

#endif // GFLAGSLINE_H
