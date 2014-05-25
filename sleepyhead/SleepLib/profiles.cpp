/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 *
 * SleepLib Profiles Implementation
 *
 * Copyright (c) 2011-2014 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#include <QString>
#include <QDateTime>
#include <QDir>
#include <QMessageBox>
#include <QDebug>
#include <algorithm>
#include <cmath>

#include "preferences.h"
#include "profiles.h"
#include "machine.h"
#include "machine_loader.h"

#include <QApplication>
#include "mainwindow.h"

extern MainWindow *mainwin;
Preferences *p_pref;
Preferences *p_layout;
Profile *p_profile;

Profile::Profile(QString path)
  : is_first_day(true),
     m_opened(false)
{
    p_name = STR_GEN_Profile;

    if (path.isEmpty()) {
        p_path = GetAppRoot();
    } else {
        p_path = path;
    }

    (*this)[STR_GEN_DataFolder] = p_path;
    path = path.replace("\\", "/");

    if (!p_path.endsWith("/")) {
        p_path += "/";
    }

    p_filename = p_path + p_name + STR_ext_XML;
    machlist.clear();

    doctor = nullptr;
    user = nullptr;
    cpap = nullptr;
    oxi = nullptr;
    appearance = nullptr;
    session = nullptr;
    general = nullptr;
}

Profile::~Profile()
{
    if (m_opened) {
        delete user;
        delete doctor;
        delete cpap;
        delete oxi;
        delete appearance;
        delete session;
        delete general;

        for (QHash<MachineID, Machine *>::iterator it = machlist.begin(); it != machlist.end(); it++) {
            delete it.value();
        }
        m_opened=false;
    }
}

bool Profile::Save(QString filename)
{
    if (m_opened) {
        return Preferences::Save(filename);
    } else return false;
}

bool Profile::Open(QString filename)
{
    if (filename.isEmpty()) {
        filename=p_filename;
    }
    if (m_opened) {
        qDebug() << "Profile" << filename << "all ready open";
        return true;
    }
    bool b = Preferences::Open(filename);

    m_opened=true;
    doctor = new DoctorInfo(this);
    user = new UserInfo(this);
    cpap = new CPAPSettings(this);
    oxi = new OxiSettings(this);
    appearance = new AppearanceSettings(this);
    session = new SessionSettings(this);
    general = new UserSettings(this);
    return b;
}

