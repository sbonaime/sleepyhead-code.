/* SleepLib Machine Class Implementation
 *
 * Copyright (c) 2011-2014 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#include <QApplication>
#include <QDir>
#include <QDebug>
#include <QString>
#include <QObject>
#include <QThreadPool>
#include <QFile>
#include <QDataStream>

#include <QDialog>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include "mainwindow.h"

#include "progressdialog.h"

#include <time.h>

#include "machine.h"
#include "profiles.h"
#include <algorithm>
#include "SleepLib/schema.h"
#include "SleepLib/day.h"

extern QProgressBar *qprogress;

//////////////////////////////////////////////////////////////////////////////////////////
// Machine Base-Class implmementation
//////////////////////////////////////////////////////////////////////////////////////////
Machine::Machine(MachineID id)
{
    day.clear();
    highest_sessionid = 0;

    if (!id) {
        srand(time(nullptr));
        MachineID temp;

        do {
            temp = rand();
        } while (p_profile->machlist.find(temp) != p_profile->machlist.end());

        m_id = temp;

    } else { m_id = id; }
    m_loader = nullptr;

   // qDebug() << "Create Machine: " << hex << m_id; //%lx",m_id);
    m_type = MT_UNKNOWN;
    firstsession = true;
}
Machine::~Machine()
{
    saveSessionInfo();
    qDebug() << "Destroy Machine" << info.loadername << hex << m_id;
}
Session *Machine::SessionExists(SessionID session)
{
    if (sessionlist.find(session) != sessionlist.end()) {
        return sessionlist[session];
    } else {
        return nullptr;
    }
}

const quint16 sessinfo_version = 0;

bool Machine::saveSessionInfo()
{
    QFile file(getDataPath() + "Sessions.info");
    file.open(QFile::WriteOnly);

    QDataStream out(&file);
    out.setByteOrder(QDataStream::LittleEndian);
    out.setVersion(QDataStream::Qt_5_0);

    out << magic;
    out << filetype_sessenabled;
    out << sessinfo_version;

    QHash<SessionID, Session *>::iterator s;

    out << (int)sessionlist.size();
    for (s = sessionlist.begin(); s != sessionlist.end(); ++s) {
        Session * sess = s.value();
        out << (quint32) sess->session();
        out << (bool)(sess->enabled());
    }
    return true;
}

bool Machine::loadSessionInfo()
{
    QHash<SessionID, Session *>::iterator s;
    QFile file(getDataPath() + "Sessions.info");
    if (!file.open(QFile::ReadOnly)) {
        for (s = sessionlist.begin(); s!= sessionlist.end(); ++s) {
            Session * sess = s.value();
            QHash<ChannelID, QVariant>::iterator it = sess->settings.find(SESSION_ENABLED);

            bool b = true;
            if (it != sess->settings.end()) {
                b = it.value().toBool();
            }
            sess->setEnabled(b);        // Extract from session settings and save..
        }
        saveSessionInfo();
        return true;
    }

    QDataStream in(&file);
    in.setByteOrder(QDataStream::LittleEndian);
    in.setVersion(QDataStream::Qt_5_0);

    quint32 mag32;
    in >> mag32;

    quint16 ft16, version;
    in >> ft16;
    in >> version;

    int size;
    in >> size;

    quint32 sid;
    bool b;

    for (int i=0; i< size; ++i) {
        in >> sid;
        in >> b;
        s = sessionlist.find(sid);

        if (s != sessionlist.end()) {
            Session * sess = s.value();

            sess->setEnabled(b);
        }
    }
    return true;
}

// Find date this session belongs in
QDate Machine::pickDate(qint64 first)
{
    QTime split_time = p_profile->session->daySplitTime();
    int combine_sessions = p_profile->session->combineCloseSessions();

    QDateTime d2 = QDateTime::fromTime_t(first / 1000);

    QDate date = d2.date();
    QTime time = d2.time();

    int closest_session = 0;

    if (time < split_time) {
        date = date.addDays(-1);
    } else if (combine_sessions > 0) {
        QMap<QDate, Day *>::iterator dit = day.find(date.addDays(-1)); // Check Day Before

        if (dit != day.end()) {
            QDateTime lt = QDateTime::fromTime_t(dit.value()->last() / 1000L);
            closest_session = lt.secsTo(d2) / 60;

            if (closest_session < combine_sessions) {
                date = date.addDays(-1);
            }
        }
    }

    return date;
}

bool Machine::AddSession(Session *s)
{
    invalidateCache();

    Q_ASSERT(s != nullptr);
    Q_ASSERT(p_profile);
    Q_ASSERT(p_profile->isOpen());

    if (p_profile->session->ignoreOlderSessions()) {
        qint64 ignorebefore = p_profile->session->ignoreOlderSessionsDate().toMSecsSinceEpoch();
        if (s->last() < ignorebefore) {
            skipped_sessions++;
            return false;
        }
    }

    if (s->session() > highest_sessionid) {
        highest_sessionid = s->session();
    }

    QTime split_time;
    int combine_sessions;
    bool locksessions = p_profile->session->lockSummarySessions();

    if (locksessions) {
        split_time = s->summaryOnly() ? QTime(12,0,0) : p_profile->session->daySplitTime();
        combine_sessions = s->summaryOnly() ? 0 : p_profile->session->combineCloseSessions();
    } else {
        split_time = p_profile->session->daySplitTime();
        combine_sessions = p_profile->session->combineCloseSessions();
    }

    int ignore_sessions = p_profile->session->ignoreShortSessions();

    int session_length = s->last() - s->first();
    session_length /= 60000;

    sessionlist[s->session()] = s; // To make sure it get's saved later even if it's not wanted.

    //int drift=p_profile->cpap->clockDrift();

    QDateTime d2 = QDateTime::fromTime_t(s->first() / 1000);

    QDate date = d2.date();
    QTime time = d2.time();

    QMap<QDate, Day *>::iterator dit, nextday;

    bool combine_next_day = false;
    int closest_session = 0;


    // Multithreaded import screws this up. :(

    if (time < split_time) {
        date = date.addDays(-1);
    } else if (combine_sessions > 0) {
        dit = day.find(date.addDays(-1)); // Check Day Before

        if (dit != day.end()) {
            QDateTime lt = QDateTime::fromTime_t(dit.value()->last() / 1000);
            closest_session = lt.secsTo(d2) / 60;

            if (closest_session < combine_sessions) {
                date = date.addDays(-1);
            } else {
                if ((split_time < time) && (split_time.secsTo(time) < 2)) {
                    if (s->machine()->loaderName() == STR_MACH_ResMed) {
                        date = date.addDays(-1);
                    }
                }
            }
        } else {
            nextday = day.find(date.addDays(1)); // Check Day Afterwards

            if (nextday != day.end()) {
                QDateTime lt = QDateTime::fromTime_t(nextday.value()->first() / 1000);
                closest_session = d2.secsTo(lt) / 60;

                if (closest_session < combine_sessions) {
                    // add todays here. pull all tomorrows records to this date.
                    combine_next_day = true;
                }
            }
        }
    }

    if (session_length < ignore_sessions) {
        // keep the session to save importing it again, but don't add it to the day record this time
        return true;
    }

    if (!firstsession) {
        if (firstday > date) { firstday = date; }

        if (lastday < date) { lastday = date; }
    } else {
        firstday = lastday = date;
        firstsession = false;
    }


    Day *dd = nullptr;
    dit = day.find(date);

    if (dit == day.end()) {
        dit = day.insert(date, p_profile->addDay(date));
    }
    dd = dit.value();

    dd->addSession(s);

    if (combine_next_day) {
        for (QList<Session *>::iterator i = nextday.value()->begin(); i != nextday.value()->end(); i++) {
            // i may need to do something here
            if (locksessions && (*i)->summaryOnly()) continue; // can't move summary only sessions..
            unlinkSession(*i);
            // Add it back

            sessionlist[(*i)->session()] = *i;

            dd->addSession(*i);
        }

//        QMap<QDate, QList<Day *> >::iterator nd = p_profile->daylist.find(date.addDays(1));
//        if (nd != p_profile->daylist.end()) {
//            p_profile->unlinkDay(nd.key(), nd.value());
//        }

//        QList<Day *>::iterator iend = nd.value().end();
//        for (QList<Day *>::iterator i = nd.value()->begin(); i != iend; ++i) {
//            if (*i == nextday.value()) {
//                nd.value().erase(i);
//            }
//        }

//        day.erase(nextday);
    }

    return true;
}

bool Machine::unlinkDay(Day * d)
{
    return day.remove(day.key(d)) > 0;
}

bool Machine::unlinkSession(Session * sess)
{
    // Remove the object from the machine object's session list
    bool b=sessionlist.remove(sess->session());

    QList<QDate> dates;

    QList<Day *> days;
    QMap<QDate, Day *>::iterator it;

    Day * d;

    // Doing this in case of accidental double linkages
    for (it = day.begin(); it != day.end(); ++it) {
        d = it.value();
        if (it.value()->sessions.contains(sess)) {
            days.push_back(d);
            dates.push_back(it.key());
        }
    }

    for (int i=0; i < days.size(); ++i) {
        d = days.at(i);
        if (d->sessions.removeAll(sess)) {
            b=true;
            if (d->size() == 0) {
                day.remove(dates[i]);
                p_profile->unlinkDay(d);
            }
        }
    }

    return b;
}

// This functions purpose is murder and mayhem... It deletes all of a machines data.
bool Machine::Purge(int secret)
{
    // Boring api key to stop this function getting called by accident :)
    if (secret != 3478216) { return false; }

    QString path = getDataPath();

    QDir dir(path);

    if (!dir.exists()) { // It doesn't exist anyway.
        return true;
    }

    if (!dir.isReadable()) {
        return false;
    }

    qDebug() << "Purging" << info.loadername << info.serial << dir.absoluteFilePath(path);

    // Remove any imported file list
    QFile impfile(getDataPath()+"/imported_files.csv");
    impfile.remove();

    QFile sumfile(getDataPath()+"Summaries.dat");
    sumfile.remove();

    // Create a copy of the list so the hash can be manipulated
    QList<Session *> sessions = sessionlist.values();

    // Clean up any loaded sessions from memory first..
    bool success = true;
    for (int i=0; i < sessions.size(); ++i) {
        Session * sess = sessions[i];
        if (!sess->Destroy()) {
            qDebug() << "Could not destroy "+ info.loadername +" ("+info.serial+") session" << sess->session();
            success = false;
        } else {
//            sessionlist.remove(sess->session());
        }

        delete sess;
    }

    // Remove EVERYTHING under Events folder..
    QString eventspath = getEventsPath();
    QDir evdir(eventspath);
    evdir.removeRecursively();


    // Clean up any straggling files (like from short sessions not being loaded...)
    dir.setFilter(QDir::Files | QDir::Hidden | QDir::NoSymLinks);
    dir.setSorting(QDir::Name);

    QFileInfoList list = dir.entryInfoList();
    int could_not_kill = 0;

    int size = list.size();
    for (int i = 0; i < size; ++i) {
        QFileInfo fi = list.at(i);
        QString fullpath = fi.canonicalFilePath();

        QString ext_s = fullpath.section('.', -1);
        bool ok;
        ext_s.toInt(&ok, 10);

        if (ok) {
            qDebug() << "Deleting " << QDir::toNativeSeparators(fullpath);
            if (!dir.remove(fullpath)) {
                qDebug() << "Could not purge file" << fullpath;
                success=false;
                could_not_kill++;
            }
        } else {
            qDebug() << "Didn't bother deleting cruft file" << fullpath;
            // cruft file..
        }
    }


    if (could_not_kill > 0) {
        qWarning() << "Could not purge path" << could_not_kill << "files in " << path;
        return false;
    }

    return true;
}

void Machine::setLoaderName(QString value)
{
    info.loadername = value;
    m_loader = GetLoader(value);
}

void Machine::setInfo(MachineInfo inf)
{
    info = inf;
    m_loader = GetLoader(inf.loadername);
}


//const quint32 channel_version=1;

const QString Machine::getDataPath()
{
    return p_profile->Get("{" + STR_GEN_DataFolder + "}/" + info.loadername + "_" + (info.serial.isEmpty() ? hexid() : info.serial)) + "/";
}
const QString Machine::getEventsPath()
{
    return getDataPath() + "Events/";
}

const QString Machine::getBackupPath()
{
    return p_profile->Get("{" + STR_GEN_DataFolder + "}/" + info.loadername + "_" + (info.serial.isEmpty() ? hexid() : info.serial)  + "/Backup/");
}

bool Machine::Load()
{
    QString path = getDataPath();

    QDir dir(path);
    qDebug() << "Loading Database" << QDir::toNativeSeparators(path);

    if (!dir.exists() || !dir.isReadable()) {
        return false;
    }

    ProgressDialog * popup = new ProgressDialog(nullptr);
    QPixmap image(getCPAPPixmap(info.loadername));
    if (!image.isNull()) {
        image = image.scaled(64,64);
        popup->setPixmap(image);
    }
    popup->setMessage(QObject::tr("Loading %1 data...").arg(info.brand));
    popup->show();

    QProgressBar * progress = popup->progress;

    if (!LoadSummary()) {
        QTime time;
        time.start();
        dir.setFilter(QDir::Files | QDir::Hidden | QDir::NoSymLinks);
        dir.setSorting(QDir::Name);

        QFileInfoList list = dir.entryInfoList();

        typedef QVector<QString> StringList;
        QMap<SessionID, StringList> sessfiles;
        QMap<SessionID, StringList>::iterator s;

        QString fullpath, ext_s, sesstr;
        int ext;
        SessionID sessid;
        bool ok;



        for (int i = 0; i < list.size(); i++) {
            QFileInfo fi = list.at(i);
            fullpath = fi.canonicalFilePath();
            ext_s = fi.fileName().section(".", -1);
            ext = ext_s.toInt(&ok, 10);

            if (!ok) { continue; }

            sesstr = fi.fileName().section(".", 0, -2);
            sessid = sesstr.toLong(&ok, 16);

            if (!ok) { continue; }

            if (sessfiles[sessid].capacity() == 0) { sessfiles[sessid].resize(3); }

            sessfiles[sessid][ext] = fi.canonicalFilePath();
        }

        int size = sessfiles.size();
        int cnt = 0;

        bool haveevents = false;
        for (s = sessfiles.begin(); s != sessfiles.end(); s++) {
            if (!s.value()[1].isEmpty()) {
                haveevents = true;
                break;
            }
        }

        QString eventpath = getEventsPath();

        if (haveevents) {
            QDir dir;
            dir.mkpath(eventpath);
        }

        for (s = sessfiles.begin(); s != sessfiles.end(); s++) {
            if ((++cnt % 50) == 0) { // This is slow.. :-/
                if (progress) { progress->setValue((float(cnt) / float(size) * 100.0)); }

                QApplication::processEvents();
            }

            Session *sess = new Session(this, s.key());

            if (haveevents && !s.value()[1].isEmpty()) {
                QFile::copy(s.value()[1], eventpath+s.value()[1].section("/",-1));
                QFile::remove(s.value()[1]);
            }

            if (sess->LoadSummary(s.value()[0])) {
                AddSession(sess);
            } else {
                qWarning() << "Error unpacking summary data";
                delete sess;
            }
        }

        if (hasModifiedSessions()) {
            SaveSummary();
            for (s = sessfiles.begin(); s != sessfiles.end(); s++) {
                QString summary = s.value()[0];
                QFile file(summary);
                file.remove();
            }
        }
        qDebug() << "Loaded" << info.model << "data in" << time.elapsed() << "ms";
    }
    loadSessionInfo();
    if (progress) { progress->setValue(100); }
    QApplication::processEvents();
    popup->hide();
    delete popup;

    return true;
}

bool Machine::SaveSession(Session *sess)
{
    QString path = getDataPath();

    if (sess->IsChanged()) { sess->Store(path); }

    return true;
}

void Machine::queSaveList(Session * sess)
{
    if (!m_save_threads_running) {
        // Threads aren't being used.. so run the actual immediately...
        int i = (float(m_donetasks) / float(m_totaltasks) * 100.0);
        qprogress->setValue(i);
        QApplication::processEvents();

        sess->UpdateSummaries();
        sess->Store(getDataPath());

        if (!p_profile->session->cacheSessions()) {
            sess->TrashEvents();
        }

    } else {
        listMutex.lock();
        m_savelist.append(sess);
        listMutex.unlock();
    }
}

Session *Machine::popSaveList()
{
    Session *sess = nullptr;
    listMutex.lock();

    if (!m_savelist.isEmpty()) {
        sess = m_savelist.at(0);
        m_savelist.pop_front();
        m_donetasks++;
    }

    listMutex.unlock();
    return sess;
}

// Call any time queing starts
void Machine::StartSaveThreads()
{
    m_savelist.clear();
    if (!p_profile->session->multithreading()) return;

    QString path = getDataPath();

    int threads = QThread::idealThreadCount();
    savelistSem = new QSemaphore(threads);
    savelistSem->acquire(threads);

    m_save_threads_running = true;
    m_donetasks=0;
    m_totaltasks=0;

    for (int i = 0; i < threads; i++) {
        SaveThread * thr = new SaveThread(this, path);
        QObject::connect(thr, SIGNAL(UpdateProgress(int)), qprogress, SLOT(setValue(int)));
        thread.push_back(thr);
        thread[i]->start();
    }

}

// Call when all queing is completed
void Machine::FinishSaveThreads()
{
    if (!m_save_threads_running)
        return;

    m_save_threads_running = false;

    // Wait for all tasks to finish
    while (!savelistSem->tryAcquire(thread.size(), 250)) {
        if (qprogress) {
            QApplication::processEvents();
        }
    }

    for (int i = 0; i < thread.size(); ++i) {
        while (thread[i]->isRunning()) {
            SaveThread::msleep(250);
            QApplication::processEvents();
        }
        QObject::disconnect(thread[i], SIGNAL(UpdateProgress(int)), qprogress, SLOT(setValue(int)));

        delete thread[i];
    }

    delete savelistSem;
}

void SaveThread::run()
{
    bool running = true;
    while (running) {
        Session *sess = machine->popSaveList();
        if (sess) {
            if (machine->m_donetasks % 10 == 0) {
                int i = (float(machine->m_donetasks) / float(machine->m_totaltasks) * 100.0);
                emit UpdateProgress(i);
            }
            sess->UpdateSummaries();
            machine->saveMutex.lock();
            sess->Store(path);
            machine->saveMutex.unlock();

            sess->TrashEvents();
        } else {
            if (!machine->m_save_threads_running) {
                break; // done
            } else {
                yieldCurrentThread(); // go do something else for a while
            }
        }
    }

    machine->savelistSem->release(1);
}



class SaveTask:public ImportTask
{
public:
    SaveTask(Session * s, Machine * m): sess(s), mach(m) {}
    virtual ~SaveTask() {}
    virtual void run();

protected:
    Session * sess;
    Machine * mach;
};

void SaveTask::run()
{
    sess->UpdateSummaries();
    mach->saveMutex.lock();
    sess->Store(mach->getDataPath());
    mach->saveMutex.unlock();
    sess->TrashEvents();
}

void Machine::queTask(ImportTask * task)
{
    if (0) { //p_profile->session->multithreading()) {
        m_tasklist.push_back(task);
        return;
    }

    task->run();
    return;
}

void Machine::runTasks()
{
    if (0) { //!p_profile->session->multithreading()) {
        Q_ASSERT(m_tasklist.isEmpty());
        return;
    }
    QThreadPool * threadpool = QThreadPool::globalInstance();
    int m_totaltasks=m_tasklist.size();
    int m_currenttask=0;
    while (!m_tasklist.isEmpty()) {
        if (threadpool->tryStart(m_tasklist.at(0))) {
            m_tasklist.pop_front();
            float f = float(m_currenttask) / float(m_totaltasks) * 100.0;
            qprogress->setValue(f);
            m_currenttask++;
        }
        QApplication::processEvents();
    }
    QThreadPool::globalInstance()->waitForDone(-1);
}

bool Machine::hasModifiedSessions()
{
    QHash<SessionID, Session *>::iterator s;

    for (s = sessionlist.begin(); s != sessionlist.end(); s++) {
        if (s.value()->IsChanged()) {
            return true;
        }
    }
    return false;
}

const QString summaryFileName = "Summaries.dat";

bool Machine::LoadSummary()
{
    QTime time;
    time.start();
    qDebug() << "Loading Summaries";
    QString filename = getDataPath() + summaryFileName;

    QFile file(filename);

    if (!file.exists() || !file.open(QFile::ReadOnly)) {
        qDebug() << "Couldn't open" << filename;
        return false;
    }

    QByteArray compressed = file.readAll();
    file.close();
    QTime time2;
    time2.start();
    QByteArray data = qUncompress(compressed);

    qDebug() << "Uncompressed Summary Length" << data.size() << "in" << time2.elapsed() << "ms";

    QDataStream in(&data, QIODevice::ReadOnly);
    in.setVersion(QDataStream::Qt_5_0);
    in.setByteOrder(QDataStream::LittleEndian);

    quint32 mag32;
    quint16 ftype;
    in >> mag32;
    in >> ftype;

    int session_count;
    in >> session_count;

    for (int i=0; i< session_count; ++i) {
        Session * sess = new Session(this,0);
        in >> *sess;
        AddSession(sess);
    }

    qDebug() << "Loaded" << info.model << "data in" << time.elapsed() << "ms";

    return true;
}

bool Machine::SaveSummary()
{
    qDebug() << "Saving Summaries";
    QString filename = getDataPath() + summaryFileName;

    QByteArray data;
    QDataStream out(&data, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_0);
    out.setByteOrder(QDataStream::LittleEndian);

    out << (quint32)magic;
    out << (quint16)filetype_summary;
    out << (int)sessionlist.size();

    QHash<SessionID, Session *>::iterator s;
    for (s = sessionlist.begin(); s != sessionlist.end(); s++) {
        Session * sess = s.value();
        out << *sess;
    }

    QFile file(filename);
    file.open(QIODevice::WriteOnly);

    qDebug() << "Uncompressed Summary Length" << data.size();

    QByteArray compressed = qCompress(data);
    file.write(compressed);
    file.close();
    return true;
}

bool Machine::Save()
{
    //int size;
    int cnt = 0;

    QString path = getDataPath();
    QDir dir(path);

    if (!dir.exists()) {
        dir.mkdir(path);
    }

    QHash<SessionID, Session *>::iterator s;

    m_savelist.clear();

    // store any event summaries..
    for (s = sessionlist.begin(); s != sessionlist.end(); s++) {
        cnt++;

        if ((*s)->IsChanged()) {
            queTask(new SaveTask(*s, this));
        }
    }

    runTasks();

    return true;
}

void Machine::invalidateCache()
{
    availableCache.clear();
}

QList<ChannelID> & Machine::availableChannels(quint32 chantype)
{
    QHash<quint32, QList<ChannelID> >::iterator ac = availableCache.find(chantype);
    if (ac != availableCache.end()) {
        return ac.value();

    }

    QHash<ChannelID, int> chanhash;

    // look through the daylist and return a list of available channels for this machine
    QMap<QDate, Day *>::iterator dit;
    QMap<QDate, Day *>::iterator day_end = day.end();
    for (dit = day.begin(); dit != day_end; ++dit) {
        QList<Session *>::iterator sess_end = dit.value()->end();

        for (QList<Session *>::iterator sit = dit.value()->begin(); sit != sess_end; ++sit) {
            Session * sess = (*sit);
            if (sess->machine() != this) continue;

            int size = sess->m_availableChannels.size();
            for (int i=0; i < size; ++i) {
                ChannelID code = sess->m_availableChannels.at(i);
                QHash<ChannelID, schema::Channel *>::iterator ch = schema::channel.channels.find(code);
                schema::Channel * chan = ch.value();
                if (chan->type() & chantype) {
                    chanhash[code]++;
                }
            }
        }
    }
    return availableCache[chantype] = chanhash.keys();
}



//////////////////////////////////////////////////////////////////////////////////////////
// CPAP implmementation
//////////////////////////////////////////////////////////////////////////////////////////
CPAP::CPAP(MachineID id): Machine(id)
{
    m_type = MT_CPAP;
}

CPAP::~CPAP()
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// Oximeter Class implmementation
//////////////////////////////////////////////////////////////////////////////////////////
Oximeter::Oximeter(MachineID id): Machine(id)
{
    m_type = MT_OXIMETER;
}

Oximeter::~Oximeter()
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// SleepStage Class implmementation
//////////////////////////////////////////////////////////////////////////////////////////
SleepStage::SleepStage(MachineID id): Machine(id)
{
    m_type = MT_SLEEPSTAGE;
}
SleepStage::~SleepStage()
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// PositionSensor Class implmementation
//////////////////////////////////////////////////////////////////////////////////////////
PositionSensor::PositionSensor(MachineID id): Machine(id)
{
    m_type = MT_POSITION;
}
PositionSensor::~PositionSensor()
{
}
