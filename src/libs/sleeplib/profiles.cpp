/*

SleepLib Profiles Implementation

Author: Mark Watkins <jedimark64@users.sourceforge.net>
License: GPL

*/
#include <wx/filefn.h>
#include <wx/filename.h>
#include <wx/utils.h>
#include <wx/dir.h>
#include <wx/log.h>
#include <wx/msgdlg.h>

#include "preferences.h"
#include "profiles.h"
#include "machine.h"
#include "machine_loader.h"
#include "tinyxml/tinyxml.h"

Preferences *p_pref;
Preferences *p_layout;

Profile::Profile()
:Preferences(),is_first_day(true)
{
    p_name=wxT("Profile");
    p_path=pref.Get("{home}{sep}Profiles");
    machlist.clear();
    m_first=m_last=wxInvalidDateTime;
}
Profile::Profile(wxString path)
:Preferences(),is_first_day(true)
{
    const wxString xmlext=wxT(".xml");
    p_name=wxT("Profile");
    if (path.IsEmpty()) p_path=GetAppRoot();
    else p_path=path;
    wxString sep=wxFileName::GetPathSeparator();
    (*this)["DataFolder"]=p_path;
    if (!p_path.EndsWith(sep)) p_path+=sep;
    p_filename=p_path+p_name+xmlext;
    machlist.clear();
    m_first=m_last=wxInvalidDateTime;
}

Profile::~Profile()
{
    for (map<MachineID,Machine *>::iterator i=machlist.begin(); i!=machlist.end(); i++) {
    delete i->second;
    }
}
void Profile::LoadMachineData()
{
    for (map<MachineID,Machine *>::iterator i=machlist.begin(); i!=machlist.end(); i++) {
        Machine *m=i->second;

        MachineLoader *loader=GetLoader(m->GetClass());
        if (loader) {
            long v=loader->Version();
            long cv=0;
            if (m->properties.find(wxT("DataVersion"))==m->properties.end())
                m->properties[wxT("DataVersion")]=wxString::Format(wxT("%i"),0);
            m->properties[wxT("DataVersion")].ToLong(&cv);
            if (cv<v) {
                wxString msg=wxT("Software changes have been made that require the reimporting of the following machines data:\n\n");
                msg=msg+m->properties[wxT("Brand")]+wxT(" ")+m->properties[wxT("Model")]+wxT(" ")+m->properties[wxT("Serial")];
                msg=msg+wxT("\n\nNo attempt will be made to load previous data.\n\n");
                msg=msg+wxT("Importing ALL of your data for this machine again will rectify this problem.\n\n");
                msg=msg+wxT("However, if you have more than one seperate datacard/stash for this machine, it would be best if the machine data was purged first.\n\nWould you like me to do this for you?");

                if (wxMessageBox(msg,wxT("Machine Database Changes"),wxYES_NO,NULL)==wxYES) {

                    if (m->Purge(3478216)) { // Do not copy this line without thinking.. You will be eaten by a Grue if you do
                        m->properties[wxT("DataVersion")]=wxString::Format(wxT("%li"),v); // Dont need to nag again if they are too lazy.
                    }
                }
            } else  m->Load();
        }
    }
}

/**
 * @brief Machine XML section in profile.
 * @param root
 */
void Profile::ExtraLoad(TiXmlHandle *root)
{
    TiXmlElement *elem;
    elem=root->FirstChild("Machines").FirstChild().Element();
    if (!elem) {
        wxLogDebug(wxT("ExtraLoad: Elem is empty!!!"));
    }
    for(; elem; elem=elem->NextSiblingElement()) {
        wxString pKey(elem->Value(),wxConvUTF8);
        assert(pKey==wxT("Machine"));

        int m_id;
        elem->QueryIntAttribute("id",&m_id);
        int mt;
        elem->QueryIntAttribute("type",&mt);
        MachineType m_type=(MachineType)mt;
        wxString m_class(elem->Attribute("class"),wxConvUTF8);
        Machine *m;
        if (m_type==MT_CPAP) m=new CPAP(this,m_id);
        else if (m_type==MT_OXIMETER) m=new Oximeter(this,m_id);
        else if (m_type==MT_SLEEPSTAGE) m=new SleepStage(this,m_id);
        else m=new Machine(this,m_id);
        m->SetClass(m_class);
        AddMachine(m);
        TiXmlElement *e=elem->FirstChildElement();
        for (; e; e=e->NextSiblingElement()) {
            //wxString type(attr->Value(),wxConvUTF8);
            wxString pKey(e->Value(),wxConvUTF8);
            wxString pText(e->GetText(),wxConvUTF8);
            m->properties[pKey]=pText;
        }
    }
}
void Profile::AddMachine(Machine *m) {
    assert(m!=NULL);
    machlist[m->id()]=m;
};
void Profile::DelMachine(Machine *m) {
    assert(m!=NULL);
    machlist.erase(m->id());
};

TiXmlElement * Profile::ExtraSave()
{
    TiXmlElement *mach=new TiXmlElement("Machines");
    for (map<MachineID,Machine *>::iterator i=machlist.begin(); i!=machlist.end(); i++) {
        TiXmlElement *me=new TiXmlElement("Machine");
        Machine *m=i->second;
        //wxString t=wxT("0x")+m->hexid();
        me->SetAttribute("id",m->id());
        me->SetAttribute("type",m->GetType());
        me->SetAttribute("class",m->GetClass().mb_str());
        i->second->properties[wxT("path")]=wxT("{DataFolder}{sep}")+m->hexid();

        for (map<wxString,wxString>::iterator j=i->second->properties.begin(); j!=i->second->properties.end(); j++) {
            TiXmlElement *mp=new TiXmlElement(j->first.mb_str());
            mp->LinkEndChild(new TiXmlText(j->second.mb_str()));
            me->LinkEndChild(mp);
        }
        mach->LinkEndChild(me);
    }
    //root->LinkEndChild(mach);
    return mach;

}

