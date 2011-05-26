/*

SleepLib Preferences Header

Author: Mark Watkins <jedimark64@users.sourceforge.net>
License: GPL

*/

#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <wx/string.h>
#include <wx/variant.h>
#include <map>
#include "tinyxml/tinyxml.h"


const wxString AppName=_("Application"); // Outer tag of XML files
const wxString AppRoot=wxT("SleepApp");    // The Folder Name

extern const wxString & GetAppRoot(); //returns app root path plus trailing path separator.

inline wxString PrefMacro(wxString s)
{
    return wxT("{")+s+wxT("}");
};


class Preferences
{
public:
    Preferences(wxString name,wxString filename=wxT(""));
    Preferences();
    virtual ~Preferences();

    const wxString Get(wxString name);
    const wxString Get(const char * name) {
        wxString t(name,wxConvUTF8);
        return Get(t);
    };
    const wxString Get(int code) {
        return Get(p_codes[code]);
    };

    // operator[] will not expand {} macros

    wxVariant & operator[](wxString name) {
        return p_preferences[name];
    };
    wxVariant & operator[](const char * name) {
        wxString t(name,wxConvUTF8);
        return p_preferences[t];
    };
    wxVariant & operator[](int code) {
        return p_preferences[p_codes[code]];
    };

    void Set(wxString name,wxVariant value) {
        p_preferences[name]=value;
    };
    void Set(const char * name,wxVariant value) {
        wxString t(name,wxConvUTF8);
        p_preferences[t]=value;
    };
    void Set(int code,wxVariant value) {
        Set(p_codes[code],value);
    };

    bool Exists(wxString name) {
        return (p_preferences.find(name)!=p_preferences.end());
    };
    bool Exists(const char * name) {
        wxString t(name,wxConvUTF8);
        return Exists(t);
    };
    //bool Exists(int code) { return Exists(p_codes[code]); };

    virtual void ExtraLoad(TiXmlHandle *root) {};
    virtual TiXmlElement * ExtraSave() {
        return NULL;
    };

    virtual bool Open(wxString filename=wxT(""));
    virtual bool Save(wxString filename=wxT(""));

    void SetComment(const wxString & str) {
        p_comment=str;
    };
    int  GetCode(wxString name); // For registering/looking up new preference code.

    std::map<wxString,wxVariant> p_preferences;
protected:
    std::map<int,wxString> p_codes;
    wxString p_comment;
    wxString p_name;
    wxString p_filename;
    wxString p_path;
};


extern Preferences pref;
extern Preferences layout;

#endif // PREFERENCES_H

