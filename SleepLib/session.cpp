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
    s_waves_loaded=false;
    _first_session=true;

    s_first=s_last=0;
    s_wavefile="";
    s_eventfile="";
}
Session::~Session()
{
    TrashEvents();
    TrashWaveforms();
}
double Session::min_event_field(MachineCode mc,int field)
{
    if (events.find(mc)==events.end()) return 0;

    bool first=true;
    double min=0;
    vector<Event *>::iterator i;
    for (i=events[mc].begin(); i!=events[mc].end(); i++) {
        assert(field<(*i)->e_fields);
        if (first) {
            first=false;
            min=(*(*i))[field];
        } else {
            if (min>(*(*i))[field]) min=(*(*i))[field];
        }
    }
    return min;
}
double Session::max_event_field(MachineCode mc,int field)
{
    if (events.find(mc)==events.end()) return 0;

    bool first=true;
    double max=0;
    vector<Event *>::iterator i;
    for (i=events[mc].begin(); i!=events[mc].end(); i++) {
        assert(field<(*i)->e_fields);
        if (first) {
            first=false;
            max=(*(*i))[field];
        } else {
            if (max<(*(*i))[field]) max=(*(*i))[field];
        }
    }
    return max;
}

double Session::sum_event_field(MachineCode mc,int field)
{
    if (events.find(mc)==events.end()) return 0;

    double sum=0;
    vector<Event *>::iterator i;

    for (i=events[mc].begin(); i!=events[mc].end(); i++) {
        assert(field<(*i)->e_fields);
        sum+=(*(*i))[field];
    }
    return sum;
}
double Session::avg_event_field(MachineCode mc,int field)
{
    if (events.find(mc)==events.end()) return 0;

    double sum=0;
    int cnt=0;
    vector<Event *>::iterator i;

    for (i=events[mc].begin(); i!=events[mc].end(); i++) {
        assert(field<(*i)->e_fields);
        sum+=(*(*i))[field];
        cnt++;
    }

    return sum/cnt;
}

bool sortfunction (double i,double j) { return (i<j); }

double Session::percentile(MachineCode mc,int field,double percent)
{
    if (events.find(mc)==events.end()) return 0;

    vector<EventDataType> array;

    vector<Event *>::iterator e;

    for (e=events[mc].begin(); e!=events[mc].end(); e++) {
        Event & ev = *(*e);
        array.push_back(ev[field]);
    }
    std::sort(array.begin(),array.end(),sortfunction);
    int size=array.size();
    double i=size*percent;
    double t;
    double q=modf(i,&t);
    int j=t;
    if (j>=size-1) return array[j];

    double a=array[j-1];
    double b=array[j];
    if (a==b) return a;
    //double c=(b-a);
    //double d=c*q;
    return array[j]+q;
}

