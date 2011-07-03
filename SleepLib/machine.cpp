/*
 SleepLib Machine Class Implementation
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

#include <QDir>
#include <QProgressBar>
#include <QDebug>
#include <QString>
#include <QObject>
#include <tr1/random>
#include <sys/time.h>
#include "binary_file.h"
#include "machine.h"
#include "profiles.h"
#include <algorithm>

extern QProgressBar * qprogress;

map<MachineType,ChannelCode> MachLastCode;

/* ChannelCode RegisterChannel(MachineType type)
{
    if (MachLastCode.find(type)==MachLastCode.end()) {
        return MachLastCode[type]=0;
    }
    return ++(MachLastCode[type]);
};

const int CPAP_WHATEVER=RegisterChannel(MT_CPAP); */
//map<MachineID,Machine *> MachList;

/*map<MachineType,QString> MachineTypeString= {
    {MT_UNKNOWN, 	wxT("Unknown")},
    {MT_CPAP,	 	wxT("CPAP")},
    {MT_OXIMETER,	wxT("Oximeter")},
    {MT_SLEEPSTAGE,	wxT("SleepStage")}
}; */

/*map<QString,MachineType> MachineTypeLookup= {
    { MachineTypeString[MT_UNKNOWN].Lower(),	MT_UNKNOWN },
    { MachineTypeString[MT_CPAP].Lower(), 		MT_CPAP },
    { MachineTypeString[MT_OXIMETER].Lower(), 	MT_OXIMETER },
    { MachineTypeString[MT_SLEEPSTAGE].Lower(), MT_SLEEPSTAGE}
}; */
map<CPAPMode,QString> CPAPModeNames;
/*={
    {MODE_UNKNOWN,wxT("Undetermined")},
    {MODE_CPAP,wxT("CPAP")},
    {MODE_APAP,wxT("APAP")},
    {MODE_BIPAP,wxT("BIPAP")},
    {MODE_ASV,wxT("ASV")}
};*/
map<PRTypes,QString> PressureReliefNames;
/*={
    {PR_UNKNOWN,_("Undetermined")},
    {PR_NONE,_("None")},
    {PR_CFLEX,wxT("C-Flex")},
    {PR_CFLEXPLUS,wxT("C-Flex+")},
    {PR_AFLEX,wxT("A-Flex")},
    {PR_BIFLEX,wxT("Bi-Flex")},
    {PR_EPR,wxT("Exhalation Pressure Relief (EPR)")},
    {PR_SMARTFLEX,wxT("SmartFlex")}
}; */



// Master list. Look up local name table first.. then these if not found.
map<MachineCode,QString> DefaultMCShortNames;
/*= {
    {CPAP_Obstructive,	wxT("OA")},
    {CPAP_Hypopnea,		wxT("H")},
    {CPAP_RERA,			wxT("RE")},
    {CPAP_ClearAirway,	wxT("CA")},
    {CPAP_CSR,			wxT("CSR")},
    {CPAP_VSnore,		wxT("VS")},
    {CPAP_FlowLimit,	wxT("FL")},
    {CPAP_Pressure,		wxT("P")},
    {CPAP_Leak,			wxT("LR")},
    {CPAP_EAP,			wxT("EAP")},
    {CPAP_IAP,			wxT("IAP")},
    {PRS1_VSnore2,		wxT("VS")},
    {PRS1_PressurePulse,wxT("PP")}
}; */

