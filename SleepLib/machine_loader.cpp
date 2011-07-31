/********************************************************************
 SleepLib Machine Loader Class Implementation
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*********************************************************************/

#include <QFile>
#include <QDir>

#include "machine_loader.h"

// This crap moves to Profile
QVector<MachineLoader *> m_loaders;

QVector<MachineLoader *> GetLoaders()
{
    return m_loaders;
}

void RegisterLoader(MachineLoader *loader)
{
    m_loaders.push_back(loader);
}
void DestroyLoaders()
{
    for (QVector<MachineLoader *>::iterator i=m_loaders.begin(); i!=m_loaders.end(); i++) {
        delete (*i);
    }
    m_loaders.clear();
}

MachineLoader::MachineLoader()
{
}
/*MachineLoader::MachineLoader(Profile * profile, QString & classname, MachineType type)
:m_profile(profile), m_class(classname), m_type(type)
{
    assert(m_profile!=NULL);
    assert(!m_classname.isEmpty());
}*/
MachineLoader::~MachineLoader()
{
    for (QVector<Machine *>::iterator m=m_machlist.begin();m!=m_machlist.end();m++) {
        delete *m;
    }
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
