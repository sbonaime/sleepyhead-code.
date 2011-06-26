/*

SleepLib Preferences Header

Author: Mark Watkins <jedimark64@users.sourceforge.net>
License: GPL

*/

#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <QString>
#include <QVariant>
#include <map>
#include "tinyxml/tinyxml.h"


const QString AppName="Application"; // Outer tag of XML files
const QString AppRoot="SleepApp";    // The Folder Name

extern const QString & GetAppRoot(); //returns app root path plus trailing path separator.

inline QString PrefMacro(QString s)
{
    return "{"+s+"}";
};

const QString & getUserName();

class Preferences
{
public:
    Preferences(QString name,QString filename="");
    Preferences();
    virtual ~Preferences();

    const QString Get(QString name);
    //const QString Get(const char * name) {
//        return Get(name);
  //  };
    const QString Get(int code) {
        return Get(p_codes[code]);
    };

    // operator[] will not expand {} macros

    QVariant & operator[](QString name) {
        return p_preferences[name];
    };
    QVariant & operator[](const char * name) {
        return p_preferences[name];
    };
    QVariant & operator[](int code) {
        return p_preferences[p_codes[code]];
    };

    void Set(QString name,QVariant value) {
        p_preferences[name]=value;
    };
    /*void Set(const char * name,QVariant value) {
        QString t=name;
        p_preferences[t]=value;
    }; */
    void Set(int code,QVariant value) {
        Set(p_codes[code],value);
    };

    bool Exists(QString name) {
        return (p_preferences.find(name)!=p_preferences.end());
    };
    /*bool Exists(const char * name) {
        //QString t(name,wxConvUTF8);
        return Exists(name);
    }; */
    //bool Exists(int code) { return Exists(p_codes[code]); };

    virtual void ExtraLoad(TiXmlHandle *root) { root=root; };
    virtual TiXmlElement * ExtraSave() {
        return NULL;
    };

    virtual bool Open(QString filename="");
    virtual bool Save(QString filename="");

    void SetComment(const QString & str) {
        p_comment=str;
    };
    int  GetCode(QString name); // For registering/looking up new preference code.

    std::map<QString,QVariant> p_preferences;
protected:
    std::map<int,QString> p_codes;
    QString p_comment;
    QString p_name;
    QString p_filename;
    QString p_path;
};


extern Preferences pref;
extern Preferences laypref;

#endif // PREFERENCES_H

