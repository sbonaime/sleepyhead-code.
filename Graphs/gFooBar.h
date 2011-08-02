/********************************************************************
 gFooBar Header
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*********************************************************************/

#ifndef GFOOBAR_H
#define GFOOBAR_H

#include "graphlayer.h"

class gFooBar:public gLayer
{
    public:
        gFooBar(int offset=10,QColor handle_color=QColor("orange"),QColor line_color=QColor("dark grey"),bool shadow=false,QColor shadow_color=QColor(40,40,40,40));
        virtual ~gFooBar();
        virtual void Plot(gGraphWindow & w,float scrx,float scry);
        static const int Margin=15;
    protected:
        int m_offset;
        QColor m_handle_color;
        QColor m_line_color;
        bool m_shadow;
        QColor m_shadow_color;
};

#endif // GFOOBAR_H
