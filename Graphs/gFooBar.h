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
        gFooBar(int offset=10,QColor color1=QColor("orange"),QColor color2=QColor("dark grey"),bool funkbar=false);
        virtual ~gFooBar();
        virtual void Plot(gGraphWindow & w,float scrx,float scry);
        static const int Margin=15;
    protected:
        bool m_funkbar;
        int m_offset;
};

#endif // GFOOBAR_H
