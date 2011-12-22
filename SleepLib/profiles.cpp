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

#include <QApplication>
#include "mainwindow.h"

extern MainWindow * mainwin;
Preferences *p_pref;
Preferences *p_layout;
Profile * p_profile;

Profile::Profile()
:Preferences(),is_first_day(true)
{
    p_name=STR_GEN_Profile;
    p_path=PREF.Get("{home}/Profiles");
    machlist.clear();

    doctor=new DoctorInfo(this);
    user=new UserInfo(this);
    cpap=new CPAPSettings(this);
    oxi=new OxiSettings(this);
    appearance=new AppearanceSettings(this);
    session=new SessionSettings(this);
    general=new UserSettings(this);
}
Profile::Profile(QString path)
:Preferences(),is_first_day(true)
{
    const QString xmlext=".xml";
    p_name=STR_GEN_Profile;
    if (path.isEmpty()) p_path=GetAppRoot();
    else p_path=path;
    (*this)[STR_GEN_DataFolder]=p_path;
    if (!p_path.endsWith("/")) p_path+="/";
    p_filename=p_path+p_name+xmlext;
    machlist.clear();

    doctor=new DoctorInfo(this);
    user=new UserInfo(this);
    cpap=new CPAPSettings(this);
    oxi=new OxiSettings(this);
    appearance=new AppearanceSettings(this);
    session=new SessionSettings(this);
    general=new UserSettings(this);
}
bool Profile::Save(QString filename)
{
    if (p_profile==this) {
        QString cachefile=Get("{DataFolder}")+QDir::separator()+"cache.day";
        QFile f(cachefile);
        if (ExistsAndTrue("TrashDayCache"))  {
            if (f.exists()) {
                f.remove();
            }
        } else {
            QMap<QDate,QList<Day *> >::iterator di;
            QHash<MachineID,QMap<QDate,QHash<ChannelID, EventDataType> > > cache;

            QHash<MachineID,QMap<QDate,QHash<ChannelID, EventDataType> > >::iterator ci;
            for (di=daylist.begin();di!=daylist.end();di++) {
                QDate date=di.key();
                for (QList<Day *>::iterator d=di.value().begin();d!=di.value().end();d++) {
                    Day *day=*d;
                    MachineID mach=day->machine->id();
                    QHash<ChannelID, EventDataType>::iterator i;

                    for (i=day->m_p90.begin();i!=day->m_p90.end();i++) {
                        cache[mach][date][i.key()]=day->m_p90[i.key()];
                    }
                }
            }
            if (f.open(QFile::WriteOnly)) {
                QDataStream out(&f);
                out.setVersion(QDataStream::Qt_4_6);
                out.setByteOrder(QDataStream::LittleEndian);
                quint16 size=cache.size();
                out << size;
                for (ci=cache.begin();ci!=cache.end();ci++) {
                    quint32 mid=ci.key();
                    out << mid;
                    out << ci.value();
                }
                f.close();
            }
        }
        (*this)["TrashDayCache"]=false;
    }
    return Preferences::Save(filename);
}


