/*

SleepLib Preferences Implementation

Author: Mark Watkins <jedimark64@users.sourceforge.net>
License: GPL

*/

#include <wx/wx.h>
#include <wx/filename.h>
#include <wx/string.h>
#include <wx/variant.h>
#include <wx/stdpaths.h>

#include "preferences.h"


const wxString & GetAppRoot()
{
    static wxString HomeAppRoot;
  //  wxLogMessage(wxStandardPathsBase::Get().GetUserDataDir());

    //HomeAppRoot=s+wxFileName::GetPathSeparator();

#if defined(__WXMSW__)
    // This conveniently maps to unix home directory for now in wine.. Change before release if necessary..
    HomeAppRoot=wxGetHomeDir()+wxFileName::GetPathSeparator()+wxT("My Documents")+wxFileName::GetPathSeparator()+AppRoot;
#elif defined(__UNIX__)
    HomeAppRoot=wxGetHomeDir()+wxFileName::GetPathSeparator()+AppRoot;
#elif defined(__WXMAC__)
    // I have no idea
    HomeAppRoot=wxGetHomeDir()+wxFileName::GetPathSeparator()+AppRoot;
#endif
    //HomeAppRoot+=wxFileName::GetPathSeparator(); // Trailing separator
    return HomeAppRoot;
}

Preferences::Preferences()
{
    p_name=wxT("Preferences");
    p_path=GetAppRoot();
}

Preferences::Preferences(wxString name,wxString filename)
{

    const wxString xmlext=wxT(".xml");
    const wxString sep=wxFileName::GetPathSeparator();

    if (name.EndsWith(xmlext)) {
        p_name=name.BeforeLast(wxChar('.'));
    } else {
        p_name=name;
    }

    if (filename.IsEmpty()) {
        p_filename=GetAppRoot()+sep+p_name+xmlext;
    } else {
        if (!filename.Contains(sep)) {
            p_filename=GetAppRoot()+sep;
        } else p_filename=wxT("");

        p_filename+=filename;

        if (!p_filename.EndsWith(xmlext)) p_filename+=xmlext;
    }
}

Preferences::~Preferences()
{
    //Save(); // Don't..Save calls a virtual function.
}

int Preferences::GetCode(wxString s)
{
    int prefcode=0;
    for (auto i=p_codes.begin(); i!=p_codes.end(); i++) {
        if (i->second==s) return i->first;
        prefcode++;
    }
    p_codes[prefcode]=s;
    return prefcode;
}

const wxString Preferences::Get(wxString name)
{
    wxString temp;
    wxChar obr=wxChar('{');
    wxChar cbr=wxChar('}');
    wxString t,a,ref; // How I miss Regular Expressions here..
    if (p_preferences.find(name)!=p_preferences.end()) {
        temp=wxT("");
        t=p_preferences[name].MakeString();
        if (p_preferences[name].GetType()!=wxT("string")) {
            return t;
        }
    } else {
        t=name; // parse the string..
    }
    while (t.Contains(obr)) {
        temp+=t.BeforeFirst(obr);
        a=t.AfterFirst(obr);
        if (a.StartsWith(wxT("{"))) {
            temp+=obr;
            t=a.AfterFirst(obr);
            continue;
        }
        ref=a.BeforeFirst(cbr);

        if (ref.Lower()==wxT("home")) {
            temp+=GetAppRoot();
        } else if (ref.Lower()==wxT("user")) {
            temp+=wxGetUserName();
        } else if (ref.Lower()==wxT("sep")) {
            temp+=wxFileName::GetPathSeparator();
        } else {
            temp+=Get(ref);
        }
        t=a.AfterFirst(cbr);
    }
    temp+=t;
    temp.Replace(wxT("}}"),wxT("}"),true); // Make things look a bit better when escaping braces.

    return temp;
}

bool Preferences::Open(wxString filename)
{
    if (!filename.IsEmpty()) p_filename=filename;

    wxLogVerbose(wxT("Opening ")+p_filename);
    TiXmlDocument xml(p_filename.mb_str());
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

    std::map<wxString,wxString> p_types;
    pElem=hRoot.FirstChild(p_name.mb_str()).FirstChild().Element();
    for( ; pElem; pElem=pElem->NextSiblingElement()) {

        TiXmlAttribute *attr=pElem->FirstAttribute();
        assert(attr!=NULL);
        wxString type(attr->Value(),wxConvUTF8);
        wxString pKey(pElem->Value(),wxConvUTF8);
        wxString pText(pElem->GetText(),wxConvUTF8);
        if (!pKey.IsEmpty() && !pText.IsEmpty()) {
            if (type==wxT("double")) {
                double d;
                pText.ToDouble(&d);
                p_preferences[pKey]=d;
            } else if (type==wxT("long")) {
                long d;
                pText.ToLong(&d);
                p_preferences[pKey]=d;
            } else if (type==wxT("bool")) {
                long d;
                pText.ToLong(&d);
                p_preferences[pKey]=(bool)d;
            } else if (type==wxT("datetime")) {
                wxDateTime d;
#if wxCHECK_VERSION(2,9,0)
                wxString::const_iterator end;
                d.ParseFormat(pText,wxT("%Y-%m-%d %H:%M:%S"),&end);
                assert(end==pText.end());
#else
                const wxChar *end=d.ParseFormat(pText,wxT("%Y-%m-%d %H:%M:%S"));
                assert(end!=NULL);
#endif
                p_preferences[pKey]=d;

            } else {
                p_preferences[pKey]=pText;
            }
        }
    }
    ExtraLoad(&hRoot);
    return true;
}

bool Preferences::Save(wxString filename)
{
    if (!filename.IsEmpty()) p_filename=filename;

    TiXmlDocument xml;
    TiXmlElement* msg;
    TiXmlComment * comment;
    TiXmlDeclaration *decl=new TiXmlDeclaration( "1.0", "", "" );
    xml.LinkEndChild(decl);
    TiXmlElement *root=new TiXmlElement(AppName.mb_str());
    xml.LinkEndChild(root);

    if (!p_comment.IsEmpty()) {
        comment = new TiXmlComment();
        wxString s=wxT(" ")+p_comment+wxT(" ");
        comment->SetValue(s.mb_str());
        root->LinkEndChild(comment);
    }

    TiXmlElement * msgs = new TiXmlElement(p_name.mb_str());
    root->LinkEndChild(msgs);
    for (auto i=p_preferences.begin(); i!=p_preferences.end(); i++) {
        msg=new TiXmlElement(i->first.mb_str());
        wxString type=i->second.GetType();
        msg->SetAttribute("type",type.mb_str());
        wxString t;

        if (type==wxT("datetime")) {
            t=i->second.GetDateTime().Format(wxT("%Y-%m-%d %H:%M:%S"));
        } else {
            t=i->second.MakeString();
        }
        msg->LinkEndChild(new TiXmlText(t.mb_str()));
        msgs->LinkEndChild(msg);
    }
    TiXmlElement *extra=ExtraSave();
    if (extra) root->LinkEndChild(extra);

    xml.SaveFile(p_filename.mb_str());
    return true;
}


