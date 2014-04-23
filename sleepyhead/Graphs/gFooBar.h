/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 *
 * gFooBar Header
 *
 * Copyright (c) 2011-2014 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#ifndef GFOOBAR_H
#define GFOOBAR_H

#include "Graphs/gVertexBuffer.h"
#include "Graphs/layer.h"

/*! \class gShadowArea
    \brief Displays a Shadow for all graph areas not highlighted (used in Event Flags)
    */
class gShadowArea: public Layer
{
  public:
    gShadowArea(QColor shadow_color = QColor(40, 40, 40, 40), QColor line_color = Qt::blue);
    virtual ~gShadowArea();
    virtual void paint(gGraph &w, int left, int top, int width, int height);
  protected:
    QColor m_shadow_color;
    QColor m_line_color;
    gVertexBuffer *quads;
    gVertexBuffer *lines;
};

/*! \class gFooBar
    \brief Was a kind of scrollbar thingy that used to be used for representing the overall graph areas.
    Currently Unused and empty.
    */
class gFooBar: public Layer
{
  public:
    gFooBar(int offset = 10, QColor handle_color = QColor("orange"),
            QColor line_color = QColor("dark grey"));
    virtual ~gFooBar();
    virtual void paint(gGraph &w, int left, int top, int width, int height);
    static const int Margin = 15;
  protected:
    int m_offset;
    QColor m_handle_color;
    QColor m_line_color;
};

#endif // GFOOBAR_H
