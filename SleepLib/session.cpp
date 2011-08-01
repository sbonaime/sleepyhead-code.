/*
 SleepLib Session Implementation
 This stuff contains the base calculation smarts
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

#include "session.h"
#include <cmath>
#include <QDir>
#include <QDebug>
#include <QMessageBox>
#include <QMetaType>
#include <algorithm>

using namespace std;

Session::Session(Machine * m,SessionID session)
{
    if (!session) {
        session=m->CreateSessionID();
    }
    s_machine=m;
    s_session=session;
    s_changed=false;
    s_events_loaded=false;
    _first_session=true;

    s_first=s_last=0;
    s_eventfile="";
}
Session::~Session()
{
    TrashEvents();
}

void Session::TrashEvents()
// Trash this sessions Events and release memory.
{
    QHash<ChannelID,QVector<EventList *> >::iterator i;
    QVector<EventList *>::iterator j;
    for (i=eventlist.begin(); i!=eventlist.end(); i++) {
        for (j=i.value().begin(); j!=i.value().end(); j++) {
            delete *j;
        }
    }
    s_events_loaded=false;
    eventlist.clear();
}

//const int max_pack_size=128;
bool Session::OpenEvents() {
    if (s_events_loaded)
        return true;

    s_events_loaded=eventlist.size() > 0;
    if (s_events_loaded)
        return true;

    if (!s_eventfile.isEmpty()) {
        bool b=LoadEvents(s_eventfile);
        if (!b) {
            qWarning() << "Error Unpacking Events" << s_eventfile;
            return false;
        }
    }


    return s_events_loaded=true;
};

bool Session::Store(QString path)
// Storing Session Data in our format
// {DataDir}/{MachineID}/{SessionID}.{ext}
{
    QDir dir(path);
    if (!dir.exists(path))
        dir.mkpath(path);

    QString base;
    base.sprintf("%08lx",s_session);
    base=path+"/"+base;
    //qDebug() << "Storing Session: " << base;
    bool a;
    a=StoreSummary(base+".000"); // if actually has events
    //qDebug() << " Summary done";
    if (eventlist.size()>0) {
        s_eventfile=base+".001";
        StoreEvents(s_eventfile);
    } else {
        qDebug() << "Trying to save empty events file";
    }
    //qDebug() << " Events done";
    s_changed=false;
    s_events_loaded=true;

        //TrashEvents();
    //} else {
    //    qDebug() << "Session::Store() No event data saved" << s_session;
    //}

    return a;
}

const quint16 filetype_summary=0;
const quint16 filetype_data=1;
const quint32 magic=0xC73216AB;

bool Session::StoreSummary(QString filename)
{

    QFile file(filename);
    file.open(QIODevice::WriteOnly);

    QDataStream out(&file);
    out.setVersion(QDataStream::Qt_4_6);
    out.setByteOrder(QDataStream::LittleEndian);

    out << (quint32)magic;
    out << (quint16)dbversion;
    out << (quint16)filetype_summary;
    out << (quint32)s_machine->id();

    out << (quint32)s_session;
    out << s_first;  // Session Start Time
    out << s_last;   // Duration of sesion in seconds.
    //out << (quint16)settings.size();

    out << settings;
    out << m_cnt;
    out << m_sum;
    out << m_avg;
    out << m_wavg;
    out << m_90p;
    out << m_min;
    out << m_max;
    out << m_cph;
    out << m_sph;
    out << m_firstchan;
    out << m_lastchan;



    // First output the Machine Code and type for each summary record
/*    for (QHash<ChannelID,QVariant>::iterator i=settings.begin(); i!=settings.end(); i++) {
        ChannelID mc=i.key();
        out << (qint16)mc;
        out << i.value();
    } */
    file.close();
    return true;
}

