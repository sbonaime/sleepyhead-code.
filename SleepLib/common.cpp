/*
 SleepLib Common Functions
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

#include <QDateTime>

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
