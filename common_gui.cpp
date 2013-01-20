/*
 Common GUI Functions Implementation
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

#include "common_gui.h"
#include "qglobal.h"

#ifndef BUILD_WITH_MSVC
#include <unistd.h>
#endif

#ifdef Q_OS_WIN32
#include "windows.h"
void sh_delay(int ms)
{
    Sleep(ms);
}
#else
void sh_delay(int ms)
{
    sleep(ms/1000);
}
#endif


#if (QT_VERSION >= QT_VERSION_CHECK(4,8,0))
// Qt 4.8 makes this a whole lot easier
Qt::DayOfWeek firstDayOfWeekFromLocale()
{
    return QLocale::system().firstDayOfWeek();
}

#else
/*#if defined(Q_OS_MAC)
#include <Cocoa/Cocoa.h>
#endif */

#ifdef Q_OS_WIN
#include "windows.h"
#endif

#ifdef __GLIBC__
#include <langinfo.h>
#endif


#ifdef Q_OS_WIN32
#include <dos.h>
#endif


// This function has been "borrowed".. Ahem..
Qt::DayOfWeek firstDayOfWeekFromLocale()
{
    Qt::DayOfWeek firstDay = Qt::Monday; // Fallback, acknowledging the awesome concept of Week End.

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
/*#elif defined(Q_OS_MAC)
    // Unsure if this will work.. Most Mac users use 4.8 anyway, so won't see this code..
    NSCalendar *cal = [NSCalendar currentCalendar];
    int day = ([cal firstWeekday] + 5) % 7 + 1;

    firstDay = (Qt::DayOfWeek)(unsigned char)day; */
#endif
    return firstDay;
}

#endif

