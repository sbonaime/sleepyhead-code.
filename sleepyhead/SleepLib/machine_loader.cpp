/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 *
 * SleepLib Machine Loader Class Implementation
 *
 * Copyright (c) 2011-2014 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#include <QProgressBar>
#include <QApplication>
#include <QFile>
#include <QDir>
#include <QThreadPool>

extern QProgressBar *qprogress;

#include "machine_loader.h"

// This crap moves to Profile
QList<MachineLoader *> m_loaders;

QList<MachineLoader *> GetLoaders(MachineType mt)
{
    QList<MachineLoader *> list;
    for (int i=0; i < m_loaders.size(); ++i) {
        if (mt == MT_UNKNOWN) {
            list.push_back(m_loaders.at(i));
        } else {
            MachineType mtype = m_loaders.at(i)->type();
            if (mtype == mt) {
                list.push_back(m_loaders.at(i));
            }
        }
    }
    return list;
}

MachineLoader * lookupLoader(Machine * m)
{
    for (int i=0; i < m_loaders.size(); ++i) {
        MachineLoader * loader = m_loaders.at(i);
        if (loader->loaderName() == m->loaderName())
            return loader;
    }
    return nullptr;
}

QHash<QString, QHash<QString, Machine *> > MachineList;


Machine * MachineLoader::CreateMachine(MachineInfo info, MachineID id)
{
    Q_ASSERT(p_profile != nullptr);

    Machine *m = nullptr;

    QHash<QString, QHash<QString, Machine *> >::iterator mlit = MachineList.find(info.loadername);

    if (mlit != MachineList.end()) {
        QHash<QString, Machine *>::iterator mit = mlit.value().find(info.serial);
        if (mit != mlit.value().end()) {
            mit.value()->setInfo(info); // update info
            return mit.value();
        }
    }

    switch (info.type) {
    case MT_CPAP:
        m = new CPAP(id);
        break;
    case MT_SLEEPSTAGE:
        m = new SleepStage(id);
        break;
    case MT_OXIMETER:
        m = new Oximeter(id);
        break;
    case MT_POSITION:
        m = new PositionSensor(id);
        break;
    case MT_JOURNAL:
        m = new Machine(id);
        break;
    default:
        m = new Machine(id);

        break;
    }

    m->setInfo(info);

    qDebug() << "Create" << info.loadername << "Machine" << (info.serial.isEmpty() ? m->hexid() : info.serial);

    MachineList[info.loadername][info.serial] = m;
    p_profile->AddMachine(m);

    return m;
}


void RegisterLoader(MachineLoader *loader)
{
    m_loaders.push_back(loader);
}
void DestroyLoaders()
{
    for (QList<MachineLoader *>::iterator i = m_loaders.begin(); i != m_loaders.end(); i++) {
        delete(*i);
    }

    m_loaders.clear();
}

MachineLoader::MachineLoader() :QObject(nullptr)
{
    m_abort = false;
    m_type = MT_UNKNOWN;
    m_status = NEUTRAL;
}

MachineLoader::~MachineLoader()
{
    for (QList<Machine *>::iterator m = m_machlist.begin(); m != m_machlist.end(); m++) {
        delete *m;
    }
}

void MachineLoader::finishAddingSessions()
{
    QMap<SessionID, Session *>::iterator it;
    QMap<SessionID, Session *>::iterator it_end = new_sessions.end();

    // Using a map specifically so they are inserted in order.
    for (it = new_sessions.begin(); it != it_end; ++it) {
        Session * sess = it.value();
        Machine * mach = sess->machine();
        mach->AddSession(sess);
    }
    new_sessions.clear();
}

