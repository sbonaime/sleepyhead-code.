/********************************************************************
 gCandleStick Graph Header
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*********************************************************************/

#ifndef GCANDLESTICK_H
#define GCANDLESTICK_H

#include "graphlayer.h"

class gCandleStick:public gLayer
{
    public:
        gCandleStick(Qt::Orientation o=Qt::Horizontal);
        virtual ~gCandleStick();
        virtual void SetDay(Day *d);

        virtual void Plot(gGraphWindow & w,float scrx,float scry);
        void AddSlice(MachineCode code,QColor color,QString name="");

    protected:
        Qt::Orientation m_orientation;
        map<MachineCode,int> m_counts;
        map<MachineCode,QString> m_names;
        map<MachineCode,QColor> m_colors;
        int m_total;
};


#endif // GCANDLESTICK_H
