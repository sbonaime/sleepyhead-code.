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
        gCandleStick(MachineCode code=MC_UNKNOWN,Qt::Orientation o=Qt::Horizontal);
        virtual ~gCandleStick();

        virtual void Plot(gGraphWindow & w,float scrx,float scry);
        void AddName(QString name) { m_names.push_back(name); };

    protected:
        Qt::Orientation m_orientation;
        vector<QString> m_names;

};


#endif // GCANDLESTICK_H
