/*

SleepLib Preferences Implementation

Author: Mark Watkins <jedimark64@users.sourceforge.net>
License: GPL

*/

//#include <wx/wx.h>
//#include <wx/filename.h>
//#include <wx/stdpaths.h>

#include <QString>
#include <QVariant>
#include <QDateTime>
#include <QDir>
#include <QDesktopServices>

#include "preferences.h"


const QString & getUserName()
{
    static QString userName;
    userName=getenv("USER");

    if (userName.isEmpty()) {
        userName="Windows User";

        // FIXME: I DONT KNOW QT'S WINDOWS DEFS
#if defined (WINDOWS)
    #if defined(UNICODE)
    if ( qWinVersion() & Qt::WV_NT_based ) {
        TCHAR winUserName[UNLEN + 1]; // UNLEN is defined in LMCONS.H
        DWORD winUserNameSize = sizeof(winUserName);
        GetUserName( winUserName, &winUserNameSize );
        userName = qt_winQString( winUserName );
    } else
    #endif
    {
        char winUserName[UNLEN + 1]; // UNLEN is defined in LMCONS.H
        DWORD winUserNameSize = sizeof(winUserName);
        GetUserNameA( winUserName, &winUserNameSize );
        userName = QString::fromLocal8Bit( winUserName );
    }
#endif
    }

    return userName;
}

const QString & GetAppRoot()
{
    static QString HomeAppRoot=QDesktopServices::storageLocation(QDesktopServices::HomeLocation)+"/"+AppRoot;

  //  wxLogMessage(wxStandardPathsBase::Get().GetUserDataDir());

    //HomeAppRoot=s+wxFileName::GetPathSeparator();

/*#if defined(__WXMSW__)
    // This conveniently maps to unix home directory for now in wine.. Change before release if necessary..
    HomeAppRoot=wxGetHomeDir()+"/"+wxT("My Documents")+"/"+AppRoot;
#elif defined(__UNIX__)
    HomeAppRoot=wxGetHomeDir()+"/"+AppRoot;
#elif defined(__WXMAC__)
    // I have no idea
    HomeAppRoot=wxGetHomeDir()+"/"+AppRoot;
#endif
    //HomeAppRoot+=wxFileName::GetPathSeparator(); // Trailing separator */
    return HomeAppRoot;
}

Preferences::Preferences()
{
    p_name="Preferences";
    p_path=GetAppRoot();
}

Preferences::Preferences(QString name,QString filename)
{

    const QString xmlext=".xml";

    if (name.endsWith(xmlext)) {
        p_name=name.section(".",0,0);
    } else {
        p_name=name;
    }

    if (filename.isEmpty()) {
        p_filename=GetAppRoot()+"/"+p_name+xmlext;
    } else {
        if (!filename.contains("/")) {
            p_filename=GetAppRoot()+"/";
        } else p_filename="";

        p_filename+=filename;

        if (!p_filename.endsWith(xmlext)) p_filename+=xmlext;
    }
}

Preferences::~Preferences()
{
    //Save(); // Don't..Save calls a virtual function.
}

int Preferences::GetCode(QString s)
{
    int prefcode=0;
    for (std::map<int,QString>::iterator i=p_codes.begin(); i!=p_codes.end(); i++) {
        if (i->second==s) return i->first;
        prefcode++;
    }
    p_codes[prefcode]=s;
    return prefcode;
}

const QString Preferences::Get(QString name)
{
    QString temp;
    QChar obr=QChar('{');
    QChar cbr=QChar('}');
    QString t,a,ref; // How I miss Regular Expressions here..
    if (p_preferences.find(name)!=p_preferences.end()) {
        temp="";
        t=p_preferences[name].toString();
        if (p_preferences[name].type()!=QVariant::String) {
            return t;
        }
    } else {
        t=name; // parse the string..
    }
    while (t.contains(obr)) {
        temp+=t.section(obr,0,0);
        a=t.section(obr,1);
        if (a.startsWith("{")) {
            temp+=obr;
            t=a.section(obr,1);
            continue;
        }
        ref=a.section(cbr,0,0);

        if (ref.toLower()=="home") {
            temp+=GetAppRoot();
        } else if (ref.toLower()=="user") {
            temp+=getUserName();
        } else if (ref.toLower()=="sep") { // redundant in QT
            temp+="/";
        } else {
            temp+=Get(ref);
        }
        t=a.section(cbr,1);
    }
    temp+=t;
    temp.replace("}}","}"); // Make things look a bit better when escaping braces.

    return temp;
}

