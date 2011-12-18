/*
 Common GUI Functions Header
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

#ifndef COMMON_GUI_H
#define COMMON_GUI_H

#include <QLocale>

//! \brief Gets the first day of week from the system locale, to show in the calendars.
Qt::DayOfWeek firstDayOfWeekFromLocale();

//! \brief Delay of ms milliseconds
void sh_delay(int ms);

#endif
