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

#include <QFile>
#include <QDir>

#include "machine_loader.h"

// This crap moves to Profile
QList<MachineLoader *> m_loaders;

QList<MachineLoader *> GetLoaders()
{
    return m_loaders;
}

void RegisterLoader(MachineLoader *loader)
{
    m_loaders.push_back(loader);
}
void DestroyLoaders()
{
    for (auto  i = m_loaders.begin(); i != m_loaders.end(); i++) {
        delete(*i);
    }

    m_loaders.clear();
}

MachineLoader::MachineLoader()
{
}

MachineLoader::~MachineLoader()
{
    for (auto m = m_machlist.begin(); m != m_machlist.end(); m++) {
        delete *m;
    }
}

bool MachineLoader::compressFile(QString inpath, QString outpath)
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