void Profile::AddDay(wxDateTime date,Day *day,MachineType mt) {
    //date+=wxTimeSpan::Day();
    if (is_first_day) {
        m_first=m_last=date;
        is_first_day=false;
    }
    if (m_first>date) m_first=date;
    if (m_last<date) m_last=date;

    // Check for any other machines of same type.. Throw an exception if one already exists.
    vector<Day *> & dl=daylist[date];
    for (vector<Day *>::iterator a=dl.begin();a!=dl.end();a++) {
        if ((*a)->machine->GetType()==mt) {
            throw OneTypePerDay();
        }
    }

    daylist[date].push_back(day);
}
Day * Profile::GetDay(wxDateTime date,MachineType type)
{
    Day *day=NULL;
    if (profile->daylist.find(date)!=profile->daylist.end()) {
        for (vector<Day *>::iterator di=profile->daylist[date].begin();di!=profile->daylist[date].end();di++) {
            if ((*di)->machine_type()==type) {
                day=(*di);
                break;
            }
        }
    }
    return day;
}


/**
 * @brief Import Machine Data
 * @param path
 */
void Profile::Import(wxString path)
{
    int c=0;
    wxLogDebug(wxT("Importing ")+path);
    list<MachineLoader *>loaders=GetLoaders();
    for (list<MachineLoader *>::iterator i=loaders.begin(); i!=loaders.end(); i++) {
        if (c+=(*i)->Open(path,this)) break;
    }
}

MachineLoader * GetLoader(wxString name)
{
    MachineLoader *l=NULL;
    list<MachineLoader *>loaders=GetLoaders();
    for (list<MachineLoader *>::iterator i=loaders.begin(); i!=loaders.end(); i++) {
        if ((*i)->ClassName()==name) {
            l=*i;
            break;
        }
    }
    return l;
}


vector<Machine *> Profile::GetMachines(MachineType t)
// Returns a vector containing all machine objects regisered of type t
{
    vector<Machine *> vec;
    map<MachineID,Machine *>::iterator i;

    for (i=machlist.begin(); i!=machlist.end(); i++) {
        assert(i->second!=NULL);
        if (i->second->GetType()==t) {
            vec.push_back(i->second);
        }
    }
    return vec;
}


Profile *profile=NULL;
wxString SHA1(wxString pass)
{
    return pass;
}

namespace Profiles
{

std::map<wxString,Profile *> profiles;

void Done()
{
    pref.Save();
    layout.Save();
    for (map<wxString,Profile *>::iterator i=profiles.begin(); i!=profiles.end(); i++) {
        i->second->Save();
        delete i->second;
    }
    profiles.clear();
    delete p_pref;
    delete p_layout;
}

Profile *Get(wxString name)
{
    return profiles[name];
}
Profile *Create(wxString name,wxString realname,wxString password)
{
    wxString path=pref.Get("{home}{sep}Profiles");
    if (!wxDirExists(path)) wxMkdir(path);
    path+=wxFileName::GetPathSeparator()+name;
    if (!wxDirExists(path)) wxMkdir(path);
    Profile *prof=new Profile(path);
    prof->Open();
    profiles[name]=prof;
    prof->Set("Username",name);
    prof->Set("Realname",realname);
    if (!password.IsEmpty()) prof->Set("Password",SHA1(password));
    prof->Set("DataFolder",wxT("{home}{sep}Profiles{sep}{Username}"));
    return prof;
}

Profile *Get()
{
    return profile;
}



/**
 * @brief Scan Profile directory loading user profiles
 */
void Scan()
{
    InitMapsWithoutAwesomeInitializerLists();

    p_pref=new Preferences(wxT("Preferences"));
    p_layout=new Preferences(wxT("Layout"));

    pref.Open();
    layout.Open();

    wxString path=pref.Get("{home}{sep}Profiles");

    if (!wxDirExists(path)) {
        wxString tmp=pref.Get("{home}");
        //wxLogMessage(wxT("zzz")+tmp);
        if (!wxDirExists(tmp)) wxMkdir(tmp);
        Create(wxGetUserId(),wxGetUserName(),wxT(""));
        return;
    }
    wxDir dir;
    dir.Open(path);
    // windows beyatching doesn'topen it.
    if (!dir.IsOpened()) {
        wxLogError(wxT("Wierded out opening ")+path);
        return;
    }
    wxString filename;
    bool cont=dir.GetFirst(&filename);
    list<wxString> names;
    while (cont) {
        names.push_back(filename);
        cont=dir.GetNext(&filename);
    }
    if (names.size()==0) {
        Create(wxGetUserId(),wxGetUserName(),wxT(""));
        return;
    }
    for (list<wxString>::iterator i=names.begin(); i!=names.end(); i++) {
        wxString newpath=path+wxFileName::GetPathSeparator()+filename;
        Profile *prof=new Profile(newpath);
        prof->Open();
        profiles[filename]=prof;
    }

}

}; // namespace Profiles

