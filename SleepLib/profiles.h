/*

SleepLib Profiles Header

Author: Mark Watkins <jedimark64@users.sourceforge.net>
License: GPL

*/

#ifndef PROFILES_H
#define PROFILES_H

#include <QString>
#include <map>
#include "machine.h"
#include "machine_loader.h"
#include "preferences.h"

class Machine;
/**
 * @class Profile
 * @author Mark Watkins
 * @date 28/04/11
 * @file profiles.h
 * @brief User profile system
 */
class Profile:public Preferences
{
public:
    Profile(QString name);
    Profile();
    virtual ~Profile();

    bool is_first_day;
    map<MachineID,Machine *> machlist;
    void AddMachine(Machine *m);
    void DelMachine(Machine *m);
    void LoadMachineData();
    int Import(QString path);

    void AddDay(QDate date,Day *day,MachineType mt);
    Day * GetDay(QDate date,MachineType type=MT_UNKNOWN);

    vector<Machine *> GetMachines(MachineType t);
    Machine * GetMachine(MachineType t,QDate date);
    Machine * GetMachine(MachineType t);

    virtual void ExtraLoad(QDomElement & root);
    virtual QDomElement ExtraSave(QDomDocument & doc);

    map<QDate,vector<Day *> > daylist;
    const QDate & FirstDay() { return m_first; }
    const QDate & LastDay() { return m_last; }

protected:
    QDate m_first,m_last;

};

class MachineLoader;
extern MachineLoader * GetLoader(QString name);

extern Preferences *p_pref;
extern Preferences *p_layout;

// these are bad and must change
#define pref (*p_pref)
#define laypref (*p_layout)


extern Profile *profile;

namespace Profiles
{

extern map<QString,Profile *> profiles;
void Scan(); // Initialize and load Profile
void Done(); // Save all Profile objects and clear list

Profile *Create(QString name,QString realname,QString password);
Profile *Get(QString name);
Profile *Get();

};

#endif //PROFILES_H

