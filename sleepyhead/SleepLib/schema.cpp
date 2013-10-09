/*
 Schema Implementation (Parse Channel XML data)
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

#include <QFile>
#include <QDebug>
#include <QDomDocument>
#include <QDomElement>
#include <QDomNode>
#include <QMessageBox>
#include <QApplication>
#include "schema.h"



namespace schema {

ChannelList channel;
Channel EmptyChannel;
Channel *SessionEnabledChannel;

QHash<QString,ChanType> ChanTypes;
QHash<QString,DataType> DataTypes;
QHash<QString,ScopeType> Scopes;

bool schema_initialized=false;

void init()
{
    if (schema_initialized) return;
    schema_initialized=true;

    EmptyChannel=Channel(0,DATA,DAY,"Empty","Empty Channel","","");
    SessionEnabledChannel=new Channel(1,DATA,DAY,"Enabled","Session Enabled","","");

    channel.channels[1]=SessionEnabledChannel;
    channel.names["Enabled"]=SessionEnabledChannel;
    SESSION_ENABLED=1;
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

    if (!schema::channel.Load(":/docs/channels.xml")) {
        QMessageBox::critical(0,"Ugh!","Couldn't parse Channels.xml, this build is seriously borked, no choice but to abort!!",QMessageBox::Ok);
        QApplication::exit(-1);
    }

    NoChannel=0;
    CPAP_IPAP=schema::channel["IPAP"].id();
    CPAP_IPAPLo=schema::channel["IPAPLo"].id();
    CPAP_IPAPHi=schema::channel["IPAPHi"].id();
    CPAP_EPAP=schema::channel["EPAP"].id();
    CPAP_Pressure=schema::channel["Pressure"].id();
    CPAP_PS=schema::channel["PS"].id();
    CPAP_PSMin=schema::channel["PSMin"].id();
    CPAP_PSMax=schema::channel["PSMax"].id();
    CPAP_Mode=schema::channel["PAPMode"].id();
    CPAP_BrokenSummary=schema::channel["BrokenSummary"].id();
    CPAP_BrokenWaveform=schema::channel["BrokenWaveform"].id();
    CPAP_PressureMin=schema::channel["PressureMin"].id();
    CPAP_PressureMax=schema::channel["PressureMax"].id();
    CPAP_RampTime=schema::channel["RampTime"].id();
    CPAP_RampPressure=schema::channel["RampPressure"].id();
    CPAP_Obstructive=schema::channel["Obstructive"].id();
    CPAP_Hypopnea=schema::channel["Hypopnea"].id();
    CPAP_ClearAirway=schema::channel["ClearAirway"].id();
    CPAP_Apnea=schema::channel["Apnea"].id();
    CPAP_CSR=schema::channel["CSR"].id();
    CPAP_LeakFlag=schema::channel["LeakFlag"].id();
    CPAP_ExP=schema::channel["ExP"].id();
    CPAP_NRI=schema::channel["NRI"].id();
    CPAP_VSnore=schema::channel["VSnore"].id();
    CPAP_VSnore2=schema::channel["VSnore2"].id();
    CPAP_RERA=schema::channel["RERA"].id();
    CPAP_PressurePulse=schema::channel["PressurePulse"].id();
    CPAP_FlowLimit=schema::channel["FlowLimit"].id();
    CPAP_FlowRate=schema::channel["FlowRate"].id();
    CPAP_MaskPressure=schema::channel["MaskPressure"].id();
    CPAP_MaskPressureHi=schema::channel["MaskPressureHi"].id();
    CPAP_RespEvent=schema::channel["RespEvent"].id();
    CPAP_Snore=schema::channel["Snore"].id();
    CPAP_MinuteVent=schema::channel["MinuteVent"].id();
    CPAP_RespRate=schema::channel["RespRate"].id();
    CPAP_TidalVolume=schema::channel["TidalVolume"].id();
    CPAP_PTB=schema::channel["PTB"].id();
    CPAP_Leak=schema::channel["Leak"].id();
    CPAP_LeakMedian=schema::channel["LeakMedian"].id();
    CPAP_LeakTotal=schema::channel["LeakTotal"].id();
    CPAP_MaxLeak=schema::channel["MaxLeak"].id();
    CPAP_FLG=schema::channel["FLG"].id();
    CPAP_IE=schema::channel["IE"].id();
    CPAP_Te=schema::channel["Te"].id();
    CPAP_Ti=schema::channel["Ti"].id();
    CPAP_TgMV=schema::channel["TgMV"].id();
    CPAP_Test1=schema::channel["TestChan1"].id();
    CPAP_Test2=schema::channel["TestChan2"].id();

    CPAP_PresReliefSet=schema::channel["PresRelSet"].id();
    CPAP_PresReliefMode=schema::channel["PresRelMode"].id();
    CPAP_PresReliefType=schema::channel["PresRelType"].id();

    CPAP_UserFlag1=schema::channel["UserFlag1"].id();
    CPAP_UserFlag2=schema::channel["UserFlag2"].id();
    CPAP_UserFlag3=schema::channel["UserFlag3"].id();
    RMS9_E01=schema::channel["RMS9_E01"].id();
    RMS9_E02=schema::channel["RMS9_E02"].id();
    RMS9_EPR=schema::channel["EPR"].id();
    RMS9_EPRSet=schema::channel["EPRSet"].id();
    RMS9_SetPressure=schema::channel["SetPressure"].id();
    PRS1_00=schema::channel["PRS1_00"].id();
    PRS1_01=schema::channel["PRS1_01"].id();
    PRS1_08=schema::channel["PRS1_08"].id();
    PRS1_0A=schema::channel["PRS1_0A"].id();
    PRS1_0B=schema::channel["PRS1_0B"].id();
    PRS1_0C=schema::channel["PRS1_0C"].id();
    PRS1_0E=schema::channel["PRS1_0E"].id();
    PRS1_0F=schema::channel["PRS1_0F"].id();
    PRS1_10=schema::channel["PRS1_10"].id();
    PRS1_12=schema::channel["PRS1_12"].id();
    PRS1_FlexMode=schema::channel["FlexMode"].id();
    PRS1_FlexSet=schema::channel["FlexSet"].id();
    PRS1_HumidStatus=schema::channel["HumidStat"].id();
    CPAP_HumidSetting=schema::channel["HumidSet"].id();
    PRS1_SysLock=schema::channel["SysLock"].id();
    PRS1_SysOneResistStat=schema::channel["SysOneResistStat"].id();
    PRS1_SysOneResistSet=schema::channel["SysOneResistSet"].id();
    PRS1_HoseDiam=schema::channel["HoseDiam"].id();
    PRS1_AutoOn=schema::channel["AutoOn"].id();
    PRS1_AutoOff=schema::channel["AutoOff"].id();
    PRS1_MaskAlert=schema::channel["MaskAlert"].id();
    PRS1_ShowAHI=schema::channel["ShowAHI"].id();
    INTELLIPAP_Unknown1=schema::channel["IntUnk1"].id();
    INTELLIPAP_Unknown2=schema::channel["IntUnk2"].id();
    OXI_Pulse=schema::channel["Pulse"].id();
    OXI_SPO2=schema::channel["SPO2"].id();
    OXI_PulseChange=schema::channel["PulseChange"].id();
    OXI_SPO2Drop=schema::channel["SPO2Drop"].id();
    OXI_Plethy=schema::channel["Plethy"].id();
    CPAP_AHI=schema::channel["AHI"].id();
    CPAP_RDI=schema::channel["RDI"].id();
    Journal_Notes=schema::channel["Journal"].id();
    Journal_Weight=schema::channel["Weight"].id();
    Journal_BMI=schema::channel["BMI"].id();
    Journal_ZombieMeter=schema::channel["ZombieMeter"].id();
    Bookmark_Start=schema::channel["BookmarkStart"].id();
    Bookmark_End=schema::channel["BookmarkEnd"].id();
    Bookmark_Notes=schema::channel["BookmarkNotes"].id();

    ZEO_SleepStage=schema::channel["SleepStage"].id();
    ZEO_ZQ=schema::channel["ZeoZQ"].id();
    ZEO_Awakenings=schema::channel["Awakenings"].id();
    ZEO_MorningFeel=schema::channel["MorningFeel"].id();
    ZEO_TimeInWake=schema::channel["TimeInWake"].id();
    ZEO_TimeInREM=schema::channel["TimeInREM"].id();
    ZEO_TimeInLight=schema::channel["TimeInLight"].id();
    ZEO_TimeInDeep=schema::channel["TimeInDeep"].id();
    ZEO_TimeToZ=schema::channel["TimeToZ"].id();
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
    for (QHash<ChannelID,Channel *>::iterator i=channels.begin();i!=channels.end();i++) {
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
    //bool multi;
    ScopeType scope;
    int line;
    for (int i=0;i<grp.size();i++) {
        node=grp.at(i);
        group=node.toElement().attribute("name");
        //qDebug() << "Group Name" << group;
        // Why do I have to skip the first node here? (shows up empty)
        n=node.firstChildElement();

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
                //multi=true;
            } //multi=false;
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
            //qDebug() << "Channel" << id << name << label;
            groups[group][name]=chan;
            if (linkid>0) {
                if (channels.contains(linkid)) {
                    Channel *it=channels[linkid];
                    it->m_links.push_back(chan);
                    //int i=0;
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
    Q_UNUSED(filename)
    return false;
}

}

//typedef schema::Channel * ChannelID;