bool Preferences::Open(QString filename)
{
    if (!filename.isEmpty()) p_filename=filename;

    qDebug(("Opening "+p_filename).toLatin1());
    TiXmlDocument xml(p_filename.toLatin1());
    if (!xml.LoadFile()) {
        return false;
    }
    TiXmlHandle hDoc(&xml);
    TiXmlElement* pElem;
    TiXmlHandle hRoot(0);
    p_preferences.clear();

    pElem=hDoc.FirstChildElement().Element();
    // should always have a valid root but handle gracefully if it does
    if (!pElem) return false;

    hRoot=TiXmlHandle(pElem);

    std::map<QString,QString> p_types;
    pElem=hRoot.FirstChild(p_name.toLatin1()).FirstChild().Element();
    for( ; pElem; pElem=pElem->NextSiblingElement()) {

        TiXmlAttribute *attr=pElem->FirstAttribute();
        assert(attr!=NULL);
        QString type=attr->Value();
        QString pKey=pElem->Value();
        QString pText=pElem->GetText();
        bool ok;
        if (!pKey.isEmpty() && !pText.isEmpty()) {
            if (type=="double") {
                double d=pText.toDouble(&ok);
                if (!ok)
                    qDebug("String to number conversion error in Preferences::Open()");
                else
                    p_preferences[pKey]=d;
            } else if (type=="qlonglong") {
                qlonglong d=pText.toLongLong(&ok);
                if (!ok)
                    qDebug("String to number conversion error in Preferences::Open()");
                else
                    p_preferences[pKey]=d;
            } else if (type=="bool") {
                int d=pText.toInt(&ok);
                if (!ok)
                    qDebug("String to number conversion error in Preferences::Open()");
                else
                    p_preferences[pKey]=(bool)d;
            } else if (type=="QDateTime") {
                QDateTime d;
                d.fromString("yyyy-MM-dd HH:mm:ss");
                if (d.isValid())
                    p_preferences[pKey]=d;
                else
                    qWarning(("Invalid DateTime record in "+filename).toLatin1());

            } else { // Assume string
                p_preferences[pKey]=pText;
            }
        }
    }
    ExtraLoad(&hRoot);
    return true;
}

bool Preferences::Save(QString filename)
{
    if (!filename.isEmpty())
        p_filename=filename;

    TiXmlDocument xml;
    TiXmlElement* msg;
    TiXmlComment * comment;
    TiXmlDeclaration *decl=new TiXmlDeclaration( "1.0", "", "" );
    xml.LinkEndChild(decl);
    TiXmlElement *root=new TiXmlElement(AppName.toLatin1());
    xml.LinkEndChild(root);

    if (!p_comment.isEmpty()) {
        comment = new TiXmlComment();
        QString s=" "+p_comment+" ";
        comment->SetValue(s.toLatin1());
        root->LinkEndChild(comment);
    }

    TiXmlElement * msgs = new TiXmlElement(p_name.toLatin1());
    root->LinkEndChild(msgs);
    for (std::map<QString,QVariant>::iterator i=p_preferences.begin(); i!=p_preferences.end(); i++) {
        QVariant::Type type=i->second.type();
        if (type==QVariant::Invalid) continue;

        msg=new TiXmlElement(i->first.toLatin1());
        qDebug(i->first.toLatin1());
        msg->SetAttribute("type",i->second.typeName());
        QString t;


        if (type==QVariant::DateTime) {
            t=i->second.toDateTime().toString("yyyy-MM-dd HH:mm:ss");
        } else {
            t=i->second.toString();
        }
        msg->LinkEndChild(new TiXmlText(t.toLatin1()));
        msgs->LinkEndChild(msg);
    }
    TiXmlElement *extra=ExtraSave();
    if (extra) root->LinkEndChild(extra);

    xml.SaveFile(p_filename.toLatin1());
    return true;
}


