#ifndef SCHEMA_H
#define SCHEMA_H

#include <QColor>
#include <QHash>
#include <QVariant>
#include <QString>

namespace schema {

enum Function {
    NONE=0, AVG, WAVG, MIN, MAX, SUM, CNT, P90, CPH, SPH, HOURS, SET
};

enum ChanType {
    DATA=0, SETTING
};

enum DataType {
    DEFAULT=0, INTEGER, BOOL, DOUBLE, STRING, RICHTEXT, DATE, TIME, DATETIME
};
enum ScopeType {
    GLOBAL=0, MACHINE, DAY, SESSION
};
class Channel;
extern Channel EmptyChannel;

// this is really a channel definition.
class Channel
{
public:
    Channel() { m_id=0; }
    Channel(int id, ChanType type, ScopeType scope, QString name, QString description, QString label, QString unit,DataType datatype=DEFAULT, QColor=Qt::black, int link=0);
    void addColor(Function f, QColor color) { m_colors[f]=color; }
    void addOption(int i, QString option) { m_options[i]=option; }

    const int & id() { return m_id; }
    const ChanType & type() { return m_type; }
    const QString & name() { return m_name; }
    const QString & description() { return m_description; }
    const QString & label() { return m_label; }
    QHash<int,QString> m_options;
    QHash<Function,QColor> m_colors;
    QList<Channel *> m_links;              // better versions of this data type
    bool isNull();
protected:
    int m_id;
    ChanType m_type;
    ScopeType m_scope;
    QString m_name;
    QString m_description;
    QString m_label;
    QString m_unit;
    DataType m_datatype;
    QColor m_defaultcolor;

    int m_link;
};

class ChannelList
{
public:
    ChannelList();
    virtual ~ChannelList();
    bool Load(QString filename);
    bool Save(QString filename);
    Channel & operator[](int i) {
        if (channels.contains(i))
            return *channels[i];
        else
            return EmptyChannel;
    }
    Channel & operator[](QString name) {
        if (names.contains(name))
            return *names[name];
        else
            return EmptyChannel;
    }

    QHash<int,Channel *> channels;
    QHash<QString,Channel *> names;
    QHash<QString,QHash<QString,Channel *> > groups;
    QString m_doctype;
};

extern ChannelList channel;

void init();

} // namespace

#endif // SCHEMA_H