double Session::weighted_avg_event_field(MachineCode mc,int field)
{
    if (events.find(mc)==events.end()) return 0;

    int cnt=0;

    bool first=true;
    qint64 last;
    int lastval=0,val;
    const int max_slots=2600;
    qint64 vtime[max_slots]={0};


    double mult;
    if ((mc==CPAP_Pressure) || (mc==CPAP_EAP) ||  (mc==CPAP_IAP) | (mc==CPAP_PS)) {
        mult=10.0;

    } else mult=10.0;

    vector<Event *>::iterator i;

    for (i=events[mc].begin(); i!=events[mc].end(); i++) {
        Event & e =(*(*i));
        val=e[field]*mult;
        assert(field<e.e_fields);
        if (first) {
            first=false;
        } else {
            int d=(e.e_time-last)/1000L;
            if (lastval>max_slots) {
                qWarning("max_slots to small in Session::weighted_avg_event_fied()");
                return 0;
            }

            vtime[lastval]+=d;

        }
        cnt++;
        last=e.e_time;
        lastval=val;
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

void Session::AddEvent(Event * e)
{
    events[e->code()].push_back(e);
    if (s_first) {
        if (e->time()<s_first) s_first=e->time();
        if (e->time()>s_last) s_last=e->time();
    } else {
        s_first=s_last=e->time();
        //_first_session=false;
    }
    //qDebug((e->time().toString("yyyy-MM-dd HH:mm:ss")+" %04i %02i").toLatin1(),e->code(),e->fields());
}
void Session::AddWaveform(Waveform *w)
{
    waveforms[w->code()].push_back(w);
    if (!s_last) {
        if (w->start()<s_first) s_first=w->start();
        if (w->end()>s_last) s_last=w->end();
    } else {
        s_first=w->start();
        s_last=w->end();
    }

    // Could parse the flow data for Breaths per minute info here..
}
void Session::TrashEvents()
// Trash this sessions Events and release memory.
{
    map<MachineCode,vector<Event *> >::iterator i;
    vector<Event *>:: iterator j;
    for (i=events.begin(); i!=events.end(); i++) {
        for (j=i->second.begin(); j!=i->second.end(); j++) {
            delete *j;
        }
    }
    events.clear();
}
void Session::TrashWaveforms()
// Trash this sessions Waveforms and release memory.
{
    map<MachineCode,vector<Waveform *> >::iterator i;
    vector<Waveform *>:: iterator j;
    for (i=waveforms.begin(); i!=waveforms.end(); i++) {
        for (j=i->second.begin(); j!=i->second.end(); j++) {
            delete *j;
        }
    }
    waveforms.clear();
}


//const int max_pack_size=128;
bool Session::OpenEvents() {
    if(s_events_loaded)
        return true;
    bool b;
    b=LoadEvents(s_eventfile);
    if (!b) {
        qWarning() << "Error Unkpacking Events" << s_eventfile;
    }

    s_events_loaded=b;
    return b;
};
bool Session::OpenWaveforms() {
    if (s_waves_loaded)
        return true;
    bool b;
    b=LoadWaveforms(s_wavefile);
    if (!b) {
        qWarning() << "Error Unkpacking Wavefile" << s_wavefile;
    }
    s_waves_loaded=b;
    return b;
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
    //qDebug(("Storing Session: "+base).toLatin1());
    bool a,b,c;
    a=StoreSummary(base+".000"); // if actually has events
    if (events.size()>0) b=StoreEvents(base+".001");
    if (waveforms.size()>0) c=StoreWaveforms(base+".002");
    if (a) {
        s_changed=false;
    }
    return a;
}

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

    out << (quint32)magic;
    out << (quint32)s_machine->id();
    out << (quint32)s_session;
    out << (quint16)0 << (quint16)0;

    quint32 starttime=s_first/1000L;
    quint32 duration=(s_last-s_first)/1000L;

    out << (quint32)starttime;  // Session Start Time
    out << (quint32)duration;   // Duration of sesion in seconds.
    out << (quint16)summary.size();

    map<MachineCode,MCDataType> mctype;

    // First output the Machine Code and type for each summary record
    map<MachineCode,QVariant>::iterator i;
    for (i=summary.begin(); i!=summary.end(); i++) {
        MachineCode mc=i->first;

        QVariant::Type type=i->second.type(); // Urkk.. this is a mess.
        if (type==QVariant::Bool) {
            mctype[mc]=MC_bool;
        } else if (type==QVariant::Int) {
            mctype[mc]=MC_int;
        } else if (type==QVariant::LongLong) {
            mctype[mc]=MC_long;
        } else if (type==QVariant::Double) {
            mctype[mc]=MC_double;
        } else if (type==QVariant::String) {
            mctype[mc]=MC_string;
        } else if (type==QVariant::DateTime) {
            mctype[mc]=MC_datetime;
        } else {
            QString t=i->second.typeToName(type);
            qWarning() << "Error in Session->StoreSummary: Can't pack variant type " << t;
            return false;
            //exit(1);
        }
        out << (qint16)mc;
        out << (qint8)mctype[mc];
    }
    // Then dump out the actual data, according to format.
    for (i=summary.begin(); i!=summary.end(); i++) {
        MachineCode mc=i->first;
        if (mctype[mc]==MC_bool) {
            out << i->second.toBool();
        } else if (mctype[mc]==MC_int) {
            out << (qint32)i->second.toInt();
        } else if (mctype[mc]==MC_long) {
            out << (qint64)i->second.toLongLong();
        } else if (mctype[mc]==MC_double) {
            out << (double)i->second.toDouble();
        } else if (mctype[mc]==MC_string) {
            out << i->second.toString();
        } else if (mctype[mc]==MC_datetime) {
            out << i->second.toDateTime();
        }
    }
    file.close();
    return true;
}

bool Session::LoadSummary(QString filename)
{
    if (filename.isEmpty()) return false;
    //qDebug(("Loading Summary "+filename).toLatin1());

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Couldn't open file" << filename;
        return false;
    }

    QDataStream in(&file);
    in.setVersion(QDataStream::Qt_4_6);

    quint64 t64;
    quint32 t32;
    quint16 t16;
    quint8 t8;

    qint16 sumsize;
    map<MachineCode,MCDataType> mctype;
    vector<MachineCode> mcorder;
    in >> t32;
    if (t32!=magic) {
        qDebug() << "Wrong magic number in " << filename;
        return false;
    }

    in >> t32;     // MachineID (dont need this result)

    in >> t32;     // Sessionid;
    s_session=t32;

    in >> t16;      // File Type
    if (t16!=0) {
        qDebug() << "Wrong file type"; //wrong file type
        return false;
    }

    in >> t16;      // File Version
    // dont care yet

    in >> t32;      // Start time
    s_first=qint64(t32)*1000L;

    in >> t32;       // Duration // (16bit==Limited to 18 hours)
    s_last=s_first+qint64(t32)*1000L;

    in >> sumsize;  // Summary size (number of Machine Code lists)

    for (int i=0; i<sumsize; i++) {
        in >> t16;      // Machine Code
        MachineCode mc=(MachineCode)t16;
        in >> t8;       // Data Type
        mctype[mc]=(MCDataType)t8;
        mcorder.push_back(mc);
    }

    for (int i=0; i<sumsize; i++) {					// Load each summary entry according to type
        MachineCode mc=mcorder[i];
        if (mctype[mc]==MC_bool) {
            bool b;
            in >> b;
            summary[mc]=b;
        } else if (mctype[mc]==MC_int) {
            in >> t32;
            summary[mc]=(qint32)t32;
        } else if (mctype[mc]==MC_long) {
            in >> t64;
            summary[mc]=(qint64)t64;
        } else if (mctype[mc]==MC_double) {
            double dl;
            in >> dl;
            summary[mc]=(double)dl;
        } else if (mctype[mc]==MC_string) {
            QString s;
            in >> s;
            summary[mc]=s;
        } else if (mctype[mc]==MC_datetime) {
            QDateTime dt;
            in >> dt;
            summary[mc]=dt;
        }

    }
    return true;
}

