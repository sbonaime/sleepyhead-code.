/********************************************************************
 Custom graph data Implementations
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*********************************************************************/

#include <math.h>
#include "graphdata_custom.h"


WaveData::WaveData(MachineCode _code, int _size)
:gPointData(_size),code(_code)
{
}
WaveData::~WaveData()
{
}
void WaveData::Reload(Day *day)
{
    vc=0;
    if (!day) {
        m_ready=false;
        return;
    }
    min_x=day->first().toMSecsSinceEpoch()/86400000.0;
    max_x=day->last().toMSecsSinceEpoch()/86400000.0;
    if (max_x<min_x) {
        min_y=max_y=0;
        m_ready=false;
        return;
    }
    max_y=0;
    bool first=true;
    int chunk=0;
    for (vector<Session *>::iterator s=day->begin();s!=day->end(); s++) {
        //qDebug("Processing waveform chunk %i",chunk++);
        if ((*s)->waveforms.find(code)==(*s)->waveforms.end()) continue;
        for (vector<Waveform *>::iterator l=(*s)->waveforms[code].begin();l!=(*s)->waveforms[code].end();l++) {
            int ps=point.size();
            if (vc>=ps) {
                AddSegment(max_points); // TODO: Add size limit capabilities.
            }
            int t=0;

            Waveform *w=(*l);
            double st=w->start().toMSecsSinceEpoch()/86400000.0;
            double rate=(w->duration()/w->samples())/86400.0;
            qDebug("Waveform Chunk contains %i samples",w->samples());
            for (int i=0;i<w->samples();i++) {
                QPointD r(st,(*w)[i]);
                st+=rate;
                (point[vc][t++])=r;
                assert(t<max_points);
                if (first) {
                    max_y=min_y=r.y();
                    first=false;
                } else {
                    if (r.y()<min_y) min_y=r.y();
                    if (r.y()>max_y) max_y=r.y();
                }
            }
            np[vc]=t;
            vc++;
        }
    }
    min_y=floor(min_y);
    max_y=ceil(max_y);

    //double t1=MAX(fabs(min_y),fabs(max_y));
    // Get clever here..
    if (min_y<0) {
        if (max_y>128) {
            double j=MAX(max_y,fabs(min_y));
            min_y=-j;
            max_y=j;
        } else if (max_y>90) {
            max_y=120;
            min_y=-120;
        } else if (max_y>60) {
            min_y=-90;
            max_y=90;
        } else  {
            min_y=-60;
            max_y=60;
        }
    }

    if (force_min_y!=force_max_y) {
        min_y=force_min_y;
        max_y=force_max_y;
    }

    real_min_x=min_x;
    real_min_y=min_y;
    real_max_x=max_x;
    real_max_y=max_y;
    m_ready=true;
    //graph->Refresh(false);
}


EventData::EventData(MachineCode _code,int _field,int _size,bool _skipzero)
:gPointData(_size),code(_code),field(_field),skipzero(_skipzero)
{
}
EventData::~EventData()
{
}
void EventData::Reload(Day *day)
{
    vc=0;
    if (!day) {
        m_ready=false;
        return;
    }

    min_x=day->first().toMSecsSinceEpoch()/86400000.0;
    max_x=day->last().toMSecsSinceEpoch()/86400000.0;
    if (min_x>max_x) {
        int a=5;
        //assert(min_x<max_x);
    }
    min_y=max_y=0;
    int tt=0;
    bool first=true;
    EventDataType lastp=0;
    for (vector<Session *>::iterator s=day->begin();s!=day->end(); s++) {
        if ((*s)->events.find(code)==(*s)->events.end()) continue;
        if (vc>=(int)point.size()) {
            AddSegment(max_points);
        }

        int t=0;
        EventDataType p;
        for (vector<Event *>::iterator ev=(*s)->events[code].begin(); ev!=(*s)->events[code].end(); ev++) {
            p=(*(*ev))[field];
            if (((p!=0) && skipzero) || !skipzero) {
                QPointD r((*ev)->time().toMSecsSinceEpoch()/86400000.0,p);
                point[vc][t++]=r;
                assert(t<max_points);
                if (first) {
                    max_y=min_y=r.y();
                    //lastp=p;
                    first=false;
                } else {
                    if (r.y()<min_y) min_y=r.y();
                    if (r.y()>max_y) max_y=r.y();
                }
            } else {
                if ((p!=lastp) && (t>0)) { // There really should not be consecutive zeros.. just in case..
                    np[vc]=t;
                    tt+=t;
                    t=0;
                    vc++;
                    if (vc>=(int)point.size()) {
                        AddSegment(max_points);
                    }
                }
            }
            lastp=p;
        }
        np[vc]=t;
        if (t>0) {
            tt+=t;
            vc++;
        }

    }
    if (tt>0) {
        min_y=floor(min_y);
        max_y=ceil(max_y+1);
        if (min_y>1) min_y-=1;
    }

    if (force_min_y!=force_max_y) {
        min_y=force_min_y;
        max_y=force_max_y;
    }


    real_min_x=min_x;
    real_min_y=min_y;
    real_max_x=max_x;
    real_max_y=max_y;
    m_ready=true;

}


