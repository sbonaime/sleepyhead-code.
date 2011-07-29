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
#include "SleepLib/event.h"
class EventList;
class Machine;

class Session
{
public:
    Session(Machine *,SessionID);
    virtual ~Session();

    bool Store(QString path);
    bool StoreSummary(QString filename);
    bool StoreEvents(QString filename);
    //bool Load(QString path);
    bool LoadSummary(QString filename);
    bool LoadEvents(QString filename);

    bool OpenEvents();
    void TrashEvents();

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

    map<MachineCode,QVariant> summary;
    void SetChanged(bool val) {
        s_changed=val;
        s_events_loaded=val; // dirty hack putting this here
    };
    bool IsChanged() {
        return s_changed;
    };

    map<MachineCode,vector<EventList *> > eventlist;

    bool IsLoneSession() { return s_lonesession; }
    void SetLoneSession(bool b) { s_lonesession=b; }
    void SetEventFile(QString & filename) { s_eventfile=filename; }

    inline void UpdateFirst(qint64 v) { if (!s_first) s_first=v; else if (s_first>v) s_first=v; }
    inline void UpdateLast(qint64 v) { if (!s_last) s_last=v; else if (s_last<v) s_last=v; }

    EventDataType min(MachineCode code);
    EventDataType max(MachineCode code);
    qint64 first(MachineCode code);
    qint64 last(MachineCode code);
    int count(MachineCode code);
    double sum(MachineCode mc);
    EventDataType avg(MachineCode mc);
    EventDataType weighted_avg(MachineCode mc);
    EventDataType percentile(MachineCode mc,double percentile);

protected:
    SessionID s_session;

    Machine *s_machine;
    qint64 s_first;
    qint64 s_last;
    bool s_changed;
    bool s_lonesession;
    bool _first_session;

    bool s_events_loaded;
    QString s_eventfile;
};


#endif // SESSION_H
