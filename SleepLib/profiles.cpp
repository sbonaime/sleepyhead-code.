/*

SleepLib Profiles Implementation

Author: Mark Watkins <jedimark64@users.sourceforge.net>
License: GPL

*/

#include <QString>
#include <QDateTime>
#include <QDir>
#include <QMessageBox>
#include <QDebug>

#include "preferences.h"
#include "profiles.h"
#include "machine.h"
#include "machine_loader.h"

Preferences *p_pref;
Preferences *p_layout;
Profile * p_profile;

Profile::Profile()
:Preferences(),is_first_day(true)
{
    p_name="Profile";
    p_path=PREF.Get("{home}/Profiles");
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
    for (QHash<MachineID,Machine *>::iterator i=machlist.begin(); i!=machlist.end(); i++) {
        delete i.value();
    }
}
void Profile::DataFormatError(Machine *m)
{
    QString msg="Software changes have been made that require the reimporting of the following machines data:\n\n";
    msg=msg+m->properties["Brand"]+" "+m->properties["Model"]+" "+m->properties["Serial"];
    msg=msg+"\n\nThis is still only alpha software and these changes are sometimes necessary.\n\n";
    msg=msg+"I can automatically purge this data for you, or you can cancel now and continue to run in a previous version.\n\n";
    msg=msg+"Would you like me to purge this data this for you so you can run the new version?";

    if (QMessageBox::warning(NULL,"Machine Database Changes",msg,QMessageBox::Yes | QMessageBox::Cancel,QMessageBox::Yes)==QMessageBox::Yes) {

        if (!m->Purge(3478216)) { // Do not copy this line without thinking.. You will be eaten by a Grue if you do
            QMessageBox::critical(NULL,"Purge Failed","Sorry, I could not purge this data, which means this version of SleepyHead can't start.. SleepyHead's Data folder needs to be removed manually\n\nThis folder currently resides at the following location:\n"+PREF["DataFolder"].toString(),QMessageBox::Ok);
            exit(-1);
        }
    } else {
        exit(-1);
    }
    return;

}
void Profile::LoadMachineData()
{
    for (QHash<MachineID,Machine *>::iterator i=machlist.begin(); i!=machlist.end(); i++) {
        Machine *m=i.value();

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
                DataFormatError(m);
                // It may exit above and not return here..
                QString s;
                s.sprintf("%li",v);
                m->properties["DataVersion"]=s; // Dont need to nag again if they are too lazy.
            } else {
                try {
                    m->Load();
                } catch (OldDBVersion e){
                    DataFormatError(m);
                }
            }
        } else {
            m->Load();
        }
    }
}

/**
 * @brief Machine XML section in profile.
 * @param root
 */
void Profile::ExtraLoad(QDomElement & root)
{
    if (root.tagName()!="Machines") {
        qDebug() << "No Machines Tag in Profiles.xml";
        return;
    }
    QDomElement elem=root.firstChildElement();
    while (!elem.isNull()) {
        QString pKey=elem.tagName();

        if (pKey!="Machine") {
            qWarning() << "Profile::ExtraLoad() pKey!=\"Machine\"";
            elem=elem.nextSiblingElement();
            continue;
        }
        int m_id;
        bool ok;
        m_id=elem.attribute("id","").toInt(&ok);
        int mt;
        mt=elem.attribute("type","").toInt(&ok);
        MachineType m_type=(MachineType)mt;

        QString m_class=elem.attribute("class","");
        //MachineLoader *ml=GetLoader(m_class);
        Machine *m;
        //if (ml) {
        //   ml->CreateMachine
        //}
        if (m_type==MT_CPAP) m=new CPAP(this,m_id);
        else if (m_type==MT_OXIMETER) m=new Oximeter(this,m_id);
        else if (m_type==MT_SLEEPSTAGE) m=new SleepStage(this,m_id);
        else {
            m=new Machine(this,m_id);
            m->SetType(m_type);
        }
        m->SetClass(m_class);
        AddMachine(m);
        QDomElement e=elem.firstChildElement();
        for (; !e.isNull(); e=e.nextSiblingElement()) {
            QString pKey=e.tagName();
            m->properties[pKey]=e.text();
        }
        elem=elem.nextSiblingElement();
    }
}
void Profile::AddMachine(Machine *m) {
    if (!m) {
        qWarning() << "Empty Machine in Profile::AddMachine()";
        return;
    }
    machlist[m->id()]=m;
};
void Profile::DelMachine(Machine *m) {
    if (!m) {
        qWarning() << "Empty Machine in Profile::AddMachine()";
        return;
    }
    machlist.erase(machlist.find(m->id()));
};


// Potential Memory Leak Here..
QDomElement Profile::ExtraSave(QDomDocument & doc)
{
    QDomElement mach=doc.createElement("Machines");
    for (QHash<MachineID,Machine *>::iterator i=machlist.begin(); i!=machlist.end(); i++) {
        QDomElement me=doc.createElement("Machine");
        Machine *m=i.value();
        //QString t=wxT("0x")+m->hexid();
        me.setAttribute("id",(int)m->id());
        me.setAttribute("type",(int)m->GetType());
        me.setAttribute("class",m->GetClass());
        i.value()->properties["path"]="{DataFolder}/"+m->hexid();

        for (QHash<QString,QString>::iterator j=i.value()->properties.begin(); j!=i.value()->properties.end(); j++) {
            QDomElement mp=doc.createElement(j.key());
            mp.appendChild(doc.createTextNode(j.value()));
            //mp->LinkEndChild(new QDomText(j->second.toLatin1()));
            me.appendChild(mp);
        }
        mach.appendChild(me);
    }
    return mach;

}

