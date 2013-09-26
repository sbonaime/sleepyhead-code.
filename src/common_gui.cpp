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


// Flag Colors
QColor COLOR_Hypopnea=Qt::blue;
QColor COLOR_Obstructive=COLOR_Aqua;
QColor COLOR_Apnea=Qt::darkGreen;
QColor COLOR_CSR=COLOR_LightGreen;
QColor COLOR_ClearAirway=QColor("#b254cd");
QColor COLOR_RERA=COLOR_Gold;
QColor COLOR_VibratorySnore=QColor("#ff4040");
QColor COLOR_FlowLimit=QColor("#404040");
QColor COLOR_LeakFlag=QColor("#40c0c0"); //Qt::darkBlue;
QColor COLOR_NRI=Qt::darkMagenta;
QColor COLOR_ExP=Qt::darkCyan;
QColor COLOR_PressurePulse=Qt::red;
QColor COLOR_PulseChange=COLOR_LightGray;
QColor COLOR_SPO2Drop=COLOR_LightBlue;
QColor COLOR_UserFlag1=QColor("#e0e0e0");

// Chart Colors
QColor COLOR_EPAP=Qt::blue;
QColor COLOR_IPAP=Qt::red;
QColor COLOR_IPAPLo=Qt::darkRed;
QColor COLOR_IPAPHi=Qt::darkRed;
QColor COLOR_Plethy=Qt::darkBlue;
QColor COLOR_Pulse=Qt::red;
QColor COLOR_SPO2=Qt::blue;
QColor COLOR_FlowRate=Qt::black;
QColor COLOR_Pressure=Qt::darkGreen;
QColor COLOR_RDI=COLOR_LightGreen;
QColor COLOR_AHI=COLOR_LightGreen;
QColor COLOR_Leak=COLOR_DarkMagenta;
QColor COLOR_LeakTotal=COLOR_DarkYellow;
QColor COLOR_MaxLeak=COLOR_DarkRed;
QColor COLOR_Snore=COLOR_DarkGray;
QColor COLOR_RespRate=COLOR_DarkBlue;
QColor COLOR_MaskPressure=COLOR_Blue;
QColor COLOR_PTB=COLOR_Gray;              //Patient Triggered Breathing
QColor COLOR_MinuteVent=COLOR_Cyan;
QColor COLOR_TgMV=COLOR_DarkCyan;
QColor COLOR_TidalVolume=COLOR_Magenta;
QColor COLOR_FLG=COLOR_DarkBlue;          // Flow Limitation Graph
QColor COLOR_IE=COLOR_DarkRed;            // Inspiratory Expiratory Ratio
QColor COLOR_Te=COLOR_DarkGreen;
QColor COLOR_Ti=COLOR_DarkBlue;
QColor COLOR_SleepStage=COLOR_Gray;




