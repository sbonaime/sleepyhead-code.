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

float brightness(QColor color) {
    return color.redF()*0.299 + color.greenF()*0.587 + color.blueF()*0.114;
}


#ifdef BUILD_WITH_MSVC

#if (_MSC_VER < 1800)
double round(double number)
{
    return number < 0.0 ? ceil(number - 0.5) : floor(number + 0.5);
}
#endif
#endif

