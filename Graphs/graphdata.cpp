/********************************************************************
 gGraphData Implementation
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*********************************************************************/

#include "graphdata.h"
/*
gGraphData::gGraphData(int mp,gDataType t)
:vc(0),type(t),max_points(mp)
{
    m_ready=false;
    force_min_y=force_max_y=0;
}
gGraphData::~gGraphData()
{
}
void gGraphData::Update(Day *day)
{
    Reload(day);

    for (list<gLayer *>::iterator i=notify_layers.begin();i!=notify_layers.end();i++) {
        gGraphData *g=this;
        if (!day) g=NULL;
        (*i)->DataChanged(g);
    }
}
void gGraphData::AddLayer(gLayer *g)
{
    notify_layers.push_back(g);
}
bool gGraphData::isEmpty()
{
    bool b=((vc==1) && (np[0]==0)) || vc==0;
    return b;
}

gPointData::gPointData(int mp)
:gGraphData(mp,gDT_Point)
{
}
gPointData::~gPointData()
{
    for (vector<QPointD *>::iterator i=point.begin();i!=point.end();i++)
        delete [] (*i);
}
void gPointData::AddSegment(int max_points)
{
    maxsize.push_back(max_points);
    np.push_back(0);
    QPointD *p=new QPointD [max_points];
    point.push_back(p);
}
double gPointData::CalcAverage()
{
    double x,val=0;
    int cnt=0;
    for (int i=0;i<np[0];i++) {
        x=point[0][i].x();
        if ((x<min_x) || (x>max_x)) continue;
        val+=point[0][i].y();
        cnt++;
    }
    if (!cnt) return 0;
    val/=cnt;
    return val;
}
double gPointData::CalcMinY()
{
    double x,val=0;
    double min;
    bool first=true;
    for (int i=0;i<np[0];i++) {
        x=point[0][i].x();
        if ((x<min_x) || (x>max_x)) continue;
        val=point[0][i].y();
        if (first) {
            min=val;
            first=false;
        } else
        if (min>val) min=val;
    }
    return min;
}
double gPointData::CalcMaxY()
{
    double x,val=0;
    double max;
    bool first=true;
    for (int i=0;i<np[0];i++) {
        x=point[0][i].x();
        if ((x<min_x) || (x>max_x)) continue;
        val=point[0][i].y();
        if (first) {
            max=val;
            first=false;
        } else
        if (max<val) max=val;
    }
    return max;
}
*/
