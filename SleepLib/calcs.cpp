/*
 Custom CPAP/Oximetry Calculations Header
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

#include "calcs.h"
#include "profiles.h"

Calculation::Calculation(ChannelID id,QString name)
    :m_id(id),m_name(name)
{
}

Calculation::~Calculation()
{
}

CalcRespRate::CalcRespRate(ChannelID id)
    :Calculation(id,"Resp. Rate")
{
}

// Generate RespiratoryRate graph
int CalcRespRate::calculate(Session *session)
{
    if (session->eventlist.contains(CPAP_RespRate)) return 0; // already exists?

    if (!session->eventlist.contains(CPAP_FlowRate)) return 0; //need flow waveform

    EventList *flow, *rr;
    int cnt=0;
    for (int ws=0; ws < session->eventlist[CPAP_FlowRate].size(); ws++) {
        flow=session->eventlist[CPAP_FlowRate][ws];
        if (flow->count() > 5) {
            rr=new EventList(EVL_Event);
            session->eventlist[CPAP_RespRate].push_back(rr);
            cnt+=filterFlow(flow,rr,flow->rate());
        }
    }
    return cnt;
}

int CalcRespRate::filterFlow(EventList *in, EventList *out, double rate)
{
    int size=in->count();
    EventDataType *stage1=new EventDataType [size];
    EventDataType *stage2=new EventDataType [size];

    QVector<EventDataType> med;
    med.reserve(8);

    EventDataType r;
    int cnt;

    EventDataType c;
    //double avg;
    int i;

    /*i=3;
    stage1[0]=in->data(0);
    stage1[1]=in->data(1);
    stage1[2]=in->data(2);
    for (;i<size-2;i++) {
        med.clear();
        for (quint32 k=0;k<5;k++) {
            med.push_back(in->data(i-2+k));
        }
        qSort(med);
        stage1[i]=med[3];
    }
    stage1[i]=in->data(i);
    i++;
    stage1[i]=in->data(i);
    i++;
    stage1[i]=in->data(i);
    */

    //i++;
    //stage1[i]=in->data(i);

    // Anti-Alias the flow waveform to get rid of jagged edges.
    stage2[0]=stage1[0];
    stage2[1]=stage1[1];
    stage2[2]=stage1[2];

    i=3;
    for (;i<size-3;i++) {
        cnt=0;
        r=0;
        for (quint32 k=0;k<7;k++) {
            //r+=stage1[i-3+k];
            r+=in->data(i-3+k);
            cnt++;
        }
        c=r/float(cnt);
        stage2[i]=c;
    }
    stage2[i]=in->data(i);
    i++;
    stage2[i]=in->data(i);
    i++;
    stage2[i]=in->data(i);
    //i++;
    //stage2[i]=in->data(i);

    float weight=0.6;
    //stage2[0]=in->data(0);
    stage1[0]=stage2[0];
    for (int i=1;i<size;i++) {
        //stage2[i]=in->data(i);
        stage1[i]=weight*stage2[i]+(1.0-weight)*stage1[i-1];
    }

    qint64 time=in->first();
    qint64 u1=0,u2=0,len,l1=0,l2=0;
    EventDataType lastc=0,thresh=0;
    QVector<int> breaths;
    QVector<qint64> breaths_start;

    for (i=0;i<size;i++) {
        c=stage1[i];
        if (c>thresh) {
            if (lastc<=thresh) {
                u2=u1;
                u1=time;
                if (u2>0) {
                    len=abs(u2-u1);
                    //if (len>1500) {
                        breaths_start.push_back(time);
                        breaths.push_back(len);
                    //}
                }
            }
        } else {
            if (lastc>thresh) {
                l2=l1;
                l1=time;
                if (l2>0) {
                    len=abs(l2-l1);
                    //if (len>1500) {
                    //    breaths2_start.push_back(time);
                    //  breaths2.push_back(len);
                    //}
                }
            }

        }
        lastc=c;
        time+=rate;
    }

    qint64 window=60000;
    qint64 t1=in->first()-window/2;
    qint64 t2=in->first()+window/2;
    qint64 t;
    EventDataType br,q;
    //int z=0;
    int l;

    QVector<int> breaths2;
    QVector<qint64> breaths2_start;

    int fir=0;
    do {
        br=0;
        bool first=true;
        bool cont=false;
        for (int i=fir;i<breaths.size();i++) {
            t=breaths_start[i];
            l=breaths[i];
            if (t+l < t1) continue;
            if (t > t2) break;

            if (first) {
                first=false;
                fir=i;
            }
            //q=1;
            if (t<t1) {
                // move to start of previous breath
                t1=breaths_start[++i];
                t2=t1+window;
                fir=i;
                cont=true;
                break;
                //q=(t+l)-t1;
                //br+=(1.0/double(l))*double(q);

            } else if (t+l>t2) {
                q=t2-t;
                br+=(1.0/double(l))*double(q);
                continue;
            } else
            br+=1.0;
        }
        if (cont) continue;
        breaths2.push_back(br);
        breaths2_start.push_back(t1+window/2);
        //out->AddEvent(t,br);
        //stage2[z++]=br;

        t1+=window/2.0;
        t2+=window/2.0;
    } while (t2<in->last());


    for (int i=1;i<breaths2.size()-2;i++) {
        t=breaths2_start[i];
        med.clear();
        for (int j=0;j<4;j++)  {
            med.push_back(breaths2[i+j-1]);
        }
        qSort(med);
        br=med[2];
        out->AddEvent(t,br);
    }

    delete [] stage2;
    delete [] stage1;

    return out->count();
}

EventDataType calcAHI(Session *session,qint64 start, qint64 end)
{
    double hours,ahi,cnt;
    if ((start==end) && (start==0)) {
        // much faster..
        hours=session->hours();
        cnt=session->count(CPAP_Obstructive)
                +session->count(CPAP_Hypopnea)
                +session->count(CPAP_ClearAirway)
                +session->count(CPAP_Apnea);

        ahi=cnt/hours;
    } else {
        hours=double(end-start)/3600000L;
        cnt=session->rangeCount(CPAP_Obstructive,start,end)
                +session->rangeCount(CPAP_Hypopnea,start,end)
                +session->rangeCount(CPAP_ClearAirway,start,end)
                +session->rangeCount(CPAP_Apnea,start,end);

        ahi=cnt/hours;
    }
    return ahi;
}

CalcAHIGraph::CalcAHIGraph(ChannelID id):
    Calculation(id,"AHI/hour")
{
}

int CalcAHIGraph::calculate(Session *session)
{
    if (session->eventlist.contains(CPAP_AHI)) return 0; // abort if already there

    const qint64 winsize=30000; // 30 second windows

    qint64 first=session->first(),
           last=session->last(),
           f;

    EventList *AHI=new EventList(EVL_Event);
    session->eventlist[CPAP_AHI].push_back(AHI);

    EventDataType ahi;

    for (qint64 ti=first;ti<=last;ti+=winsize) {
        f=ti-3600000L;
        ahi=calcAHI(session,f,ti);
        AHI->AddEvent(ti,ahi);
        ti+=winsize;
    }

    return AHI->count();
}
