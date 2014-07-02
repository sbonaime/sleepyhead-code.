/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 *
 * Version
 *
 * Copyright (c) 2011-2014 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#ifndef VERSION_H
#define VERSION_H

#include <qglobal.h>

const int major_version = 0;   // incompatible API changes
const int minor_version = 9;   // new features that don't break things
const int patch_number = 7;    // bugfixes, revisions

#ifdef TEST_BUILD
const QString ReleaseStatus = "testing";
#else
const QString ReleaseStatus = "beta";
#endif

const QString VersionString = QString().sprintf("%i.%i.%i", major_version, minor_version, patch_number);
const QString FullVersionString = QString().sprintf("%i.%i.%i", major_version, minor_version, patch_number)+"-"+ReleaseStatus;

#ifdef Q_OS_MAC
const QString PlatformString = "MacOSX";
#elif defined(Q_OS_WIN32)
const QString PlatformString = "Win32";
#elif defined(Q_OS_LINUX)
const QString PlatformString = "Linux";
#endif

#endif // VERSION_H
