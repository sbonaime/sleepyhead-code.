/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 *
 * SleepLib Machine Class Header
 *
 * Copyright (c) 2011-2014 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#ifndef MACHINE_H
#define MACHINE_H


#include <QString>
#include <QVariant>
#include <QDateTime>
#include <QRunnable>
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

/*! \class SaveThread
    \brief This class is used in the multithreaded save code.. It accelerates the indexing of summary data.
    */
class SaveThread: public QThread
{
    Q_OBJECT
  public:
    SaveThread(Machine *m, QString p) { machine = m; path = p; }

    //! \brief Static millisecond sleep function.. Can be used from anywhere
    static void msleep(unsigned long msecs) { QThread::msleep(msecs); }

    //! \brief Start Save processing thread running
    virtual void run();
  protected:
    Machine *machine;
    QString path;
  signals:
    //! \brief Signal sent to update the Progress Bar
    void UpdateProgress(int i);
};

class ImportTask:public QRunnable
{
public:
    explicit ImportTask() {}
    virtual ~ImportTask() {}
    virtual void run() {}
};

/*! \class Machine
    \brief This Machine class is the Heart of SleepyLib, representing a single Machine and holding it's data

    */
class Machine
{
    friend class SaveThread;

  public:
    /*! \fn Machine(MachineID id=0);
        \brief Constructs a Machine object with MachineID id

        If supplied MachineID is zero, it will generate a new unused random one.
        */
    Machine(MachineID id = 0);
    virtual ~Machine();

    //! \brief Load all Machine summary data
    bool Load();
    //! \brief Save all Sessions where changed bit is set.
    bool Save();

    //! \brief Save individual session
    bool SaveSession(Session *sess);

    //! \brief Deletes the crud out of all machine data in the SleepLib database
    bool Purge(int secret);

    //! \brief Contains a secondary index of day data, containing just this machines sessions
    QMap<QDate, Day *> day;

    //! \brief Contains all sessions for this machine, indexed by SessionID
    QHash<SessionID, Session *> sessionlist;

    //! \brief List of text machine properties, like brand, model, etc...
    QHash<QString, QString> properties;

    //! \brief Returns a pointer to a valid Session object if SessionID exists
    Session *SessionExists(SessionID session);

    //! \brief Adds the session to this machine object, and the Master Profile list. (used during load)
    bool AddSession(Session *s);

    //! \brief Find the date this session belongs in, according to profile settings
    QDate pickDate(qint64 start);

    //! \brief Sets the Class of machine (Used to reference the particular loader that created it)
    void SetClass(QString t) { m_class = t; }

    //! \brief Sets the type of machine, according to MachineType enum
    void SetType(MachineType t) { m_type = t; }

    //! \brief Returns the Class of machine (Used to reference the particular loader that created it)
    const QString &GetClass() { return m_class; }

    //! \brief Returns the type of machine, according to MachineType enum
    const MachineType &GetType() const { return m_type; }

    //! \brief Returns the machineID as a lower case hexadecimal string
    QString hexid() { return QString().sprintf("%08lx", m_id); }


    //! \brief Unused, increments the most recent sessionID
    SessionID CreateSessionID() { return highest_sessionid + 1; }

    //! \brief Returns this objects MachineID
    const MachineID &id() { return m_id; }

    //! \brief Returns the date of the first loaded Session
    const QDate &FirstDay() { return firstday; }

    //! \brief Returns the date of the most recent loaded Session
    const QDate &LastDay() { return lastday; }

    //! \brief Add a new task to the multithreaded save code
    void queSaveList(Session * sess);

    //! \brief Grab the next task in the multithreaded save code
    Session *popSaveList();

    //! \brief Start the save threads which handle indexing, file storage and waveform processing
    void StartSaveThreads();

    //! \brief Finish the save threads and safely close them
    void FinishSaveThreads();

    //! \brief The list of sessions that need saving (for multithreaded save code)
    QList<Session *> m_savelist;

    //yuck
    QVector<SaveThread *>thread;
    volatile int savelistCnt;
    int savelistSize;
    QMutex listMutex;
    QSemaphore *savelistSem;

    void lockSaveMutex() { listMutex.lock(); }
    void unlockSaveMutex() { listMutex.unlock(); }
    void skipSaveTask() { lockSaveMutex(); m_donetasks++; unlockSaveMutex(); }

    void clearSkipped() { skipped_sessions = 0; }
    int skippedSessions() { return skipped_sessions; }

    inline int totalTasks() { return m_totaltasks; }
    inline void setTotalTasks(int value) { m_totaltasks = value; }
    inline int doneTasks() { return m_donetasks; }


    // much more simpler multithreading...
    void queTask(ImportTask * task);
    void runTasks();
    QMutex saveMutex;
  protected:
    QDate firstday, lastday;
    SessionID highest_sessionid;
    MachineID m_id;
    QString m_class;
    MachineType m_type;
    QString m_path;

    bool changed;
    bool firstsession;
    int m_totaltasks;
    int m_donetasks;

    int skipped_sessions;
    volatile bool m_save_threads_running;

    QList<ImportTask *> m_tasklist;

};


/*! \class CPAP
    \brief A CPAP classed machine object..
    */
class CPAP: public Machine
{
  public:
    CPAP(MachineID id = 0);
    virtual ~CPAP();
};


/*! \class Oximeter
    \brief An Oximeter classed machine object..
    */
class Oximeter: public Machine
{
  public:
    Oximeter(MachineID id = 0);
    virtual ~Oximeter();
  protected:
};

/*! \class SleepStage
    \brief A SleepStage classed machine object..
    */
class SleepStage: public Machine
{
  public:
    SleepStage(MachineID id = 0);
    virtual ~SleepStage();
  protected:
};

/*! \class PositionSensor
    \brief A PositionSensor classed machine object..
    */
class PositionSensor: public Machine
{
  public:
    PositionSensor(MachineID id = 0);
    virtual ~PositionSensor();
  protected:
};

#endif // MACHINE_H

