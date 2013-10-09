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

#include "SleepLib/calcs.h"
#include "SleepLib/profiles.h"

using namespace std;


const quint16 filetype_summary=0;
const quint16 filetype_data=1;

// This is the uber important database version for SleepyHeads internal storage
// Increment this after stuffing with Session's save & load code.
const quint16 summary_version=11;
const quint16 events_version=10;

Session::Session(Machine * m,SessionID session)
{
    s_lonesession=false;
    if (!session) {
        session=m->CreateSessionID();
    }
    s_machine=m;
    s_session=session;
    s_changed=false;
    s_events_loaded=false;
    _first_session=true;
    s_enabled=-1;

    s_first=s_last=0;
    s_eventfile="";
    s_evchecksum_checked=false;

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
}

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
    } else { // who cares..
        //qDebug() << "Trying to save empty events file";
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

bool Session::StoreSummary(QString filename)
{

    QFile file(filename);
    file.open(QIODevice::WriteOnly);

    QDataStream out(&file);
    out.setVersion(QDataStream::Qt_4_6);
    out.setByteOrder(QDataStream::LittleEndian);

    out << (quint32)magic;
    out << (quint16)summary_version;
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
    out << m_min;
    out << m_max;
    out << m_cph;
    out << m_sph;
    out << m_firstchan;
    out << m_lastchan;

    out << m_valuesummary;
    out << m_timesummary;
    out << m_gain;

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

    //QHash<ChannelID,MCDataType> mctype;
    //QVector<ChannelID> mcorder;
    in >> t32;
    if (t32!=magic) {
        qDebug() << "Wrong magic number in " << filename;
        return false;
    }

    quint16 version;
    in >> version;      // DB Version
    if (version<6) {
        //throw OldDBVersion();
        qWarning() << "Old dbversion "<< version << "summary file.. Sorry, you need to purge and reimport";
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

    QHash<ChannelID,EventDataType> cruft;

    if (version<7) {
        // This code is deprecated.. just here incase anyone tries anything crazy...
        QHash<QString,QVariant> v1;
        in >> v1;
        settings.clear();
        ChannelID code;
        for (QHash<QString,QVariant>::iterator i=v1.begin();i!=v1.end();i++) {
            code=schema::channel[i.key()].id();
            settings[code]=i.value();
        }
        QHash<QString,int> zcnt;
        in >> zcnt;
        for (QHash<QString,int>::iterator i=zcnt.begin();i!=zcnt.end();i++) {
            code=schema::channel[i.key()].id();
            m_cnt[code]=i.value();
        }
        QHash<QString,double> zsum;
        in >> zsum;
        for (QHash<QString,double>::iterator i=zsum.begin();i!=zsum.end();i++) {
            code=schema::channel[i.key()].id();
            m_cnt[code]=i.value();
        }
        QHash<QString,EventDataType> ztmp;
        in >> ztmp; // avg
        for (QHash<QString,EventDataType>::iterator i=ztmp.begin();i!=ztmp.end();i++) {
            code=schema::channel[i.key()].id();
            m_avg[code]=i.value();
        }
        ztmp.clear();
        in >> ztmp; // wavg
        for (QHash<QString,EventDataType>::iterator i=ztmp.begin();i!=ztmp.end();i++) {
            code=schema::channel[i.key()].id();
            m_wavg[code]=i.value();
        }
        ztmp.clear();
        in >> ztmp; // 90p
        ztmp.clear();
        in >> ztmp; // min
        for (QHash<QString,EventDataType>::iterator i=ztmp.begin();i!=ztmp.end();i++) {
            code=schema::channel[i.key()].id();
            m_min[code]=i.value();
        }
        ztmp.clear();
        in >> ztmp; // max
        for (QHash<QString,EventDataType>::iterator i=ztmp.begin();i!=ztmp.end();i++) {
            code=schema::channel[i.key()].id();
            m_max[code]=i.value();
        }
        ztmp.clear();
        in >> ztmp; // cph
        for (QHash<QString,EventDataType>::iterator i=ztmp.begin();i!=ztmp.end();i++) {
            code=schema::channel[i.key()].id();
            m_cph[code]=i.value();
        }
        ztmp.clear();
        in >> ztmp; // sph
        for (QHash<QString,EventDataType>::iterator i=ztmp.begin();i!=ztmp.end();i++) {
            code=schema::channel[i.key()].id();
            m_sph[code]=i.value();
        }
        QHash<QString,quint64> ztim;
        in >> ztim; //firstchan
        for (QHash<QString,quint64>::iterator i=ztim.begin();i!=ztim.end();i++) {
            code=schema::channel[i.key()].id();
            m_firstchan[code]=i.value();
        }
        ztim.clear();
        in >> ztim; // lastchan
        for (QHash<QString,quint64>::iterator i=ztim.begin();i!=ztim.end();i++) {
            code=schema::channel[i.key()].id();
            m_lastchan[code]=i.value();
        }
        //SetChanged(true);
    } else {
        in >> settings;
        in >> m_cnt;
        in >> m_sum;
        in >> m_avg;
        in >> m_wavg;
        if (version < 11) {
            cruft.clear();
            in >> cruft; // 90%
            if (version >= 10) {
                cruft.clear();
                in >> cruft;// med
                cruft.clear();
                in >> cruft; //p95
            }
        }
        in >> m_min;
        in >> m_max;
        in >> m_cph;
        in >> m_sph;
        in >> m_firstchan;
        in >> m_lastchan;

        if (version >= 8) {
            in >> m_valuesummary;
            in >> m_timesummary;
            if (version >= 9) {
                in >> m_gain;
            }
        }


    }

    // not really a good idea to do this... should flag and do a reindex
    if (version < summary_version) {

        qDebug() << "Upgrading Summary file to version" << summary_version;
        UpdateSummaries();
        StoreSummary(filename);
    }
    return true;
}

const quint16 compress_method=1;

bool Session::StoreEvents(QString filename)
{
    QFile file(filename);
    file.open(QIODevice::WriteOnly);

    QByteArray headerbytes;
    QDataStream header(&headerbytes,QIODevice::WriteOnly);
    header.setVersion(QDataStream::Qt_4_6);
    header.setByteOrder(QDataStream::LittleEndian);

    header << (quint32)magic;      // New Magic Number
    header << (quint16)events_version; // File Version
    header << (quint16)filetype_data;  // File type 1 == Event
    header << (quint32)s_machine->id();// Machine Type
    header << (quint32)s_session;      // This session's ID
    header << s_first;
    header << s_last;

    quint16 compress=0;

    if (p_profile->session->compressSessionData())
        compress=compress_method;

    header << (quint16)compress;

    header << (quint16)s_machine->GetType();// Machine Type

    QByteArray databytes;
    QDataStream out(&databytes,QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_6);
    out.setByteOrder(QDataStream::LittleEndian);

    out << (qint16)eventlist.size(); // Number of event categories

    QHash<ChannelID,QVector<EventList *> >::iterator i;

    for (i=eventlist.begin(); i!=eventlist.end(); i++) {
        out << i.key(); // ChannelID
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
            out << e.Min();
            out << e.Max();
            out << e.dimension();
            out << e.hasSecondField();
            if (e.hasSecondField()) {
                out << e.min2();
                out << e.max2();
            }
        }
    }
    for (i=eventlist.begin(); i!=eventlist.end(); i++) {
        for (int j=0;j<i.value().size();j++) {
            EventList &e=*i.value()[j];
            // ****** This is assuming little endian ******

            // Store the raw event list data in EventStoreType (16bit short)
            EventStoreType *ptr=e.m_data.data();
            out.writeRawData((char *)ptr,e.count() << 1);

            //*** Don't delete these comments ***
//            for (quint32 c=0;c<e.count();c++) {
//                out << *ptr++;//e.raw(c);
//            }

            // Store the second field, only if there
            if (e.hasSecondField()) {
                ptr=e.m_data2.data();
                out.writeRawData((char *)ptr,e.count() << 1);
                //*** Don't delete these comments ***
//                for (quint32 c=0;c<e.count();c++) {
//                    out << *ptr++; //e.raw2(c);
//                }
            }

            // Store the time delta fields for non-waveform EventLists
            if (e.type()!=EVL_Waveform) {
                quint32 * tptr=e.m_time.data();
                out.writeRawData((char *)tptr,e.count() << 2);
                //*** Don't delete these comments ***
//                for (quint32 c=0;c<e.count();c++) {
//                    out << *tptr++; //e.getTime()[c];
//                }
            }
        }
    }

    qint32 datasize=databytes.size();

    // Checksum the _uncompressed_ data
    quint16 chk=0;
    if (compress) {
        // This checksum is hideously slow.. only using during compression, and not sure I should at all :-/
        chk=qChecksum(databytes.data(),databytes.size());
    }

    header << datasize;
    header << chk;

    QByteArray data;

    if (compress>0) {
        data=qCompress(databytes);
    } else {
        data=databytes;
    }

    file.write(headerbytes);
    file.write(data);
    file.close();
    return true;
}
bool Session::LoadEvents(QString filename)
{
    quint32 magicnum,machid,sessid;
    quint16 version,type,crc16,machtype,compmethod;
    quint8 t8;
    qint32 datasize;

    if (filename.isEmpty()) {
        qDebug() << "Session::LoadEvents() Filename is empty";
        return false;
    }

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Couldn't open file" << filename;
        return false;
    }

    QByteArray headerbytes=file.read(42);

    QDataStream header(headerbytes);
    header.setVersion(QDataStream::Qt_4_6);
    header.setByteOrder(QDataStream::LittleEndian);

    header >> magicnum;         // Magic Number (quint32)
    header >> version;          // Version (quint16)
    header >> type;             // File type (quint16)
    header >> machid;           // Machine ID (quint32)
    header >> sessid;           //(quint32)
    header >> s_first;          //(qint64)
    header >> s_last;           //(qint64)

    if (type!=filetype_data) {
        qDebug() << "Wrong File Type in " << filename;
        return false;
    }

    if (magicnum!=magic) {
         qWarning() << "Wrong Magic number in " << filename;
         return false;
    }

    if (version<6) {    // prior to version 6 is too old to deal with
        qDebug() << "Old File Version, can't open file";
        return false;
    }

    if (version<10) {
        file.seek(32);
    } else {
        header >> compmethod;   // Compression Method (quint16)
        header >> machtype;     // Machine Type (quint16)
        header >> datasize;     // Size of Uncompressed Data (quint32)
        header >> crc16;        // CRC16 of Uncompressed Data (quint16)
    }

    QByteArray databytes,temp=file.readAll();
    file.close();

    if (version>=10) {
        if (compmethod>0) {
            databytes=qUncompress(temp);
            if (!s_evchecksum_checked) {
                if (databytes.size()!=datasize) {
                    qDebug() << "File" << filename << "has returned wrong datasize";
                    return false;
                }
                quint16 crc=qChecksum(databytes.data(),databytes.size());
                if (crc!=crc16) {
                    qDebug() << "CRC Doesn't match in" << filename;
                    return false;
                }
                s_evchecksum_checked=true;
            }
        } else {
            databytes=temp;
        }
    } else databytes=temp;

    QDataStream in(databytes);
    in.setVersion(QDataStream::Qt_4_6);
    in.setByteOrder(QDataStream::LittleEndian);

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
    QString dim;
    for (int i=0;i<mcsize;i++) {
        if (version<8) {
            QString txt;
            in >> txt;
            code=schema::channel[txt].id();
        } else {
            in >> code;
        }
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
            in >> dim;
            bool second_field=false;
            if (version>=7) // version 7 added this field
                in >> second_field;

            EventList *elist=AddEventList(code,elt,gain,offset,mn,mx,rate,second_field);
            elist->setDimension(dim);

            //eventlist[code].push_back(elist);
            elist->m_count=evcount;
            elist->m_first=ts1;
            elist->m_last=ts2;

            if (second_field) {
                EventDataType min,max;
                in >> min;
                in >> max;
                elist->setMin2(min);
                elist->setMax2(max);
            }
        }
    }

    //EventStoreType t;
    //quint32 x;

    for (int i=0;i<mcsize;i++) {
        code=mcorder[i];
        size2=sizevec[i];
        for (int j=0;j<size2;j++) {
            EventList &evec=*eventlist[code][j];
            evec.m_data.resize(evec.m_count);
            EventStoreType *ptr=evec.m_data.data();

            // ****** This is assuming little endian ******

            in.readRawData((char *)ptr, evec.m_count << 1);
            //*** Don't delete these comments ***
//            for (quint32 c=0;c<evec.m_count;c++) {
//                in >> t;
//                *ptr++=t;
//            }
            if (evec.hasSecondField()) {
                evec.m_data2.resize(evec.m_count);
                ptr=evec.m_data2.data();

                in.readRawData((char *)ptr,evec.m_count << 1);
                //*** Don't delete these comments ***
//                for (quint32 c=0;c<evec.m_count;c++) {
//                    in >> t;
//                    *ptr++=t;
//                }
            }
            if (evec.type()!=EVL_Waveform) {
                evec.m_time.resize(evec.m_count);
                quint32 * tptr=evec.m_time.data();

                in.readRawData((char *)tptr,evec.m_count << 2);
                //*** Don't delete these comments ***
//                for (quint32 c=0;c<evec.m_count;c++) {
//                    in >> x;
//                    *tptr++=x;
//                }
            }
        }
    }

    if (version<events_version) {
        qDebug() << "Upgrading Events file" << filename << "to version" << events_version;
        UpdateSummaries();
        StoreEvents(filename);
    }
    return true;
}

