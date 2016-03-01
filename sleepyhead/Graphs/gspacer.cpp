/* graph spacer Implementation
 *
 * Copyright (c) 2011-2016 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#include "gspacer.h"

gSpacer::gSpacer(int space)
    : Layer(NoChannel)
{
    m_space = space;
}