bool Session::StoreEvents(QString filename)
{
    QFile file(filename);
    file.open(QIODevice::WriteOnly);

    QDataStream out(&file);
    out.setVersion(QDataStream::Qt_4_6);

    out << (quint32)magic; 			// Magic Number
    out << (quint32)s_machine->id();  // Machine ID
    out << (quint32)s_session;		// This session's ID
    out << (quint16)1;				// File type 1 == Event
    out << (quint16)0;				// File Version

    quint32 starttime=s_first/1000L;
    quint32 duration=(s_last-s_first)/1000L;

    out << starttime;
    out << duration;

    out << (qint16)events.size(); // Number of event categories


    map<MachineCode,vector<Event *> >::iterator i;
    vector<Event *>::iterator j;

    for (i=events.begin(); i!=events.end(); i++) {
        out << (qint16)i->first; // MachineID
        out << (qint16)i->second.size(); // count of events in this category
        j=i->second.begin();
        out << (qint8)(*j)->fields(); 	// number of data fields in this event type
    }
    bool first;
    EventDataType tf;
    qint64 last=0,eventtime,delta;

    for (i=events.begin(); i!=events.end(); i++) {
        first=true;
        for (j=i->second.begin(); j!=i->second.end(); j++) {
            eventtime=(*j)->time();
            if (first) {
                out << (*j)->time();
                first=false;
            } else {
                delta=eventtime-last;
                if (delta>0xffffffff) {
                    qDebug("StoreEvent: Delta too big.. needed to use bigger value");
                    return false;
                }
                out << (quint32)delta;
            }
            for (int k=0; k<(*j)->fields(); k++) {
                tf=(*(*j))[k];
                out << tf;
            }
            last=eventtime;
        }
    }
    //file.close();
    return true;
}
bool Session::LoadEvents(QString filename)
{
    if (filename.isEmpty()) return false;

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Couldn't open file" << filename;
        return false;
    }
    QDataStream in(&file);
    in.setVersion(QDataStream::Qt_4_6);

    quint32 t32;
    quint16 t16;
    quint8 t8;
    //qint16 i16;

//    qint16 sumsize;

    in >> t32;      // Magic Number
    if (t32!=magic) {
        qWarning() << "Wrong Magic number in " << filename;
        return false;
    }
    in >> t32;      // MachineID

    in >> t32;      // Sessionid;
    s_session=t32;

    in >> t16;      // File Type
    if (t16!=1) {
        qDebug() << "Wrong File Type in " << filename;
        return false;
    }

    in >> t16;      // File Version
    // dont give a crap yet..

    in >> t32;      // Start time
    s_first=qint64(t32)*1000L;
    in >> t32;      // Duration // (16bit==Limited to 18 hours)
    s_last=s_first+qint64(t32)*1000L;

    qint16 evsize;
    in >> evsize;   // Summary size (number of Machine Code lists)

    map<MachineCode,qint16> mcsize;
    map<MachineCode,qint16> mcfields;
    vector<MachineCode> mcorder;

    MachineCode mc;
    for (int i=0; i<evsize; i++) {
        in >> t16;      // MachineCode
        mc=(MachineCode)t16;
        mcorder.push_back(mc);
        in >> t16;      // Count of this Event
        mcsize[mc]=t16;
        in >> t8;       // Type
        mcfields[mc]=t8;
    }

    for (int i=0; i<evsize; i++) {
        mc=mcorder[i];
        bool first=true;
        qint64 d;
        events[mc].clear();
        for (int e=0; e<mcsize[mc]; e++) {
            if (first) {
                in >> d;     // Timestamp
                first=false;
            } else {
                in >> t32;   // Time Delta
                d=d+t32;
            }
            EventDataType ED[max_number_event_fields];
            for (int c=0; c<mcfields[mc]; c++) {
                in >> ED[c];   // Data Fields in float format
            }
            Event *ev=new Event(d,mc,ED,mcfields[mc]);

            AddEvent(ev);
        }
    }
    return true;
}