void Session::destroyEvent(ChannelID code)
{
    QHash<ChannelID,QVector<EventList *> >::iterator it=eventlist.find(code);
    if (it!=eventlist.end()) {
        for (int i=0;i<it.value().size();i++) {
            delete it.value()[i];
        }
        eventlist.erase(it);
    }
    m_gain.erase(m_gain.find(code));
    m_firstchan.erase(m_firstchan.find(code));
    m_lastchan.erase(m_lastchan.find(code));
    m_sph.erase(m_sph.find(code));
    m_cph.erase(m_cph.find(code));
    m_min.erase(m_min.find(code));
    m_max.erase(m_max.find(code));
    m_avg.erase(m_avg.find(code));
    m_wavg.erase(m_wavg.find(code));
    m_sum.erase(m_sum.find(code));
    m_cnt.erase(m_cnt.find(code));
    m_valuesummary.erase(m_valuesummary.find(code));
    m_timesummary.erase(m_timesummary.find(code));
    // does not trash settings..
}

void Session::updateCountSummary(ChannelID code)
{
    QHash<ChannelID,QVector<EventList *> >::iterator ev=eventlist.find(code);
    if (ev==eventlist.end()) return;

    QHash<ChannelID,QHash<EventStoreType, EventStoreType> >::iterator vs=m_valuesummary.find(code);
    if (vs!=m_valuesummary.end()) // already calculated?
        return;

    QHash<EventStoreType, EventStoreType> valsum;
    QHash<EventStoreType, quint32> timesum;

    EventDataType raw,lastraw=0;
    qint64 start,time,lasttime=0;
    qint32 len,cnt;
    quint32 * tptr;
    EventStoreType * dptr, * eptr;
    for (int i=0;i<ev.value().size();i++) {
        EventList & e=*(ev.value()[i]);
        start=e.first();
        cnt=e.count();
        dptr=e.rawData();
        eptr=dptr+cnt;

        EventDataType rate=0;

        m_gain[code]=e.gain();

        if (e.type()==EVL_Event) {
            lastraw=*dptr++;
            tptr=e.rawTime();
            lasttime=start + *tptr++;
            // Event version

            for (;dptr < eptr; dptr++) {
                 time=start + *tptr++;
                 raw=*dptr;

                 valsum[raw]++;

                 // elapsed time in seconds since last event occurred
                 len=(time-lasttime) / 1000L;

                 timesum[lastraw]+=len;

                 lastraw=raw;
                 lasttime=time;
            }
        } else {
            // Waveform version, first just count
            for (;dptr < eptr; dptr++) {
                raw=*dptr;
                valsum[raw]++;
            }

            // Then process the list of values, time is simply (rate * count)
            rate=e.rate();
            EventDataType t;
            for (QHash<EventStoreType, EventStoreType>::iterator it=valsum.begin(); it!=valsum.end(); it++) {
                t=EventDataType(it.value())*rate;
                timesum[it.key()]+=t;
            }
        }
    }
    if (valsum.size()==0) return;

    m_valuesummary[code]=valsum;
    m_timesummary[code]=timesum;
}

