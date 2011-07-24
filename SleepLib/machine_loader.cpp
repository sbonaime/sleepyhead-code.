/********************************************************************
 SleepLib Machine Loader Class Implementation
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*********************************************************************/

#include <QFile>
#include <QDir>

#include "machine_loader.h"

// This crap moves to Profile
list<MachineLoader *> m_loaders;

list<MachineLoader *> GetLoaders()
{
    return m_loaders;
}

void RegisterLoader(MachineLoader *loader)
{
    m_loaders.push_back(loader);
}
void DestroyLoaders()
{
    for (list<MachineLoader *>::iterator i=m_loaders.begin(); i!=m_loaders.end(); i++) {
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
    for (vector<Machine *>::iterator m=m_machlist.begin();m!=m_machlist.end();m++) {
        delete *m;
    }
}
/*const QString machine_profile_name="MachineList.xml";

void MachineLoader::LoadMachineList()
{
    QString filename=(*profile)["ProfileDirectory"]+"/"+m_classname+"/"+machine_profile_name;

    QFile f(filename);
    if (!f.exists()) {
        qDebug() << "XML file does not exist" << filename;
        return;
    }
    TiXmlDocument xml(filename.toLatin1());
    if (!xml.LoadFile()) {
        qDebug() << "Couldn't read XML file " << filename;
        return;
    }
    TiXmlHandle hDoc(&xml);
    TiXmlElement * pElem;
    //TiXmlHandle hRoot(0);
    pElem=hDoc.FirstChildElement().Element();

    if (!pElem) {
        qDebug("MachineList is empty.");
        return;
    }

    //hRoot=TiXmlHandle(pElem);
    //pElem=hRoot.FirstChild("MachineList").FirstChild().Element();

    if (pElem->Value()!="MachineList") {
        qDebug() << "MachineLoader::LoadMachineList expected a MachineList";
    }

    int mt;

    Machine *mach;
    pElem->QueryIntAttribute("type",&mt);
    MachineType m_type=(MachineType)mt;
    QString m_class=pElem->Attribute("class");

    TiXmlElement *elem;
    elem=pElem->FirstChildElement();
    if (!elem) {
        qDebug("Machine is empty.");
        return;
    }

    int m_id;
    for(; elem; elem=elem->NextSiblingElement()) {
        QString pKey=elem->Value();
        if (!pKey=="Machine") continue;

        elem->QueryIntAttribute("id",&m_id);

        mach=CreateMachine(m_id);

        TiXmlElement *e=elem->FirstChildElement();
        for (; e; e=e->NextSiblingElement()) {
            QString pKey=e->Value();
            mach->properties[pKey]=e->GetText();
        }
//        QString filename=(*profile)["ProfileDirectory"]+"/"+m_classname+"/"+mach->hexid();
//        mach->LoadSummaries(filename);
    }
}

void MachineLoader::StoreMachineList()
{
    QString filename=(*profile)["ProfileDirectory"]+"/"+m_classname+"/"+machine_profile_name;

    TiXmlDocument xml;
    TiXmlElement* msg;
    TiXmlComment * comment;
    TiXmlDeclaration *decl=new TiXmlDeclaration( "1.0", "", "" );
    xml.LinkEndChild(decl);
    TiXmlElement *root=new TiXmlElement("MachineList");

    char *cc=m_class.toLatin1().data();
    root->SetAttribute("type",(int)m_type);
    root->SetAttribute("class",cc);

    xml.LinkEndChild(root);

    if (!m_comment.isEmpty()) {
        comment = new TiXmlComment();
        comment->SetValue((QString(" ")+m_comment+QString(" ")).toLatin1());
        root->LinkEndChild(comment);
    }

    for (int i=0;i<m_machlist.size();i++)  {
        Machine * m=m_machlist[i];
        // Save any changes/new sessions to machine record.

        TiXmlElement *me=new TiXmlElement("Machine");
        me->SetAttribute("id",m->id());

        for (map<QString,QString>::iterator j=i->second->properties.begin(); j!=i->second->properties.end(); j++) {
            TiXmlElement *mp=new TiXmlElement(j->first.toLatin1());
            mp->LinkEndChild(new TiXmlText(j->second.toLatin1()));
            me->LinkEndChild(mp);
        }
        root->LinkEndChild(me);
    }
    xml.SaveFile(p_filename.toLatin1());
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
