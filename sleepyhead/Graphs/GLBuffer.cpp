/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 *
 * Copyright (c) 2011-2014 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#include "Graphs/GLBuffer.h"

#include <QDebug>

#include "SleepLib/profiles.h"

extern int lines_drawn_this_frame;
extern int quads_drawn_this_frame;

GLFloatBuffer::GLFloatBuffer(int max, int type, bool stippled)
    : GLBuffer(max, type, stippled)
{
    buffer = new GLfloat[max + 8];
    colors = new GLubyte[max * 4 + (8 * 4)];
}

GLFloatBuffer::~GLFloatBuffer()
{
    if (colors) {
        delete [] colors;
    }

    if (buffer) {
        delete [] buffer;
    }
}

void GLFloatBuffer::add(GLfloat x, GLfloat y, QColor &color)
{
#ifdef ENABLE_THREADED_DRAWING
    mutex.lock();
#endif
    if (m_cnt < m_max + 2) {
        buffer[m_cnt++] = x;
        buffer[m_cnt++] = y;
        colors[m_colcnt++] = color.red();
        colors[m_colcnt++] = color.green();
        colors[m_colcnt++] = color.blue();
        colors[m_colcnt++] = color.alpha();

    } else {
        qDebug() << "GLBuffer overflow";
    }
#ifdef ENABLE_THREADED_DRAWING
    mutex.unlock();
#endif
}

void GLFloatBuffer::add(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2, QColor &color)
{
    if (m_cnt < m_max + 4) {
        qDebug() << "GLFloatBuffer overflow";
        return;
    }

#ifdef ENABLE_THREADED_DRAWING
    mutex.lock();
#endif
    buffer[m_cnt++] = x1;
    buffer[m_cnt++] = y1;
    buffer[m_cnt++] = x2;
    buffer[m_cnt++] = y2;
    colors[m_colcnt++] = color.red();
    colors[m_colcnt++] = color.green();
    colors[m_colcnt++] = color.blue();
    colors[m_colcnt++] = color.alpha();
    colors[m_colcnt++] = color.red();
    colors[m_colcnt++] = color.green();
    colors[m_colcnt++] = color.blue();
    colors[m_colcnt++] = color.alpha();
#ifdef ENABLE_THREADED_DRAWING
    mutex.unlock();
#endif
}

void GLFloatBuffer::add(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2, GLfloat x3, GLfloat y3,
                        GLfloat x4, GLfloat y4, QColor &color) // add with vertex colors
{
    if (m_cnt >= m_max + 8) {
        qDebug() << "GLFloatBuffer overflow";
        return;
    }

#ifdef ENABLE_THREADED_DRAWING
    mutex.lock();
#endif
    buffer[m_cnt++] = x1;
    buffer[m_cnt++] = y1;
    buffer[m_cnt++] = x2;
    buffer[m_cnt++] = y2;
    buffer[m_cnt++] = x3;
    buffer[m_cnt++] = y3;
    buffer[m_cnt++] = x4;
    buffer[m_cnt++] = y4;
    colors[m_colcnt++] = color.red();
    colors[m_colcnt++] = color.green();
    colors[m_colcnt++] = color.blue();
    colors[m_colcnt++] = color.alpha();
    colors[m_colcnt++] = color.red();
    colors[m_colcnt++] = color.green();
    colors[m_colcnt++] = color.blue();
    colors[m_colcnt++] = color.alpha();
    colors[m_colcnt++] = color.red();
    colors[m_colcnt++] = color.green();
    colors[m_colcnt++] = color.blue();
    colors[m_colcnt++] = color.alpha();
    colors[m_colcnt++] = color.red();
    colors[m_colcnt++] = color.green();
    colors[m_colcnt++] = color.blue();
    colors[m_colcnt++] = color.alpha();
#ifdef ENABLE_THREADED_DRAWING
    mutex.unlock();
#endif
}

