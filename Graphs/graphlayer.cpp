/*
 gLayer Implementation
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

#include "graphlayer.h"

gLayer::gLayer(ChannelID code,QString title)
:m_code(code),m_title(title)
{
    m_visible = true;
    m_movable = false;
    //color.push_back(QColor("red"));
    //color.push_back(QColor("green"));
    m_day=NULL;
    m_miny=m_maxy=0;
    m_minx=m_maxx=0;
}

gLayer::~gLayer()
{

}

void gLayer::SetDay(Day * d)
{
    m_day=d;
    if (!d) return;
    m_minx=d->first(m_code);
    m_maxx=d->last(m_code);
    m_miny=d->min(m_code);
    m_maxy=d->max(m_code);
}


bool gLayer::isEmpty()
{
    if (m_day && (m_day->count(m_code)!=0))
        return false;
    return true;
}

gLayerGroup::gLayerGroup():gLayer(EmptyChannel)
{
}
gLayerGroup::~gLayerGroup()
{
}
bool gLayerGroup::isEmpty()
{
    bool empty=true;
    for (int i=0;i<layers.size();i++) {
        if (layers[i]->isEmpty()) {
            empty=false;
            break;
        }
    }
    return empty;
}
void gLayerGroup::SetDay(Day * d)
{
    for (int i=0;i<layers.size();i++) {
         layers[i]->SetDay(d);
    }
    m_day=d;
}

void gLayerGroup::AddLayer(gLayer *l)
{
    layers.push_back(l);
}

qint64 gLayerGroup::Minx()
{
    bool first=true;
    qint64 m=0,t;
    for (int i=0;i<layers.size();i++)  {
        t=layers[i]->Minx();
        if (!t) continue;
        if (first) {
            m=t;
            first=false;
        } else
        if (m>t) m=t;
    }
    return m;
}
qint64 gLayerGroup::Maxx()
{
    bool first=true;
    qint64 m=0,t;
    for (int i=0;i<layers.size();i++)  {
        t=layers[i]->Maxx();
        if (!t) continue;
        if (first) {
            m=t;
            first=false;
        } else
        if (m<t) m=t;
    }
    return m;
}
EventDataType gLayerGroup::Miny()
{
    bool first=true;
    EventDataType m=0,t;
    for (int i=0;i<layers.size();i++)  {
        t=layers[i]->Miny();
        if (t==layers[i]->Minx()) continue;
        if (first) {
            m=t;
            first=false;
        } else {
            if (m>t) m=t;
        }
    }
    return m;
}
EventDataType gLayerGroup::Maxy()
{
    bool first=true;
    EventDataType m=0,t;
    for (int i=0;i<layers.size();i++)  {
        t=layers[i]->Maxy();
        if (t==layers[i]->Miny()) continue;
        if (first) {
            m=t;
            first=false;
        } else
        if (m<t) m=t;
    }
    return m;
}