void Profile::DataFormatError(Machine *m)
{
    QString msg =
        QObject::tr("Software changes have been made that require the reimporting of the following machines data:\n\n");
    msg = msg + m->properties[STR_PROP_Brand] + " " + m->properties[STR_PROP_Model] + " " +
          m->properties[STR_PROP_Serial];
    msg = msg +
          QObject::tr("I can automatically purge this data for you, or you can cancel now and continue to run in a previous version.\n\n");
    msg = msg +
          QObject::tr("Would you like me to purge this data this for you so you can run the new version?");

    if (QMessageBox::warning(nullptr,
                             QObject::tr("Machine Database Changes"),
                             msg,
                             QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Yes) == QMessageBox::Yes) {


        if (!m->Purge(3478216)) {
            // Do not copy this line without thinking.. You will be eaten by a Grue if you do

            QMessageBox::critical(nullptr, STR_MessageBox_Error,
                                  QObject::tr("Sorry, the purge operation failed, which means this version of SleepyHead can't start.")+"\n\n"+
                                  QObject::tr("The machine data folder needs to be removed manually.")+"\n\n"+
                                  QObject::tr("This folder currently resides at the following location:")+"\n\n"+
                                  QDir::toNativeSeparators(PREF[STR_GEN_DataFolder].toString()), QMessageBox::Ok);
            QApplication::exit(-1);
        }
        // Note: I deliberately haven't added a Profile help for this
        PROFILE.p_preferences[STR_PREF_ReimportBackup] = true;
        PROFILE.Save();
    } else {
        QApplication::exit(-1);
    }

    return;

}
void Profile::LoadMachineData()
{
    QHash<MachineID, QMap<QDate, QHash<ChannelID, EventDataType> > > cache;

    for (QHash<MachineID, Machine *>::iterator i = machlist.begin(); i != machlist.end(); i++) {
        Machine *m = i.value();

        MachineLoader *loader = GetLoader(m->GetClass());

        if (loader) {
            long v = loader->Version();
            long cv = 0;

            if (m->properties.find(STR_PROP_DataVersion) == m->properties.end()) {
                m->properties[STR_PROP_DataVersion] = "0";
            }

            bool ok;
            cv = m->properties[STR_PROP_DataVersion].toLong(&ok);

            if (!ok || cv < v) {
                DataFormatError(m);
                // It may exit above and not return here..
                QString s;
                s.sprintf("%li", v);
                m->properties[STR_PROP_DataVersion] = s; // Dont need to nag again if they are too lazy.
            } else {
                try {
                    m->Load();
                } catch (OldDBVersion e) {
                    Q_UNUSED(e)
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
void Profile::ExtraLoad(QDomElement &root)
{
    if (root.tagName() != "Machines") {
        qDebug() << "No Machines Tag in Profiles.xml";
        return;
    }

    QDomElement elem = root.firstChildElement();

    while (!elem.isNull()) {
        QString pKey = elem.tagName();

        if (pKey != "Machine") {
            qWarning() << "Profile::ExtraLoad() pKey!=\"Machine\"";
            elem = elem.nextSiblingElement();
            continue;
        }

        int m_id;
        bool ok;
        m_id = elem.attribute("id", "").toInt(&ok);
        int mt;
        mt = elem.attribute("type", "").toInt(&ok);
        MachineType m_type = (MachineType)mt;

        QString m_class = elem.attribute("class", "");
        //MachineLoader *ml=GetLoader(m_class);
        Machine *m;

        //if (ml) {
        //   ml->CreateMachine
        //}
        if (m_type == MT_CPAP) {
            m = new CPAP(this, m_id);
        } else if (m_type == MT_OXIMETER) {
            m = new Oximeter(this, m_id);
        } else if (m_type == MT_SLEEPSTAGE) {
            m = new SleepStage(this, m_id);
        } else if (m_type == MT_POSITION) {
            m = new PositionSensor(this, m_id);
        } else {
            m = new Machine(this, m_id);
            m->SetType(m_type);
        }

        m->SetClass(m_class);
        AddMachine(m);
        QDomElement e = elem.firstChildElement();

        for (; !e.isNull(); e = e.nextSiblingElement()) {
            QString pKey = e.tagName();
            m->properties[pKey] = e.text();
        }

        elem = elem.nextSiblingElement();
    }
}
void Profile::AddMachine(Machine *m)
{
    if (!m) {
        qWarning() << "Empty Machine in Profile::AddMachine()";
        return;
    }

    machlist[m->id()] = m;
};
void Profile::DelMachine(Machine *m)
{
    if (!m) {
        qWarning() << "Empty Machine in Profile::AddMachine()";
        return;
    }

    machlist.erase(machlist.find(m->id()));
};


// Potential Memory Leak Here..
QDomElement Profile::ExtraSave(QDomDocument &doc)
{
    QDomElement mach = doc.createElement("Machines");

    for (QHash<MachineID, Machine *>::iterator i = machlist.begin(); i != machlist.end(); i++) {
        QDomElement me = doc.createElement("Machine");
        Machine *m = i.value();
        me.setAttribute("id", (int)m->id());
        me.setAttribute("type", (int)m->GetType());
        me.setAttribute("class", m->GetClass());

        if (!m->properties.contains(STR_PROP_Path)) { m->properties[STR_PROP_Path] = "{DataFolder}/" + m->GetClass() + "_" + m->hexid(); }

        for (QHash<QString, QString>::iterator j = i.value()->properties.begin();
                j != i.value()->properties.end(); j++) {
            QDomElement mp = doc.createElement(j.key());
            mp.appendChild(doc.createTextNode(j.value()));
            //mp->LinkEndChild(new QDomText(j->second.toLatin1()));
            me.appendChild(mp);
        }

        mach.appendChild(me);
    }

    return mach;

}

void Profile::AddDay(QDate date, Day *day, MachineType mt)
{
    //date+=wxTimeSpan::Day();
    if (!day)  {
        qDebug() << "Profile::AddDay called with null day object";
        return;
    }

    if (is_first_day) {
        m_first = m_last = date;
        is_first_day = false;
    }

    if (m_first > date) {
        m_first = date;
    }

    if (m_last < date) {
        m_last = date;
    }

    // Check for any other machines of same type.. Throw an exception if one already exists.
    QList<Day *> &dl = daylist[date];

    for (QList<Day *>::iterator a = dl.begin(); a != dl.end(); a++) {
        if ((*a)->machine->GetType() == mt) {

            // disabled this because two machines isn't all that bad
            //            if (QMessageBox::question(nullptr,"Different Machine Detected","This data comes from another machine to what's usually imported, and has overlapping data.\nThis new data will override any older data from the old machine. Are you sure you want to do this?",QMessageBox::Yes,QMessageBox::No)==QMessageBox::No) {
            //                throw OneTypePerDay();
            //            }
            daylist[date].erase(a);
            break;
        }
    }

    daylist[date].push_back(day);
}

// Get Day record if data available for date and machine type,
// and has enabled session data, else return nullptr
Day *Profile::GetGoodDay(QDate date, MachineType type)
{
    Day *day = GetDay(date, type);
    if (!day)
        return nullptr;

    // Just return the day if not matching for a machine.
    if (type == MT_UNKNOWN)
        return day;

    // For a machine match, find at least one enabled Session.
    Q_ASSERT(day->machine_type() == type);
    for (int i = 0; i < day->size(); ++i) {
        if ((*day)[i]->enabled())
            return day;
    }

    // No enabled Sessions were found.
    return nullptr;
}

Day *Profile::GetDay(QDate date, MachineType type)
{
    if (!daylist.contains(date))
        return nullptr;

    QList<Day *> list(daylist.value(date));

    QList<Day *>::iterator it = list.begin();
    QList<Day *>::iterator list_end = list.end();

    for (; it != list_end; ++it) {
        Day * day = (*it);

        Q_ASSERT(day != nullptr);

        // Just return the day if not matching for a machine.
        if (day->machine_type() == type || type == MT_UNKNOWN) {
            return day;
        }
    }

    return nullptr;
}


int Profile::Import(QString path)
{
    int c = 0;
    qDebug() << "Importing " << path;
    path = path.replace("\\", "/");

    if (path.endsWith("/")) {
        path.chop(1);
    }

    QList<MachineLoader *>loaders = GetLoaders(MT_CPAP);

    Q_FOREACH(MachineLoader * loader, loaders) {
        if (c += loader->Open(path, this)) {
            break;
        }
    }

    return c;
}

MachineLoader *GetLoader(QString name)
{
    QList<MachineLoader *>loaders = GetLoaders(MT_CPAP);

    Q_FOREACH(MachineLoader * loader, loaders) {
        if (loader->ClassName() == name) {
            return loader;
        }
    }

    return nullptr;
}


// Returns a QVector containing all machine objects regisered of type t
QList<Machine *> Profile::GetMachines(MachineType t)
{
    QList<Machine *> vec;
    QHash<MachineID, Machine *>::iterator i;
    QHash<MachineID, Machine *>::iterator machlist_end=machlist.end();

    for (i = machlist.begin(); i != machlist_end; i++) {
        if (!i.value()) {
            qWarning() << "Profile::GetMachines() i->second == nullptr";
            continue;
        }

        MachineType mt = i.value()->GetType();

        if ((t == MT_UNKNOWN) || (mt == t)) {
            vec.push_back(i.value());
        }
    }

    return vec;
}

Machine *Profile::GetMachine(MachineType t)
{
    QList<Machine *>vec = GetMachines(t);

    if (vec.size() == 0) {
        return nullptr;
    }

    return vec[0];
}

void Profile::RemoveSession(Session *sess)
{
    QMap<QDate, QList<Day *> >::iterator di;
    QMap<QDate, QList<Day *> >::iterator daylist_end=daylist.end();

    for (di = daylist.begin(); di != daylist_end; di++) {
        for (int d = 0; d < di.value().size(); d++) {
            Day *day = di.value()[d];

            int i = day->getSessions().indexOf(sess);

            if (i >= 0) {
                for (; i < day->getSessions().size() - 1; i++) {
                    day->getSessions()[i] = day->getSessions()[i + 1];
                }

                day->getSessions().pop_back();
                qint64 first = 0, last = 0;

                for (int i = 0; i < day->getSessions().size(); i++) {
                    Session &sess = *day->getSessions()[i];

                    if (!first || first > sess.first()) {
                        first = sess.first();
                    }

                    if (!last || last < sess.last()) {
                        last = sess.last();
                    }
                }

                if (day->size() == 0) {
                    di.value().removeAll(day);
                    delete day;
                }

                // day->setFirst(first);
                // day->setLast(last);
                return;
            }

        }
    }
}


//Profile *profile=nullptr;
QString SHA1(QString pass)
{
    return pass;
}

namespace Profiles {

QMap<QString, Profile *> profiles;

void Done()
{
    PREF.Save();
    LAYOUT.Save();

    Q_FOREACH(Profile * profile, profiles) {
        // TODO: only save open profiles.. (maybe add an open bit?)
        profile->Save();
        delete profile;
    }

    profiles.clear();
    delete p_pref;
    delete p_layout;
    DestroyLoaders();
}

Profile *Get(QString name)
{
    if (profiles.find(name) != profiles.end()) {
        return profiles[name];
    }

    return nullptr;
}
Profile *Create(QString name)
{
    QString path = PREF.Get("{home}/Profiles/") + name;
    QDir dir(path);

    if (!dir.exists(path)) {
        dir.mkpath(path);
    }

    //path+="/"+name;
    Profile *prof = new Profile(path);
    prof->Open();
    profiles[name] = prof;
    prof->user->setUserName(name);
    //prof->Set("Realname",realname);
    //if (!password.isEmpty()) prof.user->setPassword(password);
    prof->Set(STR_GEN_DataFolder, QString("{home}/Profiles/{") + QString(STR_UI_UserName) +
              QString("}"));

    Machine *m = new Machine(prof, 0);
    m->SetClass("Journal");
    m->properties[STR_PROP_Brand] = "Journal";
    m->properties[STR_PROP_Model] = "Journal Data Machine Object";
    m->properties[STR_PROP_Serial] = m->hexid();
    m->properties[STR_PROP_Path] = "{DataFolder}/" + m->GetClass() + "_" + m->hexid();
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
    QString path = PREF.Get("{home}/Profiles");
    QDir dir(path);

    if (!dir.exists(path)) {
        return;
    }

    if (!dir.isReadable()) {
        qWarning() << "Can't open " << path;
        return;
    }

    dir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
    //dir.setSorting(QDir::Name);

    QFileInfoList list = dir.entryInfoList();

    // Iterate through subdirectories and load profiles..
    for (int i = 0; i < list.size(); i++) {
        QFileInfo fi = list.at(i);
        QString npath = fi.canonicalFilePath();
        Profile *prof = new Profile(npath);
//        prof->Open();  // Read it's XML file..
        profiles[fi.fileName()] = prof;
    }

}


} // namespace Profiles

int Profile::countDays(MachineType mt, QDate start, QDate end)
{
    if (!start.isValid()) {
        return 0;
    }

    //start=LastDay(mt);
    if (!end.isValid()) {
        return 0;
    }

    //end=LastDay(mt);
    QDate date = start;

    if (date.isNull()) {
        return 0;
    }

    int days = 0;

    do {
        Day *day = GetGoodDay(date, mt);

        if (day) {
            if ((mt == MT_UNKNOWN) || (day->machine->GetType() == mt)) { days++; }
        }

        date = date.addDays(1);
    } while (date <= end);

    return days;

}

int Profile::countCompliantDays(MachineType mt, QDate start, QDate end)
{
    EventDataType compliance = cpap->complianceHours();

    if (!start.isValid()) {
        return 0;
    }

    if (!end.isValid()) {
        return 0;
    }

    QDate date = start;

    if (date.isNull()) {
        return 0;
    }

    int days = 0;

    do {
        Day *day = GetGoodDay(date, mt);

        if (day) {
            if ((day->machine->GetType() == mt) && (day->hours() > compliance)) { days++; }
        }

        date = date.addDays(1);
    } while (date <= end);

    return days;
}


EventDataType Profile::calcCount(ChannelID code, MachineType mt, QDate start, QDate end)
{
    if (!start.isValid()) {
        start = LastGoodDay(mt);
    }

    if (!end.isValid()) {
        end = LastGoodDay(mt);
    }

    QDate date = start;

    if (date.isNull()) {
        return 0;
    }

    double val = 0;

    do {
        Day *day = GetGoodDay(date, mt);

        if (day) {
            val += day->count(code);
        }

        date = date.addDays(1);
    } while (date <= end);

    return val;
}

double Profile::calcSum(ChannelID code, MachineType mt, QDate start, QDate end)
{
    if (!start.isValid()) {
        start = LastGoodDay(mt);
    }

    if (!end.isValid()) {
        end = LastGoodDay(mt);
    }

    QDate date = start;

    double val = 0;

    do {
        Day *day = GetGoodDay(date, mt);

        if (day) {
            val += day->sum(code);
        }

        date = date.addDays(1);
    } while (date <= end);

    return val;
}

EventDataType Profile::calcHours(MachineType mt, QDate start, QDate end)
{
    if (!start.isValid()) {
        start = LastGoodDay(mt);
    }

    if (!end.isValid()) {
        end = LastGoodDay(mt);
    }

    QDate date = start;

    if (date.isNull()) {
        return 0;
    }

    double val = 0;

    do {
        Day *day = GetGoodDay(date, mt);

        if (day) {
            val += day->hours();
        }

        date = date.addDays(1);
    } while (date <= end);

    return val;
}

EventDataType Profile::calcAboveThreshold(ChannelID code, EventDataType threshold, MachineType mt,
                                 QDate start, QDate end)
{
    if (!start.isValid()) {
        start = LastGoodDay(mt);
    }

    if (!end.isValid()) {
        end = LastGoodDay(mt);
    }

    QDate date = start;

    if (date.isNull()) {
        return 0;
    }

    EventDataType val = 0;

    do {
        Day *day = GetGoodDay(date, mt);

        if (day) {
            val += day->timeAboveThreshold(code, threshold);
        }

        date = date.addDays(1);
    } while (date <= end);

    return val;
}

EventDataType Profile::calcBelowThreshold(ChannelID code, EventDataType threshold, MachineType mt,
                                 QDate start, QDate end)
{
    if (!start.isValid()) {
        start = LastGoodDay(mt);
    }

    if (!end.isValid()) {
        end = LastGoodDay(mt);
    }

    QDate date = start;

    if (date.isNull()) {
        return 0;
    }

    EventDataType val = 0;

    do {
        Day *day = GetGoodDay(date, mt);

        if (day) {
            val += day->timeBelowThreshold(code, threshold);
        }

        date = date.addDays(1);
    } while (date <= end);

    return val;
}


EventDataType Profile::calcAvg(ChannelID code, MachineType mt, QDate start, QDate end)
{
    if (!start.isValid()) {
        start = LastGoodDay(mt);
    }

    if (!end.isValid()) {
        end = LastGoodDay(mt);
    }

    QDate date = start;

    if (date.isNull()) {
        return 0;
    }

    double val = 0;
    int cnt = 0;

    do {
        Day *day = GetGoodDay(date, mt);

        if (day) {
            val += day->sum(code);
            cnt++;
        }

        date = date.addDays(1);
    } while (date <= end);

    if (!cnt) {
        return 0;
    }

    return val / float(cnt);
}

EventDataType Profile::calcWavg(ChannelID code, MachineType mt, QDate start, QDate end)
{
    if (!start.isValid()) {
        start = LastGoodDay(mt);
    }

    if (!end.isValid()) {
        end = LastGoodDay(mt);
    }

    QDate date = start;

    if (date.isNull()) {
        return 0;
    }

    double val = 0, tmp, tmph, hours = 0;

    do {
        Day *day = GetGoodDay(date, mt);

        if (day) {
            tmph = day->hours();
            tmp = day->wavg(code);
            val += tmp * tmph;
            hours += tmph;
        }

        date = date.addDays(1);
    } while (date <= end);

    if (!hours) {
        return 0;
    }

    val = val / hours;
    return val;
}

EventDataType Profile::calcMin(ChannelID code, MachineType mt, QDate start, QDate end)
{
    if (!start.isValid()) {
        start = LastGoodDay(mt);
    }

    if (!end.isValid()) {
        end = LastGoodDay(mt);
    }

    QDate date = start;

    if (date.isNull()) {
        return 0;
    }


    double min = 99999999, tmp;

    do {
        Day *day = GetGoodDay(date, mt);

        if (day) {
            tmp = day->Min(code);

            if (min > tmp) {
                min = tmp;
            }
        }

        date = date.addDays(1);
    } while (date <= end);

    if (min >= 99999999) {
        min = 0;
    }

    return min;
}
EventDataType Profile::calcMax(ChannelID code, MachineType mt, QDate start, QDate end)
{
    if (!start.isValid()) {
        start = LastGoodDay(mt);
    }

    if (!end.isValid()) {
        end = LastGoodDay(mt);
    }

    QDate date = start;

    double max = -99999999, tmp;

    do {
        Day *day = GetGoodDay(date, mt);

        if (day) {
            tmp = day->Max(code);

            if (max < tmp) {
                max = tmp;
            }
        }

        date = date.addDays(1);
    } while (date <= end);

    if (max <= -99999999) {
        max = 0;
    }

    return max;
}
EventDataType Profile::calcSettingsMin(ChannelID code, MachineType mt, QDate start, QDate end)
{
    if (!start.isValid()) {
        start = LastGoodDay(mt);
    }

    if (!end.isValid()) {
        end = LastGoodDay(mt);
    }

    QDate date = start;

    if (date.isNull()) {
        return 0;
    }

    double min = 99999999, tmp;

    do {
        Day *day = GetGoodDay(date, mt);

        if (day) {
            tmp = day->settings_min(code);

            if (min > tmp) {
                min = tmp;
            }
        }

        date = date.addDays(1);
    } while (date <= end);

    if (min >= 99999999) {
        min = 0;
    }

    return min;
}

EventDataType Profile::calcSettingsMax(ChannelID code, MachineType mt, QDate start, QDate end)
{
    if (!start.isValid()) {
        start = LastGoodDay(mt);
    }

    if (!end.isValid()) {
        end = LastGoodDay(mt);
    }

    QDate date = start;

    if (date.isNull()) {
        return 0;
    }

    double max = -99999999, tmp;

    do {
        Day *day = GetGoodDay(date, mt);

        if (day) {
            tmp = day->settings_max(code);

            if (max < tmp) {
                max = tmp;
            }
        }

        date = date.addDays(1);
    } while (date <= end);

    if (max <= -99999999) {
        max = 0;
    }

    return max;
}

struct CountSummary {
    CountSummary(EventStoreType v) : val(v), count(0), time(0) {}
    EventStoreType val;
    EventStoreType count;
    quint32 time;
};

EventDataType Profile::calcPercentile(ChannelID code, EventDataType percent, MachineType mt,
                                      QDate start, QDate end)
{
    if (!start.isValid()) {
        start = LastGoodDay(mt);
    }

    if (!end.isValid()) {
        end = LastGoodDay(mt);
    }

    QDate date = start;

    if (date.isNull()) {
        return 0;
    }

    QMap<EventDataType, qint64> wmap;
    QMap<EventDataType, qint64>::iterator wmi;

    QHash<ChannelID, QHash<EventStoreType, EventStoreType> >::iterator vsi;
    QHash<ChannelID, QHash<EventStoreType, quint32> >::iterator tsi;
    EventDataType gain;
    //bool setgain=false;
    EventDataType value;
    int weight;

    qint64 SN = 0;
    bool timeweight;

    do {
        Day *day = GetGoodDay(date, mt);

        if (day) {
            for (int i = 0; i < day->size(); i++) {
                for (QList<Session *>::iterator s = day->begin(); s != day->end(); s++) {
                    if (!(*s)->enabled()) {
                        continue;
                    }

                    Session *sess = *s;
                    gain = sess->m_gain[code];

                    if (!gain) { gain = 1; }

                    vsi = sess->m_valuesummary.find(code);

                    if (vsi == sess->m_valuesummary.end()) { continue; }

                    tsi = sess->m_timesummary.find(code);
                    timeweight = (tsi != sess->m_timesummary.end());

                    QHash<EventStoreType, EventStoreType> &vsum = vsi.value();
                    QHash<EventStoreType, quint32> &tsum = tsi.value();

                    if (timeweight) {
                        for (QHash<EventStoreType, quint32>::iterator k = tsum.begin(); k != tsum.end(); k++) {
                            weight = k.value();
                            value = EventDataType(k.key()) * gain;

                            SN += weight;
                            wmi = wmap.find(value);

                            if (wmi == wmap.end()) {
                                wmap[value] = weight;
                            } else {
                                wmi.value() += weight;
                            }
                        }
                    } else {
                        for (QHash<EventStoreType, EventStoreType>::iterator k = vsum.begin(); k != vsum.end(); k++) {
                            weight = k.value();
                            value = EventDataType(k.key()) * gain;

                            SN += weight;
                            wmi = wmap.find(value);

                            if (wmi == wmap.end()) {
                                wmap[value] = weight;
                            } else {
                                wmi.value() += weight;
                            }
                        }
                    }
                }
            }
        }

        date = date.addDays(1);
    } while (date <= end);

    QVector<ValueCount> valcnt;

    // Build sorted list of value/counts
    for (wmi = wmap.begin(); wmi != wmap.end(); wmi++) {
        ValueCount vc;
        vc.value = wmi.key();
        vc.count = wmi.value();
        vc.p = 0;
        valcnt.push_back(vc);
    }

    // sort by weight, then value
    qSort(valcnt);

    //double SN=100.0/double(N); // 100% / overall sum
    double p = 100.0 * percent;

    double nth = double(SN) * percent; // index of the position in the unweighted set would be
    double nthi = floor(nth);

    qint64 sum1 = 0, sum2 = 0;
    qint64 w1, w2 = 0;
    double v1 = 0, v2 = 0;

    int N = valcnt.size();
    int k = 0;

    for (k = 0; k < N; k++) {
        v1 = valcnt[k].value;
        w1 = valcnt[k].count;
        sum1 += w1;

        if (sum1 > nthi) {
            return v1;
        }

        if (sum1 == nthi) {
            break; // boundary condition
        }
    }

    if (k >= N) {
        return v1;
    }

    v2 = valcnt[k + 1].value;
    w2 = valcnt[k + 1].count;
    sum2 = sum1 + w2;
    // value lies between v1 and v2

    double px = 100.0 / double(SN); // Percentile represented by one full value

    // calculate percentile ranks
    double p1 = px * (double(sum1) - (double(w1) / 2.0));
    double p2 = px * (double(sum2) - (double(w2) / 2.0));

    // calculate linear interpolation
    double v = v1 + ((p - p1) / (p2 - p1)) * (v2 - v1);

    //  p1.....p.............p2
    //  37     55            70

    return v;
}

// Lookup first day record of the specified machine type, or return the first day overall if MT_UNKNOWN
QDate Profile::FirstDay(MachineType mt)
{
    if ((mt == MT_UNKNOWN) || (!m_last.isValid()) || (!m_first.isValid())) {
        return m_first;
    }

    QDate d = m_first;

    do {
        if (GetDay(d, mt) != nullptr) {
            return d;
        }

        d = d.addDays(1);
    } while (d <= m_last);

    return m_last;
}

// Lookup last day record of the specified machine type, or return the first day overall if MT_UNKNOWN
QDate Profile::LastDay(MachineType mt)
{
    if ((mt == MT_UNKNOWN) || (!m_last.isValid()) || (!m_first.isValid())) {
        return m_last;
    }

    QDate d = m_last;

    do {
        if (GetDay(d, mt) != nullptr) {
            return d;
        }

        d = d.addDays(-1);
    } while (d >= m_first);

    return m_first;
}

QDate Profile::FirstGoodDay(MachineType mt)
{
    if (mt == MT_UNKNOWN) { //|| (!m_last.isValid()) || (!m_first.isValid()))
        return FirstDay();
    }

    QDate d = FirstDay(mt);
    QDate l = LastDay(mt);

    if (!d.isValid() || !l.isValid()) {
        return QDate();
    }

    do {
        if (GetGoodDay(d, mt) != nullptr) {
            return d;
        }

        d = d.addDays(1);
    } while (d <= l);

    return l; //m_last;
}
QDate Profile::LastGoodDay(MachineType mt)
{
    if (mt == MT_UNKNOWN) {
        return FirstDay();
    }

    QDate d = LastDay(mt);
    QDate f = FirstDay(mt);

    if (!(d.isValid() && f.isValid())) {
        return QDate();
    }

    do {
        if (GetGoodDay(d, mt) != nullptr) {
            return d;
        }

        d = d.addDays(-1);
    } while (d >= f);

    return f;
}
bool Profile::hasChannel(ChannelID code)
{
    QDate d = LastDay();
    QDate f = FirstDay();

    if (!(d.isValid() && f.isValid())) {
        return false;
    }

    QMap<QDate, QList<Day *> >::iterator dit;
    QList<Day *>::iterator di;
    QList<Day *>::iterator di_end;

    bool found = false;

    do {
        dit = daylist.find(d);

        if (dit != daylist.end()) {

            di = dit.value().begin();
            di_end = dit.value().end();
            for (; di != di_end; ++di) {
                Day *day = (*di);

                if (day->channelHasData(code)) {
                    found = true;
                    break;
                }
            }
        }

        if (found) {
            break;
        }

        d = d.addDays(-1);
    } while (d >= f);

    return found;
}
