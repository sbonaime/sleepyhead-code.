/********************************************************************
 gLayer Headers
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*********************************************************************/

#ifndef GRAPHLAYER_H
#define GRAPHLAYER_H

#include <QString>
#include "graphwindow.h"

#include "graphdata.h"

class gGraphWindow;
class gGraphData;
class gPointData;
class gLayer
{
public:
    gLayer(gPointData *g=NULL,QString title="");
    virtual ~gLayer();
    virtual void Plot(gGraphWindow & w,float scrx,float scry)=0;
    vector<QColor> color;

    virtual void SetData(gPointData * gd);;
    virtual gPointData * GetData();;

    virtual void DataChanged(gGraphData *src);

    virtual double MinX();
    virtual double MaxX();
    virtual double MinY();
    virtual double MaxY();

    virtual double RealMinX();
    virtual double RealMaxX();
    virtual double RealMinY();
    virtual double RealMaxY();

    virtual void SetMinX(double v);
    virtual void SetMaxX(double v);
    virtual void SetMinY(double v);
    virtual void SetMaxY(double v);


    void NotifyGraphWindow(gGraphWindow *g);
    void SetVisible(bool v) { m_visible=v; };
    bool IsVisible() { return m_visible; };

protected:
    bool m_visible;
    bool m_movable;
    QString m_title;
    gPointData *data;   // Data source
    list<gGraphWindow *> m_graph; // notify list of graphs that attach this layer.

};

#endif // GRAPHLAYER_H