#include <QMessageBox>
void Profile::AddDay(QDate date,Day *day,MachineType mt) {
    //date+=wxTimeSpan::Day();
    if (is_first_day) {
        m_first=m_last=date;
        is_first_day=false;
    }
    if (m_first>date) m_first=date;
    if (m_last<date) m_last=date;

    // Check for any other machines of same type.. Throw an exception if one already exists.
    QList<Day *> & dl=daylist[date];
    for (QList<Day *>::iterator a=dl.begin();a!=dl.end();a++) {
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
        for (QList<Day *>::iterator di=daylist[date].begin();di!=daylist[date].end();di++) {
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
int Profile::Import(QString path)
{
    int c=0;
    qDebug() << "Importing " << path;
    path=path.replace("\\","/");
    if (path.endsWith("/")) path.chop(1);

    QList<MachineLoader *>loaders=GetLoaders();
    for (QList<MachineLoader *>::iterator i=loaders.begin(); i!=loaders.end(); i++) {
        if (c+=(*i)->Open(path,this)) break;
    }
    //qDebug() << "Import Done";
    return c;
}

MachineLoader * GetLoader(QString name)
{
    MachineLoader *l=NULL;
    QList<MachineLoader *>loaders=GetLoaders();
    for (QList<MachineLoader *>::iterator i=loaders.begin(); i!=loaders.end(); i++) {
        if ((*i)->ClassName()==name) {
            l=*i;
            break;
        }
    }
    return l;
}


QList<Machine *> Profile::GetMachines(MachineType t)
// Returns a QVector containing all machine objects regisered of type t
{
    QList<Machine *> vec;
    QHash<MachineID,Machine *>::iterator i;

    for (i=machlist.begin(); i!=machlist.end(); i++) {
        if (!i.value()) {
            qWarning() << "Profile::GetMachines() i->second == NULL";
            continue;
        }
        if (i.value()->GetType()==t) {
            vec.push_back(i.value());
        }
    }
    return vec;
}

Machine * Profile::GetMachine(MachineType t)
{
    QList<Machine *>vec=GetMachines(t);
    if (vec.size()==0) return NULL;
    return vec[0];

}

void Profile::RemoveSession(Session * sess)
{
    QMap<QDate,QList<Day *> >::iterator di;

    for (di=daylist.begin();di!=daylist.end();di++) {
        for (int d=0;d<di.value().size();d++) {
            Day *day=di.value()[d];

            int i=day->getSessions().indexOf(sess);
            if (i>=0) {
                for (;i<day->getSessions().size()-1;i++) {
                    day->getSessions()[i]=day->getSessions()[i+1];
                }
                day->getSessions().pop_back();
                qint64 first=0,last=0;
                for (int i=0;i<day->getSessions().size();i++) {
                    Session & sess=*day->getSessions()[i];
                    if (!first || first>sess.first()) first=sess.first();
                    if (!last || last<sess.last()) last=sess.last();
                }
                day->setFirst(first);
                day->setLast(last);
                return;
            }
        }
    }
}


//Profile *profile=NULL;
QString SHA1(QString pass)
{
    return pass;
}

namespace Profiles
{

QHash<QString,Profile *> profiles;

void Done()
{
    PREF.Save();
    LAYOUT.Save();
    // Only save the open profile..
    for (QHash<QString,Profile *>::iterator i=profiles.begin(); i!=profiles.end(); i++) {
        i.value()->Save();
        delete i.value();
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
Profile *Create(QString name)
{
    QString path=PREF.Get("{home}/Profiles/")+name;
    QDir dir(path);
    if (!dir.exists(path)) dir.mkpath(path);
    //path+="/"+name;
    Profile *prof=new Profile(path);
    prof->Open();
    profiles[name]=prof;
    prof->Set("Username",name);
    //prof->Set("Realname",realname);
    //if (!password.isEmpty()) prof->Set("Password",SHA1(password));
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
    //InitMapsWithoutAwesomeInitializerLists();
    p_pref=new Preferences("Preferences");
    p_layout=new Preferences("Layout");

    PREF.Open();
    LAYOUT.Open();

    QString path=PREF.Get("{home}/Profiles");
    QDir dir(path);

    if (!dir.exists(path)) {
        //dir.mkpath(path);
        // Just silently create a new user record and get on with it.
        //Create(getUserName(),getUserName(),"");
        return;
    }
    if (!dir.isReadable()) {
        qWarning() << "Can't open " << path;
        return;
    }

    dir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
    //dir.setSorting(QDir::Name);

    QFileInfoList list=dir.entryInfoList();

    //QString username=getUserName();
    //if (list.size()==0) { // No profiles.. Create one.
        //Create(username,username,"");
        //return;
    //}

    // Iterate through subdirectories and load profiles..
    for (int i=0;i<list.size();i++) {
        QFileInfo fi=list.at(i);
        QString npath=fi.canonicalFilePath();
        Profile *prof=new Profile(npath);
        prof->Open();  // Read it's XML file..
        profiles[fi.fileName()]=prof;
    }

}


}; // namespace Profiles

