/*
 Custom CPAP/Oximetry Calculations Header
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

#include <cmath>

#include "calcs.h"
#include "profiles.h"

extern double round(double number);

bool SearchApnea(Session *session, qint64 time, qint64 dist=15000)
{
    if (session->SearchEvent(CPAP_Obstructive,time,dist)) return true;
    if (session->SearchEvent(CPAP_Apnea,time,dist)) return true;
    if (session->SearchEvent(CPAP_ClearAirway,time,dist)) return true;
    if (session->SearchEvent(CPAP_Hypopnea,time,dist)) return true;

    return false;
}

// Support function for calcRespRate()
int filterFlow(Session *session, EventList *in, EventList *out, EventList *tv, EventList *mv, double rate)
{

    int size=in->count();
    EventDataType *stage1=new EventDataType [size];
    EventDataType *stage2=new EventDataType [size];

    QVector<EventDataType> med;
    med.reserve(8);

    EventDataType r,tmp;
    int cnt;

    EventDataType c;
    //double avg;
    int i;

    // Extra median filter stage
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
    stage2[0]=in->data(0);
    stage2[1]=in->data(1);
    stage2[2]=in->data(2);

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


//    float weight=0.6;
//    stage2[0]=in->data(0);
//    stage1[0]=stage2[0];
//    for (int i=1;i<size;i++) {
//        //stage2[i]=in->data(i);
//        stage1[i]=weight*stage2[i]+(1.0-weight)*stage1[i-1];
//    }

    qint64 time=in->first();
    qint64 u1=0,u2=0,len,l1=0,l2=0;
    int z1=0,z2=0;
    EventDataType lastc=0,thresh=-1;
    QVector<int> breaths;
    QVector<EventDataType> TV;
    QVector<qint64> breaths_start;
    QVector<qint64> breaths_min_peak;
    QVector<EventDataType> breaths_min;
    QVector<qint64> breaths_max_peak;
    QVector<EventDataType> breaths_max;

//    const int ringsize=256;
//    EventDataType ringmax[ringsize]={0};
//    EventDataType ringmin[ringsize]={0};
//    qint64 ringtime[ringsize]={0};
//    int rpos=0;
    EventDataType min=0,max=0;
    qint64 peakmin=0, peakmax=0;
    for (i=0;i<size;i++) {
        c=stage2[i];

        if (c>thresh) {
            // Crossing the zero line, going up
            if (lastc<=thresh) {
                u2=u1;
                u1=time;
                if (u2>0) {
                    z2=i;
                    len=qAbs(u2-u1);
                    if (tv) { // Tidal Volume Calculations
                        EventDataType t=0;
                        // looking at only half the waveform
                        for (int g=z1;g<z2;g++) {
                            tmp=-stage2[g];
                            t+=tmp;
                        }
                        TV.push_back(t);
                    }
                    // keep zero crossings points
                    breaths_start.push_back(time);
                    breaths.push_back(len);

                    // keep previously calculated negative peak
                    breaths_min_peak.push_back(peakmin);
                    breaths_min.push_back(min);
                    max=0;
                }
            } else {
                // Find the positive peak
                if (c>=max) {
                    max=c;
                    peakmax=time+rate;
                }
            }
        } else {
            // Crossing the zero line, going down
            if (lastc>thresh) {
                l2=l1;
                l1=time;
                if (l2>0) {
                    z1=i;
                    len=qAbs(l2-l1);
                    if (tv) {
                        // Average the other half of the breath to increase accuracy.
                        EventDataType t=0;
                        for (int g=z2;g<z1;g++) {
                            tmp=stage2[g];
                            t+=tmp;
                        }
                        int ts=TV.size()-2;
                        if (ts>=0) {
                          //  TV[ts]=(TV[ts]+t)/2.0;
                        }
                    }
                    // keep previously calculated positive peak
                    breaths_max_peak.push_back(peakmax);
                    breaths_max.push_back(max);
                    min=0;

                }
            } else {
                // Find the negative peak
                if (c<=min) {
                    min=c;
                    peakmin=time;
                }

            }

        }
        lastc=c;
        time+=rate;
    }
    if (!breaths.size()) {
        return 0;
    }
    double avgmax=0;
    for (int i=0;i<breaths_max.size();i++)  {
        max=breaths_max[i];
        avgmax+=max;
    }
    avgmax/=EventDataType(breaths_max.size());

    double avgmin=0;
    for (int i=0;i<breaths_min.size();i++)  {
        min=breaths_min[i];
        avgmin+=min;
    }
    avgmin/=EventDataType(breaths_min.size());

    QVector<qint64> goodb;
    for (int i=0;i<breaths_max.size();i++)  {
        max=breaths_max[i];
        time=breaths_max_peak[i];

        if (max > avgmax*0.2) {
            goodb.push_back(time);
        }
    }
    for (int i=0;i<breaths_min.size();i++)  {
        min=breaths_min[i];
        time=breaths_min_peak[i];
        if (min < avgmin*0.2) {
            goodb.push_back(time);
        }
    }
    EventList *uf=NULL;

    qSort(goodb);
    for (int  i=1;i<goodb.size();i++) {
        qint64 len=qAbs(goodb[i]-goodb[i-1]);
        if (len>=10000) {
            time=goodb[i-1]+len/2;
            if (!SearchApnea(session,time)) {
                if (!uf) {
                    uf=new EventList(EVL_Event,1,0,0,0,0,true);
                    session->eventlist[CPAP_UserFlag1].push_back(uf);
                }
                uf->AddEvent(time,len/1000L,1);
            }
        }
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

    QVector<EventDataType> TV2;
    QVector<qint64> TV2_start;

    int fir=0;//,fir2=0;
    EventDataType T,L;
    bool done=false;
    do {
        br=0;
        bool first=true;
        bool cont=false;
        T=0;
        L=0;
        for (int i=fir;i<breaths.size();i++) {
            t=breaths_start[i];
            l=breaths[i];
            if (t+l < t1) continue;
            if (t > t2) break;

            if (first) {
                first=false;
                fir=i;
            }
            //q=1; // 22:28pm
            if (t<t1) {
                // move to start of next breath
                i++;
                if (i>=breaths.size()) {
                    done=true;
                    break;
                } else {
                    t1=breaths_start[i];
                    t2=t1+window;
                    fir=i;
                    cont=true;
                    break;
                }
                //q=(t+l)-t1;
                //br+=(1.0/double(l))*double(q);

            } else if (t+l>t2) {
                q=t2-t;
                br+=(1.0/double(l))*double(q);
                continue;
            } else
            br+=1.0;

            T+=TV[i]/2.0;
            L+=l/1000.0;
        }
        if (cont) continue;
        breaths2.push_back(br);
        breaths2_start.push_back(t1+window/2);
        //TV2_start.push_back(t2);
        TV2.push_back(T);
        //out->AddEvent(t,br);
        //stage2[z++]=br;

        t1+=window/2.0;
        t2+=window/2.0;
    } while (t2<in->last() && !done);

    for (int i=2;i<breaths2.size()-2;i++) {
        t=breaths2_start[i];
        med.clear();
        for (int j=0;j<5;j++)  {
            med.push_back(breaths2[i+j-2]);
        }
        qSort(med);
        br=med[2];
        if (out) out->AddEvent(t,br);

        //t=TV2_start[i];
        med.clear();
        for (int j=0;j<5;j++)  {
            med.push_back(TV2[i+j-2]);
        }
        qSort(med);
        tmp=med[3];

        if (tv) tv->AddEvent(t,tmp);

        if (mv) mv->AddEvent(t,(tmp*br)/1000.0);
    }

    delete [] stage2;
    delete [] stage1;

    return out->count();
}

// Generate RespiratoryRate graph
int calcRespRate(Session *session)
{
    if (session->machine()->GetType()!=MT_CPAP) return 0;
    if (session->machine()->GetClass()!=STR_MACH_PRS1) return 0;
    if (!session->eventlist.contains(CPAP_FlowRate))
        return 0; //need flow waveform

    if (session->eventlist.contains(CPAP_RespRate))
        return 0; // already exists?

    EventList *flow, *rr=NULL,  *tv=NULL, *mv=NULL;

    if (!session->eventlist.contains(CPAP_TidalVolume)) {
        tv=new EventList(EVL_Event);
    } else tv=NULL;
    if (!session->eventlist.contains(CPAP_MinuteVent)) {
        mv=new EventList(EVL_Event);
    } else mv=NULL;
    if (!session->eventlist.contains(CPAP_RespRate)) {
        rr=new EventList(EVL_Event);
    } else rr=NULL;

    if (!rr && !tv && !mv) return 0; // don't bother, but flagging won't run either..
    if (rr) session->eventlist[CPAP_RespRate].push_back(rr);
    if (tv) session->eventlist[CPAP_TidalVolume].push_back(tv);
    if (mv) session->eventlist[CPAP_MinuteVent].push_back(mv);

    int cnt=0;
    for (int ws=0; ws < session->eventlist[CPAP_FlowRate].size(); ws++) {
        flow=session->eventlist[CPAP_FlowRate][ws];
        if (flow->count() > 5) {
            cnt+=filterFlow(session, flow,rr,tv,mv,flow->rate());
        }
    }
    return cnt;
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

int calcAHIGraph(Session *session)
{
    const qint64 window_size=3600000L;
    const qint64 window_step=30000; // 30 second windows

    if (session->machine()->GetType()!=MT_CPAP) return 0;
    if (session->eventlist.contains(CPAP_AHI)) return 0; // abort if already there

    if (!session->channelExists(CPAP_Obstructive) &&
            !session->channelExists(CPAP_Hypopnea) &&
            !session->channelExists(CPAP_Apnea) &&
            !session->channelExists(CPAP_ClearAirway)) return 0;

    qint64 first=session->first(),
           last=session->last(),
           f;

    EventList *AHI=new EventList(EVL_Event);
    session->eventlist[CPAP_AHI].push_back(AHI);

    EventDataType ahi;

    qint64 ti;
    double avg=0;
    int cnt=0;
    for (ti=first;ti<last;ti+=window_step) {
        f=ti-window_size;
        ahi=calcAHI(session,f,ti);
        if  (ti>=last) {
            AHI->AddEvent(last,ahi);
            avg+=ahi;
            cnt++;
            break;
        }
        AHI->AddEvent(ti,ahi);
        ti+=window_step;
    }
    AHI->AddEvent(last,0);
    if (!cnt) avg=0; else avg/=double(cnt);
    session->setAvg(CPAP_AHI,avg);

    return AHI->count();
}

int calcLeaks(Session *session)
{
    if (session->machine()->GetType()!=MT_CPAP) return 0;
    if (session->eventlist.contains(CPAP_Leak)) return 0; // abort if already there
    if (!session->eventlist.contains(CPAP_LeakTotal)) return 0; // can't calculate without this..

    if (session->settings[CPAP_Mode].toInt()>MODE_APAP) return 0; // Don't bother calculating when in APAP mode

    const qint64 winsize=3600000; // 5 minute window

    //qint64 first=session->first(), last=session->last(), f;

    EventList *leak=new EventList(EVL_Event);
    session->eventlist[CPAP_Leak].push_back(leak);

    const int rbsize=128;
    EventDataType rbuf[rbsize],tmp,median;
    qint64 rtime[rbsize],ti;
    int rpos=0;
    int tcnt=0;
    QVector<EventDataType> med;

    for (int i=0;i<session->eventlist[CPAP_LeakTotal].size();i++) {
        EventList & el=*session->eventlist[CPAP_LeakTotal][i];
        for (unsigned j=0;j<el.count();j++) {
            tmp=el.data(j);
            ti=el.time(j);
            rbuf[rpos]=tmp;
            rtime[rpos]=ti;
            tcnt++;
            rpos++;

            int rcnt;
            if (tcnt<rbsize) rcnt=tcnt; else rcnt=rbsize;

            med.clear();
            for (int k=0;k<rcnt;k++) {
                if (rtime[k] > ti-winsize) // if fits in time window, add to the list
                    med.push_back(rbuf[k]);
            }
            qSort(med);

            int idx=float(med.size() * 0.0);
            if (idx>=med.size()) idx--;
            median=tmp-med[idx];
            if (median<0) median=0;
            leak->AddEvent(ti,median);

            rpos=rpos % rbsize;
        }
    }
    return leak->count();
}


int calcPulseChange(Session *session)
{
    if (session->eventlist.contains(OXI_PulseChange)) return 0;

    QHash<ChannelID,QVector<EventList *> >::iterator it=session->eventlist.find(OXI_Pulse);
    if (it==session->eventlist.end()) return 0;

    EventDataType val,val2,change,tmp;
    qint64 time,time2;
    qint64 window=PROFILE.oxi->pulseChangeDuration();
    window*=1000;

    change=PROFILE.oxi->pulseChangeBPM();

    EventList *pc=new EventList(EVL_Event,1,0,0,0,0,true);
    pc->setFirst(session->first(OXI_Pulse));
    qint64 lastt;
    EventDataType lv=0;
    int li=0;

    int max;
    for (int e=0;e<it.value().size();e++) {
        EventList & el=*(it.value()[e]);

        for (unsigned i=0;i<el.count();i++) {
            val=el.data(i);
            time=el.time(i);


            lastt=0;
            lv=change;
            max=0;

            for (unsigned j=i+1;j<el.count();j++) { // scan ahead in the window
                time2=el.time(j);
                if (time2 > time+window) break;
                val2=el.data(j);
                tmp=qAbs(val2-val);
                if (tmp > lv) {
                    lastt=time2;
                    if (tmp>max) max=tmp;
                    //lv=tmp;
                    li=j;
                }
            }
            if (lastt>0) {
                qint64 len=(lastt-time)/1000.0;
                pc->AddEvent(lastt,len,tmp);
                i=li;

            }
        }
    }
    if (pc->count()==0) {
        delete pc;
        return 0;
    }
    session->eventlist[OXI_PulseChange].push_back(pc);
    session->setMin(OXI_PulseChange,pc->Min());
    session->setMax(OXI_PulseChange,pc->Max());
    session->setCount(OXI_PulseChange,pc->count());
    session->setFirst(OXI_PulseChange,pc->first());
    session->setLast(OXI_PulseChange,pc->last());
    return pc->count();
}


int calcSPO2Drop(Session *session)
{
    if (session->eventlist.contains(OXI_SPO2Drop)) return 0;

    QHash<ChannelID,QVector<EventList *> >::iterator it=session->eventlist.find(OXI_SPO2);
    if (it==session->eventlist.end()) return 0;

    EventDataType val,val2,change,tmp;
    qint64 time,time2;
    qint64 window=PROFILE.oxi->spO2DropDuration();
    window*=1000;
    change=PROFILE.oxi->spO2DropPercentage();

    EventList *pc=new EventList(EVL_Event,1,0,0,0,0,true);
    qint64 lastt;
    EventDataType lv=0;
    int li=0;

    // Fix me.. Time scale varies.
    //const unsigned ringsize=30;
    //EventDataType ring[ringsize]={0};
    //qint64 rtime[ringsize]={0};
    //int rp=0;
    int min;
    int cnt=0;
    tmp=0;

    qint64 start=0;

    // Calculate median baseline
    QList<EventDataType> med;
    for (int e=0;e<it.value().size();e++) {
        EventList & el=*(it.value()[e]);
        for (unsigned i=0;i<el.count();i++) {
            val=el.data(i);
            time=el.time(i);
            if (val>0) med.push_back(val);
            if (!start) start=time;
            if (time > start+3600000) break; // just look at the first hour
            tmp+=val;
            cnt++;
        }
    }
    qSort(med);
    int midx=float(med.size())*0.90;
    if (midx>med.size()-1) midx=med.size()-1;
    EventDataType baseline=med[midx];
    session->settings[OXI_SPO2Drop]=baseline;
    //EventDataType baseline=round(tmp/EventDataType(cnt));
    EventDataType current;
    qDebug() << "Calculated baseline" << baseline;

    for (int e=0;e<it.value().size();e++) {
        EventList & el=*(it.value()[e]);
        for (unsigned i=0;i<el.count();i++) {
            current=el.data(i);
            if (!current) continue;
            time=el.time(i);
            /*ring[rp]=val;
            rtime[rp]=time;
            rp++;
            rp=rp % ringsize;
            if (i<ringsize)  {
                for (unsigned j=i;j<ringsize;j++) {
                    ring[j]=val;
                    rtime[j]=0;
                }
            }
            tmp=0;
            cnt=0;
            for (unsigned j=0;j<ringsize;j++) {
                if (rtime[j] > time-300000) {  // only look at recent entries..
                    tmp+=ring[j];
                    cnt++;
                }
            }
            if (!cnt) {
                unsigned j=abs((rp-1) % ringsize);
                tmp=(ring[j]+val)/2;
            } else tmp/=EventDataType(cnt); */

            val=baseline;
            lastt=0;
            lv=val;


            min=val;
            for (unsigned j=i;j<el.count();j++) { // scan ahead in the window
                time2=el.time(j);
                //if (time2 > time+window) break;
                val2=el.data(j);

                if (val2 > baseline-change) break;
                lastt=time2;
                li=j+1;
            }
            if (lastt>0) {
                qint64 len=(lastt-time);
                if (len>=window) {
                    pc->AddEvent(lastt,len/1000,val-min);
                    i=li;
                }
            }
        }
    }
    if (pc->count()==0) {
        delete pc;
        return 0;
    }
    session->eventlist[OXI_SPO2Drop].push_back(pc);
    session->setMin(OXI_SPO2Drop,pc->Min());
    session->setMax(OXI_SPO2Drop,pc->Max());
    session->setCount(OXI_SPO2Drop,pc->count());
    session->setFirst(OXI_SPO2Drop,pc->first());
    session->setLast(OXI_SPO2Drop,pc->last());
    return pc->count();
}
