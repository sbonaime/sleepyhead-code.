/*
 Custom CPAP/Oximetry Calculations Header
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

#include <cmath>
#include <algorithm>

#include "calcs.h"
#include "profiles.h"

extern double round(double number);

bool SearchApnea(Session *session, qint64 time, qint64 dist)
{
    if (session->SearchEvent(CPAP_Obstructive,time,dist)) return true;
    if (session->SearchEvent(CPAP_Apnea,time,dist)) return true;
    if (session->SearchEvent(CPAP_ClearAirway,time,dist)) return true;
    if (session->SearchEvent(CPAP_Hypopnea,time,dist)) return true;

    return false;
}

// Sort BreathPeak by peak index
bool operator<(const BreathPeak & p1, const BreathPeak & p2)
{
    return p1.start < p2.start;
}

//! \brief Filters input to output with a percentile filter with supplied width.
//! \param samples Number of samples
//! \param width number of surrounding samples to consider
//! \param percentile fractional percentage, between 0 and 1
void percentileFilter(EventDataType * input, EventDataType * output, int samples, int width, EventDataType percentile)
{
    if (samples<=0)
        return;

    if (percentile>1)
        percentile=1;

    QVector<EventDataType> buf(width);
    int s,e;
    int z1=width/2;
    int z2=z1+(width % 2);
    int nm1=samples-1;
    //int j;

    // Scan through all of input
    for (int k=0;k < samples;k++) {

        s=k-z1;
        e=k+z2;

        // Cap bounds
        if (s < 0) s=0;
        if (e > nm1) e=nm1;

        //
        int j=0;
        for (int i=s; i < e; i++) {
            buf[j++]=input[i];
        }
        j--;

        EventDataType val=j * percentile;
        EventDataType fl=floor(val);

        // If even percentile, or already max value..
        if ((val==fl) || (j>=width-1)) {
            nth_element(buf.begin(),buf.begin()+j,buf.begin()+width-1);
            val=buf[j];
        } else {
            // Percentile lies between two points, interpolate.
            double v1,v2;
            nth_element(buf.begin(),buf.begin()+j,buf.begin()+width-1);
            v1=buf[j];
            nth_element(buf.begin(),buf.begin()+j+1,buf.begin()+width-1);
            v2=buf[j+1];

            val = v1 + (v2-v1)*(val-fl);
        }
        output[k]=val;
    }
}

void xpassFilter(EventDataType * input, EventDataType * output, int samples, EventDataType weight)
{
    // prime the first value
    output[0]=input[0];

    for (int i=1;i<samples-1;i++) {
        output[i]=weight*input[i] + (1.0-weight)*output[i-1];
    }
    output[samples-1]=input[samples-1];
}

FlowParser::FlowParser()
{
    m_session=NULL;
    m_flow=NULL;
    m_filtered=NULL;
    m_gain=1;
    m_samples=0;
    m_startsUpper=true;

    // Allocate filter chain buffers..
    for (int i=0;i<num_filter_buffers;i++) {
        m_buffers[i]=(EventDataType *) malloc(max_filter_buf_size);
    }
    m_filtered=(EventDataType *) malloc(max_filter_buf_size);
}
FlowParser::~FlowParser()
{
    free (m_filtered);
    for (int i=0;i<num_filter_buffers;i++) {
        free(m_buffers[i]);
    }
}
void FlowParser::clearFilters()
{
    m_filters.clear();
}

EventDataType * FlowParser::applyFilters(EventDataType * data, int samples)
{
    EventDataType *in=NULL,*out=NULL;
    if (m_filters.size()==0) {
        qDebug() << "Trying to apply empty filter list in FlowParser..";
        return NULL;
    }

    int numfilt=m_filters.size();

    for (int i=0;i<numfilt;i++) {
        if (i==0) {
            in=data;
            out=m_buffers[0];
            if (in==out) {
                qDebug() << "Error: If you need to use internal m_buffers as initial input, use the second one. No filters were applied";
                return NULL;
            }
        } else {
            in=m_buffers[(i+1) % 2];
            out=m_buffers[i % 2];
        }

        // If final link in chain, pass it back out to input data
        if (i==numfilt-1) {
            out=data;
        }
        Filter & filter=m_filters[i];
        if (filter.type==FilterNone) {
            // Just copy it..
            memcpy(in,out,samples*sizeof(EventDataType));
        } else if (filter.type==FilterPercentile) {
            percentileFilter(in,out,samples,filter.param1,filter.param2);
        } else if (filter.type==FilterXPass) {
            xpassFilter(in,out,samples,filter.param1);
        }
    }

    return out;
}
void FlowParser::openFlow(Session * session, EventList * flow)
{
    if (!flow) {
        qDebug() << "called FlowParser::processFlow() with a null EventList!";
        return;
    }
    m_session=session;
    m_flow=flow;
    m_gain=flow->gain();
    m_rate=flow->rate();
    m_samples=flow->count();
    EventStoreType *inraw=flow->rawData();

    // Make sure we won't overflow internal buffers
    if (m_samples > max_filter_buf_size) {
        qDebug() << "Error: Sample size exceeds max_filter_buf_size in FlowParser::openFlow().. Capping!!!";
        m_samples=max_filter_buf_size;
    }

    // Begin with the second internal buffer
    EventDataType * buf=m_filtered;
    EventDataType c;
    // Apply gain to waveform
    for (int i=0;i<m_samples;i++) {
        c= EventDataType(*inraw++) * m_gain;
        *buf++ = c;
    }

    // Apply the rest of the filters chain
    buf=applyFilters(m_filtered, m_samples);
    if (buf!=m_filtered) {
        int i=5;
    }

    calcPeaks(m_filtered, m_samples);
}

// Calculates breath upper & lower peaks for a chunk of EventList data
void FlowParser::calcPeaks(EventDataType * input, int samples)
{
    if (samples<=0)
        return;

    EventDataType min=0,max=0, c, lastc=0;

    EventDataType zeroline=0;

    // For each sample in flow waveform
    double rate=m_flow->rate();

    double flowstart=m_flow->first();
    double lasttime,time;

    double peakmax=flowstart, peakmin=flowstart;

    time=lasttime=flowstart;

    // Estimate storage space needed using typical average breaths per minute.
    m_minutes=double(m_flow->last() - m_flow->first()) / 60000.0;
    const double avgbpm=20;
    int guestimate=m_minutes*avgbpm;
    breaths_lower.reserve(guestimate);
    breaths_upper.reserve(guestimate);

    // Prime min & max, and see which side of the zero line we are starting from.
    c=input[0];
    min=max=c;
    lastc=c;
    m_startsUpper=(c >= zeroline);

    qint32 start=0,middle=0;//,end=0;

    breaths.clear();

    int sps=1000/m_rate;
    // For each samples, find the peak upper and lower breath components
    //bool dirty=false;
    int len=0, lastk=0; //lastlen=0

    //EventList *uf1=m_session->AddEventList(CPAP_UserFlag1,EVL_Event);
    //EventList *uf2=m_session->AddEventList(CPAP_UserFlag2,EVL_Event);

    qint64 sttime=time;//, ettime=time;

    for (int k=0; k < samples; k++) {
        c=input[k];
      //  dirty=false;

        if (c >= zeroline) {

            // Did we just cross the zero line going up?
            if (lastc < zeroline) {
                // This helps filter out dirty breaths..
                len=k-start;
                if ((max>3) && ((max-min) > 8) && (len>sps) && (middle > start))  {
                    breaths.push_back(BreathPeak(min, max, start, peakmax, middle, peakmin, k));
                    //EventDataType g0=(0-lastc) / (c-lastc);
                    //double d=(m_rate*g0);
                    //double d1=flowstart+ (start*rate);
                    //double d2=flowstart+ (k*rate);

                    //uf1->AddEvent(d1,0);
                    //uf2->AddEvent(d2,0);
                    //lastlen=k-start;
                    // Set max for start of the upper breath cycle
                    max=c;
                    peakmax=time;


                    // Starting point of next breath cycle
                    start=k;
                    sttime=time;
                } //else {
//                    dirty=true;
//                    lastc=-1;
//                }
            } else if (c > max) {
                // Update upper breath peak
                max=c;
                peakmax=time;
            }
        }

        if (c < zeroline) {
            // Did we just cross the zero line going down?

            if (lastc >= zeroline) {
                // Set min for start of the lower breath cycle
                min=c;
                peakmin=time;
                middle=k;

            } else if (c < min) {
                // Update lower breath peak
                min=c;
                peakmin=time;
            }

        }
        lasttime=time;
        time+=rate;
        //if (!dirty)
        lastc=c;
        lastk=k;
    }
}


//! \brief Calculate Respiratory Rate, TidalVolume, Minute Ventilation, Ti & Te..
// These are grouped together because, a) it's faster, and b) some of these calculations rely on others.
void FlowParser::calc(bool calcResp, bool calcTv, bool calcTi, bool calcTe, bool calcMv)
{
    if (!m_session) {
        return;
    }

    // Don't even bother if only a few breaths in this chunk
    const int lowthresh=4;
    int nm=breaths.size();
    if (nm < lowthresh)
        return;

    const qint64 minute=60000;


    double start=m_flow->first();
    double time=start;

    int bs,be,bm;
    double st,et,mt;

    /////////////////////////////////////////////////////////////////////////////////
    // Respiratory Rate setup
    /////////////////////////////////////////////////////////////////////////////////
    EventDataType minrr,maxrr;
    EventList * RR=NULL;
    quint32 * rr_tptr=NULL;
    EventStoreType * rr_dptr=NULL;
    if (calcResp) {
        RR=m_session->AddEventList(CPAP_RespRate,EVL_Event);
        minrr=RR->Min(), maxrr=RR->Max();
        RR->setFirst(time+minute);
        RR->getData().resize(nm);
        RR->getTime().resize(nm);
        rr_tptr=RR->rawTime();
        rr_dptr=RR->rawData();
    }

    int rr_count=0;

    double len, st2,et2,adj, stmin, b, rr=0;
    int idx;



    /////////////////////////////////////////////////////////////////////////////////
    // Inspiratory / Expiratory Time setup
    /////////////////////////////////////////////////////////////////////////////////
    double lastte2=0,lastti2=0,lastte=0, lastti=0, te, ti, ti1, te1,c;
    EventList * Te=NULL, * Ti=NULL;
    if (calcTi) {
        Ti=m_session->AddEventList(CPAP_Ti,EVL_Event);
        Ti->setGain(0.1);
    }
    if (calcTe) {
        Te=m_session->AddEventList(CPAP_Te,EVL_Event);
        Te->setGain(0.1);
    }


    /////////////////////////////////////////////////////////////////////////////////
    // Tidal Volume setup
    /////////////////////////////////////////////////////////////////////////////////
    EventList * TV;
    EventDataType mintv, maxtv, tv;
    double val1, val2;
    quint32 * tv_tptr;
    EventStoreType * tv_dptr;
    int tv_count=0;
    if (calcTv) {
        TV=m_session->AddEventList(CPAP_TidalVolume,EVL_Event);
        mintv=TV->Min(), maxtv=TV->Max();
        TV->setFirst(start);
        TV->getData().resize(nm);
        TV->getTime().resize(nm);
        tv_tptr=TV->rawTime();
        tv_dptr=TV->rawData();
    }
    /////////////////////////////////////////////////////////////////////////////////
    // Minute Ventilation setup
    /////////////////////////////////////////////////////////////////////////////////
    EventList * MV=NULL;
    EventDataType mv;
    if (calcMv) {
        MV=m_session->AddEventList(CPAP_MinuteVent,EVL_Event);
        MV->setGain(0.1);
    }

    EventDataType sps=(1000.0/m_rate); // Samples Per Second


    qint32 timeval=0; // Time relative to start

    for (idx=0;idx<nm;idx++) {
        bs=breaths[idx].start;
        bm=breaths[idx].middle;
        be=breaths[idx].end;

        // Calculate start, middle and end time of this breath
        st=start+bs * m_rate;
        mt=start+bm * m_rate;

        timeval=be * m_rate;

        et=start+timeval;



        /////////////////////////////////////////////////////////////////////
        // Calculate Inspiratory Time (Ti) for this breath
        /////////////////////////////////////////////////////////////////////
        if (calcTi) {
            ti=(mt-st)/100.0;
            ti1=(lastti2+lastti+ti)/3.0;
            Ti->AddEvent(mt,ti1);
            lastti2=lastti;
            lastti=ti;
        }
        /////////////////////////////////////////////////////////////////////
        // Calculate Expiratory Time (Te) for this breath
        /////////////////////////////////////////////////////////////////////
        if (calcTe) {
            te=(et-mt)/100.0; // (/1000 * 10)
            // Average last three values..
            te1=(lastte2+lastte+te)/3.0;
            Te->AddEvent(mt,te1);

            lastte2=lastte;
            lastte=te;
        }
        /////////////////////////////////////////////////////////////////////
        // Calculate TidalVolume
        /////////////////////////////////////////////////////////////////////
        if (calcTv) {
            val1=0, val2=0;
            // Scan the upper breath
            for (int j=bs;j<bm;j++)  {
                // convert flow to ml/s to L/min and divide by samples per second
                c=double(qAbs(m_filtered[j])) * 1000.0 / 60.0 / sps;
                val2+=c;
                //val2+=c*c; // for RMS
            }
            // calculate root mean square
            //double n=bm-bs;
            //double q=(1/n)*val2;
            //double x=sqrt(q)*2;
            //val2=x;
            tv=val2;

            if (tv < mintv) mintv=tv;
            if (tv > maxtv) maxtv=tv;
            *tv_tptr++ = timeval;
            *tv_dptr++ = tv * 10.0;
            tv_count++;

        }

        /////////////////////////////////////////////////////////////////////
        // Respiratory Rate Calculations
        /////////////////////////////////////////////////////////////////////
        if (calcResp) {
            stmin=et-minute;
            if (stmin < start)
                stmin=start;
            len=et-stmin;
            if (len < minute)
                continue;

            rr=0;
            //et2=et;

            // Step back through last minute and count breaths
            for (int i=idx;i>=0;i--) {
                st2=start + double(breaths[i].start) * m_rate;
                et2=start + double(breaths[i].end) * m_rate;
                if (et2 < stmin)
                    break;

                len=et2-st2;
                if (st2 < stmin) {
                    // Partial breath
                    st2=stmin;
                    adj=et2 - st2;
                    b=(1.0 / len) * adj;
                } else b=1;
                rr+=b;
            }

            // Calculate min & max
            if (rr < minrr) minrr=rr;
            if (rr > maxrr) maxrr=rr;

            // Add manually.. (much quicker)
            *rr_tptr++ = timeval;
            *rr_dptr++ = rr * 50.0;
            rr_count++;

            //rr->AddEvent(et,br * 50.0);
        }
        if (calcMv && calcResp && calcTv) {
            mv=(tv/1000.0) * rr;
            MV->AddEvent(et,mv * 10.0);
        }
    }

    /////////////////////////////////////////////////////////////////////
    // Respiratory Rate post filtering
    /////////////////////////////////////////////////////////////////////

    if (calcResp) {
        RR->setGain(0.02);
        RR->setMin(minrr);
        RR->setMax(maxrr);
        RR->setFirst(start);
        RR->setLast(et);
        RR->setCount(rr_count);
    }
    /////////////////////////////////////////////////////////////////////
    // Tidal Volume post filtering
    /////////////////////////////////////////////////////////////////////

    if (calcTv) {
        TV->setGain(0.1);
        TV->setMin(mintv);
        TV->setMax(maxtv);
        TV->setFirst(start);
        TV->setLast(et);
        TV->setCount(tv_count);
    }
}


/*
// Support function for calcRespRate()
int filterFlow(Session *session, EventList *in, EventList *out, EventList *tv, EventList *mv, double rate)
{
    int size=in->count();
    int samples=size;

    // Create two buffers for filter stages.
    EventDataType *stage1=new EventDataType [samples];
    EventDataType *stage2=new EventDataType [samples];

    QVector<EventDataType> med;
    med.reserve(8);

    EventDataType r,tmp;
    int cnt;

    EventDataType c;
    int i;

    percentileFilter(in->rawData(), stage1, samples, 11,  0.5);
    percentileFilter(stage1, stage2, samples, 7,  0.5);


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

    EventDataType min=0,max=0;
    qint64 peakmin=0, peakmax=0;
    double avgmax=0;
    double avgmin=0;
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
                    if (peakmin) {
                        breaths_min_peak.push_back(peakmin);
                        breaths_min.push_back(min);
                        avgmin+=min;
                    }
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
                    if (peakmax>0) {
                        breaths_max_peak.push_back(peakmax);
                        breaths_max.push_back(max);
                        avgmax+=max;
                    }
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

    avgmax/=EventDataType(breaths_max.size());
    avgmin/=EventDataType(breaths_min.size());

    if ((breaths_max.size()>5) && (breaths_min.size()>5) && (p_profile->cpap->userEventFlagging())) {
        EventDataType maxperc,minperc;

        int n=breaths_max.size()*0.8;
        if (n > breaths_max.size()-1) n-=1;
        nth_element(breaths_max.begin(),breaths_max.begin()+n,breaths_max.end());
        maxperc=breaths_max[n];

        n=breaths_min.size()*0.2;
        if (n > breaths_min.size()-1) n-=1;
        nth_element(breaths_min.begin(),breaths_min.begin()+n,breaths_min.end());
        minperc=breaths_min[n];


        QVector<qint64> goodb;

        EventDataType restriction=p_profile->cpap->userFlowRestriction()/100.0;

        // This is faster than vector access.
        EventDataType *dptr=breaths_max.data();
        qint64 * tptr=breaths_max_peak.data();

        EventDataType restrict=maxperc * restriction;

        // Knock out all the upper breath components above the flow restriction
        for (int i=0;i<breaths_max.size();i++)  {
            max=*dptr++; //breaths_max[i];
            time=*tptr++; //breaths_max_peak[i];

            if ((time > 0) && (max > restrict)) {
                goodb.push_back(time);
            }
        }
        dptr=breaths_min.data();
        tptr=breaths_min_peak.data();

        restrict=minperc * restriction;

        // Knock out all the lower breath components above the flow restriction
        for (int i=0;i<breaths_min.size();i++)  {
            min=*dptr++; //breaths_min[i];
            time=*tptr++; //breaths_min_peak[i];
            if ((time > 0) && (min < restrict)) {
                goodb.push_back(time);
            }
        }
        EventList *uf=NULL;

        if (goodb.size()>2) {
            qint64 duration=p_profile->cpap->userEventDuration()*1000;

            qSort(goodb);

            tptr=goodb.data();

            qint64 g0=*tptr++,g1;

            EventDataType lf;
            //
            for (int i=1;i<goodb.size();i++) {
                g1=*tptr++;
                qint64 len=g1-g0;
                if (len >= duration) {
                    time=g0 + (len/2);
                    if (!SearchApnea(session,time)) {
                        if (!uf) {
                            uf=new EventList(EVL_Event,1,0,0,0,0,true);
                            session->eventlist[CPAP_UserFlag1].push_back(uf);
                        }
                        lf=double(len)/1000.0;
                        if (lf>30) {
                            int i=5;
                        }
                        uf->AddEvent(time,lf,1);
                    }
                }
                g0=g1;
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

 */

