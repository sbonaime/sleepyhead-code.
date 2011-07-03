/********************************************************************
 SleepLib Machine Class Header
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
 *********************************************************************/

#ifndef MACHINE_H
#define MACHINE_H

#include <QString>
#include <QVariant>
#include <QDateTime>

#include <map>
#include <vector>
#include <list>

#include "tinyxml/tinyxml.h"
#include "SleepLib/preferences.h"

#include "SleepLib/machine_common.h"
#include "SleepLib/event.h"
#include "SleepLib/waveform.h"
#include "SleepLib/session.h"

#include "SleepLib/day.h"

class Day;
class Session;
class Profile;
class Machine;


class Machine
{
public:
    Machine(Profile *p,MachineID id=0);
    virtual ~Machine();
//	virtual bool Open(QString path){};

    bool Load();
    bool Save();
    bool SaveSession(Session *sess);
    bool Purge(int secret);

    map<QDate,Day *> day;
    map<SessionID,Session *> sessionlist;
    map<QString,QString> properties;

    Session * SessionExists(SessionID session);
    Day *AddSession(Session *s,Profile *p);

    void SetClass(QString t) {
        m_class=t;
    };
    void SetType(MachineType t) {
        m_type=t;
    };
    const QString & GetClass() {
        return m_class;
    };
    const MachineType & GetType() {
        return m_type;
    };
    const QString hexid() {
        QString s;
        s.sprintf("%08lx",m_id);
        return s;
    };
    SessionID CreateSessionID() { return highest_sessionid+1; };
    const MachineID & id() { return m_id; };
    const QDate & FirstDay() { return firstday; };
    const QDate & LastDay() { return lastday; };

protected:
    QDate firstday,lastday;
    SessionID highest_sessionid;
    MachineID m_id;
    QString m_class;
    MachineType m_type;
    QString m_path;
    Profile *profile;
    bool changed;
    bool firstsession;
};




class CPAP:public Machine
{
public:
    CPAP(Profile *p,MachineID id=0);
    virtual ~CPAP();
   // map<MachineCode,wxColour> FlagColours;
   // map<MachineCode,FlagType> FlagTypes;
    //list<MachineCode> SleepFlags;
};

class Oximeter:public Machine
{
public:
    Oximeter(Profile *p,MachineID id=0);
    virtual ~Oximeter();
protected:
};

class SleepStage:public Machine
{
public:
    SleepStage(Profile *p,MachineID id=0);
    virtual ~SleepStage();
protected:
};


#endif // MACHINE_H

