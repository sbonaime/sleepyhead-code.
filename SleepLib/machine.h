/*
 SleepLib Machine Class Header
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
 */

#ifndef MACHINE_H
#define MACHINE_H

#include <QString>
#include <QVariant>
#include <QDateTime>
#include <QThread>
#include <QMutex>
#include <QSemaphore>

#include <QHash>
#include <QVector>
#include <list>

#include "SleepLib/preferences.h"

#include "SleepLib/machine_common.h"
#include "SleepLib/event.h"
#include "SleepLib/session.h"

#include "SleepLib/day.h"

class Day;
class Session;
class Profile;
class Machine;

class SaveThread:public QThread
{
    Q_OBJECT
public:
    SaveThread(Machine *m,QString p) { machine=m; path=p; }
    static void msleep(unsigned long msecs) { QThread::msleep(msecs); }
    virtual void run();
protected:
    Machine *machine;
    QString path;
signals:
    void UpdateProgress(int i);
};


class Machine
{
public:
    Machine(Profile *p,MachineID id=0);
    virtual ~Machine();

    bool Load();
    bool Save();
    bool SaveSession(Session *sess);
    bool Purge(int secret);

    QMap<QDate,Day *> day;
    QHash<SessionID,Session *> sessionlist;
    QHash<QString,QString> properties;

    Session * SessionExists(SessionID session);
    Day *AddSession(Session *s,Profile *p);

    void SetClass(QString t) { m_class=t; }
    void SetType(MachineType t) { m_type=t; }
    const QString & GetClass() { return m_class; }
    const MachineType & GetType() { return m_type; }
    QString hexid() { return QString().sprintf("%08lx",m_id); }
    SessionID CreateSessionID() { return highest_sessionid+1; }
    const MachineID & id() { return m_id; }
    const QDate & FirstDay() { return firstday; }
    const QDate & LastDay() { return lastday; }

    Session *popSaveList();
    QList<Session *> m_savelist;
    volatile int savelistCnt;
    int savelistSize;
    QMutex savelistMutex;
    QSemaphore *savelistSem;

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

qint64 timezoneOffset();

#endif // MACHINE_H