TAPData::TAPData(MachineCode _code)
:gPointData(256),code(_code)
{
    AddSegment(max_points);
}
TAPData::~TAPData()
{
}
void TAPData::Reload(Day *day)
{
    if (!day) {
        m_ready=false;
        return;
    }


    for (int i=0;i<max_slots;i++) pTime[i]=0;

    int cnt=0;

    bool first;
    QDateTime last;
    int lastval=0,val;

    int field=0;

    for (vector<Session *>::iterator s=day->begin();s!=day->end();s++) {
        if ((*s)->events.find(code)==(*s)->events.end()) continue;
        first=true;
        for (vector<Event *>::iterator e=(*s)->events[code].begin(); e!=(*s)->events[code].end(); e++) {
            Event & ev =(*(*e));
            val=ev[field]*10.0;
            assert(field<ev.fields());
            //if (field > ev.fields()) throw BoundsError();
            if (first) {
                first=false; // only bother setting lastval (below) this time.
            } else {
                double d=last.msecsTo(ev.time())/1000.0;

                assert(lastval<max_slots);
                if (lastval>max_slots) {
                    int i=0;
                  //  throw BoundsError();
                }
                pTime[lastval]+=d;
            }
            cnt++;
            last=ev.time();
            lastval=val;
        }
    }
    double TotalTime;
    for (int i=0; i<max_slots; i++) {
        TotalTime+=pTime[i];
    }

    int jj=0;
    for (int i=0; i<max_slots; i++) {
        if (pTime[i]>0) {
            point[0][jj].setX(i/10.0);
            point[0][jj].setY((100.0/TotalTime)*pTime[i]);
            jj++;
        }
    }
    np[0]=jj;
    //graph->Refresh();
    m_ready=true;
}

AHIData::AHIData()
:gPointData(256)
{
    AddSegment(max_points);
}
AHIData::~AHIData()
{
}
void AHIData::Reload(Day *day)
{
    if (!day) {
        m_ready=false;
        return;
    }
    point[0][0].setY(day->count(CPAP_Hypopnea)/day->hours());
    point[0][0].setX(point[0][0].y());
    point[0][1].setY(day->count(CPAP_Obstructive)/day->hours());
    point[0][1].setX(point[0][1].y());
    point[0][2].setY(day->count(CPAP_ClearAirway)/day->hours());
    point[0][2].setX(point[0][2].y());
    point[0][3].setY(day->count(CPAP_RERA)/day->hours());
    point[0][3].setX(point[0][3].y());
    point[0][4].setY(day->count(CPAP_FlowLimit)/day->hours());
    point[0][4].setX(point[0][4].y());
    point[0][5].setY((100.0/day->hours())*(day->sum(CPAP_CSR)/3600.0));
    point[0][5].setX(point[0][5].y());
    np[0]=6;
    m_ready=true;
    //REFRESH??
}

FlagData::FlagData(MachineCode _code,double _value,int _field,int _offset)
:gPointData(65536),code(_code),value(_value),field(_field),offset(_offset)
{
    AddSegment(max_points);
}
FlagData::~FlagData()
{
}
void FlagData::Reload(Day *day)
{
    if (!day) {
        m_ready=false;
        return;
    }
    int c=0;
    vc=0;
    double v1,v2;
    bool first;
    min_x=day->first().toMSecsSinceEpoch()/86400000.0;
    max_x=day->last().toMSecsSinceEpoch()/86400000.0;

    for (vector<Session *>::iterator s=day->begin();s!=day->end();s++) {
        if ((*s)->events.find(code)==(*s)->events.end()) continue;
        first=true;
        for (vector<Event *>::iterator e=(*s)->events[code].begin(); e!=(*s)->events[code].end(); e++) {
            Event & ev =(*(*e));
            v2=v1=ev.time().toMSecsSinceEpoch()/86400000.0;
            if (offset>=0)
                v1-=ev[offset]/86400.0;
            point[vc][c].setX(v1);
            point[vc][c].setY(v2);
            v1=point[vc][c].x();
            v2=point[vc][c].y();
            //point[vc][c].z=value;
            c++;
            assert(c<max_points);
            /*if (first) {
                min_y=v1;
                first=false;
            } else {

       if (v1<min_x) min_x=v1;
            }
            if (v2>max_x) max_x=v2; */
        }
    }
    min_y=-value;
    max_y=value;
    np[vc]=c;
    vc++;
    real_min_x=min_x;
    real_min_y=min_y;
    real_max_x=max_x;
    real_max_y=max_y;
    m_ready=true;
}

////////////////////////////////////////////////////////////////////////////////////////////
// HistoryData Implementation
////////////////////////////////////////////////////////////////////////////////////////////

