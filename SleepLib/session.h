/********************************************************************
 SleepLib Session Header
 This stuff contains the base calculation smarts
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*********************************************************************/

#ifndef SESSION_H
#define SESSION_H

#include <QDebug>
#include "SleepLib/machine.h"

class Machine;
class Session
{
public:
    Session(Machine *,SessionID);
    virtual ~Session();

    void AddEvent(Event *e);
    void AddWaveform(Waveform *w);

    bool Store(QString path);
    bool StoreSummary(QString filename);
    bool StoreEvents(QString filename);
    bool StoreWaveforms(QString filename);
    //bool Load(QString path);
    bool LoadSummary(QString filename);
    bool LoadEvents(QString filename);
    bool LoadWaveforms(QString filename);

    bool OpenEvents();
    bool OpenWaveforms();
    void TrashEvents();
    void TrashWaveforms();

    const SessionID & session() {
        return s_session;
    };
    qint64 first() {
        return s_first;
    };
    qint64 last() {
        return s_last;
    };
    void SetSessionID(SessionID s) {
        s_session=s;
    };
    void set_first(qint64 d) {
        if (!s_first) s_first=d;
        else if (d<s_first) s_first=d;
    };
    void set_last(qint64 d) {
        if (d<=s_first) {
            qWarning() << "Session::set_last() d<=s_first";
            return;
        }
        if (!s_last) s_last=d;
        else if (s_last<d) s_last=d;
    };

    double hours() {
        double t=(s_last-s_first)/3600000.0;
        return t;
    };
    int count_events(MachineCode mc) {
        if (events.find(mc)==events.end()) return 0;
        return events[mc].size();
    };
    double min_event_field(MachineCode mc,int field);
    double max_event_field(MachineCode mc,int field);
    double sum_event_field(MachineCode mc,int field);
    double avg_event_field(MachineCode mc,int field);
    double weighted_avg_event_field(MachineCode mc,int field);
    double percentile(MachineCode mc,int field,double percentile);

    map<MachineCode,QVariant> summary;
    void SetChanged(bool val) {
        s_changed=val;
        s_events_loaded=val; // dirty hack putting this here
        s_waves_loaded=val;
    };
    bool IsChanged() {
        return s_changed;
    };
    map<MachineCode,vector<Event *> > events;
    map<MachineCode,vector<Waveform *> > waveforms;

    bool IsLoneSession() { return s_lonesession; };
    void SetLoneSession(bool b) { s_lonesession=b; };
    void SetEventFile(QString & filename) { s_eventfile=filename; };
    void SetWaveFile(QString & filename) { s_wavefile=filename; };

protected:
    SessionID s_session;

    Machine *s_machine;
    qint64 s_first;
    qint64 s_last;
    bool s_changed;
    bool s_lonesession;
    bool _first_session;

    bool s_events_loaded;
    bool s_waves_loaded;
    QString s_eventfile;
    QString s_wavefile;
};


#endif // SESSION_H
