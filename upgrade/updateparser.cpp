#include <QDebug>


#include "updateparser.h"

bool UpdateParser::startDocument()
{
    inRelease=false;
    inUpdate=false;
    inNotes=false;
    inUpdateNotes=false;
    release=NULL;
    update=NULL;
    return true;
}

bool UpdateParser::endElement(const QString &namespaceURI, const QString &localName, const QString &qName)
{
    QString name=qName.toLower();
    if (name=="release") {
        inRelease=false;
        release=NULL;
    } else if (inRelease && name=="update") {
        inUpdate=false;
        update=NULL;
    } else if (inUpdate && name=="notes")
        inUpdateNotes=false;
    else if (inRelease && name=="notes")
        inNotes=false;
    return true;
}
bool UpdateParser::characters(const QString &ch)
{
    if (inUpdateNotes) {
        update->notes=ch;
    } else if (inNotes) {
        release->notes[platform]=ch;
    }
    return true;
}

bool UpdateParser::startElement(const QString &namespaceURI, const QString &localName, const QString &qName, const QXmlAttributes &atts)
{
    QString name=qName.toLower();
    if (inRelease && name=="update") {
        QString ver, type;
        for (int i=0;i<atts.count();i++) {
            if (atts.localName(i)=="type")
                type=atts.value(i).toLower();
            if (atts.localName(i)=="version")
                ver=atts.value(i).toLower();
            if (atts.localName(i)=="platform")
                platform=atts.value(i).toLower();
            if (atts.localName(i)=="release_date")
                release_date=atts.value(i);
        }
        QDate date=QDate::fromString(release_date,"yyyy-MM-dd");
        if (!date.isValid()) date=QDate::currentDate();

        release->updates[platform].push_back(Update(type,ver,platform,date));
        update=&release->updates[platform][release->updates[platform].size()-1];
        inUpdate=true;
    } else if (inRelease && name=="info") {
        QString tmp=atts.value("url");
        if (tmp.isEmpty()) return false;
        release->info_url=tmp;
    } else if (inUpdate && name=="file") {
        for (int i=0;i<atts.count();i++) {
            if (atts.localName(i)=="name")
                update->filename=atts.value(i);
            if (atts.localName(i)=="size") {
                bool ok;
                update->size=atts.value(i).toLongLong(&ok);
                //if (!ok) return false;
            }
            if (atts.localName(i)=="url")
                update->url=atts.value(i);
            if (atts.localName(i)=="hash")
                update->hash=atts.value(i).toLower();
        }
    } else if (inUpdate && name=="notes") {
        inUpdateNotes=true;
    } else if (inRelease && name=="notes") {
        platform="";
        if (atts.count()>=1) {
            if (atts.localName(0)=="platform") {
                platform=atts.value(0);
            }
        }
        inNotes=true;
    } else if (name=="release") {
        inRelease=true;
        QString codename,status;
        for (int i=0;i<atts.count();i++) {
            if (atts.localName(i)=="version")
                version=atts.value(i).toLower();
            if (atts.localName(i)=="codename")
                codename=atts.value(i);
            if (atts.localName(i)=="status")
                status=atts.value(i);
        }
        releases[version]=Release(version,codename,status);
        release=&releases[version];
        if (version>latest_version) latest_version=version;
    }
    return true;
}
bool UpdateParser::endDocument()
{
    /*for (QHash<QString,Release>::iterator r=releases.begin();r!=releases.end();r++) {
        Release & rel=r.value();
        qDebug() << "New Version" << r.key() << rel.codename << rel.notes;
        for (QHash<QString,Update>::iterator u=rel.files.begin();u!=rel.files.end();u++) {
            Update & up=u.value();
            qDebug() << "Platform:" << u.key() << up.filename << up.date;
        }
    }*/
    return true;
}