void GLFloatBuffer::quadGrLR(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2, GLfloat x3,
                             GLfloat y3, GLfloat x4, GLfloat y4, QColor &color, QColor &color2)  // add with vertex colors
{
    if (m_cnt >= m_max + 8) {
        qDebug() << "GLFloatBuffer overflow";
        return;
    }

#ifdef ENABLE_THREADED_DRAWING
    mutex.lock();
#endif
    buffer[m_cnt++] = x1;
    buffer[m_cnt++] = y1;
    buffer[m_cnt++] = x2;
    buffer[m_cnt++] = y2;
    buffer[m_cnt++] = x3;
    buffer[m_cnt++] = y3;
    buffer[m_cnt++] = x4;
    buffer[m_cnt++] = y4;
    colors[m_colcnt++] = color.red();
    colors[m_colcnt++] = color.green();
    colors[m_colcnt++] = color.blue();
    colors[m_colcnt++] = color.alpha();
    colors[m_colcnt++] = color.red();
    colors[m_colcnt++] = color.green();
    colors[m_colcnt++] = color.blue();
    colors[m_colcnt++] = color.alpha();

    colors[m_colcnt++] = color2.red();
    colors[m_colcnt++] = color2.green();
    colors[m_colcnt++] = color2.blue();
    colors[m_colcnt++] = color2.alpha();
    colors[m_colcnt++] = color2.red();
    colors[m_colcnt++] = color2.green();
    colors[m_colcnt++] = color2.blue();
    colors[m_colcnt++] = color2.alpha();
#ifdef ENABLE_THREADED_DRAWING
    mutex.unlock();
#endif
}
void GLFloatBuffer::quadGrTB(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2, GLfloat x3,
                             GLfloat y3, GLfloat x4, GLfloat y4, QColor &color, QColor &color2)
{
    if (m_cnt >= m_max + 8) {
        qDebug() << "GLFloatBuffer overflow";
        return;
    }

#ifdef ENABLE_THREADED_DRAWING
    mutex.lock();
#endif
    buffer[m_cnt++] = x1;
    buffer[m_cnt++] = y1;
    buffer[m_cnt++] = x3;
    buffer[m_cnt++] = y3;
    buffer[m_cnt++] = x2;
    buffer[m_cnt++] = y2;
    buffer[m_cnt++] = x4;
    buffer[m_cnt++] = y4;
    colors[m_colcnt++] = color.red();
    colors[m_colcnt++] = color.green();
    colors[m_colcnt++] = color.blue();
    colors[m_colcnt++] = color.alpha();
    colors[m_colcnt++] = color.red();
    colors[m_colcnt++] = color.green();
    colors[m_colcnt++] = color.blue();
    colors[m_colcnt++] = color.alpha();

    colors[m_colcnt++] = color2.red();
    colors[m_colcnt++] = color2.green();
    colors[m_colcnt++] = color2.blue();
    colors[m_colcnt++] = color2.alpha();
    colors[m_colcnt++] = color2.red();
    colors[m_colcnt++] = color2.green();
    colors[m_colcnt++] = color2.blue();
    colors[m_colcnt++] = color2.alpha();
#ifdef ENABLE_THREADED_DRAWING
    mutex.unlock();
#endif
}

void GLFloatBuffer::draw()
{
    if (m_cnt <= 0) {
        return;
    }

    bool antialias = m_forceantialias || (PROFILE.appearance->antiAliasing() && m_antialias);
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
        glLineWidth(size);

        if (m_stippled) {
            glLineStipple(1, 0xAAAA);
            glEnable(GL_LINE_STIPPLE);
        }

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
        glScissor(s1, s2, s3, s4);
        glEnable(GL_SCISSOR_TEST);
    }

    glVertexPointer(2, GL_FLOAT, 0, buffer);

    glColorPointer(4, GL_UNSIGNED_BYTE, 0, colors);

    //glColor4ub(200,128,95,200);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);


    glDrawArrays(m_type, 0, m_cnt >> 1);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);

    //qDebug() << "I Drew" << m_cnt << "vertices";
    m_cnt = 0;
    m_colcnt = 0;

    if (m_scissor) {
        glDisable(GL_SCISSOR_TEST);
        m_scissor = false;
    }

    if (m_type == GL_POLYGON) {
        glPolygonMode(GL_BACK, GL_FILL);
    }

    if (antialias) {
        if (m_type == GL_LINES || m_type == GL_LINE_LOOP) {
            if (m_stippled) { glDisable(GL_LINE_STIPPLE); }

            glDisable(GL_LINE_SMOOTH);
        } else if (m_type == GL_POLYGON) {
            glDisable(GL_POLYGON_SMOOTH);
        }

        glDisable(GL_BLEND);
    }
}
