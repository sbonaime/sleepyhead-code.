/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 *
 * Copyright (c) 2011-2014 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#include "Graphs/gVertexBuffer.h"

#include "SleepLib/profiles.h"

extern int lines_drawn_this_frame;
extern int quads_drawn_this_frame;

inline quint32 swaporder(quint32 color)
{
    return ((color & 0xFF00FF00) |
            ((color & 0xFF0000) >> 16) |
            ((color & 0xFF) << 16));
}

void gVertexBuffer::setColor(QColor col)
{
    m_color = swaporder(col.rgba());
}

void gVertexBuffer::draw()
{
    bool antialias = m_forceantialias || (PROFILE.appearance->antiAliasing() && m_antialias);

    if (m_stippled) { antialias = false; }

    float size = m_size;

    if (antialias) {
        glEnable(GL_BLEND);
        glBlendFunc(m_blendfunc1,  m_blendfunc2);

        if (m_type == GL_LINES || m_type == GL_LINE_LOOP) {
            glEnable(GL_LINE_SMOOTH);
            glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
            size += 0.5;
        } else if (m_type == GL_POLYGON) {
            glEnable(GL_POLYGON_SMOOTH);
            glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
        }
    }

    if (m_type == GL_LINES || m_type == GL_LINE_LOOP) {
        if (m_stippled) {
            glLineStipple(1, m_stipple);
            //size=1;
            glEnable(GL_LINE_STIPPLE);
        } else {
            //glLineStipple(1, 0xFFFF);
        }

        glLineWidth(size);

        lines_drawn_this_frame += m_cnt / 2;
    } else if (m_type == GL_POINTS) {
        glPointSize(size);
    } else if (m_type == GL_POLYGON) {
        glPolygonMode(GL_BACK, GL_FILL);
        lines_drawn_this_frame += m_cnt / 2;
    } else if (m_type == GL_QUADS) {
        quads_drawn_this_frame += m_cnt / 4;
    }

    if (m_scissor) {
        glScissor(s_x, s_y, s_width, s_height);
        glEnable(GL_SCISSOR_TEST);
    }

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glVertexPointer(2, GL_SHORT, 8, (GLvoid *)buffer);
    glColorPointer(4, GL_UNSIGNED_BYTE, 8, ((char *)buffer) + 4);

    glDrawArrays(m_type, 0, m_cnt);

    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    m_cnt = 0;

    if (m_scissor) {
        glDisable(GL_SCISSOR_TEST);
        m_scissor = false;
    }

    if (m_type == GL_POLYGON) {
        glPolygonMode(GL_BACK, GL_FILL);
    }

    if (m_type == GL_LINES || m_type == GL_LINE_LOOP) {
        if (m_stippled) {
            glDisable(GL_LINE_STIPPLE);
            glLineStipple(1, 0xFFFF);
        }
    }

    if (antialias) {
        if (m_type == GL_LINES || m_type == GL_LINE_LOOP) {
            glDisable(GL_LINE_SMOOTH);
        } else if (m_type == GL_POLYGON) {
            glDisable(GL_POLYGON_SMOOTH);
        }

        glDisable(GL_BLEND);
    }
}
void gVertexBuffer::add(GLshort x1, GLshort y1, RGBA color)
{
    if (m_cnt < m_max) {
        gVertex &v = buffer[m_cnt];

        v.color = swaporder(color);
        v.x = x1;
        v.y = y1;

        m_cnt++;
    }
}
void gVertexBuffer::add(GLshort x1, GLshort y1, GLshort x2, GLshort y2, RGBA color)
{
    if (m_cnt < (m_max - 1)) {
        gVertex *v = &buffer[m_cnt];

        v->x = x1;
        v->y = y1;
        v->color = swaporder(color);

        v++;
        v->x = x2;
        v->y = y2;
        v->color = swaporder(color);

        m_cnt += 2;
    }
}
void gVertexBuffer::add(GLshort x1, GLshort y1, GLshort x2, GLshort y2, GLshort x3, GLshort y3,
                        GLshort x4, GLshort y4, RGBA color)
{
    if (m_cnt < (m_max - 3)) {
        gVertex *v = &buffer[m_cnt];

        v->color = swaporder(color);
        v->x = x1;
        v->y = y1;
        v++;

        v->color = swaporder(color);
        v->x = x2;
        v->y = y2;

        v++;
        v->color = swaporder(color);
        v->x = x3;
        v->y = y3;

        v++;
        v->color = swaporder(color);
        v->x = x4;
        v->y = y4;

        m_cnt += 4;
    }
}

