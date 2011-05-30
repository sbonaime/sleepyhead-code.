/*

SleepLib Profiles Header

Author: Mark Watkins <jedimark64@users.sourceforge.net>
License: GPL

*/

#ifndef PROFILES_H
#define PROFILES_H

#include <wx/string.h>
#include <map>
#include "machine.h"
#include "preferences.h"
#include "tinyxml/tinyxml.h"

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
    Profile(wxString name);
    Profile();
    virtual ~Profile();

    bool is_first_day;
    map<MachineID,Machine *> machlist;
    void AddMachine(Machine *m);
    void LoadMachineData();
    void Import(wxString path);

    void AddDay(wxDateTime date,Day *day);

    vector<Machine *> GetMachines(MachineType t);
    Machine * GetMachine(MachineType t,wxDateTime date);

    virtual void ExtraLoad(TiXmlHandle *root);
    virtual TiXmlElement * ExtraSave();
    map<wxDateTime,vector<Day *> > daylist;
    const wxDateTime & FirstDay() { return m_first; };
    const wxDateTime & LastDay() { return m_last; };

protected:
    wxDateTime m_first,m_last;

};

extern Preferences *p_pref;
extern Preferences *p_layout;
#define pref (*p_pref)
#define layout (*p_layout)

extern Profile *profile;

namespace Profiles
{

extern map<wxString,Profile *> profiles;
void Scan(); // Initialize and load Profile
void Done(); // Save all Profile objects and clear list

Profile *Create(wxString name,wxString realname,wxString password);
Profile *Get(wxString name);
Profile *Get();

};

#endif //PROFILES_H

