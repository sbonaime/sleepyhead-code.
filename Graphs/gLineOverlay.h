/*
 gLineOverlayBar Header
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

#ifndef GLINEOVERLAY_H
#define GLINEOVERLAY_H

#include "graphlayer.h"

class gLineOverlayBar:public gLayer
{
    public:
        gLineOverlayBar(ChannelID code,QColor col=QColor("black"),QString _label="",FlagType _flt=FT_Bar);
        virtual ~gLineOverlayBar();

        virtual void Plot(gGraphWindow & w,float scrx,float scry);
        virtual EventDataType Miny() { return 0; }
        virtual EventDataType Maxy() { return 0; }
        virtual bool isEmpty() { return true; }
    protected:
        QString m_label;
        FlagType m_flt;
};

#endif // GLINEOVERLAY_H
