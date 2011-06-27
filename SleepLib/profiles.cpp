/*

SleepLib Profiles Implementation

Author: Mark Watkins <jedimark64@users.sourceforge.net>
License: GPL

*/
//#include <wx/filefn.h>
//#include <wx/filename.h>
//#include <wx/utils.h>
//#include <wx/dir.h>
//#include <wx/log.h>
//#include <wx/msgdlg.h>

#include <QString>
#include <QDateTime>
#include <QDir>
#include <QMessageBox>

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
    p_name="Profile";
    p_path=pref.Get("{home}/Profiles");
    machlist.clear();
    //m_first=m_last=
}
Profile::Profile(QString path)
:Preferences(),is_first_day(true)
{
    const QString xmlext=".xml";
    p_name="Profile";
    if (path.isEmpty()) p_path=GetAppRoot();
    else p_path=path;
    (*this)["DataFolder"]=p_path;
    if (!p_path.endsWith("/")) p_path+="/";
    p_filename=p_path+p_name+xmlext;
    machlist.clear();
    //m_first=m_last=NULL;
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
            if (m->properties.find("DataVersion")==m->properties.end()) {
                m->properties["DataVersion"]="0";
            }
            bool ok;
            cv=m->properties["DataVersion"].toLong(&ok);
            if (!ok || cv<v) {
                QString msg="Software changes have been made that require the reimporting of the following machines data:\n\n";
                msg=msg+m->properties["Brand"]+" "+m->properties["Model"]+" "+m->properties["Serial"];
                msg=msg+"\n\nNo attempt will be made to load previous data.\n\n";
                msg=msg+"Importing ALL of your data for this machine again will rectify this problem.\n\n";
                msg=msg+"However, if you have more than one seperate datacard/stash for this machine, it would be best if the machine data was purged first.\n\nWould you like me to do this for you?";

                if (QMessageBox::warning(NULL,"Machine Database Changes",msg,QMessageBox::Yes | QMessageBox::No,QMessageBox::Yes)==QMessageBox::Yes) {

                    if (m->Purge(3478216)) { // Do not copy this line without thinking.. You will be eaten by a Grue if you do
                        QString s;
                        s.sprintf("%li",v);
                        m->properties["DataVersion"]=s; // Dont need to nag again if they are too lazy.
                    }
                }
            } else  m->Load();
        } else {
            m->Load();
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
        qDebug("ExtraLoad: Elem is empty.");
    }
    for(; elem; elem=elem->NextSiblingElement()) {
        QString pKey=elem->Value();
        assert(pKey=="Machine");

        int m_id;
        elem->QueryIntAttribute("id",&m_id);
        int mt;
        elem->QueryIntAttribute("type",&mt);
        MachineType m_type=(MachineType)mt;
        QString m_class=elem->Attribute("class");
        Machine *m;
        if (m_type==MT_CPAP) m=new CPAP(this,m_id);
        else if (m_type==MT_OXIMETER) m=new Oximeter(this,m_id);
        else if (m_type==MT_SLEEPSTAGE) m=new SleepStage(this,m_id);
        else {
            m=new Machine(this,m_id);
            m->SetType(m_type);
        }
        m->SetClass(m_class);
        AddMachine(m);
        TiXmlElement *e=elem->FirstChildElement();
        for (; e; e=e->NextSiblingElement()) {
            QString pKey=e->Value();
            m->properties[pKey]=e->GetText();
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
        //QString t=wxT("0x")+m->hexid();
        me->SetAttribute("id",m->id());
        me->SetAttribute("type",(int)m->GetType());
        char *cc=m->GetClass().toLatin1().data();
        me->SetAttribute("class",cc);
        i->second->properties["path"]="{DataFolder}/"+m->hexid();

        for (map<QString,QString>::iterator j=i->second->properties.begin(); j!=i->second->properties.end(); j++) {
            TiXmlElement *mp=new TiXmlElement(j->first.toLatin1());
            mp->LinkEndChild(new TiXmlText(j->second.toLatin1()));
            me->LinkEndChild(mp);
        }
        mach->LinkEndChild(me);
    }
    //root->LinkEndChild(mach);
    return mach;

}