bool Session::LoadSummary(QString filename)
{
    if (filename.isEmpty()) {
        qDebug() << "Empty summary filename";
        return false;
    }

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Couldn't open summary file" << filename;
        return false;
    }

    QDataStream in(&file);
    in.setVersion(QDataStream::Qt_4_6);
    in.setByteOrder(QDataStream::LittleEndian);

    quint32 t32;
    quint16 t16;

    QHash<ChannelID,MCDataType> mctype;
    QVector<ChannelID> mcorder;
    in >> t32;
    if (t32!=magic) {
        qDebug() << "Wrong magic number in " << filename;
        return false;
    }

    in >> t16;      // DB Version
    if (t16!=dbversion) {
        throw OldDBVersion();
        //qWarning() << "Old dbversion "<< t16 << "summary file.. Sorry, you need to purge and reimport";
        return false;
    }

    in >> t16;      // File Type
    if (t16!=filetype_summary) {
        qDebug() << "Wrong file type"; //wrong file type
        return false;
    }


    qint32 ts32;
    in >> ts32;      // MachineID (dont need this result)
    if (ts32!=s_machine->id()) {
        qWarning() << "Machine ID does not match in" << filename << " I will try to load anyway in case you know what your doing.";
    }

    in >> t32;      // Sessionid;
    s_session=t32;

    in >> s_first;  // Start time
    in >> s_last;   // Duration // (16bit==Limited to 18 hours)


    in >> settings;
    in >> m_cnt;
    in >> m_sum;
    in >> m_avg;
    in >> m_wavg;
    in >> m_90p;
    in >> m_min;
    in >> m_max;
    in >> m_cph;
    in >> m_sph;
    in >> m_firstchan;
    in >> m_lastchan;

    /*qint16 sumsize;
    in >> sumsize;  // Summary size (number of Machine Code lists)

    for (int i=0; i<sumsize; i++) {
        in >> t16;      // Machine Code
        ChannelID mc=(ChannelID)t16;
        in >> settings[mc];
    } */

    return true;
}

