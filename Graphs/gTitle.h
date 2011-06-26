/********************************************************************
 gTitle (Graph Title) Header
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*********************************************************************/

#ifndef GTITLE_H
#define GTITLE_H

#include "graphlayer.h"

class gTitle:public gLayer
{
    public:
        gTitle(const QString & _title,QColor color=QColor("black"),QFont font=QFont());
        virtual ~gTitle();
        virtual void Plot(gGraphWindow & w,float scrx,float scry);
        static const int Margin=20;

    protected:
        QString m_title;
        QColor m_color;
        QFont  m_font;
};

#endif // GTITLE_H
