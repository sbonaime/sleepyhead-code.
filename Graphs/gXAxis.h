/********************************************************************
 gXAxis Header
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*********************************************************************/

#ifndef GXAXIS_H
#define GXAXIS_H
#include "graphlayer.h"

class gXAxis:public gLayer
{
    public:
        gXAxis(QColor col=QColor("black"));
        virtual ~gXAxis();
        virtual void Plot(gGraphWindow & w,float scrx,float scry);
        static const int Margin=25; // How much room does this take up. (Bottom margin)
        void SetShowMinorLines(bool b) { m_show_minor_lines=b; };
        void SetShowMajorLines(bool b) { m_show_major_lines=b; };
        bool ShowMinorLines() { return m_show_minor_lines; };
        bool ShowMajorLines() { return m_show_major_lines; };
        void SetShowMinorTicks(bool b) { m_show_minor_ticks=b; };
        void SetShowMajorTicks(bool b) { m_show_major_ticks=b; };
        bool ShowMinorTicks() { return m_show_minor_ticks; };
        bool ShowMajorTicks() { return m_show_major_ticks; };

    protected:
//        virtual const wxString & Format(double v) { static wxString t; wxDateTime d; d.Set(v); t=d.Format(wxT("%H:%M")); return t; };
        bool m_show_major_lines;
        bool m_show_minor_lines;
        bool m_show_minor_ticks;
        bool m_show_major_ticks;

};
#endif // GXAXIS_H
