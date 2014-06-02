/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 *
 * Common GUI Functions Implementation
 *
 * Copyright (c) 2011-2014 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#include "common_gui.h"

#ifndef BUILD_WITH_MSVC
# include <unistd.h>
#endif


#if (QT_VERSION >= QT_VERSION_CHECK(4,8,0))
// Qt 4.8 makes this a whole lot easier
Qt::DayOfWeek firstDayOfWeekFromLocale()
{
    return QLocale::system().firstDayOfWeek();
}
#elif defined(__GLIBC__)
# include <langinfo.h>
Qt::DayOfWeek firstDayOfWeekFromLocale()
{
    const unsigned char *const s = nl_langinfo(_NL_TIME_FIRST_WEEKDAY);

    if (s && *s >= 1 && *s <= 7) {
        // Map between nl_langinfo and Qt:
        //              Sun Mon Tue Wed Thu Fri Sat
        // nl_langinfo:  1   2   3   4   5   6   7
        //   DayOfWeek:  7   1   2   3   4   5   6
        return (Qt::DayOfWeek)((*s + 5) % 7 + 1);
    }

    return Qt::Monday;
}
#elif defined(Q_OS_WIN)
# include "windows.h"
Qt::DayOfWeek firstDayOfWeekFromLocale()
{
    Qt::DayOfWeek firstDay = Qt::Monday; // Fallback, acknowledging the awesome concept of weekends.
    WCHAR wsDay[4];
# if defined(_WIN32_WINNT_VISTA) && WINVER >= _WIN32_WINNT_VISTA && defined(LOCALE_NAME_USER_DEFAULT)

    if (GetLocaleInfoEx(LOCALE_NAME_USER_DEFAULT, LOCALE_IFIRSTDAYOFWEEK, wsDay, 4)) {
# else

    if (GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_IFIRSTDAYOFWEEK, wsDay, 4)) {
# endif
        bool ok;
        int wfd = QString::fromWCharArray(wsDay).toInt(&ok) + 1;

        if (ok) {
            return (Qt::DayOfWeek)(unsigned char)wfd;
        }
    }

    return firstDay;
}

#endif // QT_VERSION

// Flag Colors
QColor COLOR_Hypopnea       = Qt::blue;
QColor COLOR_Obstructive    = COLOR_Aqua;
QColor COLOR_Apnea          = Qt::darkGreen;
QColor COLOR_CSR            = COLOR_LightGreen;
QColor COLOR_LargeLeak      = COLOR_LightGray;
QColor COLOR_ClearAirway    = QColor("#b254cd");
QColor COLOR_RERA           = COLOR_Gold;
QColor COLOR_VibratorySnore = QColor("#ff4040");
QColor COLOR_FlowLimit      = QColor("#404040");
QColor COLOR_SensAwake      = COLOR_Gold;
QColor COLOR_LeakFlag       = QColor("#40c0c0"); // Qt::darkBlue;
QColor COLOR_NRI            = QColor("orange"); //COLOR_ClearAirway;
QColor COLOR_ExP            = Qt::darkCyan;
QColor COLOR_PressurePulse  = Qt::red;
QColor COLOR_PulseChange    = COLOR_LightGray;
QColor COLOR_SPO2Drop       = COLOR_LightBlue;
QColor COLOR_UserFlag1      = QColor("#e0e0e0");

// Chart Colors
QColor COLOR_EPAP         = Qt::blue;
QColor COLOR_IPAP         = Qt::red;
QColor COLOR_IPAPLo       = Qt::darkRed;
QColor COLOR_IPAPHi       = Qt::darkRed;
QColor COLOR_Plethy       = Qt::darkBlue;
QColor COLOR_Pulse        = Qt::red;
QColor COLOR_SPO2         = Qt::blue;
QColor COLOR_FlowRate     = Qt::black;
QColor COLOR_Pressure     = Qt::darkGreen;
QColor COLOR_RDI          = COLOR_LightGreen;
QColor COLOR_AHI          = COLOR_LightGreen;
QColor COLOR_Leak         = COLOR_DarkMagenta;
QColor COLOR_LeakTotal    = COLOR_DarkYellow;
QColor COLOR_MaxLeak      = COLOR_DarkRed;
QColor COLOR_Snore        = COLOR_DarkGray;
QColor COLOR_RespRate     = COLOR_DarkBlue;
QColor COLOR_MaskPressure = COLOR_Blue;
QColor COLOR_PTB          = COLOR_Gray;       // Patient-Triggered Breathing
QColor COLOR_MinuteVent   = COLOR_Cyan;
QColor COLOR_TgMV         = COLOR_DarkCyan;
QColor COLOR_TidalVolume  = COLOR_Magenta;
QColor COLOR_FLG          = COLOR_DarkBlue;   // Flow Limitation Graph
QColor COLOR_IE           = COLOR_DarkRed;    // Inspiratory Expiratory Ratio
QColor COLOR_Te           = COLOR_DarkGreen;
QColor COLOR_Ti           = COLOR_DarkBlue;
QColor COLOR_SleepStage   = COLOR_Gray;
