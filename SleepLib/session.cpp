/*
 SleepLib Session Implementation
 This stuff contains the base calculation smarts
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

#include "session.h"
#include "math.h"
#include <QDir>
#include <QDebug>
#include <QMessageBox>
#include <QMetaType>
#include <vector>
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
    map<MachineCode,vector<EventList *> >::iterator i;
    vector<EventList *>::iterator j;
    for (i=eventlist.begin(); i!=eventlist.end(); i++) {
        for (j=i->second.begin(); j!=i->second.end(); j++) {
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

    bool b=LoadEvents(s_eventfile);
    if (!b) {
        qWarning() << "Error Unkpacking Events" << s_eventfile;
        return false;
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
    if (eventlist.size()>0)
        StoreEvents(base+".001");
    //qDebug() << " Events done";
    s_changed=false;
    s_eventfile=base+".001";
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


bool IsPlatformLittleEndian()
{
    quint32 j=1;
    *((char*)&j) = 0;
    return j!=1;
}

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
    out << (quint16)summary.size();

    // First output the Machine Code and type for each summary record
    for (map<MachineCode,QVariant>::iterator i=summary.begin(); i!=summary.end(); i++) {
        MachineCode mc=i->first;
        out << (qint16)mc;
        out << i->second;
    }
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

    map<MachineCode,MCDataType> mctype;
    vector<MachineCode> mcorder;
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

    qint16 sumsize;
    in >> sumsize;  // Summary size (number of Machine Code lists)

    for (int i=0; i<sumsize; i++) {
        in >> t16;      // Machine Code
        MachineCode mc=(MachineCode)t16;
        in >> summary[mc];
    }

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


    map<MachineCode,vector<EventList *> >::iterator i;
    vector<EventList *>::iterator j;

    for (i=eventlist.begin(); i!=eventlist.end(); i++) {
        out << (qint16)i->first; // MachineCode
        out << (qint16)i->second.size();
        for (unsigned j=0;j<i->second.size();j++) {
            EventList &e=*i->second[j];
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
        for (unsigned j=0;j<i->second.size();j++) {
            EventList &e=*i->second[j];

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
    in >> mcsize;   // Summary size (number of Machine Code lists)

    MachineCode code;
    qint64 ts1,ts2;
    qint32 evcount;
    EventListType elt;
    EventDataType rate,gain,offset,mn,mx;
    qint16 size2;
    vector<MachineCode> mcorder;
    vector<qint16> sizevec;
    for (int i=0;i<mcsize;i++) {
        in >> t16;
        code=(MachineCode)t16;
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



EventDataType Session::min(MachineCode code)
{
    if (eventlist.find(code)==eventlist.end())
        return 0;
    vector<EventList *> & evec=eventlist[code];
    bool first=true;
    EventDataType min,t1;
    for (unsigned i=0;i<evec.size();i++) {
        t1=evec[i]->min();
        if (t1==evec[i]->max()) continue;
        if (first) {
            min=t1;
            first=false;
        } else {
            if (min>t1) min=t1;
        }
    }
    return min;
}
EventDataType Session::max(MachineCode code)
{
    if (eventlist.find(code)==eventlist.end())
        return 0;
    vector<EventList *> & evec=eventlist[code];
    bool first=true;
    EventDataType max,t1;
    for (unsigned i=0;i<evec.size();i++) {
        t1=evec[i]->max();
        if (t1==evec[i]->min()) continue;
        if (first) {
            max=t1;
            first=false;
        } else {
            if (max<t1) max=t1;
        }
    }
    return max;
}
qint64 Session::first(MachineCode code)
{
    if (eventlist.find(code)==eventlist.end())
        return 0;
    vector<EventList *> & evec=eventlist[code];
    bool first=true;
    qint64 min,t1;
    for (unsigned i=0;i<evec.size();i++) {
        t1=evec[i]->first();
        if (first) {
            min=t1;
            first=false;
        } else {
            if (min>t1) min=t1;
        }
    }
    return min;
}
qint64 Session::last(MachineCode code)
{
    if (eventlist.find(code)==eventlist.end())
        return 0;
    vector<EventList *> & evec=eventlist[code];
    bool first=true;
    qint64 max,t1;
    for (unsigned i=0;i<evec.size();i++) {
        t1=evec[i]->last();
        if (first) {
            max=t1;
            first=false;
        } else {
            if (max<t1) max=t1;
        }
    }
    return max;
}
int Session::count(MachineCode code)
{
    if (eventlist.find(code)==eventlist.end())
        return 0;
    vector<EventList *> & evec=eventlist[code];
    int sum=0;
    for (unsigned i=0;i<evec.size();i++) {
        sum+=evec[i]->count();
    }
    return sum;
}
double Session::sum(MachineCode mc)
{
    if (eventlist.find(mc)==eventlist.end()) return 0;

    vector<EventList *> & evec=eventlist[mc];
    double sum=0;
    for (unsigned i=0;i<evec.size();i++) {
        for (int j=0;j<evec[i]->count();j++) {
            sum+=evec[i]->data(j);
        }
    }
    return sum;
}
EventDataType Session::avg(MachineCode mc)
{
    if (eventlist.find(mc)==eventlist.end()) return 0;

    double cnt=count(mc);
    if (cnt==0) return 0;
    EventDataType val=sum(mc)/cnt;

    return val;
}

bool sortfunction (double i,double j) { return (i<j); }

EventDataType Session::percentile(MachineCode mc,double percent)
{
    if (eventlist.find(mc)==eventlist.end()) return 0;

    vector<EventDataType> array;

    vector<EventList *> & evec=eventlist[mc];
    for (unsigned i=0;i<evec.size();i++) {
        for (int j=0;j<evec[i]->count();j++) {
            array.push_back(evec[i]->data(j));
        }
    }
    std::sort(array.begin(),array.end(),sortfunction);
    int size=array.size();
    if (!size) return 0;
    double i=size*percent;
    double t;
    modf(i,&t);
    int j=t;

    //if (j>=size-1) return array[j];

    return array[j];
}

EventDataType Session::weighted_avg(MachineCode mc)
{
    if (eventlist.find(mc)==eventlist.end()) return 0;

    int cnt=0;

    bool first=true;
    qint64 last;
    int lastval=0,val;
    const int max_slots=4096;
    qint64 vtime[max_slots]={0};


    double mult=10.0;
    //if ((mc==CPAP_Pressure) || (mc==CPAP_EAP) ||  (mc==CPAP_IAP) | (mc==CPAP_PS)) {
    //    mult=10.0;
    //} else mult=10.0;

   // vector<Event *>::iterator i;

    vector<EventList *> & evec=eventlist[mc];
    for (unsigned i=0;i<evec.size();i++) {
        for (int j=0;j<evec[i]->count();j++) {
            val=evec[i]->data(j)*mult;
            if (first) {
                first=false;
            } else {
                int d=(evec[i]->time(j)-last)/1000L;
                if (lastval>max_slots) {
                    qWarning("max_slots to small in Session::weighted_avg()");
                    return 0;
                }
                vtime[lastval]+=d;
            }
            cnt++;
            last=evec[i]->time(j);
            lastval=val;
        }
    }

    qint64 total=0;
    for (int i=0; i<max_slots; i++) total+=vtime[i];
    //double hours=total.GetSeconds().GetLo()/3600.0;

    qint64 s0=0,s1=0,s2=0;
    if (total==0) return 0;
    for (int i=0; i<max_slots; i++) {
        if (vtime[i] > 0) {
            s0=vtime[i];
            s1+=i*s0;
            s2+=s0;
        }
    }
    double j=double(s1)/double(total);
    return j/mult;
}

