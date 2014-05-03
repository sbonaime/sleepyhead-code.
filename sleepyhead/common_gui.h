/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 *
 * Common GUI Functions Header
 *
 * Copyright (c) 2011-2014 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#ifndef COMMON_GUI_H
#define COMMON_GUI_H

#include <QLocale>
#include "Graphs/glcommon.h"

//! \brief Gets the first day of week from the system locale, to show in the calendars.
Qt::DayOfWeek firstDayOfWeekFromLocale();

// Flag Colors
extern QColor COLOR_Hypopnea;
extern QColor COLOR_Obstructive;
extern QColor COLOR_Apnea;
extern QColor COLOR_CSR;
extern QColor COLOR_LargeLeak;
extern QColor COLOR_ClearAirway;
extern QColor COLOR_RERA;
extern QColor COLOR_VibratorySnore;
extern QColor COLOR_FlowLimit;
extern QColor COLOR_LeakFlag;
extern QColor COLOR_NRI;
extern QColor COLOR_ExP;
extern QColor COLOR_PressurePulse;
extern QColor COLOR_PulseChange;
extern QColor COLOR_SPO2Drop;
extern QColor COLOR_UserFlag1;

// Chart Colors
extern QColor COLOR_EPAP;
extern QColor COLOR_IPAP;
extern QColor COLOR_IPAPLo;
extern QColor COLOR_IPAPHi;
extern QColor COLOR_Plethy;
extern QColor COLOR_Pulse;
extern QColor COLOR_SPO2;
extern QColor COLOR_FlowRate;
extern QColor COLOR_Pressure;
extern QColor COLOR_RDI;
extern QColor COLOR_AHI;
extern QColor COLOR_Leak;
extern QColor COLOR_LeakTotal;
extern QColor COLOR_MaxLeak;
extern QColor COLOR_Snore;
extern QColor COLOR_RespRate;
extern QColor COLOR_MaskPressure;
extern QColor COLOR_PTB;            // Patient Triggered Breathing
extern QColor COLOR_MinuteVent;
extern QColor COLOR_TgMV;
extern QColor COLOR_TidalVolume;
extern QColor COLOR_FLG;            // Flow Limitation Graph
extern QColor COLOR_IE;             // Inspiratory Expiratory Ratio
extern QColor COLOR_Te;
extern QColor COLOR_Ti;
extern QColor COLOR_SleepStage;

#endif // COMMON_GUI_H
