/*
 gLineOverlayBar Header
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

#ifndef GLINEOVERLAY_H
#define GLINEOVERLAY_H

#include "gGraphView.h"

class gLineOverlayBar:public Layer
{
    public:
        gLineOverlayBar(ChannelID code,QColor col,QString _label="",FlagType _flt=FT_Bar);
        virtual ~gLineOverlayBar();

        virtual void paint(gGraph & w,int left, int top, int width, int height);
        virtual EventDataType Miny() { return 0; }
        virtual EventDataType Maxy() { return 0; }
        virtual bool isEmpty() { return true; }
        int count() { return m_count; }
    protected:
        QColor m_flag_color;
        QString m_label;
        FlagType m_flt;
        int m_count;

        GLShortBuffer *points,*quads, *lines;
};

class gLineOverlaySummary:public Layer
{
    public:
        gLineOverlaySummary(QString text, int x, int y);
        virtual ~gLineOverlaySummary();

        virtual void paint(gGraph & w,int left, int top, int width, int height);
        virtual EventDataType Miny() { return 0; }
        virtual EventDataType Maxy() { return 0; }
        virtual bool isEmpty() { return true; }
        gLineOverlayBar *add(gLineOverlayBar *bar) { m_overlays.push_back(bar); return bar; }
    protected:
        QVector<gLineOverlayBar *> m_overlays;
        QString m_text;
        int m_x,m_y;
  };


#endif // GLINEOVERLAY_H