void gVertexBuffer::add(GLshort x1, GLshort y1, GLshort x2, GLshort y2, GLshort x3, GLshort y3,
                        GLshort x4, GLshort y4, RGBA color1, RGBA color2)
{
    if (m_cnt < (m_max - 3)) {
        gVertex *v = &buffer[m_cnt];

        v->color = swaporder(color1);
        v->x = x1;
        v->y = y1;
        v++;

        v->color = swaporder(color1);
        v->x = x2;
        v->y = y2;

        v++;
        v->color = swaporder(color2);
        v->x = x3;
        v->y = y3;

        v++;
        v->color = swaporder(color2);
        v->x = x4;
        v->y = y4;

        m_cnt += 4;
    }
}
void gVertexBuffer::unsafe_add(GLshort x1, GLshort y1)
{
    gVertex &v = buffer[m_cnt++];

    v.color = m_color;
    v.x = x1;
    v.y = y1;
}
void gVertexBuffer::add(GLshort x1, GLshort y1)
{
    if (m_cnt < m_max) {
        gVertex &v = buffer[m_cnt++];

        v.color = m_color;
        v.x = x1;
        v.y = y1;
    }
}
void gVertexBuffer::unsafe_add(GLshort x1, GLshort y1, GLshort x2, GLshort y2)
{
    gVertex *v = &buffer[m_cnt];

    v->x = x1;
    v->y = y1;
    v->color = m_color;

    v++;
    v->x = x2;
    v->y = y2;
    v->color = m_color;

    m_cnt += 2;
}

void gVertexBuffer::add(GLshort x1, GLshort y1, GLshort x2, GLshort y2)
{
    if (m_cnt < (m_max - 1)) {
        gVertex *v = &buffer[m_cnt];

        v->x = x1;
        v->y = y1;
        v->color = m_color;

        v++;
        v->x = x2;
        v->y = y2;
        v->color = m_color;

        m_cnt += 2;
    }
}
void gVertexBuffer::add(GLshort x1, GLshort y1, GLshort x2, GLshort y2, GLshort x3, GLshort y3,
                        GLshort x4, GLshort y4)
{
    if (m_cnt < (m_max - 3)) {
        gVertex *v = &buffer[m_cnt];

        v->color = m_color;
        v->x = x1;
        v->y = y1;
        v++;

        v->color = m_color;
        v->x = x2;
        v->y = y2;

        v++;
        v->color = m_color;
        v->x = x3;
        v->y = y3;

        v++;
        v->color = m_color;
        v->x = x4;
        v->y = y4;

        m_cnt += 4;
    }
}
void gVertexBuffer::unsafe_add(GLshort x1, GLshort y1, GLshort x2, GLshort y2, GLshort x3,
                               GLshort y3, GLshort x4, GLshort y4)
{
    gVertex *v = &buffer[m_cnt];

    v->color = m_color;
    v->x = x1;
    v->y = y1;
    v++;

    v->color = m_color;
    v->x = x2;
    v->y = y2;

    v++;
    v->color = m_color;
    v->x = x3;
    v->y = y3;

    v++;
    v->color = m_color;
    v->x = x4;
    v->y = y4;

    m_cnt += 4;
}
