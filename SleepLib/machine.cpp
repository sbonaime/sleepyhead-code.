/*
 SleepLib Machine Class Implementation
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

#include <QApplication>
#include <QDir>
#include <QProgressBar>
#include <QDebug>
#include <QString>
#include <QObject>
#include <tr1/random>
#include <sys/time.h>
#include "machine.h"
#include "profiles.h"
#include <algorithm>

extern QProgressBar * qprogress;

//map<MachineType,ChannelCode> MachLastCode;

map<CPAPMode,QString> CPAPModeNames;

map<PRTypes,QString> PressureReliefNames;

// Master list. Look up local name table first.. then these if not found.
map<MachineCode,QString> DefaultMCShortNames;

// Master list. Look up local name table first.. then these if not found.
map<MachineCode,QString> DefaultMCLongNames;

void InitMapsWithoutAwesomeInitializerLists()
{
    CPAPModeNames[MODE_UNKNOWN]=QObject::tr("Undetermined");
    CPAPModeNames[MODE_CPAP]=QObject::tr("CPAP");
    CPAPModeNames[MODE_APAP]=QObject::tr("Auto");
    CPAPModeNames[MODE_BIPAP]=QObject::tr("BIPAP");
    CPAPModeNames[MODE_ASV]=QObject::tr("ASV");

    PressureReliefNames[PR_UNKNOWN]=QObject::tr("Undetermined");
    PressureReliefNames[PR_NONE]=QObject::tr("None");
    PressureReliefNames[PR_CFLEX]=QObject::tr("C-Flex");
    PressureReliefNames[PR_CFLEXPLUS]=QObject::tr("C-Flex+");
    PressureReliefNames[PR_AFLEX]=QObject::tr("A-Flex");
    PressureReliefNames[PR_BIFLEX]=QObject::tr("Bi-Flex");
    PressureReliefNames[PR_EPR]=QObject::tr("Exhalation Pressure Relief (EPR)");
    PressureReliefNames[PR_SMARTFLEX]=QObject::tr("SmartFlex");

    DefaultMCShortNames[CPAP_Obstructive]=QObject::tr("OA");
    DefaultMCShortNames[CPAP_Apnea]=QObject::tr("A");
    DefaultMCShortNames[CPAP_Hypopnea]=QObject::tr("H");
    DefaultMCShortNames[CPAP_RERA]=QObject::tr("RE");
    DefaultMCShortNames[CPAP_ClearAirway]=QObject::tr("CA");
    DefaultMCShortNames[CPAP_CSR]=QObject::tr("CSR/PB");
    DefaultMCShortNames[CPAP_VSnore]=QObject::tr("VS");
    DefaultMCShortNames[PRS1_VSnore2]=QObject::tr("VS2");
    DefaultMCShortNames[CPAP_FlowLimit]=QObject::tr("FL");
    DefaultMCShortNames[CPAP_Pressure]=QObject::tr("P");
    DefaultMCShortNames[CPAP_Leak]=QObject::tr("LR");
    DefaultMCShortNames[CPAP_EAP]=QObject::tr("EPAP");
    DefaultMCShortNames[CPAP_IAP]=QObject::tr("IPAP");
    DefaultMCShortNames[PRS1_PressurePulse]=QObject::tr("PP");

    DefaultMCLongNames[CPAP_Obstructive]=QObject::tr("Obstructive Apnea");
    DefaultMCLongNames[CPAP_Hypopnea]=QObject::tr("Hypopnea");
    DefaultMCLongNames[CPAP_Apnea]=QObject::tr("Apnea");
    DefaultMCLongNames[CPAP_RERA]=QObject::tr("RERA");
    DefaultMCLongNames[CPAP_ClearAirway]=QObject::tr("Clear Airway Apnea");
    DefaultMCLongNames[CPAP_CSR]=QObject::tr("Periodic Breathing");
    DefaultMCLongNames[CPAP_VSnore]=QObject::tr("Vibratory Snore"); // flags type
    DefaultMCLongNames[CPAP_FlowLimit]=QObject::tr("Flow Limitation");
    DefaultMCLongNames[CPAP_Pressure]=QObject::tr("Pressure");
    DefaultMCLongNames[CPAP_Leak]=QObject::tr("Leak Rate");
    DefaultMCLongNames[CPAP_PS]=QObject::tr("Pressure Support");
    DefaultMCLongNames[CPAP_EAP]=QObject::tr("BIPAP EPAP");
    DefaultMCLongNames[CPAP_IAP]=QObject::tr("BIPAP IPAP");
    DefaultMCLongNames[CPAP_Snore]=QObject::tr("Vibratory Snore");  // Graph data
    DefaultMCLongNames[PRS1_VSnore2]=QObject::tr("Vibratory Snore (Graph)");
    DefaultMCLongNames[PRS1_PressurePulse]=QObject::tr("Pressure Pulse");
    DefaultMCLongNames[PRS1_Unknown0E]=QObject::tr("Unknown 0E");
    DefaultMCLongNames[PRS1_Unknown00]=QObject::tr("Unknown 00");
    DefaultMCLongNames[PRS1_Unknown01]=QObject::tr("Unknown 01");
    DefaultMCLongNames[PRS1_Unknown0B]=QObject::tr("Unknown 0B");
    DefaultMCLongNames[PRS1_Unknown10]=QObject::tr("Unknown 10");
}


//////////////////////////////////////////////////////////////////////////////////////////
// Machine Base-Class implmementation
//////////////////////////////////////////////////////////////////////////////////////////
Machine::Machine(Profile *p,MachineID id)
{
    day.clear();
    highest_sessionid=0;
    profile=p;
    if (!id) {
        std::tr1::minstd_rand gen;
        std::tr1::uniform_int<MachineID> unif(1, 0x7fffffff);
        gen.seed((unsigned int) time(NULL));
        MachineID temp;
        do {
            temp = unif(gen); //unif(gen) << 32 |
        } while (profile->machlist.find(temp)!=profile->machlist.end());

        m_id=temp;

    } else m_id=id;
    qDebug() << "Create Machine: " << hex << m_id; //%lx",m_id);
    m_type=MT_UNKNOWN;
    firstsession=true;
}
Machine::~Machine()
{
    qDebug() << "Destroy Machine";
    map<QDate,Day *>::iterator d;
    for (d=day.begin();d!=day.end();d++) {
        delete d->second;
    }
}
Session *Machine::SessionExists(SessionID session)
{
    if (sessionlist.find(session)!=sessionlist.end()) {
        return sessionlist[session];
    } else {
        return NULL;
    }
}

Day *Machine::AddSession(Session *s,Profile *p)
{
    double span=0;
    if (!s) {
        qWarning() << "Empty Session in Machine::AddSession()";
        return NULL;
    }
    if (!p) {
        qWarning() << "Empty Profile in Machine::AddSession()";
        return NULL;
    }
    if (s->session()>highest_sessionid)
        highest_sessionid=s->session();

    QDateTime d1,d2=QDateTime::fromMSecsSinceEpoch(s->first());

    QDate date=d2.date();
    //QTime time=d2.time();

    if (pref.Exists("NoonDataSplit") && pref["NoonDateSplit"].toBool()) {
        int hour=d2.time().hour();
        if (hour<12)
            date=date.addDays(-1);
            //date=date.addDays(1);
    } else {
        date=date.addDays(-1);

        const int hours_since_last_session=4;
        const int hours_since_midnight=12;

        bool previous=false;
        // Find what day session belongs to.
        if (day.find(date)!=day.end()) {
            // Previous day record exists...

            // Calculate time since end of previous days last session
            span=(s->first() - day[date]->last())/3600000.0;

            // less than n hours since last session yesterday?
            if (span < hours_since_last_session) {
                previous=true;
            }
        }

        if (!previous) {
            // Calculate Hours since midnight.
            d1=d2;
            d1.setTime(QTime(0,0,0));
            float secsto=d1.secsTo(d2);
            span=secsto/3600.0;

            // Bedtime was late last night.
            if (span < hours_since_midnight) {

                previous=true;
            }
        }

        if (!previous) {
            // Revert to sessions original day.
            date=date.addDays(1);
        }
    }
    sessionlist[s->session()]=s;

    if (!firstsession) {
        if (firstday>date) firstday=date;
        if (lastday<date) lastday=date;
    } else {
        firstday=lastday=date;
        firstsession=false;
    }

    if (day.find(date)==day.end()) {
        //QString dstr=date.toString("yyyyMMdd");
        //qDebug("Adding Profile Day %s",dstr.toAscii().data());
        day[date]=new Day(this);
        // Add this Day record to profile
        //QDateTime d=QDateTime::fromMSecsSinceEpoch(date);
        //qDebug() << "New day: " << d.toString("yyyy-MM-dd HH:mm:ss");
        p->AddDay(date,day[date],m_type);
    }
    day[date]->AddSession(s);

    return day[date];
}

// This functions purpose is murder and mayhem... It deletes all of a machines data.
// Therefore this is the most dangerous function in this software..
bool Machine::Purge(int secret)
{
    // Boring api key to stop this function getting called by accident :)
    if (secret!=3478216) return false;


    // It would be joyous if this function screwed up..
    QString path=profile->Get("DataFolder")+"/"+hexid();

    QDir dir(path);

    if (!dir.exists() || !dir.isReadable())
        return false;


    qDebug() << "Purging " << QDir::toNativeSeparators(path);

    dir.setFilter(QDir::Files | QDir::Hidden | QDir::NoSymLinks);
    dir.setSorting(QDir::Name);

    QFileInfoList list=dir.entryInfoList();
    int could_not_kill=0;

    for (int i=0;i<list.size();++i) {
        QFileInfo fi=list.at(i);
        QString fullpath=fi.canonicalFilePath();
        //int j=fullpath.lastIndexOf(".");

        QString ext_s=fullpath.section('.',-1);//right(j);
        bool ok;
        ext_s.toInt(&ok,10);
        if (ok) {
            qDebug() << "Deleting " << fullpath;
            dir.remove(fullpath);
        } else could_not_kill++;

    }
    if (could_not_kill>0) {
        qWarning() << "Could not purge path\n" << path << "\n\n" << could_not_kill << " file(s) remain.. Suggest manually deleting this path\n";
        return false;
    }

    return true;
}
bool Machine::Load()
{
    QString path=profile->Get("DataFolder")+"/"+hexid();
    QDir dir(path);
    qDebug() << "Loading " << path;

    if (!dir.exists() || !dir.isReadable())
        return false;


    dir.setFilter(QDir::Files | QDir::Hidden | QDir::NoSymLinks);
    dir.setSorting(QDir::Name);

    QFileInfoList list=dir.entryInfoList();

    typedef vector<QString> StringList;
    map<SessionID,StringList> sessfiles;
    map<SessionID,StringList>::iterator s;

    QString fullpath,ext_s,sesstr;
    int ext;
    SessionID sessid;
    bool ok;
    for (int i=0;i<list.size();i++) {
        QFileInfo fi=list.at(i);
        fullpath=fi.canonicalFilePath();
        ext_s=fi.fileName().section(".",-1);
        ext=ext_s.toInt(&ok,10);
        if (!ok) continue;
        sesstr=fi.fileName().section(".",0,-2);
        sessid=sesstr.toLong(&ok,16);
        if (!ok) continue;
        if (sessfiles[sessid].capacity()==0) sessfiles[sessid].resize(3);
        sessfiles[sessid][ext]=fi.canonicalFilePath();
    }

    int size=sessfiles.size();
    int cnt=0;
    for (s=sessfiles.begin(); s!=sessfiles.end(); s++) {
        cnt++;
        if ((cnt % 10)==0)
            if (qprogress) qprogress->setValue((float(cnt)/float(size)*100.0));

        Session *sess=new Session(this,s->first);

        if (sess->LoadSummary(s->second[0])) {
             sess->SetEventFile(s->second[1]);
             AddSession(sess,profile);
        } else {
            qWarning() << "Error unpacking summary data";
            delete sess;
        }
    }
    if (qprogress) qprogress->setValue(100);
    return true;
}
bool Machine::SaveSession(Session *sess)
{
    QString path=profile->Get("DataFolder")+"/"+hexid();
    if (sess->IsChanged()) sess->Store(path);
    return true;
}
bool Machine::Save()
{
    map<QDate,Day *>::iterator d;
    vector<Session *>::iterator s;
    int size=0;
    int cnt=0;

    QString path=profile->Get("DataFolder")+"/"+hexid();

    // Calculate size for progress bar
    for (d=day.begin();d!=day.end();d++)
        size+=d->second->size();

    for (d=day.begin();d!=day.end();d++) {

        //qDebug() << "Day Save Commenced";
        for (s=d->second->begin(); s!=d->second->end(); s++) {
            cnt++;
            if (qprogress) qprogress->setValue(66.0+(float(cnt)/float(size)*33.0));
            if ((*s)->IsChanged()) (*s)->Store(path);
            (*s)->TrashEvents();
            QApplication::processEvents();

        }
        //qDebug() << "Day Save Completed";

    }
    return true;
}

//////////////////////////////////////////////////////////////////////////////////////////
// CPAP implmementation
//////////////////////////////////////////////////////////////////////////////////////////
CPAP::CPAP(Profile *p,MachineID id):Machine(p,id)
{
    m_type=MT_CPAP;

//    FlagColours=DefaultFlagColours;
}

CPAP::~CPAP()
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// Oximeter Class implmementation
//////////////////////////////////////////////////////////////////////////////////////////
Oximeter::Oximeter(Profile *p,MachineID id):Machine(p,id)
{
    m_type=MT_OXIMETER;
}

Oximeter::~Oximeter()
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// SleepStage Class implmementation
//////////////////////////////////////////////////////////////////////////////////////////
SleepStage::SleepStage(Profile *p,MachineID id):Machine(p,id)
{
    m_type=MT_SLEEPSTAGE;
}
SleepStage::~SleepStage()
{
}