void calcRespRate(Session *session, FlowParser * flowparser)
{
    if (session->machine()->GetType()!=MT_CPAP) return;

//    if (session->machine()->GetClass()!=STR_MACH_PRS1) return;

    if (!session->eventlist.contains(CPAP_FlowRate)) {
        qDebug() << "calcRespRate called without FlowRate waveform available";
        return; //need flow waveform
    }

    bool trashfp;
    if (!flowparser) {
        flowparser=new FlowParser();
        trashfp=true;
        qDebug() << "calcRespRate called without valid FlowParser object.. using a slow throw-away!";
        //return;
    } else {
        trashfp=false;
    }

    bool calcResp=!session->eventlist.contains(CPAP_RespRate);
    bool calcTv=!session->eventlist.contains(CPAP_TidalVolume);
    bool calcTi=!session->eventlist.contains(CPAP_Ti);
    bool calcTe=!session->eventlist.contains(CPAP_Te);
    bool calcMv=!session->eventlist.contains(CPAP_MinuteVent);


    // If any of these three missing, remove all, and switch all on
    if (!(calcResp & calcTv & calcMv)) {
        calcTv=calcMv=calcResp=true;

        QVector<EventList *> & list=session->eventlist[CPAP_RespRate];
        for (int i=0;i<list.size();i++) {
            delete list[i];
        }
        session->eventlist[CPAP_RespRate].clear();

        QVector<EventList *> & list2=session->eventlist[CPAP_TidalVolume];
        for (int i=0;i<list2.size();i++) {
            delete list2[i];
        }
        session->eventlist[CPAP_TidalVolume].clear();

        QVector<EventList *> & list3=session->eventlist[CPAP_MinuteVent];
        for (int i=0;i<list3.size();i++) {
            delete list3[i];
        }
        session->eventlist[CPAP_MinuteVent].clear();
    }

    flowparser->clearFilters();

    // No filters works rather well with the new peak detection algorithm..
    // Although the output could use filtering.

    //flowparser->addFilter(FilterPercentile,7,0.5);
    //flowparser->addFilter(FilterPercentile,5,0.5);
    //flowparser->addFilter(FilterXPass,0.5);
    EventList *flow;
    int cnt=0;
    for (int ws=0; ws < session->eventlist[CPAP_FlowRate].size(); ws++) {
        flow=session->eventlist[CPAP_FlowRate][ws];
        if (flow->count() > 20) {
            flowparser->openFlow(session, flow);
            flowparser->calc(calcResp, calcTv, calcTi ,calcTe, calcMv);
        }
    }
    if (trashfp) {
        delete flowparser;
    }
}


