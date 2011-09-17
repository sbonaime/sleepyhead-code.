#include <QFile>
#include <QDebug>
#include <QDomDocument>
#include <QDomElement>
#include <QDomNode>
#include "schema.h"



namespace schema {

ChannelList channel;
Channel EmptyChannel;

QHash<QString,ChanType> ChanTypes;
QHash<QString,DataType> DataTypes;
QHash<QString,ScopeType> Scopes;

bool schema_initialized=false;

void init()
{
    if (schema_initialized) return;
    schema_initialized=true;

    EmptyChannel=Channel(0,DATA,DAY,"Empty","Empty Channel","Empty","");

    ChanTypes["data"]=DATA;
    //Types["waveform"]=WAVEFORM;
    ChanTypes["setting"]=SETTING;

    Scopes["session"]=SESSION;
    Scopes["day"]=DAY;
    Scopes["machine"]=MACHINE;
    Scopes["global"]=GLOBAL;

    DataTypes[""]=DEFAULT;
    DataTypes["bool"]=BOOL;
    DataTypes["double"]=DOUBLE;
    DataTypes["integer"]=INTEGER;
    DataTypes["string"]=STRING;
    DataTypes["richtext"]=RICHTEXT;
    DataTypes["date"]=DATE;
    DataTypes["datetime"]=DATETIME;
    DataTypes["time"]=TIME;

    schema::channel.Load(":/docs/channels.xml");

}

Channel::Channel(int id, ChanType type, ScopeType scope, QString name, QString description, QString label, QString unit, DataType datatype, QColor color, int link):
    m_id(id),
    m_type(type),
    m_scope(scope),
    m_name(name),
    m_description(description),
    m_label(label),
    m_unit(unit),
    m_datatype(datatype),
    m_defaultcolor(color),
    m_link(link)
{
}
bool Channel::isNull()
{
    return (this==&EmptyChannel);
}

ChannelList::ChannelList()
    :m_doctype("channels")
{
}
ChannelList::~ChannelList()
{
    for (QHash<int,Channel *>::iterator i=channels.begin();i!=channels.end();i++) {
        delete i.value();
    }
}
bool ChannelList::Load(QString filename)
{
    QDomDocument doc(m_doctype);
    QFile file(filename);
    qDebug() << "Opening " << filename;
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Could not open" << filename;
        return false;
    }
    QString errorMsg;
    int errorLine;
    if (!doc.setContent(&file,false,&errorMsg,&errorLine)) {
        qWarning() << "Invalid XML Content in" << filename;
        qWarning() << "Error line" << errorLine <<":" << errorMsg;
        return false;
    }
    file.close();


    QDomElement root=doc.documentElement();
    if (root.tagName().toLower() != "channels") {
        return false;
    }
    QString language=root.attribute("language","en");
    QString version=root.attribute("version","");

    if (version.isEmpty()) {
        qWarning() << "No Version Field in" << m_doctype << "Schema, assuming 1.0" << filename;
        version="1.0";
    }

    qDebug() << "Processing xml file:" << m_doctype << language << version;
    QDomNodeList grp=root.elementsByTagName("group");
    QDomNode node,n,ch;
    QDomElement e;

    bool ok;
    int id,linkid;
    QString chantype,scopestr,typestr,name,group,idtxt,details,label,unit,datatypestr,defcolor,link;
    ChanType type;
    DataType datatype;
    Channel *chan;
    QColor color;
    bool multi;
    ScopeType scope;
    int line;
    for (int i=0;i<grp.size();i++) {
        node=grp.at(i);
        group=node.toElement().attribute("name");
        //qDebug() << "Group Name" << group;
        // Why do I have to skip the first node here? (shows up empty)
        n=node.firstChild().nextSibling();

        while (!n.isNull()) {
            line=n.lineNumber();
            e=n.toElement();

            if (e.nodeName().toLower()!="channel") {
                qWarning() << "Ignoring unrecognized schema type "<< e.nodeName() << "in" << filename << "line" << line;
                continue;
            }

            ch=n.firstChild();
            n=n.nextSibling();

            idtxt=e.attribute("id");
            id=idtxt.toInt(&ok,16);
            if (!ok) {
                qWarning() << "Dodgy ID number "<< e.nodeName() << "in" << filename << "line" << line;
                continue;
            }

            chantype=e.attribute("class","data").toLower();
            if (!ChanTypes.contains(chantype)) {
                qWarning() << "Dodgy class "<< chantype << "in" << filename << "line" << line;
                continue;
            }
            type=ChanTypes[chantype];

            scopestr=e.attribute("scope","session");
            if (scopestr.at(0)==QChar('!')) {
                scopestr=scopestr.mid(1);
                multi=true;
            } multi=false;
            if (!Scopes.contains(scopestr)) {
                qWarning() << "Dodgy Scope "<< scopestr << "in" << filename << "line" << line;
                continue;
            }
            scope=Scopes[scopestr];
            name=e.attribute("name","");
            details=e.attribute("details","");
            label=e.attribute("label","");

            if (name.isEmpty() || details.isEmpty() || label.isEmpty()) {
                qWarning() << "Missing name,details or label attribute in" << filename << "line" << line;
                continue;
            }
            unit=e.attribute("unit");

            defcolor=e.attribute("color","black");
            color=QColor(defcolor);
            if (!color.isValid()) {
                qWarning() << "Invalid Color "<< defcolor<< "in" << filename << "line" << line;
                color=Qt::black;
            }

            datatypestr=e.attribute("type","").toLower();

            link=e.attribute("link","");
            if (!link.isEmpty()) {
                linkid=link.toInt(&ok,16);
                if (!ok) {
                    qWarning() << "Dodgy Link ID number "<< e.nodeName() << "in" << filename << " line" << line;
                }
            } else linkid=0;

            if (DataTypes.contains(datatypestr)) {
                datatype=DataTypes[typestr];
            } else {
                qWarning() << "Ignoring unrecognized schema datatype in" << filename <<"line" << line;
                continue;
            }

            if (channels.contains(id)) {
                qWarning() << "Schema already contains id" << id << "in" << filename <<"line" << line;
                continue;
            }
            if (names.contains(name)) {
                qWarning() << "Schema already contains name" << name << "in" << filename <<"line" << line;
                continue;
            }
            chan=new Channel(id,type,scope,name,details,label,unit,datatype,color,linkid);
            channels[id]=chan;
            names[name]=chan;
            groups[group][name]=chan;
            if (linkid>0) {
                if (channels.contains(linkid)) {
                    Channel *it=channels[linkid];
                    it->m_links.push_back(chan);
                    int i=0;
                } else {
                    qWarning() << "Linked channel must be defined first in" << filename <<"line" << line;
                }
            }

            // process children
            while(!ch.isNull()) {
                e=ch.toElement();
                QString sub=ch.nodeName().toLower();
                QString id2str,name2str;
                int id2;
                if (sub=="option") {
                    id2str=e.attribute("id");
                    id2=id2str.toInt(&ok,10);
                    name2str=e.attribute("value");
                    //qDebug() << sub << id2 << name2str;
                    chan->addOption(id2,name2str);
                } else if (sub=="color") {
                }
                ch=ch.nextSibling();
            }
        }
    }

    return true;
}
bool ChannelList::Save(QString filename)
{
    return false;
}

}

typedef schema::Channel * ChannelID;
