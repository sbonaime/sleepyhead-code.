/*
 gLayer Headers
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

#ifndef GRAPHLAYER_H
#define GRAPHLAYER_H

#include <QString>
#include "SleepLib/day.h"
#include "graphwindow.h"

enum FlagType { FT_Bar, FT_Dot, FT_Span };

class gGraphWindow;
class gLayer
{
public:
    gLayer(ChannelID code=EmptyChannel,QString title="");
    virtual ~gLayer();
    virtual void Plot(gGraphWindow & w,float scrx,float scry)=0;
    //QVector<QColor> color;

    virtual void SetDay(Day * d);
    virtual void SetCode(ChannelID c) { m_code=c; }
    virtual qint64 Minx() { if (m_day) return m_day->first(); return m_minx; }
    virtual qint64 Maxx() { if (m_day) return m_day->last(); return m_maxx; }
    virtual EventDataType Miny() { return m_miny; }
    virtual EventDataType Maxy() { return m_maxy; }
    virtual void setMinY(EventDataType val) { m_miny=val; }
    virtual void setMaxY(EventDataType val) { m_maxy=val; }
    virtual void setMinX(qint64 val) { m_minx=val; }
    virtual void setMaxX(qint64 val) { m_maxx=val; }
    virtual void setVisible(bool v) { m_visible=v; }
    virtual bool isVisible() { return m_visible; }
    virtual bool isEmpty();
    inline const ChannelID & code() { return m_code; }

protected:
    Day *m_day;
    bool m_visible;
    bool m_movable;
    qint64 m_minx,m_maxx;
    EventDataType m_miny,m_maxy;
    ChannelID m_code;
    QString m_title;
};

class gLayerGroup:public gLayer
{
public:
    gLayerGroup();
    virtual ~gLayerGroup();
    virtual void AddLayer(gLayer *l);

    virtual qint64 Minx();
    virtual qint64 Maxx();
    virtual EventDataType Miny();
    virtual EventDataType Maxy();
    virtual bool isEmpty();
    virtual void SetDay(Day * d);

protected:
    QVector<gLayer *> layers;
};

#endif // GRAPHLAYER_H
