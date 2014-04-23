/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 *
 * Copyright (c) 2011-2014 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#ifndef graphs_gvertexbuffer_h
#define graphs_gvertexbuffer_h

#include <QtGlobal>
#include <QGLWidget>

#include "Graphs/glcommon.h"

typedef quint32 RGBA;

#ifdef BUILD_WITH_MSVC
__declspec(align(1))
#endif
struct gVertex {
    gVertex(GLshort _x, GLshort _y, GLuint _c) { x = _x; y = _y; color = _c; }
    GLshort x;
    GLshort y;
    RGBA color;
}
#ifndef BUILD_WITH_MSVC
__attribute__((packed))
#endif
;

class gVertexBuffer
{
  public:
    gVertexBuffer(int max = 2048, int type = GL_LINES)
        : m_max(max), m_type(type), m_cnt(0), m_size(1),
          m_scissor(false), m_antialias(false), m_forceantialias(false), m_stippled(false),
          buffer(nullptr),
          s_x(0), s_y(0), s_width(0), s_height(0),
          m_color(0),
          m_stipple(0xffff),
          m_blendfunc1(GL_SRC_ALPHA),
          m_blendfunc2(GL_ONE_MINUS_SRC_ALPHA)
    {
        // FIXME: sstangl: Really should not allocate in constructor.
        buffer = (gVertex *)calloc(max, sizeof(gVertex));
    }

    ~gVertexBuffer() {
        if (buffer) {
            free(buffer);
        }
    }

    void add(GLshort x1, GLshort y1, RGBA color);
    void add(GLshort x1, GLshort y1, GLshort x2, GLshort y2, RGBA color);
    void add(GLshort x1, GLshort y1, GLshort x2, GLshort y2,
             GLshort x3, GLshort y3, GLshort x4, GLshort y4, RGBA color);
    void add(GLshort x1, GLshort y1, GLshort x2, GLshort y2,
             GLshort x3, GLshort y3, GLshort x4, GLshort y4, RGBA color, RGBA color2);

    void add(GLshort x1, GLshort y1);
    void add(GLshort x1, GLshort y1, GLshort x2, GLshort y2);
    void add(GLshort x1, GLshort y1, GLshort x2, GLshort y2,
             GLshort x3, GLshort y3, GLshort x4, GLshort y4);

    void unsafe_add(GLshort x1, GLshort y1);
    void unsafe_add(GLshort x1, GLshort y1, GLshort x2, GLshort y2);
    void unsafe_add(GLshort x1, GLshort y1, GLshort x2, GLshort y2,
                    GLshort x3, GLshort y3, GLshort x4, GLshort y4);

    void draw();

    void scissor(GLshort x, GLshort y, GLshort width, GLshort height) {
        s_x = x;
        s_y = y;
        s_width = width;
        s_height = height;
        m_scissor = true;
    }

    int Max() const { return m_max; }
    int cnt() const { return m_cnt; }
    GLuint type() const { return m_type; }
    float size() const { return m_size; }
    bool full() const { return m_cnt >= m_max; }

    void reset() { m_cnt = 0; }
    void forceAntiAlias(bool b) { m_forceantialias = b; }
    void setSize(float f) { m_size = f; }
    void setAntiAlias(bool b) { m_antialias = b; }
    void setStipple(GLshort stipple) { m_stipple = stipple; }
    void setStippleOn(bool b) { m_stippled = b; }
    void setBlendFunc(GLuint b1, GLuint b2) { m_blendfunc1 = b1; m_blendfunc2 = b2; }
    void setColor(QColor col);

  protected:
    //! \brief Maximum number of gVertex points contained in buffer
    int m_max;
    //! \brief Indicates type of GL vertex information (GL_LINES, GL_QUADS, etc)
    GLuint m_type;
    //! \brief Count of Vertex points used this draw cycle.
    int m_cnt;
    //! \brief Line/Point thickness
    float m_size;

    bool m_scissor;
    bool m_antialias;
    bool m_forceantialias;
    bool m_stippled;

    //! \brief Contains list of Vertex & Color points
    gVertex *buffer;
    //! \brief GL Scissor parameters
    GLshort s_x, s_y, s_width, s_height;
    //! \brief Current drawing color
    GLuint m_color;
    //! \brief Stipple bitfield
    GLshort m_stipple;
    //! \brief Source GL Blend Function
    GLuint m_blendfunc1;
    //! \brief Destination GL Blend Function
    GLuint m_blendfunc2;
};

#endif // graphs_gvertexbuffer_h
