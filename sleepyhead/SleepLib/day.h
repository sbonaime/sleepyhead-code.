/*
 SleepLib Day Class Header
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

#ifndef DAY_H
#define DAY_H

#include "SleepLib/common.h"
#include "SleepLib/machine_common.h"
#include "SleepLib/machine.h"
#include "SleepLib/event.h"
#include "SleepLib/session.h"

/*! \class OneTypePerDay
    \brief An Exception class to catch multiple machine records per day
    */
class OneTypePerDay
{
};

class Machine;
class Session;

/*! \class Day
    \brief Contains a list of all Sessions for single date, for a single machine
    */
class Day
{
public:
    Day(Machine *m);
    ~Day();

    //! \brief Add Session to this Day object (called during Load)
    void AddSession(Session *s);

    //! \brief Returns this machines type
    MachineType machine_type();

    //! \brief Returns the count of all this days sessions' events for this day
    int count(ChannelID code);

    //! \brief Returns the Minimum of all this sessions' events for this day
    EventDataType Min(ChannelID code);

    //! \brief Returns the Maximum of all sessions' events for this day
    EventDataType Max(ChannelID code);

    //! \brief Returns the Count-per-hour of all sessions' events for this day
    EventDataType cph(ChannelID code);

    //! \brief Returns the Sum-per-hour of all this sessions' events for this day
    EventDataType sph(ChannelID code);

    //! \brief Returns (and caches) the 90th Percentile of all this sessions' events for this day
    EventDataType p90(ChannelID code);

    //! \brief Returns the Average of all this sessions' events for this day
    EventDataType avg(ChannelID code);

    //! \brief Returns the Sum of all this sessions' events for this day
    EventDataType sum(ChannelID code);

    //! \brief Returns the Time-Weighted Average of all this sessions' events for this day
    EventDataType wavg(ChannelID code);

    //! \brief Returns a requested Percentile of all this sessions' events for this day
    EventDataType percentile(ChannelID code,EventDataType percentile);

    //! \brief Returns if the cache contains SummaryType information about the requested code
    bool hasData(ChannelID code, SummaryType type);

    //! \brief Returns the Average of all Sessions setting 'code' for this day
    EventDataType settings_avg(ChannelID code);

    //! \brief Returns the Time-Weighted Average of all Sessions setting 'code' for this day
    EventDataType settings_wavg(ChannelID code);

    //! \brief Returns the Sum of all Sessions setting 'code' for this day
    EventDataType settings_sum(ChannelID code);

    //! \brief Returns the Minimum of all Sessions setting 'code' for this day
    EventDataType settings_min(ChannelID code);

    //! \brief Returns the Maximum of all Sessions setting 'code' for this day
    EventDataType settings_max(ChannelID code);

    //! \brief Returns the first session time of this day
    qint64 first();

    //! \brief Returns the last session time of this day
    qint64 last();

    // //! \brief Sets the first session time of this day
    // void setFirst(qint64 val) { d_first=val; }

    // //! \brief Sets the last session time of this day
    // void setLast(qint64 val) { d_last=val; }

    //! \brief Returns the last session time of this day for the supplied Channel code
    qint64 first(ChannelID code);

    //! \brief Returns the last session time of this day for the supplied Channel code
    qint64 last(ChannelID code);

    //! \brief Returns the total time in milliseconds for this day
    qint64 total_time();

    //! \brief Returns true if this day has enabled sessions
    bool hasEnabledSessions();

    //! \brief Return the total time in decimal hours for this day
    EventDataType hours() { return double(total_time())/3600000.0; }

    //! \brief Return the session indexed by i
    Session *operator [](int i) { return sessions[i]; }

    //! \brief Return the first session as a QVector<Session*>::iterator
    QList<Session *>::iterator begin() { return sessions.begin(); }
    //! \brief Return the end session record as a QVector<Session*>::iterator
    QList<Session *>::iterator end() { return sessions.end(); }

    //! \brief Finds and returns the index of a session, otherwise -1 if it's not there
    int find(Session * sess) { return sessions.indexOf(sess); }

    Session *find(SessionID sessid);

    //! \brief Returns the number of Sessions in this day record
    int size() { return sessions.size(); }

    Machine *machine;

    //! \brief Loads all Events files for this Days Sessions
    void OpenEvents();

    //! \brief Closes all Events files for this Days Sessions
    void CloseEvents();

    //! \brief Returns this days sessions list
    QList<Session *> & getSessions() { return sessions; }

    //! \brief Returns true if this Day contains loaded Event Data for this channel.
    bool channelExists(ChannelID id);

    //! \brief Returns true if session events are loaded
    bool eventsLoaded();

    //! \brief Returns true if this Day contains loaded Event Data or a cached count for this channel
    bool channelHasData(ChannelID id);

    //! \brief Returns true if this day contains the supplied settings Channel id
    bool settingExists(ChannelID id);

    void removeSession(Session * sess);

protected:
    //! \brief A Vector containing all sessions for this day
    QList<Session *> sessions;
    QHash<ChannelID, QHash<EventDataType, EventDataType> > perc_cache;
    //qint64 d_first,d_last;
private:
    bool d_firstsession;
};


#endif // DAY_H
