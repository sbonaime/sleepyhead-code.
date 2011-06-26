/********************************************************************
 gYAxis Header
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*********************************************************************/

#ifndef GYAXIS_H
#define GYAXIS_H

#include "graphlayer.h"

class gYAxis:public gLayer
{
    public:
        gYAxis(QColor col=QColor("black"));
        virtual ~gYAxis();
        virtual void Plot(gGraphWindow & w,float scrx,float scry);
        void SetShowMinorLines(bool b) { m_show_minor_lines=b; };
        void SetShowMajorLines(bool b) { m_show_major_lines=b; };
        bool ShowMinorLines() { return m_show_minor_lines; };
        bool ShowMajorLines() { return m_show_major_lines; };
        void SetShowMinorTicks(bool b) { m_show_minor_ticks=b; };
        void SetShowMajorTicks(bool b) { m_show_major_ticks=b; };
        bool ShowMinorTicks() { return m_show_minor_ticks; };
        bool ShowMajorTicks() { return m_show_major_ticks; };
        virtual const QString & Format(double v) { static QString t; t.sprintf("%.1f",v); return t; };
        static const int Margin=50; // Left margin space
    protected:
        bool m_show_major_lines;
        bool m_show_minor_lines;
        bool m_show_minor_ticks;
        bool m_show_major_ticks;
};

#endif // GYAXIS_H
