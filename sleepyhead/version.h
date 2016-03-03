/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 *
 * Version
 *
 * Copyright (c) 2011-2016 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#ifndef VERSION_H
#define VERSION_H

#include <qglobal.h>
#include "build_number.h"

const int major_version = 1;   // incompatible API changes
const int minor_version = 0;   // new features that don't break things
const int revision_number = 0;    // bugfixes, revisions

#ifdef TEST_BUILD
const QString ReleaseStatus = "testing";
#elif BETA_BUILD
const QString ReleaseStatus = "beta";
#else
const QString ReleaseStatus = "";
#endif


const QString VersionString = QString().sprintf("%i.%i.%i", major_version, minor_version, build_number); // )+VersionStatus+QString().sprintf("%i",
const QString FullVersionString = VersionString+"-"+ReleaseStatus;

#ifdef Q_OS_MAC
const QString PlatformString = "MacOSX";
#elif defined(Q_OS_WIN32)
const QString PlatformString = "Win32";
#elif defined(Q_OS_LINUX)
const QString PlatformString = "Linux";
#elif defined(Q_OS_HAIKU)
const QString PlatformString = "Haiku";
#endif

#endif // VERSION_H