Profile::~Profile()
{
    delete user;
    delete doctor;
    delete cpap;
    delete oxi;
    delete appearance;
    delete session;
    delete general;
    for (QHash<MachineID,Machine *>::iterator i=machlist.begin(); i!=machlist.end(); i++) {
        delete i.value();
    }
}
void Profile::DataFormatError(Machine *m)
{
    QString msg=QObject::tr("Software changes have been made that require the reimporting of the following machines data:\n\n");
    msg=msg+m->properties[STR_PROP_Brand]+" "+m->properties[STR_PROP_Model]+" "+m->properties[STR_PROP_Serial];
    msg=msg+QObject::tr("\n\nThis is still only alpha software and these changes are sometimes necessary.\n\n");
    msg=msg+QObject::tr("I can automatically purge this data for you, or you can cancel now and continue to run in a previous version.\n\n");
    msg=msg+QObject::tr("Would you like me to purge this data this for you so you can run the new version?");

    if (QMessageBox::warning(NULL,QObject::tr("Machine Database Changes"),msg,QMessageBox::Yes | QMessageBox::Cancel,QMessageBox::Yes)==QMessageBox::Yes) {

        if (!m->Purge(3478216)) { // Do not copy this line without thinking.. You will be eaten by a Grue if you do

            QMessageBox::critical(NULL,QObject::tr("Purge Failed"),QObject::tr("Sorry, I could not purge this data, which means this version of SleepyHead can't start.. SleepyHead's Data folder needs to be removed manually\n\nThis folder currently resides at the following location:\n")+PREF[STR_GEN_DataFolder].toString(),QMessageBox::Ok);
            exit(-1);
        }
    } else {
        exit(-1);
    }
    return;

}
void Profile::LoadMachineData()
{
    QHash<MachineID,QMap<QDate,QHash<ChannelID, EventDataType> > > cache;

    QString filename=Get("{DataFolder}")+QDir::separator()+"cache.day";
    QFile f(filename);
    if (f.exists(filename) && (f.open(QFile::ReadOnly))) {
        QDataStream in(&f);
        in.setVersion(QDataStream::Qt_4_6);
        in.setByteOrder(QDataStream::LittleEndian);

        quint16 size;
        quint32 mid;
        in >> size;
        for (int i=0;i<size;i++) {
            in >> mid;
            in >> cache[mid];
        }
        PROFILE.general->setRebuildCache(false);
    } else {
        if (mainwin) {
            mainwin->Notify(QObject::tr("Caching session data, this may take a little while."));
            PROFILE.general->setRebuildCache(true);

            QApplication::processEvents();
        }
    }
    for (QHash<MachineID,Machine *>::iterator i=machlist.begin(); i!=machlist.end(); i++) {
        Machine *m=i.value();

        MachineLoader *loader=GetLoader(m->GetClass());
        if (loader) {
            long v=loader->Version();
            long cv=0;
            if (m->properties.find(STR_PROP_DataVersion)==m->properties.end()) {
                m->properties[STR_PROP_DataVersion]="0";
            }
            bool ok;
            cv=m->properties[STR_PROP_DataVersion].toLong(&ok);
            if (!ok || cv<v) {
                DataFormatError(m);
                // It may exit above and not return here..
                QString s;
                s.sprintf("%li",v);
                m->properties[STR_PROP_DataVersion]=s; // Dont need to nag again if they are too lazy.
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
    for (QMap<QDate,QList<Day *> >::iterator di=daylist.begin();di!=daylist.end();di++) {
        for (QList<Day *>::iterator d=di.value().begin();d!=di.value().end();d++) {
            Day *day=*d;
            MachineID mid=day->machine->id();
            if (cache.contains(mid)) {
                if (cache[mid].contains(di.key())) {
                    day->m_p90=cache[mid][di.key()];
                }
            }
        }
    }
    // Load Day Cache here..
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
        me.setAttribute("id",(int)m->id());
        me.setAttribute("type",(int)m->GetType());
        me.setAttribute("class",m->GetClass());
        if (!m->properties.contains(STR_PROP_Path)) m->properties[STR_PROP_Path]="{DataFolder}/"+m->GetClass()+"_"+m->hexid();

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
    prof->user->setUserName(name);
    //prof->Set("Realname",realname);
    //if (!password.isEmpty()) prof.user->setPassword(password);
    prof->Set(STR_GEN_DataFolder,QString("{home}/Profiles/{")+QString(UI_STR_UserName)+QString("}"));

    Machine *m=new Machine(prof,0);
    m->SetClass("Journal");
    m->properties[STR_PROP_Brand]="Virtual";
    m->properties[STR_PROP_Path]="{DataFolder}/"+m->GetClass()+"_"+m->hexid();
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


} // namespace Profiles

// DoctorInfo Strings
const char * DI_STR_Name="DoctorName";
const char * DI_STR_Phone="DoctorPhone";
const char * DI_STR_Email="DoctorEmail";
const char * DI_STR_Practice="DoctorPractice";
const char * DI_STR_Address="DoctorAddress";
const char * DI_STR_PatientID="DoctorPatientID";

// UserInfo Strings
const char * UI_STR_DOB="DOB";
const char * UI_STR_FirstName="FirstName";
const char * UI_STR_LastName="LastName";
const char * UI_STR_UserName="UserName";
const char * UI_STR_Password="Password";
const char * UI_STR_Address="Address";
const char * UI_STR_Phone="Phone";
const char * UI_STR_EmailAddress="EmailAddress";
const char * UI_STR_Country="Country";
const char * UI_STR_Height="Height";
const char * UI_STR_Gender="Gender";
const char * UI_STR_TimeZone="TimeZone";
const char * UI_STR_Language="Language";
const char * UI_STR_DST="DST";

// OxiSettings Strings
const char * OS_STR_EnableOximetry="EnableOximetry";
const char * OS_STR_SyncOximetry="SyncOximetry";
const char * OS_STR_OximeterType="OximeterType";
const char * OS_STR_OxiDiscardThreshold="OxiDiscardThreshold";
const char * OS_STR_SPO2DropDuration="SPO2DropDuration";
const char * OS_STR_SPO2DropPercentage="SPO2DropPercentage";
const char * OS_STR_PulseChangeDuration="PulseChangeDuration";
const char * OS_STR_PulseChangeBPM="PulseChangeBPM";

// CPAPSettings Strings
const char * CS_STR_ComplianceHours="ComplianceHours";
const char * CS_STR_ShowCompliance="ShowCompliance";
const char * CS_STR_ShowLeaksMode="ShowLeaksMode";
const char * CS_STR_MaskStartDate="MaskStartDate";
const char * CS_STR_MaskDescription="MaskDescription";
const char * CS_STR_MaskType="MaskType";
const char * CS_STR_PrescribedMode="CPAPPrescribedMode";
const char * CS_STR_PrescribedMinPressure="CPAPPrescribedMinPressure";
const char * CS_STR_PrescribedMaxPressure="CPAPPrescribedMaxPressure";
const char * CS_STR_UntreatedAHI="UntreatedAHI";
const char * CS_STR_Notes="CPAPNotes";
const char * CS_STR_DateDiagnosed="DateDiagnosed";

// ImportSettings Strings
const char * IS_STR_DaySplitTime="DaySplitTime";
const char * IS_STR_CacheSessions="MemoryHog";
const char * IS_STR_CombineCloseSessions="CombineCloserSessions";
const char * IS_STR_IgnoreShorterSessions="IgnoreShorterSessions";
const char * IS_STR_Multithreading="EnableMultithreading";
const char * IS_STR_TrashDayCache="TrashDayCache";
const char * IS_STR_ShowSerialNumbers="ShowSerialNumbers";

// AppearanceSettings Strings
const char * AS_STR_GraphHeight="GraphHeight";
const char * AS_STR_AntiAliasing="UseAntiAliasing";
const char * AS_STR_HighResPrinting="HighResPrinting";
const char * AS_STR_GraphSnapshots="EnableGraphSnapshots";
const char * AS_STR_Animations="AnimationsAndTransitions";
const char * AS_STR_SquareWave="SquareWavePlots";
const char * AS_STR_OverlayType="OverlayType";

// UserSettings Strings
const char * US_STR_UnitSystem="UnitSystem";
const char * US_STR_EventWindowSize="EventWindowSize";
const char * US_STR_SkipEmptyDays="SkipEmptyDays";
const char * US_STR_RebuildCache="RebuildCache";
const char * US_STR_ShowDebug="ShowDebug";
const char * US_STR_LinkGroups="LinkGroups";

EventDataType Profile::calcCount(ChannelID code, MachineType mt, QDate start, QDate end)
{
    if (!start.isValid()) start=LastDay(mt);
    if (!end.isValid()) end=LastDay(mt);
    QDate date=start;

    double val=0;
    do {
        Day * day=GetDay(date,mt);
        if (day) {
            val+=day->count(code);
        }
        date=date.addDays(1);
    } while (date<end);
    return val;
}
double Profile::calcSum(ChannelID code, MachineType mt, QDate start, QDate end)
{
    if (!start.isValid()) start=LastDay(mt);
    if (!end.isValid()) end=LastDay(mt);
    QDate date=start;

    double val=0;
    do {
        Day * day=GetDay(date,mt);
        if (day) {
            val+=day->sum(code);
        }
        date=date.addDays(1);
    } while (date<end);
    return val;
}
EventDataType Profile::calcHours(MachineType mt, QDate start, QDate end)
{
    if (!start.isValid()) start=LastDay(mt);
    if (!end.isValid()) end=LastDay(mt);
    QDate date=start;

    double val=0;
    do {
        Day * day=GetDay(date,mt);
        if (day) {
            val+=day->hours();
        }
        date=date.addDays(1);
    } while (date<end);
    return val;
}
EventDataType Profile::calcAvg(ChannelID code, MachineType mt, QDate start, QDate end)
{
    if (!start.isValid()) start=LastDay(mt);
    if (!end.isValid()) end=LastDay(mt);
    QDate date=start;

    double val=0;
    int cnt=0;
    do {
        Day * day=GetDay(date,mt);
        if (day) {
            val+=day->sum(code);
            cnt++;
        }
        date=date.addDays(1);
    } while (date<end);
    if (!cnt) return 0;
    return val/float(cnt);
}
EventDataType Profile::calcWavg(ChannelID code, MachineType mt, QDate start, QDate end)
{
    if (!start.isValid())
        start=LastDay(mt);
    if (!end.isValid())
        end=LastDay(mt);
    QDate date=start;

    double val=0,tmp,tmph,hours=0;
    do {
        Day * day=GetDay(date,mt);
        if (day) {
            tmph=day->hours();
            tmp=day->wavg(code);
            val+=tmp*tmph;
            hours+=tmph;
        }
        date=date.addDays(1);
    } while (date<end);
    if (!hours) return 0;
    val=val/hours;
    return val;
}
EventDataType Profile::calcPercentile(ChannelID code, EventDataType percent, MachineType mt, QDate start, QDate end)
{
    if (!start.isValid()) start=LastDay(mt);
    if (!end.isValid()) end=LastDay(mt);

    QDate date=start;

    //double val=0,tmp,hours=0;
    do {
        date=date.addDays(1);
    } while (date<end);
    return 0;
}

QDate Profile::FirstDay(MachineType mt)
{
    if ((mt==MT_UNKNOWN) || (!m_last.isValid()) || (!m_first.isValid()))
        return m_first;

    QDate d=m_first;
    do {
        if (GetDay(d,mt)!=NULL) return d;
        d=d.addDays(1);
    } while (d<=m_last);
    return m_last;
}
QDate Profile::LastDay(MachineType mt)
{
    if ((mt==MT_UNKNOWN) || (!m_last.isValid()) || (!m_first.isValid()))
        return m_last;
    QDate d=m_last;
    do {
        if (GetDay(d,mt)!=NULL) return d;
        d=d.addDays(-1);
    } while (d>=m_first);
    return m_first;
}
