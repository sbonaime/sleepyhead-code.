/*

SleepLib Profiles Header

Author: Mark Watkins <jedimark64@users.sourceforge.net>
License: GPL

*/

#ifndef PROFILES_H
#define PROFILES_H

#include <QString>
#include "machine.h"
#include "machine_loader.h"
#include "preferences.h"

class Machine;
/*!
  \class Profile
  \author Mark Watkins
  \date 28/04/11
  \file profiles.h
  \brief User profile system
 */
class Profile:public Preferences
{
public:
    Profile(QString name);
    Profile();
    virtual ~Profile();

    //! \brief Save Profile object (This is an extension to Preference::Save(..))
    virtual bool Save(QString filename="");

    bool is_first_day;

    //! \brief List of machines, indexed by MachineID
    QHash<MachineID,Machine *> machlist;

    //! \brief Add machine to this profiles machlist
    void AddMachine(Machine *m);

    //! \brief Remove machine from this profiles machlist
    void DelMachine(Machine *m);

    //! \brief Loads all machine (summary) data belonging to this profile
    void LoadMachineData();

    //! \brief Barf because data format has changed. This does a purge of CPAP data for machine *m
    void DataFormatError(Machine *m);

    /*! \brief Import Machine Data
        \param path
     */
    int Import(QString path);

    //! \brief Remove a session from day object, without deleting the Session object
    void RemoveSession(Session * sess);

    //! \brief Add Day record to Profile Day list
    void AddDay(QDate date,Day *day,MachineType mt);

    //! \brief Get Day record if data available for date and machine type, else return NULL
    Day * GetDay(QDate date,MachineType type=MT_UNKNOWN);

    //! \brief Returns a list of all machines of type t
    QList<Machine *> GetMachines(MachineType t);

    //! \brief Returns the machine of type t used on date, NULL if none..
    Machine * GetMachine(MachineType t,QDate date);

    //! \brief return the first machine of type t
    Machine * GetMachine(MachineType t);

    virtual void ExtraLoad(QDomElement & root);
    virtual QDomElement ExtraSave(QDomDocument & doc);

    QMap<QDate,QList<Day *> > daylist;
    const QDate & FirstDay() { return m_first; }
    const QDate & LastDay() { return m_last; }

protected:
    QDate m_first,m_last;

};

class MachineLoader;
extern MachineLoader * GetLoader(QString name);

extern Preferences *p_pref;
extern Preferences *p_layout;
extern Profile * p_profile;

// these are bad and must change
#define PREF (*p_pref)
#define LAYOUT (*p_layout)
#define PROFILE (*p_profile)

namespace Profiles
{

extern QHash<QString,Profile *> profiles;
void Scan(); // Initialize and load Profile
void Done(); // Save all Profile objects and clear list

Profile *Create(QString name);
Profile *Get(QString name);
Profile *Get();

};

#endif //PROFILES_H

