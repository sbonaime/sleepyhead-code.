/*
 SleepLib Common Functions
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

#include <QDateTime>
#include <QDir>

#include "profiles.h"

qint64 timezoneOffset() {
    static bool ok=false;
    static qint64 _TZ_offset=0;

    if (ok) return _TZ_offset;
    QDateTime d1=QDateTime::currentDateTime();
    QDateTime d2=d1;
    d1.setTimeSpec(Qt::UTC);
    _TZ_offset=d2.secsTo(d1);
    _TZ_offset*=1000L;
    return _TZ_offset;
}

QString weightString(float kg, UnitSystem us)
{
    if (us==US_Undefined)
        us=PROFILE.general->unitSystem();

    if (us==US_Metric) {
        return QString("%1kg").arg(kg,0,'f',2);
    } else if (us==US_Archiac) {
        int oz=(kg*1000.0) / (float)ounce_convert;
        int lb=oz / 16.0;
        oz = oz % 16;
        return QString("%1lb %2oz").arg(lb,0,10).arg(oz);
    }
    return("Bad UnitSystem");
}

bool operator <(const ValueCount & a, const ValueCount & b)
{
     return a.value < b.value;
}

bool removeDir(const QString & path)
{
    bool result = true;
    QDir dir(path);

    if (dir.exists(path)) {
        Q_FOREACH(QFileInfo info, dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden  | QDir::AllDirs | QDir::Files, QDir::DirsFirst)) {
            if (info.isDir()) { // Recurse to remove this child directory
                result = removeDir(info.absoluteFilePath());
            } else { // File
                result = QFile::remove(info.absoluteFilePath());
            }

            if (!result) {
                return result;
            }
        }
        result = dir.rmdir(path);
    }

    return result;
}