bool Session::StoreEvents(QString filename)
{
    QFile file(filename);
    file.open(QIODevice::WriteOnly);

    QDataStream out(&file);
    out.setVersion(QDataStream::Qt_4_6);
    out.setByteOrder(QDataStream::LittleEndian);

    out << (quint32)magic;          // Magic Number
    out << (quint16)dbversion;      // File Version
    out << (quint16)filetype_data;  // File type 1 == Event
    out << (quint32)s_machine->id();// Machine ID

    out << (quint32)s_session;      // This session's ID
    out << s_first;
    out << s_last;

    out << (qint16)eventlist.size(); // Number of event categories


    QHash<ChannelID,QVector<EventList *> >::iterator i;

    for (i=eventlist.begin(); i!=eventlist.end(); i++) {
        out << (qint16)i.key(); // ChannelID
        out << (qint16)i.value().size();
        for (int j=0;j<i.value().size();j++) {
            EventList &e=*i.value()[j];
            out << e.first();
            out << e.last();
            out << (qint32)e.count();
            out << (qint8)e.type();
            out << e.rate();
            out << e.gain();
            out << e.offset();
            out << e.min();
            out << e.max();
        }
    }
    for (i=eventlist.begin(); i!=eventlist.end(); i++) {
        for (int j=0;j<i.value().size();j++) {
            EventList &e=*i.value()[j];

            for (int c=0;c<e.count();c++) {
                out << e.raw(c);
            }
            if (e.type()!=EVL_Waveform) {
                for (int c=0;c<e.count();c++) {
                    out << e.getTime()[c];
                }
            }
        }
    }

    return true;
}
bool Session::LoadEvents(QString filename)
{
    if (filename.isEmpty()) {
        qDebug() << "Session::LoadEvents() Filename is empty";
        return false;
    }

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Couldn't open file" << filename;
        return false;
    }
    QDataStream in(&file);
    in.setVersion(QDataStream::Qt_4_6);
    in.setByteOrder(QDataStream::LittleEndian);

    quint32 t32;
    quint16 t16;
    quint8 t8;
    //qint16 i16;

    in >> t32;      // Magic Number
    if (t32!=magic) {
        qWarning() << "Wrong Magic number in " << filename;
        return false;
    }
    in >> t16;      // File Version
    if (t16!=dbversion) {
        throw OldDBVersion();
        //qWarning() << "Old dbversion "<< t16 << "summary file.. Sorry, you need to purge and reimport";
        return false;
    }

    in >> t16;      // File Type
    if (t16!=filetype_data) {
        qDebug() << "Wrong File Type in " << filename;
        return false;
    }

    qint32 ts32;
    in >> ts32;      // MachineID
    if (ts32!=s_machine->id()) {
        qWarning() << "Machine ID does not match in" << filename << " I will try to load anyway in case you know what your doing.";
    }

    in >> t32;      // Sessionid
    s_session=t32;
    in >> s_first;
    in >> s_last;

    qint16 mcsize;
    in >> mcsize;   // number of Machine Code lists

    ChannelID code;
    qint64 ts1,ts2;
    qint32 evcount;
    EventListType elt;
    EventDataType rate,gain,offset,mn,mx;
    qint16 size2;
    QVector<ChannelID> mcorder;
    QVector<qint16> sizevec;
    for (int i=0;i<mcsize;i++) {
        in >> t16;
        code=(ChannelID)t16;
        mcorder.push_back(code);
        in >> size2;
        sizevec.push_back(size2);
        for (int j=0;j<size2;j++) {
            in >> ts1;
            in >> ts2;
            in >> evcount;
            in >> t8;
            elt=(EventListType)t8;
            in >> rate;
            in >> gain;
            in >> offset;
            in >> mn;
            in >> mx;
            EventList *elist=new EventList(code,elt,gain,offset,mn,mx,rate);

            eventlist[code].push_back(elist);
            elist->m_count=evcount;
            elist->m_first=ts1;
            elist->m_last=ts2;
        }
    }

    EventStoreType t;
    quint32 x;
    for (int i=0;i<mcsize;i++) {
        code=mcorder[i];
        size2=sizevec[i];
        for (int j=0;j<size2;j++) {
            EventList &evec=*eventlist[code][j];
            evec.m_data.reserve(evec.m_count);
            for (int c=0;c<evec.m_count;c++) {
                in >> t;
                //evec.m_data[c]=t;
                evec.m_data.push_back(t);
            }
            //last=evec.first();
            if (evec.type()!=EVL_Waveform) {
                evec.m_time.reserve(evec.m_count);
                for (int c=0;c<evec.m_count;c++) {
                    in >> x;
                    //last+=x;
                    evec.m_time.push_back(x);
                    //evec.m_time[c]=x;
                }
            }
        }
    }
    return true;
}

void Session::UpdateSummaries()
{
    ChannelID id;
    QHash<ChannelID,QVector<EventList *> >::iterator c;
    for (c=eventlist.begin();c!=eventlist.end();c++) {
        id=c.key();
        if ((channel[id].channeltype()==CT_Event) || (channel[id].channeltype()==CT_Graph)) {
            //sum(id); // avg calculates this and cnt.
            min(id);
            max(id);
            avg(id);
            wavg(id);
            p90(id);
            last(id);
            first(id);
        }
    }
}

