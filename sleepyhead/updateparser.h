/* UpdateParser Header (Autoupdater component)
 *
 * Copyright (c) 2011-2014 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#ifndef UPDATEPARSER_H
#define UPDATEPARSER_H

#include <QXmlDefaultHandler>
#include <QMetaType>
#include <QDate>

enum UpdateStatus { UPDATE_TESTING = 0, UPDATE_BETA, UPDATE_STABLE, UPDATE_CRITICAL };

/*! \struct Update
    \brief Holds platform specific information about an individual updates
  */
class Update
{
  public:
    Update();
    Update(const Update &copy);
    Update(QString _type, QString _version, QString _platform, QDate _date);
    QString type;
    QString version;
    QString platform;
    UpdateStatus status;
    QDate date;
    QString filename;
    QString url;
    QString hash;
    qint64 size;
    QString notes;
    QString unzipped_path;
};

/*! \struct Release
    \brief Holds information about an individual major release
  */
struct Release {
    Release() {}
    Release(const Release &copy) {
        version = copy.version;
        codename = copy.version;
        notes = copy.notes;
        info_url = copy.info_url;
        status = copy.status;
        updates = copy.updates;
    }

    Release(QString ver, QString code, UpdateStatus stat) { version = ver; codename = code; status = stat; }
    QString version;
    QString codename;
    UpdateStatus status;
    QString info_url;
    QHash<QString, QString> notes; // by platform
    QHash<QString, QList<Update> > updates;
};

Q_DECLARE_METATYPE(Update *)


/*! \class UpdateParser
    \brief SAX XML parser for update.xml
  */
class UpdateParser: public QXmlDefaultHandler
{
  public:
    bool startDocument();
    bool endElement(const QString &namespaceURI, const QString &localName, const QString &name);
    bool characters(const QString &ch);
    bool startElement(const QString &namespaceURI, const QString &localName, const QString &name,
                      const QXmlAttributes &atts);
    bool endDocument();
    QString latest() { return latest_version; }

    QHash<QString, Release> releases;
  private:
    Update *update;
    Release *release;
    QString version, platform;
    QString release_date;
    QString latest_version;
    bool inRelease;
    bool inUpdate;
    bool inNotes;
    bool inUpdateNotes;
};



#endif // UPDATEPARSER_H