bool Session::StoreWaveforms(QString filename)
{
    QFile file(filename);
    file.open(QIODevice::WriteOnly);

    QDataStream out(&file);
    out.setVersion(QDataStream::Qt_4_6);
    quint16 t16;

    out << (quint32)magic; 			// Magic Number
    out << (quint32)s_machine->id();  // Machine ID
    out << (quint32)s_session;		// This session's ID
    out << (quint16)2;				// File type 2 == Waveform
    out << (quint16)0;				// File Version

    quint32 starttime=s_first/1000L;
    quint32 duration=(s_last-s_first)/1000L;

    out << starttime;
    out << duration;

    out << (qint16)waveforms.size();	// Number of different waveforms

    map<MachineCode,vector<Waveform *> >::iterator i;
    vector<Waveform *>::iterator j;
    for (i=waveforms.begin(); i!=waveforms.end(); i++) {
        out << (quint16)i->first; 	// Machine Code
        t16=i->second.size();
        out << t16;                     // Number of (hopefully non-linear) waveform chunks

        for (j=i->second.begin(); j!=i->second.end(); j++) {

            Waveform &w=*(*j);
            // 64bit number..
            out << (qint64)w.start();           // Start time of first waveform chunk

            //qint32 samples;
            //double seconds;

            out << (qint32)w.samples(); // Total number of samples
            out << (qint64)w.duration();  // Total number of seconds
            out << (SampleFormat)w.min();
            out << (SampleFormat)w.max();
            out << (qint8)sizeof(SampleFormat);  // Bytes per sample
            out << (qint8)0; // signed.. all samples for now are signed 16bit.
            //t8=0; // 0=signed, 1=unsigned, 2=float

            // followed by sample data.
            for (int k=0; k<(*j)->samples(); k++) out << w[k];
        }
    }
    return true;
}


bool Session::LoadWaveforms(QString filename)
{
    if (filename.isEmpty()) return false;
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Couldn't open file" << filename;
        return false;
    }
    QDataStream in(&file);
    in.setVersion(QDataStream::Qt_4_6);

    quint32 t32;
    quint16 t16;
    quint8 t8;

    in >>t32;   // Magic Number
    if (t32!=magic) {
        qDebug() << "Wrong magic in " << filename;
        return false;
    }

    in >> t32;      // MachineID

    in >> t32;      // Sessionid;
    s_session=t32;

    in >> t16;      // File Type
    if (t16!=2) {
        qDebug() << "Wrong File Type in " << filename;
        return false;
    }

    in >> t16;      // File Version
    // dont give a crap yet..

    in >> t32;       // Start time
    s_first=qint64(t32)*1000;

    in >> t32;      // Duration // (16bit==Limited to 18 hours)
    s_last=s_first+qint64(t32)*1000;

    quint16 wvsize;
    in >> wvsize;   // Number of waveforms

    MachineCode mc;
    qint64 date;
    qint64 seconds;
    qint32 samples;
    int chunks;
    SampleFormat min,max;
    for (int i=0; i<wvsize; i++) {
        in >> t16;      // Machine Code
        mc=(MachineCode)t16;
        in >> t16;      // Number of waveform Chunks
        chunks=t16;

        for (int i=0; i<chunks; i++) {
            in >> date;     // Waveform DateTime
            in >> samples;  // Number of samples
            in >> seconds;  // Duration in seconds
            in >> min;      // Min value
            in >> max;      // Min value
            in >> t8;       // sample size in bytes
            in >> t8;       // Number of samples (?? what did I mean by this)

            SampleFormat *data=new SampleFormat [samples];


            for (int k=0; k<samples; k++) {
                in >> data[k];      // Individual Samples;
            }
            Waveform *w=new Waveform(date,mc,data,samples,seconds,min,max);
            AddWaveform(w);
        }
    }
    return true;
}