void Profile::AddDay(QDate date,Day *day,MachineType mt) {
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

Day * Profile::GetDay(QDate date,MachineType type)
{
    Day *day=NULL;
    // profile->     why did I d that??
    if (daylist.find(date)!=daylist.end()) {
        for (vector<Day *>::iterator di=daylist[date].begin();di!=daylist[date].end();di++) {
            if (type==MT_UNKNOWN) { // Who cares.. We just want to know there is data available.
                day=(*di);
                break;
            }
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
void Profile::Import(QString path)
{
    int c=0;
    qDebug(("Importing "+path).toLatin1());
    list<MachineLoader *>loaders=GetLoaders();
    for (list<MachineLoader *>::iterator i=loaders.begin(); i!=loaders.end(); i++) {
        if (c+=(*i)->Open(path,this)) break;
    }
}

MachineLoader * GetLoader(QString name)
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

Machine * Profile::GetMachine(MachineType t)
{
    vector<Machine *>vec=GetMachines(t);
    if (vec.size()==0) return NULL;
    return vec[0];

}

//Profile *profile=NULL;
QString SHA1(QString pass)
{
    return pass;
}

namespace Profiles
{

std::map<QString,Profile *> profiles;

void Done()
{
    pref.Save();
    laypref.Save();
    for (map<QString,Profile *>::iterator i=profiles.begin(); i!=profiles.end(); i++) {
        i->second->Save();
        delete i->second;
    }
    profiles.clear();
    delete p_pref;
    delete p_layout;
}

Profile *Get(QString name)
{
    if (profiles.find(name)!=profiles.end())
        return profiles[name];

    return NULL;
}
Profile *Create(QString name,QString realname,QString password)
{
    QString path=pref.Get("{home}/Profiles/")+name;
    QDir dir(path);
    if (!dir.exists(path)) dir.mkpath(path);
    //path+="/"+name;
    //if (!dir.exists(path)) wxMkdir(path);
    Profile *prof=new Profile(path);
    prof->Open();
    profiles[name]=prof;
    prof->Set("Username",name);
    prof->Set("Realname",realname);
    if (!password.isEmpty()) prof->Set("Password",SHA1(password));
    prof->Set("DataFolder","{home}/Profiles/{Username}");

    Machine *m=new Machine(prof,0);
    m->SetClass("Journal");
    m->properties["Brand"]="Virtual";
    m->SetType(MT_JOURNAL);
    prof->AddMachine(m);

    prof->Save();

    return prof;
}

Profile *Get()
{
    // username lookup
    //getUserName()
    return profiles[getUserName()];;
}



/**
 * @brief Scan Profile directory loading user profiles
 */
void Scan()
{
    InitMapsWithoutAwesomeInitializerLists();

    p_pref=new Preferences("Preferences");
    p_layout=new Preferences("Layout");

    pref.Open();
    laypref.Open();

    QString path=pref.Get("{home}/Profiles");
    QDir dir(path);

    if (!dir.exists(path)) {
        dir.mkpath(path);
        // Just silently create a new user record and get on with it.
        Create(getUserName(),getUserName(),"");
        return;
    }
    if (!dir.isReadable()) {
        qWarning(("Can't open "+path).toLatin1());
        return;
    }

    dir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
    //dir.setSorting(QDir::Name);

    QFileInfoList list=dir.entryInfoList();

    QString username=getUserName();
    if (list.size()==0) {
        Create(username,username,"");
        return;
    }
    for (int i=0;i<list.size();i++) {
        QFileInfo fi=list.at(i);
        QString npath=fi.canonicalFilePath();
        Profile *prof=new Profile(npath);
        prof->Open();
        profiles[fi.fileName()]=prof;
    }

}

}; // namespace Profiles

