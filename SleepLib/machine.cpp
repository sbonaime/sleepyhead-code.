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
#include <time.h>

#include "machine.h"
#include "profiles.h"
#include <algorithm>
#include "SleepLib/schema.h"

extern QProgressBar * qprogress;

//////////////////////////////////////////////////////////////////////////////////////////
// Machine Base-Class implmementation
//////////////////////////////////////////////////////////////////////////////////////////
Machine::Machine(Profile *p,MachineID id)
{
    day.clear();
    highest_sessionid=0;
    profile=p;
    if (!id) {
        srand(time(NULL));
        MachineID temp;
        do {
            temp = rand();
        } while (profile->machlist.find(temp)!=profile->machlist.end());

        m_id=temp;

    } else m_id=id;
    //qDebug() << "Create Machine: " << hex << m_id; //%lx",m_id);
    m_type=MT_UNKNOWN;
    firstsession=true;
}
Machine::~Machine()
{
    qDebug() << "Destroy Machine";
    for (QMap<QDate,Day *>::iterator d=day.begin();d!=day.end();d++) {
        delete d.value();
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

// Find date this session belongs in
QDate Machine::pickDate(qint64 first)
{
    QTime split_time=PROFILE.session->daySplitTime();
    int combine_sessions=PROFILE.session->combineCloseSessions();

    QDateTime d2=QDateTime::fromTime_t(first/1000);

    QDate date=d2.date();
    QTime time=d2.time();

    int closest_session=0;

    if (time<split_time) {
        date=date.addDays(-1);
    } else if (combine_sessions > 0) {
        QMap<QDate,Day *>::iterator dit=day.find(date.addDays(-1)); // Check Day Before
        if (dit != day.end()) {
            QDateTime lt=QDateTime::fromTime_t(dit.value()->last()/1000L);
            closest_session=lt.secsTo(d2)/60;
            if (closest_session < combine_sessions) {
                date=date.addDays(-1);
            }
        }
    }

    return date;
}

QDate Machine::AddSession(Session *s,Profile *p)
{
    if (!s) {
        qWarning() << "Empty Session in Machine::AddSession()";
        return QDate();
    }
    if (!p) {
        qWarning() << "Empty Profile in Machine::AddSession()";
        return QDate();
    }
    if (s->session()>highest_sessionid)
        highest_sessionid=s->session();


    QTime split_time=PROFILE.session->daySplitTime();
    int combine_sessions=PROFILE.session->combineCloseSessions();
    int ignore_sessions=PROFILE.session->ignoreShortSessions();

    int session_length=s->last()-s->first();
    session_length/=60000;

    sessionlist[s->session()]=s; // To make sure it get's saved later even if it's not wanted.

    //int drift=PROFILE.cpap->clockDrift();

    QDateTime d2=QDateTime::fromTime_t(s->first()/1000);

    QDate date=d2.date();
    QTime time=d2.time();

    QMap<QDate,Day *>::iterator dit,nextday;

    bool combine_next_day=false;
    int closest_session=0;

    if (time<split_time) {
        date=date.addDays(-1);
    } else if (combine_sessions > 0) {
        dit=day.find(date.addDays(-1)); // Check Day Before
        if (dit!=day.end()) {
            QDateTime lt=QDateTime::fromTime_t(dit.value()->last()/1000);
            closest_session=lt.secsTo(d2)/60;
            if (closest_session<combine_sessions) {
                date=date.addDays(-1);
            }
        } else {
            nextday=day.find(date.addDays(1));// Check Day Afterwards
            if (nextday!=day.end()) {
                QDateTime lt=QDateTime::fromTime_t(nextday.value()->first()/1000);
                closest_session=d2.secsTo(lt)/60;
                if (closest_session < combine_sessions) {
                    // add todays here. pull all tomorrows records to this date.
                    combine_next_day=true;
                }
            }
        }
    }

    if (session_length<ignore_sessions) {
        //if (!closest_session || (closest_session>=60))
        return QDate();
    }

    if (!firstsession) {
        if (firstday>date) firstday=date;
        if (lastday<date) lastday=date;
    } else {
        firstday=lastday=date;
        firstsession=false;
    }


    Day *dd=NULL;
    dit=day.find(date);
    if (dit==day.end()) {
        //QString dstr=date.toString("yyyyMMdd");
        //qDebug("Adding Profile Day %s",dstr.toLatin1().data());
        dd=new Day(this);
        day[date]=dd;
        // Add this Day record to profile
        p->AddDay(date,dd,m_type);
    } else {
        dd=*dit;
    }
    dd->AddSession(s);

    if (combine_next_day) {
        for (QVector<Session *>::iterator i=nextday.value()->begin();i!=nextday.value()->end();i++) {
            dd->AddSession(*i);
        }
        QMap<QDate,QList<Day *> >::iterator nd=p->daylist.find(date.addDays(1));
        for (QList<Day *>::iterator i=nd->begin();i!=nd->end();i++) {
            if (*i==nextday.value()) {
                nd.value().erase(i);
            }
        }
        day.erase(nextday);
    }
    return date;
}

// This functions purpose is murder and mayhem... It deletes all of a machines data.
// Therefore this is the most dangerous function in this software..
bool Machine::Purge(int secret)
{
    // Boring api key to stop this function getting called by accident :)
    if (secret!=3478216) return false;


    // It would be joyous if this function screwed up..

    QString path=profile->Get(properties[STR_PROP_Path]); //STR_GEN_DataFolder)+"/"+m_class+"_";
    //if (properties.contains(STR_PROP_Serial)) path+=properties[STR_PROP_Serial]; else path+=hexid();

    QDir dir(path);

    if (!dir.exists()) // It doesn't exist anyway.
        return true;
    if (!dir.isReadable())
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
      //  qWarning() << "Could not purge path\n" << path << "\n\n" << could_not_kill << " file(s) remain.. Suggest manually deleting this path\n";
    //    return false;
    }

    return true;
}

const quint32 channel_version=1;


bool Machine::Load()
{
    QString path=profile->Get(properties[STR_PROP_Path]); //STR_GEN_DataFolder)+"/"+m_class+"_"+hexid();

    QDir dir(path);
    qDebug() << "Loading " << path;

    if (!dir.exists() || !dir.isReadable())
        return false;

    dir.setFilter(QDir::Files | QDir::Hidden | QDir::NoSymLinks);
    dir.setSorting(QDir::Name);

    QFileInfoList list=dir.entryInfoList();

    typedef QVector<QString> StringList;
    QMap<SessionID,StringList> sessfiles;
    QMap<SessionID,StringList>::iterator s;

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
        if ((++cnt % 50)==0) { // This is slow.. :-/
            if (qprogress) qprogress->setValue((float(cnt)/float(size)*100.0));
            QApplication::processEvents();
        }

        Session *sess=new Session(this,s.key());

        if (sess->LoadSummary(s.value()[0])) {
             sess->SetEventFile(s.value()[1]);
             //sess->OpenEvents();
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
    QString path=profile->Get(properties[STR_PROP_Path]); //STR_GEN_DataFolder)+"/"+m_class+"_"+hexid();
    if (sess->IsChanged()) sess->Store(path);
    return true;
}

bool Machine::Save()
{
    //int size;
    int cnt=0;

    QString path=profile->Get(properties[STR_PROP_Path]); //STR_GEN_DataFolder)+"/"+m_class+"_"+hexid();
    QDir dir(path);
    if (!dir.exists()) {
        dir.mkdir(path);
    }

    QHash<SessionID,Session *>::iterator s;

    m_savelist.clear();
    for (s=sessionlist.begin(); s!=sessionlist.end(); s++) {
        cnt++;
        if ((*s)->IsChanged()) {
            m_savelist.push_back(*s);
        }
    }
    savelistCnt=0;
    savelistSize=m_savelist.size();
    bool cachesessions=PROFILE.session->cacheSessions();
    if (!PROFILE.session->multithreading()) {
        for (int i=0;i<savelistSize;i++) {
            if ((i % 10) ==0) {
                qprogress->setValue(0+(float(savelistCnt)/float(savelistSize)*100.0));
                QApplication::processEvents();
            }
            Session *s=m_savelist.at(i);
            s->UpdateSummaries();
            s->Store(path);
            if (!cachesessions)
                s->TrashEvents();
            savelistCnt++;

        }
        return true;
    }
    int threads=QThread::idealThreadCount();
    savelistSem=new QSemaphore(threads);
    savelistSem->acquire(threads);
    QVector<SaveThread*>thread;
    for (int i=0;i<threads;i++) {
        thread.push_back(new SaveThread(this,path));
        QObject::connect(thread[i],SIGNAL(UpdateProgress(int)),qprogress,SLOT(setValue(int)));
        thread[i]->start();
    }
    while (!savelistSem->tryAcquire(threads,250)) {
        if (qprogress) {
        //    qprogress->setValue(66.0+(float(savelistCnt)/float(savelistSize)*33.0));
           QApplication::processEvents();
        }
    }

    for (int i=0;i<threads;i++) {
        while (thread[i]->isRunning()) {
            SaveThread::msleep(250);
            QApplication::processEvents();
        }
        delete thread[i];
    }

    delete savelistSem;
    return true;
}

/*SaveThread::SaveThread(Machine *m,QString p)
{
    machine=m;
    path=p;
} */

void SaveThread::run()
{
    bool cachesessions=PROFILE.session->cacheSessions();

    while (Session *sess=machine->popSaveList()) {
        int i=(float(machine->savelistCnt)/float(machine->savelistSize)*100.0);
        emit UpdateProgress(i);
        sess->UpdateSummaries();
        sess->Store(path);
        if (!cachesessions)
            sess->TrashEvents();
    }
    machine->savelistSem->release(1);
}

Session *Machine::popSaveList()
{

    Session *sess=NULL;
    savelistMutex.lock();
    if (m_savelist.size()>0) {
        sess=m_savelist.at(0);
        m_savelist.pop_front();
        savelistCnt++;
    }
    savelistMutex.unlock();
    return sess;
}

//////////////////////////////////////////////////////////////////////////////////////////
// CPAP implmementation
//////////////////////////////////////////////////////////////////////////////////////////
CPAP::CPAP(Profile *p,MachineID id):Machine(p,id)
{
    m_type=MT_CPAP;
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


ChannelID NoChannel, SESSION_ENABLED;
ChannelID CPAP_IPAP, CPAP_IPAPLo, CPAP_IPAPHi, CPAP_EPAP, CPAP_Pressure, CPAP_PS, CPAP_Mode, CPAP_AHI,
CPAP_PressureMin, CPAP_PressureMax, CPAP_RampTime, CPAP_RampPressure, CPAP_Obstructive, CPAP_Hypopnea,
CPAP_ClearAirway, CPAP_Apnea, CPAP_CSR, CPAP_LeakFlag, CPAP_ExP, CPAP_NRI, CPAP_VSnore, CPAP_VSnore2,
CPAP_RERA, CPAP_PressurePulse, CPAP_FlowLimit, CPAP_FlowRate, CPAP_MaskPressure, CPAP_MaskPressureHi,
CPAP_RespEvent, CPAP_Snore, CPAP_MinuteVent, CPAP_RespRate, CPAP_TidalVolume, CPAP_PTB, CPAP_Leak,
CPAP_LeakMedian, CPAP_LeakTotal, CPAP_MaxLeak, CPAP_FLG, CPAP_IE, CPAP_Te, CPAP_Ti, CPAP_TgMV,
CPAP_UserFlag1, CPAP_UserFlag2, CPAP_UserFlag3, CPAP_BrokenSummary, CPAP_BrokenWaveform, CPAP_RDI,
CPAP_PresReliefSet, CPAP_PresReliefMode, CPAP_PresReliefType, CPAP_PSMin, CPAP_PSMax, CPAP_Test1, CPAP_Test2;


ChannelID RMS9_E01, RMS9_E02, RMS9_EPR, RMS9_EPRSet, RMS9_SetPressure;
ChannelID INTP_SmartFlex;
ChannelID INTELLIPAP_Unknown1, INTELLIPAP_Unknown2;

ChannelID PRS1_00, PRS1_01, PRS1_08, PRS1_0A, PRS1_0B, PRS1_0C, PRS1_0E, PRS1_0F, PRS1_10, PRS1_12,
PRS1_FlexMode, PRS1_FlexSet, PRS1_HumidStatus, CPAP_HumidSetting, PRS1_SysLock, PRS1_SysOneResistStat,
PRS1_SysOneResistSet, PRS1_HoseDiam, PRS1_AutoOn, PRS1_AutoOff, PRS1_MaskAlert, PRS1_ShowAHI;

ChannelID OXI_Pulse, OXI_SPO2, OXI_PulseChange, OXI_SPO2Drop, OXI_Plethy;

ChannelID Journal_Notes, Journal_Weight, Journal_BMI, Journal_ZombieMeter, Bookmark_Start, Bookmark_End, Bookmark_Notes;


ChannelID ZEO_SleepStage, ZEO_ZQ, ZEO_TotalZ, ZEO_TimeToZ, ZEO_TimeInWake, ZEO_TimeInREM, ZEO_TimeInLight, ZEO_TimeInDeep, ZEO_Awakenings,
ZEO_AlarmReason, ZEO_SnoozeTime, ZEO_WakeTone, ZEO_WakeWindow, ZEO_AlarmType, ZEO_MorningFeel, ZEO_FirmwareVersion,
ZEO_FirstAlarmRing, ZEO_LastAlarmRing, ZEO_FirstSnoozeTime, ZEO_LastSnoozeTime, ZEO_SetAlarmTime, ZEO_RiseTime;