EventDataType Session::min(ChannelID id)
{
    QHash<ChannelID,EventDataType>::iterator i=m_min.find(id);
    if (i!=m_min.end())
        return i.value();

    QHash<ChannelID,QVector<EventList *> >::iterator j=eventlist.find(id);
    if (j==eventlist.end())
        return 0;
    QVector<EventList *> & evec=j.value();

    bool first=true;
    EventDataType min,t1;
    for (int i=0;i<evec.size();i++) {
        t1=evec[i]->min();
        if (t1==evec[i]->max()) continue;
        if (first) {
            min=t1;
            first=false;
        } else {
            if (min>t1) min=t1;
        }
    }
    m_min[id]=min;
    return min;
}
EventDataType Session::max(ChannelID id)
{
    QHash<ChannelID,EventDataType>::iterator i=m_max.find(id);
    if (i!=m_max.end())
        return i.value();

    QHash<ChannelID,QVector<EventList *> >::iterator j=eventlist.find(id);
    if (j==eventlist.end())
        return 0;
    QVector<EventList *> & evec=j.value();

    bool first=true;
    EventDataType max,t1;
    for (int i=0;i<evec.size();i++) {
        t1=evec[i]->max();
        if (t1==evec[i]->min()) continue;
        if (first) {
            max=t1;
            first=false;
        } else {
            if (max<t1) max=t1;
        }
    }
    m_max[id]=max;
    return max;
}
qint64 Session::first(ChannelID id)
{
    QHash<ChannelID,quint64>::iterator i=m_firstchan.find(id);
    if (i!=m_firstchan.end())
        return i.value();

    QHash<ChannelID,QVector<EventList *> >::iterator j=eventlist.find(id);
    if (j==eventlist.end())
        return 0;
    QVector<EventList *> & evec=j.value();

    bool first=true;
    qint64 min=0,t1;
    for (int i=0;i<evec.size();i++) {
        t1=evec[i]->first();
        if (first) {
            min=t1;
            first=false;
        } else {
            if (min>t1) min=t1;
        }
    }
    m_firstchan[id]=min;
    return min;
}
qint64 Session::last(ChannelID id)
{
    QHash<ChannelID,quint64>::iterator i=m_lastchan.find(id);
    if (i!=m_lastchan.end())
        return i.value();

    QHash<ChannelID,QVector<EventList *> >::iterator j=eventlist.find(id);
    if (j==eventlist.end())
        return 0;
    QVector<EventList *> & evec=j.value();

    bool first=true;
    qint64 max=0,t1;
    for (int i=0;i<evec.size();i++) {
        t1=evec[i]->last();
        if (first) {
            max=t1;
            first=false;
        } else {
            if (max<t1) max=t1;
        }
    }

    m_lastchan[id]=max;
    return max;
}

int Session::count(ChannelID id)
{
    QHash<ChannelID,int>::iterator i=m_cnt.find(id);
    if (i!=m_cnt.end())
        return i.value();

    QHash<ChannelID,QVector<EventList *> >::iterator j=eventlist.find(id);
    if (j==eventlist.end())
        return 0;
    QVector<EventList *> & evec=j.value();

    int sum=0;
    for (int i=0;i<evec.size();i++) {
        sum+=evec[i]->count();
    }
    m_cnt[id]=sum;
    return sum;
}

double Session::sum(ChannelID id)
{
    QHash<ChannelID,double>::iterator i=m_sum.find(id);
    if (i!=m_sum.end())
        return i.value();

    QHash<ChannelID,QVector<EventList *> >::iterator j=eventlist.find(id);
    if (j==eventlist.end())
        return 0;
    QVector<EventList *> & evec=j.value();

    double sum=0;
    for (int i=0;i<evec.size();i++) {
        for (int j=0;j<evec[i]->count();j++) {
            sum+=evec[i]->data(j);
        }
    }
    m_sum[id]=sum;
    return sum;
}