EventDataType calcAHI(Session *session,qint64 start, qint64 end)
{
    double hours,ahi,cnt;
    if (start<0) {
        // much faster..
        hours=session->hours();
        cnt=session->count(CPAP_Obstructive)
                +session->count(CPAP_Hypopnea)
                +session->count(CPAP_ClearAirway)
                +session->count(CPAP_Apnea);

        ahi=cnt/hours;
    } else {
        hours=double(end-start)/3600000L;
        if (hours==0) return 0;
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
    const qint64 window_step=30000; // 30 second windows
    double window_size=p_profile->cpap->AHIWindow();
    qint64 window_size_ms=window_size*60000L;

    bool zeroreset=p_profile->cpap->AHIReset();

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

    qint64 ti=first,lastti=first;

    double avg=0;
    int cnt=0;

    double events;
    double hours=(window_size/60.0);
    if (zeroreset) {
        // I personally don't see the point of resetting each hour.
        do {
            // For each window, in 30 second increments
            for (qint64 t=ti;t < ti+window_size_ms; t+=window_step) {
                if (t > last)
                    break;
                events=session->rangeCount(CPAP_Obstructive,ti,t)
                        +session->rangeCount(CPAP_Hypopnea,ti,t)
                        +session->rangeCount(CPAP_ClearAirway,ti,t)
                        +session->rangeCount(CPAP_Apnea,ti,t);

                //ahi=calcAHI(session,ti,t)* hours;

                ahi = events / hours;

                AHI->AddEvent(t,ahi);
                avg+=ahi;
                cnt++;
            }
            lastti=ti;
            ti+=window_size_ms;
        } while (ti<last);

    } else {
        for (ti=first;ti<last;ti+=window_step) {
//            if  (ti>last) {
////                AHI->AddEvent(last,ahi);
////                avg+=ahi;
////                cnt++;
//                break;
//            }
            f=ti-window_size_ms;
            ahi=calcAHI(session,f,ti);
            avg+=ahi;
            cnt++;
            AHI->AddEvent(ti,ahi);
            lastti=ti;
            ti+=window_step;
        }
    }
    AHI->AddEvent(lastti,0);
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

    qint64 start;
    quint32 * tptr;
    EventStoreType * dptr;
    EventDataType gain;
    for (int i=0;i<session->eventlist[CPAP_LeakTotal].size();i++) {
        EventList & el=*session->eventlist[CPAP_LeakTotal][i];
        gain=el.gain();
        dptr=el.rawData();
        tptr=el.rawTime();
        start=el.first();

        for (unsigned j=0;j<el.count();j++) {
            tmp=*dptr++ * gain;
            ti=start+ *tptr++;

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

            int idx=float(med.size() * 0.0);
            if (idx>=med.size()) idx--;
            nth_element(med.begin(),med.begin()+idx,med.end());
            median=tmp-(*(med.begin()+idx));

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
