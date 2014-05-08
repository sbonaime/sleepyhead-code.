/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 *
 * glcommon GL code & font stuff
 *
 * Copyright (c) 2011-2014 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#include <cmath>
#include "glcommon.h"

#ifdef BUILD_WITH_MSVC
double round(double number)
{
    return number < 0.0 ? ceil(number - 0.5) : floor(number + 0.5);
}
#endif