EventDataType Session::avg(ChannelID id)
{
    QHash<ChannelID,EventDataType>::iterator i=m_avg.find(id);
    if (i!=m_avg.end())
        return i.value();

    QHash<ChannelID,QVector<EventList *> >::iterator j=eventlist.find(id);
    if (j==eventlist.end())
        return 0;
    QVector<EventList *> & evec=j.value();

    double val=0;
    int cnt=0;
    for (int i=0;i<evec.size();i++) {
        for (int j=0;j<evec[i]->count();j++) {
            val+=evec[i]->data(j);
            cnt++;
        }
    }

    if (cnt>0) { // Shouldn't really happen.. Should aways contain data
        val/=double(cnt);
    }
    m_avg[id]=val;
    return val;
}
EventDataType Session::cph(ChannelID id)
{
    QHash<ChannelID,EventDataType>::iterator i=m_cph.find(id);
    if (i!=m_cph.end())
        return i.value();

    EventDataType val=count(id);
    val/=hours();

    m_cph[id]=val;
    return val;
}
EventDataType Session::sph(ChannelID id)
{
    QHash<ChannelID,EventDataType>::iterator i=m_sph.find(id);
    if (i!=m_sph.end())
        return i.value();

    EventDataType val=sum(id)/3600.0;
    val=100.0 / hours() * val;
    m_sph[id]=val;
    return val;
}

EventDataType Session::p90(ChannelID id)
{
    QHash<ChannelID,EventDataType>::iterator i=m_90p.find(id);
    if (i!=m_90p.end())
        return i.value();

    if (!eventlist.contains(id))
        return 0;

    EventDataType val=percentile(id,0.9);
    m_90p[id]=val;
    return val;
}
bool sortfunction (EventStoreType i,EventStoreType j) { return (i<j); }

EventDataType Session::percentile(ChannelID id,EventDataType percent)
{
    //if (channel[id].channeltype()==CT_Graph) return 0;
    QHash<ChannelID,QVector<EventList *> >::iterator jj=eventlist.find(id);
    if (jj==eventlist.end())
        return 0;
    QVector<EventList *> & evec=jj.value();

    if (percent > 1.0) {
        qWarning() << "Session::percentile() called with > 1.0";
        return 0;
    }

    int size=evec.size();
    if (size==0)
        return 0;

    QVector<EventStoreType> array;

    EventDataType gain=evec[0]->gain();

    int tt=0,cnt;
    for (int i=0;i<size;i++) {
        cnt=evec[i]->count();
        tt+=cnt;
        array.reserve(tt);
        for (int j=0;j<cnt;j++) {
            array.push_back(evec[i]->raw(j));
        }
    }
    std::sort(array.begin(),array.end(),sortfunction);
    double s=array.size();
    if (!s)
        return 0;
    double i=s*percent;
    double t;
    modf(i,&t);
    int j=t;

    //if (j>=size-1) return array[j];

    return EventDataType(array[j])*gain;
}

EventDataType Session::wavg(ChannelID id)
{
    QHash<ChannelID,EventDataType>::iterator i=m_wavg.find(id);
    if (i!=m_wavg.end())
        return i.value();

    QHash<ChannelID,QVector<EventList *> >::iterator jj=eventlist.find(id);
    if (jj==eventlist.end())
        return 0;
    QVector<EventList *> & evec=jj.value();

    bool first=true;
    qint64 lasttime=0,time,td;
    EventStoreType val,lastval=0;

    QHash<EventStoreType,quint32> vtime;

    EventDataType gain=evec[0]->gain();

    for (int i=0;i<evec.size();i++) {
        for (int j=0;j<evec[i]->count();j++) {
            val=evec[i]->raw(j);
            time=evec[i]->time(j);
            if (first) {
                first=false;
            } else {
                td=(time-lasttime);
                if (vtime.contains(lastval)) {
                    vtime[lastval]+=td;
                } else vtime[lastval]=td;
            }

            lasttime=time;
            lastval=val;
        }
    }
    /*td=last()-lasttime;
    if (vtime.contains(lastval)) {
        vtime[lastval]+=td;
    } else vtime[lastval]=td; */

    qint64 s0=0,s1=0,s2=0; // 32bit may all be thats needed here..
    for (QHash<EventStoreType,quint32>::iterator i=vtime.begin(); i!=vtime.end(); i++) {
       s0=i.value();
       s1+=i.key()*s0;
       s2+=s0;
    }
    double j=double(s1)/double(s_last-s_first);
    EventDataType v=j*gain;
    m_wavg[id]=v;
    return v;
}