// Master list. Look up local name table first.. then these if not found.
map<MachineCode,QString> DefaultMCLongNames;
/*= {
    {CPAP_Obstructive,	wxT("Obstructive Apnea")},
    {CPAP_Hypopnea,		wxT("Hypopnea")},
    {CPAP_RERA,			wxT("Respiratory Effort / Arrousal")},
    {CPAP_ClearAirway,	wxT("Clear Airway Apnea")},
    {CPAP_CSR,			wxT("Cheyne Stokes Respiration")},
    {CPAP_VSnore,		wxT("Vibratory Snore")},
    {CPAP_FlowLimit,	wxT("Flow Limitation")},
    {CPAP_Pressure,		wxT("Pressure")},
    {CPAP_Leak,			wxT("Leak Rate")},
    {CPAP_EAP,			wxT("BIPAP EPAP")},
    {CPAP_IAP,			wxT("BIPAP IPAP")},
    {PRS1_VSnore2,		wxT("Vibratory Snore")},
    {PRS1_PressurePulse,wxT("Pressue Pulse")}
}; */

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
    DefaultMCLongNames[CPAP_RERA]=QObject::tr("RERA");
    DefaultMCLongNames[CPAP_ClearAirway]=QObject::tr("Clear Airway Apnea");
    DefaultMCLongNames[CPAP_CSR]=QObject::tr("Periodic Breathing");
    DefaultMCLongNames[CPAP_VSnore]=QObject::tr("Vibratory Snore"); // flags type
    DefaultMCLongNames[CPAP_FlowLimit]=QObject::tr("Flow Limitation");
    DefaultMCLongNames[CPAP_Pressure]=QObject::tr("Pressure");
    DefaultMCLongNames[CPAP_Leak]=QObject::tr("Leak Rate");
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



// This is technically gui related.. however I have something in mind for it.
/*const map<MachineCode,FlagType> DefaultFlagTypes= {
    {CPAP_Obstructive,	FT_BAR},
    {CPAP_Hypopnea,		FT_BAR},
    {CPAP_RERA,			FT_BAR},
    {CPAP_VSnore,		FT_BAR},
    {PRS1_VSnore2,		FT_BAR},
    {CPAP_FlowLimit,	FT_BAR},
    {CPAP_ClearAirway,	FT_BAR},
    {CPAP_CSR,			FT_SPAN},
    {PRS1_PressurePulse,FT_DOT},
    {CPAP_Pressure,		FT_DOT}
};

const unsigned char flagalpha=0x80;
const map<MachineCode,wxColour> DefaultFlagColours= {
    {CPAP_Obstructive,	wxColour(0x80,0x80,0xff,flagalpha)},
    {CPAP_Hypopnea,		wxColour(0x00,0x00,0xff,flagalpha)},
    {CPAP_RERA,			wxColour(0x40,0x80,0xff,flagalpha)},
    {CPAP_VSnore,		wxColour(0xff,0x20,0x20,flagalpha)},
    {CPAP_FlowLimit,	wxColour(0x20,0x20,0x20,flagalpha)},
    {CPAP_ClearAirway,	wxColour(0xff,0x40,0xff,flagalpha)},
    {CPAP_CSR,			wxColour(0x40,0xff,0x40,flagalpha)},
    {PRS1_VSnore2,		wxColour(0xff,0x20,0x20,flagalpha)},
    {PRS1_PressurePulse,wxColour(0xff,0x40,0xff,flagalpha)}
}; */


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
    assert(s!=NULL);
    assert(p!=NULL);

    if (s->session()>highest_sessionid)
        highest_sessionid=s->session();

    QDateTime d1,d2=QDateTime::fromMSecsSinceEpoch(s->first());

    QDate date=d2.date();
    date.addDays(-1);

    //qint64 date=s->first()/86400000;
    //date--; //=date.addDays(-1);
    //date*=86400000;

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
        int j=fullpath.lastIndexOf(".");
        QString ext_s=*(fullpath.rightRef(j+1).string());
        bool ok;
        ext_s.toInt(&ok,10);
        if (ok) {
            qDebug() << "TestMe: Deleting " << fullpath;
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
            sess->SetWaveFile(s->second[2]);

            AddSession(sess,profile);
        } else {
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

        for (s=d->second->begin(); s!=d->second->end(); s++) {
            cnt++;
            if (qprogress) qprogress->setValue(66.0+(float(cnt)/float(size)*33.0));
            if ((*s)->IsChanged()) (*s)->Store(path);
        }
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





