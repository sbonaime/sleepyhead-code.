#include "common_gui.h"
#include "qglobal.h"

#if QT_VERSION<0x400800
#if defined(Q_OS_MAC)
#include "cocoacommon.h"
#include <Cocoa/Cocoa.h>
#endif

#ifdef Q_OS_WIN
#include "windows.h"
#endif

#ifdef __GLIBC__
#include <langinfo.h>
#endif

Qt::DayOfWeek firstDayOfWeekFromLocale()
{
    Qt::DayOfWeek firstDay = Qt::Monday;
#ifdef Q_OS_WIN
    WCHAR wsDay[4];
# if defined(_WIN32_WINNT_VISTA) && WINVER >= _WIN32_WINNT_VISTA && defined(LOCALE_NAME_USER_DEFAULT)
    if (GetLocaleInfoEx(LOCALE_NAME_USER_DEFAULT, LOCALE_IFIRSTDAYOFWEEK, wsDay, 4)) {
# else
    if (GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_IFIRSTDAYOFWEEK, wsDay, 4)) {
# endif
    bool ok;
    int wfd = QString::fromWCharArray(wsDay).toInt(&ok) + 1;
        if (ok) {
            firstDay = (Qt::DayOfWeek)(unsigned char)wfd;
        }
    }
#elif defined(__GLIBC__)
    firstDay = (Qt::DayOfWeek)(unsigned char)((*nl_langinfo(_NL_TIME_FIRST_WEEKDAY) + 5) % 7 + 1);
#elif defined(Q_OS_MAC)
    NSCalendar *cal = [NSCalendar currentCalendar];
    int day = ([cal firstWeekday] + 5) % 7 + 1;

    firstDay = (Qt::DayOfWeek)(unsigned char)day;
#endif
    return firstDay;
}


#else
Qt::DayOfWeek firstDayOfWeekFromLocale()
{
    return QLocale::system().firstDayOfWeek();
}
#endif