void Session::UpdateSummaries()
{
    ChannelID id;
    QHash<ChannelID,QVector<EventList *> >::iterator c;
    calcAHIGraph(this);

    // Calculates RespRate and related waveforms (Tv, MV, Te, Ti) if missing
    calcRespRate(this);

    calcLeaks(this);
    calcSPO2Drop(this);
    calcPulseChange(this);

    for (c=eventlist.begin();c!=eventlist.end();c++) {
        id=c.key();
        if (schema::channel[id].type()==schema::DATA) {
            //sum(id); // avg calculates this and cnt.
            if (c.value().size()>0) {
                EventList * el=c.value()[0];
                EventDataType gain=el->gain();
                m_gain[id]=gain;
            }
            if (!((id==CPAP_FlowRate) || (id==CPAP_MaskPressureHi) || (id==CPAP_RespEvent) || (id==CPAP_MaskPressure)))
                updateCountSummary(id);

            Min(id);
            Max(id);
            count(id);
            last(id);
            first(id);
            if (((id==CPAP_FlowRate) || (id==CPAP_MaskPressureHi) || (id==CPAP_RespEvent) || (id==CPAP_MaskPressure)))
                continue;

            cph(id);
            sph(id);
            avg(id);
            wavg(id);
        }
    }
}

bool Session::SearchEvent(ChannelID code, qint64 time, qint64 dist)
{
    qint64 t,start;
    QHash<ChannelID,QVector<EventList *> >::iterator it;
    it=eventlist.find(code);
    quint32 * tptr;
    int cnt;
    //qint64 rate;
    if (it!=eventlist.end()) {
        for (int i=0;i<it.value().size();i++)  {
            EventList *el=it.value()[i];
//            rate=el->rate();
            cnt=el->count();

            // why would this be necessary???
            if (el->type()==EVL_Waveform) {
                qDebug() << "Called SearchEvent on a waveform object!";
                return false;
            } else {
                start=el->first();
                tptr=el->rawTime();
                for (int j=0;j<cnt;j++) {
                    t=start + *tptr++;
                    if (qAbs(time - t) < dist)
                        return true;
                }
            }
        }
    }
    return false;
}

