/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 *
 * glcommon GL code & font stuff Header
 *
 * Copyright (c) 2011-2014 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#ifndef GLCOMMON_H
#define GLCOMMON_H

#include <QtOpenGL/qgl.h>
#include <QColor>

const QColor COLOR_Black = Qt::black;
const QColor COLOR_LightGreen = QColor("light green");
const QColor COLOR_DarkGreen = Qt::darkGreen;
const QColor COLOR_Purple = QColor("purple");
const QColor COLOR_Aqua = QColor("#40c0ff");
const QColor COLOR_Magenta = Qt::magenta;
const QColor COLOR_Blue = Qt::blue;
const QColor COLOR_LightBlue = QColor("light blue");
const QColor COLOR_Gray = Qt::gray;
const QColor COLOR_LightGray = Qt::lightGray;
const QColor COLOR_DarkGray = Qt::darkGray;
const QColor COLOR_Cyan = Qt::cyan;
const QColor COLOR_DarkCyan = Qt::darkCyan;
const QColor COLOR_DarkBlue = Qt::darkBlue;
const QColor COLOR_DarkMagenta = Qt::darkMagenta;
const QColor COLOR_Gold = QColor("gold");
const QColor COLOR_White = Qt::white;
const QColor COLOR_Red = Qt::red;
const QColor COLOR_Pink = QColor("pink");
const QColor COLOR_DarkRed = Qt::darkRed;
const QColor COLOR_Yellow = Qt::yellow;
const QColor COLOR_DarkYellow = Qt::darkYellow;
const QColor COLOR_Orange = QColor("orange");
const QColor COLOR_Green = Qt::green;
const QColor COLOR_Brown = QColor("brown");

const QColor COLOR_Text = Qt::black;
const QColor COLOR_Outline = Qt::black;

const QColor COLOR_ALT_BG1 = QColor(0xd8, 0xff, 0xd8,
                                    0xff); // Alternating Background Color 1 (Event Flags)
const QColor COLOR_ALT_BG2 =
    COLOR_White;               // Alternating Background Color 2 (Event Flags)


/*! \brief Draw an outline of a rounded rectangle
    \param radius Radius of corner rounding
    \param lw Line Width
    \param color Color of drawn lines
    */
void LinedRoundedRectangle(int x, int y, int w, int h, int radius, int lw, QColor color);

/*! \brief Draws a filled rounded rectangle
    \param radius Radius of corner rounding
    \param color Color of entire rectangle
    */
void RoundedRectangle(int x, int y, int w, int h, int radius, const QColor color);

#ifdef BUILD_WITH_MSVC
// Visual C++ doesn't have either of these in it's maths header.. I'm not surprised at Microsofts maths abilities..
const double M_PI = 3.141592653589793;

double round(double number);
#endif

#endif // GLCOMMON_H