HistoryData::HistoryData(Profile * _profile)
:gPointData(1024),profile(_profile)
{
    AddSegment(max_points);
    if (profile->LastDay().isValid()) {
        QDateTime tmp;
        tmp.setDate(profile->FirstDay());
        real_min_x=tmp.toMSecsSinceEpoch()/86400000.0;
        tmp.setDate(profile->LastDay());
        real_max_x=(tmp.toMSecsSinceEpoch()/86400000.0)+1;
    }
    real_min_y=real_max_y=0;
}
HistoryData::~HistoryData()
{
}
void HistoryData::ResetDateRange()
{
    if (profile->LastDay().isValid()) {
        QDateTime tmp;
        tmp.setDate(profile->FirstDay());
        real_min_x=tmp.toMSecsSinceEpoch()/86400000.0;
        tmp.setDate(profile->LastDay());
        real_max_x=(tmp.toMSecsSinceEpoch()/86400000.0)+1;
    }
 //   Reload(NULL);
}
double HistoryData::Calc(Day *day)
{
    return (day->summary_sum(CPAP_Obstructive) + day->summary_sum(CPAP_Hypopnea) + day->summary_sum(CPAP_ClearAirway)) / day->hours();
}

void HistoryData::Reload(Day *day)
{
    QDateTime date;
    vc=0;
    int i=0;
    bool first=true;
    bool done=false;
    double y,lasty=0;
    min_y=max_y=0;
    min_x=max_x=0;
    if (real_min_x<0) return;
    if (real_max_x<0) return;
    for (double x=floor(real_min_x);x<=ceil(real_max_x);x++) {
        date=QDateTime::fromMSecsSinceEpoch(x*86400000.0L);
        date.setTime(QTime(0,0,0));
        if (profile->daylist.find(date.date())==profile->daylist.end()) continue;

        y=0;
        int z=0;
        vector<Day *> & daylist=profile->daylist[date.date()];
        for (vector<Day *>::iterator dd=daylist.begin(); dd!=daylist.end(); dd++) { // average any multiple data sets
            Day *d=(*dd);
            if (d->machine_type()==MT_CPAP) {
                y=Calc(d);
                z++;
            }
        }
        if (!z) continue;
        if (z>1) y /= z;
        if (first) {
          //  max_x=min_x=x;
            lasty=max_y=min_y=y;
            first=false;
        }
        point[vc][i].setX(x);
        point[vc][i].setY(y);
        if (y>max_y) max_y=y;
        if (y<min_y) min_y=y;
        //if (x<min_x) min_x=x;
        //if (x>max_x) max_x=x;

        i++;
        if (i>max_points) {
            qWarning("max_points is not enough in HistoryData");
            done=true;
        }
        if (done) break;
        lasty=y;
    }
    np[vc]=i;
    vc++;
    min_x=real_min_x;
    max_x=real_max_x;

   // max_x+=1;
    //real_min_x=min_x;
    //real_max_x=max_x;
    if (force_min_y!=force_max_y) {
        min_y=force_min_y;
        max_y=force_max_y;
    } else {
        if (!((min_y==max_y) && (min_y==0))) {
            if (min_y>1) min_y-=1;
            max_y++;
        }
    }
    real_min_y=min_y;
    real_max_y=max_y;
    m_ready=true;
}
double HistoryData::GetAverage()
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
void HistoryData::SetDateRange(QDate start,QDate end)
{
    double x1=QDateTime(start).toMSecsSinceEpoch()/86400000.0; // -0.5;
    double x2=QDateTime(end.addDays(1)).toMSecsSinceEpoch()/86400000.0;
    if (x1 < real_min_x) x1=real_min_x;
    if (x2 > (real_max_x)) x2=(real_max_x);
    min_x=x1;
    max_x=x2;
    for (list<gLayer *>::iterator i=notify_layers.begin();i!=notify_layers.end();i++) {
        (*i)->DataChanged(this);
    }    // Do nothing else.. Callers responsibility to Refresh window.
}


HistoryCodeData::HistoryCodeData(Profile *_profile,MachineCode _code)
:HistoryData(_profile),code(_code)
{
}
HistoryCodeData::~HistoryCodeData()
{
}
double HistoryCodeData::Calc(Day *day)
{
    return day->summary_avg(code);
}

UsageHistoryData::UsageHistoryData(Profile *_profile,T_UHD _uhd)
:HistoryData(_profile),uhd(_uhd)
{
}
UsageHistoryData::~UsageHistoryData()
{
}
double UsageHistoryData::Calc(Day *day)
{
    double d;
    if (uhd==UHD_Bedtime) {
        d=day->first().time().hour();
        if (d<12) d+=24;
        d+=(day->first().time().minute()/60.0);
        d+=(day->first().time().second()/3600.0);
        return d;
    }
    else if (uhd==UHD_Waketime) {
        d=day->last().time().hour();
        d+=(day->last().time().minute()/60.0);
        d+=(day->last().time().second()/3600.0);
        return d;
    }
    else if (uhd==UHD_Hours) return day->hours();
    else
        return 0;
}

