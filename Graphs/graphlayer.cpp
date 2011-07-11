/********************************************************************
 gLayer Implementation
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*********************************************************************/

#include "graphlayer.h"

gLayer::gLayer(gPointData *d,QString title)
:m_title(title),data(d)
{
    if (data) {
        data->AddLayer(this);
    }
    m_visible = true;
    m_movable = false;
    color.push_back(QColor("red"));
    color.push_back(QColor("green"));
}

gLayer::~gLayer()
{

}

//void gLayer::Plot(gGraphWindow & w,float scrx,float scry)
//{
//}

void gLayer::DataChanged(gGraphData *src)
{
    for (list<gGraphWindow *>::iterator i=m_graph.begin();i!=m_graph.end();i++) {
        if (src) {
            (*i)->DataChanged(this);
        } else {
            (*i)->DataChanged(NULL);
        }
    }

}
// Notify signal sent from gGraphData.. pass on to the graph so it can que a refresh and update stuff.

void gLayer::SetData(gPointData * gd) { data=gd; };
gPointData * gLayer::GetData() { return data; };
double gLayer::MinX() { if (data) return data->MinX(); return 0;};
double gLayer::MaxX() { if (data) return data->MaxX(); return 0;};
double gLayer::MinY() { if (data) return data->MinY(); return 0;};
double gLayer::MaxY() { if (data) return data->MaxY(); return 0;};

double gLayer::RealMinX() { if (data) return data->RealMinX(); return 0;};
double gLayer::RealMaxX() { if (data) return data->RealMaxX(); return 0;};
double gLayer::RealMinY() { if (data) return data->RealMinY(); return 0;};
double gLayer::RealMaxY() { if (data) return data->RealMaxY(); return 0;};

void gLayer::SetMinX(double v) { if (data) data->SetMinX(v); };
void gLayer::SetMaxX(double v) { if (data) data->SetMaxX(v); };
void gLayer::SetMinY(double v) { if (data) data->SetMinY(v); };
void gLayer::SetMaxY(double v) { if (data) data->SetMaxY(v); };

void gLayer::NotifyGraphWindow(gGraphWindow *g) { m_graph.push_back(g); };

bool gLayer::isEmpty() { if (!data) return false; return data->isEmpty(); };

gLayerGroup::gLayerGroup()
{
}
gLayerGroup::~gLayerGroup()
{
}
//void gLayerGroup::DataChanged(gGraphData *src);
void gLayerGroup::NotifyGraphWindow(gGraphWindow *g)
{
    for (unsigned i=0;i<layers.size();i++) {
        layers[i]->NotifyGraphWindow(g);
    }
}

void gLayerGroup::AddLayer(gLayer *l)
{
    layers.push_back(l);
}

double gLayerGroup::MinX()
{
    bool first=true;
    double m=0;
    for (unsigned i=0;i<layers.size();i++)  {
        if (first) {
            m=layers[i]->MinX();
            first=false;
        } else
        if (m>layers[i]->MinX()) m=layers[i]->MinX();
    }
    return m;
}
double gLayerGroup::MaxX()
{
    bool first=true;
    double m=0;
    for (unsigned i=0;i<layers.size();i++)  {
        if (first) {
            m=layers[i]->MaxX();
            first=false;
        } else
        if (m<layers[i]->MaxX()) m=layers[i]->MaxX();
    }
    return m;
}
double gLayerGroup::MinY()
{
}
double gLayerGroup::MaxY()
{
}

double gLayerGroup::RealMinX()
{
    bool first=true;
    double m=0;
    for (unsigned i=0;i<layers.size();i++)  {
        if (first) {
            m=layers[i]->RealMinX();
            first=false;
        } else
        if (m>layers[i]->RealMinX()) m=layers[i]->RealMinX();
    }
    return m;
}
double gLayerGroup::RealMaxX()
{
    bool first=true;
    double m=0;
    for (unsigned i=0;i<layers.size();i++)  {
        if (first) {
            m=layers[i]->RealMaxX();
            first=false;
        } else
        if (m>layers[i]->RealMaxX()) m=layers[i]->RealMaxX();
    }
    return m;
}
double gLayerGroup::RealMinY()
{
    return 0;
}
double gLayerGroup::RealMaxY()
{
    return 0;
}

void gLayerGroup::SetMinX(double v)
{
    for (unsigned i=0;i<layers.size();i++)
        layers[i]->SetMinX(v);
}
void gLayerGroup::SetMaxX(double v)
{
    for (unsigned i=0;i<layers.size();i++)
        layers[i]->SetMaxX(v);
}
void gLayerGroup::SetMinY(double v)
{
    for (unsigned i=0;i<layers.size();i++)
        layers[i]->SetMinY(v);
}
void gLayerGroup::SetMaxY(double v)
{
    for (unsigned i=0;i<layers.size();i++)
        layers[i]->SetMaxY(v);
}
