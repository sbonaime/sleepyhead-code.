/********************************************************************
 gLayer Headers
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*********************************************************************/

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
    gLayer(MachineCode code=MC_UNKNOWN,QString title="");
    virtual ~gLayer();
    virtual void Plot(gGraphWindow & w,float scrx,float scry)=0;
    vector<QColor> color;

    virtual void SetDay(Day * d);
    virtual void SetCode(MachineCode c) { m_code=c; }
    virtual qint64 Minx() { if (m_day) return m_day->first(); return m_minx; }
    virtual qint64 Maxx() { if (m_day) return m_day->last(); return m_maxx; }
    virtual EventDataType Miny() { return m_miny; }
    virtual EventDataType Maxy() { return m_maxy; }

    virtual void setVisible(bool v) { m_visible=v; }
    virtual bool isVisible() { return m_visible; }
    virtual bool isEmpty();
    inline const MachineCode & code() { return m_code; }
protected:
    bool m_visible;
    bool m_movable;

    qint64 m_minx,m_maxx;
    EventDataType m_miny,m_maxy;
    Day *m_day;
    MachineCode m_code;
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
    vector<gLayer *> layers;
};

#endif // GRAPHLAYER_H