bool Session::enabled()
{
    if (s_enabled>=0) {
        return s_enabled!=0;
    }
    if (!settings.contains(SESSION_ENABLED)) {
        bool b=true;
        settings[SESSION_ENABLED]=b;
        s_enabled=b ? 1 : 0;
        return b;
    }
    s_enabled=settings[SESSION_ENABLED].toBool() ? 1 : 0;
    return s_enabled;
}

void Session::setEnabled(bool b)
{
    settings[SESSION_ENABLED]=s_enabled=b;
    SetChanged(true);
}


EventDataType Session::Min(ChannelID id)
{
    QHash<ChannelID,EventDataType>::iterator i=m_min.find(id);
    if (i!=m_min.end())
        return i.value();

    QHash<ChannelID,QVector<EventList *> >::iterator j=eventlist.find(id);
    if (j==eventlist.end()) {
        m_min[id]=0;
        return 0;
    }
    QVector<EventList *> & evec=j.value();

    bool first=true;
    EventDataType min=0,t1;
    for (int i=0;i<evec.size();i++) {
        if (evec[i]->count()==0)
            continue;
        t1=evec[i]->Min();
        if (t1==0 && t1==evec[i]->Max()) continue;
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
EventDataType Session::Max(ChannelID id)
{
    QHash<ChannelID,EventDataType>::iterator i=m_max.find(id);
    if (i!=m_max.end())
        return i.value();

    QHash<ChannelID,QVector<EventList *> >::iterator j=eventlist.find(id);
    if (j==eventlist.end()) {
        m_max[id]=0;
        return 0;
    }
    QVector<EventList *> & evec=j.value();

    bool first=true;
    EventDataType max=0,t1;
    for (int i=0;i<evec.size();i++) {
        if (evec[i]->count()==0) continue;
        t1=evec[i]->Max();
        if (t1==0 && t1==evec[i]->Min()) continue;
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
    qint64 drift=qint64(PROFILE.cpap->clockDrift())*1000L;
    qint64 tmp;
    QHash<ChannelID,quint64>::iterator i=m_firstchan.find(id);
    if (i!=m_firstchan.end()) {
        tmp=i.value();
        if (s_machine->GetType()==MT_CPAP)
            tmp+=drift;
        return tmp;
    }

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
    if (s_machine->GetType()==MT_CPAP)
        min+=drift;
    return min;
}
qint64 Session::last(ChannelID id)
{
    qint64 drift=qint64(PROFILE.cpap->clockDrift())*1000L;
    qint64 tmp;
    QHash<ChannelID,quint64>::iterator i=m_lastchan.find(id);
    if (i!=m_lastchan.end()) {
        tmp=i.value();
        if (s_machine->GetType()==MT_CPAP)
            tmp+=drift;
        return tmp;
    }
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
    if (s_machine->GetType()==MT_CPAP)
        max+=drift;
    return max;
}
bool Session::channelDataExists(ChannelID id)
{
    if (s_events_loaded) {
        QHash<ChannelID,QVector<EventList *> >::iterator j=eventlist.find(id);
        if (j==eventlist.end())  // eventlist not loaded.
            return false;
        return true;
    } else {
        qDebug() << "Calling channelDataExists without open eventdata!";
    }
    return false;
}
bool Session::channelExists(ChannelID id)
{
    if (!enabled()) return false;
    if (s_events_loaded) {
        QHash<ChannelID,QVector<EventList *> >::iterator j=eventlist.find(id);
        if (j==eventlist.end())  // eventlist not loaded.
            return false;
    } else {
        QHash<ChannelID,int>::iterator q=m_cnt.find(id);
        if (q==m_cnt.end())
            return false;
        if (q.value()==0) return false;
    }
    return true;
}

int Session::rangeCount(ChannelID id, qint64 first,qint64 last)
{
    QHash<ChannelID,QVector<EventList *> >::iterator j=eventlist.find(id);
    if (j==eventlist.end()) {
        return 0;
    }
    QVector<EventList *> & evec=j.value();
    int sum=0,cnt;

    qint64 t,start;
    for (int i=0;i<evec.size();i++) {
        EventList & ev=*evec[i];
        if ((ev.last() < first) || (ev.first() > last))
            continue;

        if (ev.type() == EVL_Waveform) {
            qint64 et=last;
            if (et > ev.last())
                et=ev.last();

            qint64 st=first;
            if (st < ev.first())
                st=ev.first();

            t=(et - st) / ev.rate();
            sum+=t;
        } else {
            cnt=ev.count();
            start=ev.first();
            quint32 * tptr=ev.rawTime();
            quint32 * eptr=tptr+cnt;
            for (;tptr < eptr; tptr++) {
                t=start + *tptr;

                if (t >= first) {
                    if (t<=last) {
                        sum++;
                    } else break;
                }
            }
        }
    }
    return sum;
}
double Session::rangeSum(ChannelID id, qint64 first,qint64 last)
{
    QHash<ChannelID,QVector<EventList *> >::iterator j=eventlist.find(id);
    if (j==eventlist.end()) {
        return 0;
    }
    QVector<EventList *> & evec=j.value();
    double sum=0,gain;

    qint64 t,start;
    EventStoreType * dptr, * eptr;
    quint32 * tptr;
    int cnt,idx=0;

    qint64 rate;
    for (int i=0;i < evec.size();i++) {
        EventList & ev=*evec[i];
        if ((ev.last() < first) || (ev.first() > last))
            continue;
        start=ev.first();
        dptr=ev.rawData();
        cnt=ev.count();
        eptr=dptr+cnt;
        gain=ev.gain();
        rate=ev.rate();
        if (ev.type()==EVL_Waveform) {
            if (first > ev.first()) {
                // Skip the samples before first
                idx=(first - ev.first()) / rate;
            }

            dptr+=idx; //???? foggy.

            t=start;
            for (;dptr < eptr; dptr++) { //int j=idx;j<cnt;j++) {
                if (t <= last) {
                    sum+=EventDataType(*dptr) * gain;
                } else break;
                t+=rate;
            }
        } else {
            tptr=ev.rawTime();

            for (;dptr < eptr; dptr++) {
                t=start + *tptr++;
                if (t >= first) {
                    if (t <= last) {
                        sum+=EventDataType(*dptr) * gain;
                    } else break;
                }
            }
        }
    }
    return sum;
}
EventDataType Session::rangeMin(ChannelID id, qint64 first,qint64 last)
{
    QHash<ChannelID,QVector<EventList *> >::iterator j=eventlist.find(id);
    if (j==eventlist.end()) {
        return 0;
    }
    QVector<EventList *> & evec=j.value();
    EventDataType gain,v,min=999999999;

    qint64 t,start,rate;
    EventStoreType * dptr, * eptr;
    quint32 * tptr;
    int cnt,idx;

    for (int i=0;i<evec.size();i++) {
        EventList & ev=*evec[i];
        if ((ev.last() < first) || (ev.first() > last))
            continue;

        dptr=ev.rawData();
        start=ev.first();
        cnt=ev.count();
        eptr=dptr+cnt;
        gain=ev.gain();

        if (ev.type()==EVL_Waveform) {
            rate=ev.rate();
            t=start;
            idx=0;

            if (first > ev.first()) {
                // Skip the samples before first
                idx=(first - ev.first())/rate;
            }
            dptr+=idx;

            for (; dptr < eptr; dptr++) { //int j=idx;j<cnt;j++) {
                if (t<=last) {
                    v=EventDataType(*dptr) * gain;
                    if (v<min)
                        min=v;
                } else break;
                t+=rate;
            }
        } else {
            tptr=ev.rawTime();
            for (; dptr < eptr; dptr++) { //int j=0;j<cnt;j++) {
                t=start + *tptr++;
                if (t >= first) {
                    if (t <= last) {
                        v=EventDataType(*dptr) * gain;
                        if (v<min)
                            min=v;
                    } else break;
                }
            }
        }
    }
    return min;
}
EventDataType Session::rangeMax(ChannelID id, qint64 first,qint64 last)
{
    QHash<ChannelID,QVector<EventList *> >::iterator j=eventlist.find(id);
    if (j==eventlist.end()) {
        return 0;
    }
    QVector<EventList *> & evec=j.value();
    EventDataType gain,v,max=-999999999;

    qint64 t,start,rate;
    EventStoreType * dptr, * eptr;
    quint32 * tptr;
    int cnt,idx;

    for (int i=0;i<evec.size();i++) {
        EventList & ev=*evec[i];
        if ((ev.last() < first) || (ev.first() > last))
            continue;

        start=ev.first();
        dptr=ev.rawData();
        cnt=ev.count();
        eptr=dptr + cnt;
        gain=ev.gain();
        if (ev.type()==EVL_Waveform) {
            rate=ev.rate();
            t=start;
            idx=0;

            if (first > ev.first()) {
                // Skip the samples before first
                idx=(first - ev.first())/rate;
            }
            dptr+=idx;
            for (; dptr < eptr; dptr++) { //int j=idx;j<cnt;j++) {
                if (t<=last) {
                    v=EventDataType(*dptr) * gain;
                    if (v>max) max=v;
                } else break;
                t+=rate;
            }
        } else {
            tptr=ev.rawTime();
            for (;dptr < eptr; dptr++) {
                t=start + *tptr++;
                if (t>=first) {
                    if (t<=last) {
                        v=EventDataType(*dptr) * gain;
                        if (v>max) max=v;
                    } else break;
                }
            }
        }
    }
    return max;
}

int Session::count(ChannelID id)
{
    QHash<ChannelID,int>::iterator i=m_cnt.find(id);
    if (i!=m_cnt.end())
        return i.value();

    QHash<ChannelID,QVector<EventList *> >::iterator j=eventlist.find(id);
    if (j==eventlist.end()) {
        m_cnt[id]=0;
        return 0;
    }
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
    if (j==eventlist.end()) {
        m_sum[id]=0;
        return 0;
    }
    QVector<EventList *> & evec=j.value();

    double gain,sum=0;
    EventStoreType * dptr, * eptr;
    int cnt;
    for (int i=0;i<evec.size();i++) {
        EventList & ev=*(evec[i]);
        gain=ev.gain();
        cnt=ev.count();
        dptr=ev.rawData();
        eptr=dptr+cnt;

        for (; dptr < eptr; dptr++) {
            sum+=double(*dptr) * gain;
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
    if (j==eventlist.end()) {
        m_avg[id]=0;
        return 0;
    }
    QVector<EventList *> & evec=j.value();

    double val=0,gain;
    int cnt=0;
    EventStoreType * dptr, * eptr;
    for (int i=0;i<evec.size();i++) {
        EventList & ev=*(evec[i]);
        dptr=ev.rawData();
        gain=ev.gain();
        cnt=ev.count();
        eptr=dptr+cnt;

        for (; dptr < eptr; dptr++) {
            val+=double(*dptr) * gain;
        }
    }

    if (cnt>0) { // Shouldn't really happen.. Should aways contain data
        val/=double(cnt);
    }
    m_avg[id]=val;
    return val;
}
EventDataType Session::cph(ChannelID id) // count per hour
{
    QHash<ChannelID,EventDataType>::iterator i=m_cph.find(id);
    if (i!=m_cph.end())
        return i.value();

    EventDataType val=count(id);
    val/=hours();

    m_cph[id]=val;
    return val;
}
EventDataType Session::sph(ChannelID id) // sum per hour, assuming id is a time field in seconds
{
    QHash<ChannelID,EventDataType>::iterator i=m_sph.find(id);
    if (i!=m_sph.end())
        return i.value();

    EventDataType val=sum(id)/3600.0;
    val=100.0 / hours() * val;
    m_sph[id]=val;
    return val;
}

bool sortfunction (EventStoreType i,EventStoreType j) { return (i<j); }

EventDataType Session::percentile(ChannelID id,EventDataType percent)
{
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

    EventStoreType * dptr, * sptr, *eptr;

    int tt=0,cnt=0;
    for (int i=0;i<size;i++) {
        EventList & ev=*evec[i];
        cnt=ev.count();
        tt+=cnt;
    }
    array.resize(tt);

    for (int i=0;i<size;i++) {
        EventList & ev=*evec[i];
        sptr=ev.rawData();
        dptr=array.data();

        eptr=sptr+cnt;
        for (; sptr < eptr; sptr++) {
            *dptr++ = * sptr;
        }
    }

    int n=array.size()*percent;
    if (n>array.size()-1) n--;
    nth_element(array.begin(), array.begin()+n, array.end());

    // slack, no averaging.. fixme if this function is ever used..
    return array[n] * gain;
}

EventDataType Session::wavg(ChannelID id)
{
    QHash<EventStoreType,quint32> vtime;
    QHash<ChannelID,EventDataType>::iterator i=m_wavg.find(id);
    if (i!=m_wavg.end())
        return i.value();

    updateCountSummary(id);

    QHash<ChannelID,QHash<EventStoreType, quint32> >::iterator j2=m_timesummary.find(id);

    if (j2==m_timesummary.end())
        return 0;

    QHash<EventStoreType, quint32> & timesum=j2.value();

    if (!m_gain.contains(id))
        return 0;

    double s0=0,s1=0,s2;

    EventDataType val, gain=m_gain[id];
    for (QHash<EventStoreType, quint32>::iterator vi=timesum.begin();vi!=timesum.end();vi++) {
        val=vi.key() * gain;
        s2=vi.value();
        s0+=s2;
        s1+=val * s2;
    }
    if (s0>0) {
        val=s1/s0;
    } else val=0;

    m_wavg[id]=val;
    return val;
}

EventList * Session::AddEventList(ChannelID code, EventListType et,EventDataType gain,EventDataType offset,EventDataType min, EventDataType max,EventDataType rate,bool second_field)
{
    schema::Channel * channel=&schema::channel[code];
    if (!channel) {
        qWarning() << "Channel" << code << "does not exist!";
        //return NULL;
    }
    EventList * el=new EventList(et,gain,offset,min,max,rate,second_field);

    eventlist[code].push_back(el);
    //s_machine->registerChannel(chan);
    return el;
}
void Session::offsetSession(qint64 offset)
{
    //qDebug() << "Session starts" << QDateTime::fromTime_t(s_first/1000).toString("yyyy-MM-dd HH:mm:ss");
    s_first+=offset;
    s_last+=offset;
    QHash<ChannelID,quint64>::iterator it;

    for (it=m_firstchan.begin();it!=m_firstchan.end();it++) {
        if (it.value()>0)
            it.value()+=offset;
    }
    for (it=m_lastchan.begin();it!=m_lastchan.end();it++) {
        if (it.value()>0)
            it.value()+=offset;
    }

    QHash<ChannelID,QVector<EventList *> >::iterator i;
    for (i=eventlist.begin();i!=eventlist.end();i++) {
        for (int j=0;j<i.value().size();j++) {
            EventList *e=i.value()[j];

            e->setFirst(e->first()+offset);
            e->setLast(e->last()+offset);
        }
    }
    qDebug() << "Session now starts" << QDateTime::fromTime_t(s_first/1000).toString("yyyy-MM-dd HH:mm:ss");

}

qint64 Session::first() {
    qint64 start=s_first;
    if (s_machine->GetType()==MT_CPAP)
        start+=qint64(PROFILE.cpap->clockDrift())*1000L;
    return start;
}

qint64 Session::last() {
    qint64 last=s_last;
    if (s_machine->GetType()==MT_CPAP)
        last+=qint64(PROFILE.cpap->clockDrift())*1000L;
    return last;
}
