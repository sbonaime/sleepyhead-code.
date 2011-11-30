/*

SleepLib Preferences Header

Author: Mark Watkins <jedimark64@users.sourceforge.net>
License: GPL

*/

#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <QString>
#include <QVariant>
#include <QHash>
#include <QDomElement>
#include <QDomDocument>
#include <map>


const QString AppName="SleepyHead"; // Outer tag of XML files
const QString AppRoot="SleepApp";    // The Folder Name

extern const QString & GetAppRoot(); //returns app root path plus trailing path separator.

inline QString PrefMacro(QString s)
{
    return "{"+s+"}";
}

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
    /*const QString Get(int code) {
        return Get(p_codes[code]);
    }*/

    // operator[] will not expand {} macros

    QVariant & operator[](QString name) {
        return p_preferences[name];
    }
    /*QVariant & operator[](int code) {
        return p_preferences[p_codes[code]];
    }*/

    void Set(QString name,QVariant value) {
        p_preferences[name]=value;
    }
    /*void Set(int code,QVariant value) {
        Set(p_codes[code],value);
    }*/

    bool Exists(QString name) {
        return (p_preferences.contains(name));
    }
    bool ExistsAndTrue(QString name) {
        QHash<QString,QVariant>::iterator i=p_preferences.find(name);
        if (i==p_preferences.end()) return false;
        return i.value().toBool();
    }
    void Erase(QString name) {
        QHash<QString,QVariant>::iterator i=p_preferences.find(name);
        if (i!=p_preferences.end())
            p_preferences.erase(i);
    }

    virtual void ExtraLoad(QDomElement & root) { root=root; }
    virtual QDomElement ExtraSave(QDomDocument & doc) { doc=doc; QDomElement e; return e; }

    virtual bool Open(QString filename="");
    virtual bool Save(QString filename="");

    void SetComment(const QString & str) {
        p_comment=str;
    }

    inline QHash<QString,QVariant>::iterator find(QString key) { return p_preferences.find(key); }
    inline QHash<QString,QVariant>::iterator end() { return p_preferences.end(); }
    inline QHash<QString,QVariant>::iterator begin() { return p_preferences.begin(); }

    //int  GetCode(QString name); // For registering/looking up new preference code.

    QHash<QString,QVariant> p_preferences;
protected:
    //QHash<int,QString> p_codes;
    QString p_comment;
    QString p_name;
    QString p_filename;
    QString p_path;
};

enum PrefType { PT_Checkbox, PT_Integer, PT_Number, PT_Date, PT_Time, PT_DateTime, PT_LineEdit, PT_TextEdit, PT_Dropdown };
class Preference
{
public:
    Preference() {
        m_pref=NULL;
    }
    Preference(const Preference & copy) {
        m_pref=copy.m_pref;
        m_code=copy.m_code;
        m_type=copy.m_type;
        m_label=copy.m_label;
        m_tooltip=copy.m_tooltip;
        m_defaultValue=copy.m_defaultValue;
    }
    Preference(Preferences * pref, QString code, PrefType type, QString label, QString tooltip, QVariant default_value);
    ~Preference() {}

    QString code() { return m_code; }

    void setValue(QVariant v);
    QVariant & value();

    PrefType type() { return m_type; }
    QString label() { return m_label; }
    QString tooltip() { return m_tooltip; }
    QVariant defaultValue() { return m_defaultValue; }
protected:
    Preferences * m_pref;
    QString m_code;
    PrefType m_type;
    QString m_label;
    QString m_tooltip;
    QVariant m_defaultValue;
};

Q_DECLARE_METATYPE(Preference)

extern Preferences PREF;
extern Preferences LAYOUT;

#endif // PREFERENCES_H