bool compressFile(QString inpath, QString outpath)
{
    if (outpath.isEmpty()) {
        outpath = inpath + ".gz";
    } else if (!outpath.endsWith(".gz")) {
        outpath += ".gz";
    }

    QFile f(inpath);

    if (!f.exists(inpath)) {
        qDebug() << "compressFile()" << inpath << "does not exist";
        return false;
    }

    qint64 size = f.size();

    if (!f.open(QFile::ReadOnly)) {
        qDebug() << "compressFile() Couldn't open" << inpath;
        return false;
    }

    char *buf = new char [size];

    if (!f.read(buf, size)) {
        delete buf;
        qDebug() << "compressFile() Couldn't read all of" << inpath;
        return false;
    }

    f.close();
    gzFile gz = gzopen(outpath.toLatin1(), "wb");

    //gzbuffer(gz,65536*2);
    if (!gz) {
        qDebug() << "compressFile() Couldn't open" << outpath << "for writing";
        delete buf;
        return false;
    }

    gzwrite(gz, buf, size);
    gzclose(gz);
    delete buf;
    return true;
}

void MachineLoader::queTask(ImportTask * task)
{
    m_tasklist.push_back(task);
}

void MachineLoader::runTasks(bool threaded)
{
    m_totaltasks=m_tasklist.size();
    m_currenttask=0;

    if (!threaded) {
        while (!m_tasklist.isEmpty()) {
            ImportTask * task = m_tasklist.takeFirst();
            task->run();
            float f = float(m_currenttask) / float(m_totaltasks) * 100.0;
            qprogress->setValue(f);
            m_currenttask++;
            QApplication::processEvents();
        }
    } else {
        QThreadPool * threadpool = QThreadPool::globalInstance();
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
}


QList<ChannelID> CPAPLoader::eventFlags(Day * day)
{
    Machine * mach = day->machine;

    QList<ChannelID> list;

    if (mach->loader() != this) {
        qDebug() << "Trying to ask" << loaderName() << "for" << mach->loaderName() << "data";
        return list;
    }

    list.push_back(CPAP_ClearAirway);
    list.push_back(CPAP_Obstructive);
    list.push_back(CPAP_Hypopnea);
    list.push_back(CPAP_Apnea);

    return list;
}

/*const QString machine_profile_name="MachineList.xml";

void MachineLoader::LoadMachineList()
{
}

void MachineLoader::StoreMachineList()
{
}
void MachineLoader::LoadSummary(Machine *m, QString &filename)
{
    QFile f(filename);
    if (!f.exists())
        return;
    f.open(QIODevice::ReadOnly);
    if (!f.isReadable())
        return;


}
void MachineLoader::LoadSummaries(Machine *m)
{
    QString path=(*profile)["ProfileDirectory"]+"/"+m_classname+"/"+mach->hexid();
    QDir dir(path);
    if (!dir.exists() || !dir.isReadable())
        return false;

    dir.setFilter(QDir::Files | QDir::Hidden | QDir::NoSymLinks);
    dir.setSorting(QDir::Name);

    QString fullpath,ext_s,sesstr;
    int ext;
    SessionID sessid;
    bool ok;
    QMap<SessionID, int> sessions;
    QFileInfoList list=dir.entryInfoList();
    for (int i=0;i<list.size();i++) {
        QFileInfo fi=list.at(i);
        fullpath=fi.canonicalFilePath();
        ext_s=fi.fileName().section(".",-1);
        ext=ext_s.toInt(&ok,10);
        if (!ok) continue;
        sesstr=fi.fileName().section(".",0,-2);
        sessid=sesstr.toLong(&ok,16);
        if (!ok) continue;

    }
}

void MachineLoader::LoadAllSummaries()
{
    for (int i=0;i<m_machlist.size();i++)
        LoadSummaries(m_machlist[i]);
}
void MachineLoader::LoadAllEvents()
{
    for (int i=0;i<m_machlist.size();i++)
        LoadEvents(m_machlist[i]);
}
void MachineLoader::LoadAllWaveforms()
{
    for (int i=0;i<m_machlist.size();i++)
        LoadWaveforms(m_machlist[i]);
}
void MachineLoader::LoadAll()
{
    LoadAllSummaries();
    LoadAllEvents();
    LoadAllWaveforms();
}

void MachineLoader::Save(Machine *m)
{
}
void MachineLoader::SaveAll()
{
    for (int i=0;i<m_machlist.size();i++)
        Save(m_machlist[i]);
}
*/
