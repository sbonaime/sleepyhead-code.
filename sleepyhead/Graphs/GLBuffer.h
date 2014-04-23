/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 *
 * Copyright (c) 2011-2014 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#ifndef graphs_glbuffer_h
#define graphs_glbuffer_h

#include <QColor>
#include <QGLWidget>
#include <QMutex>

/*! \class GLBuffer
    \brief Base Object to hold an OpenGL draw list
    */
class GLBuffer
{
  public:
    GLBuffer(int max = 2048, int type = GL_LINES, bool stippled = false)
        : m_max(max), m_type(type), m_cnt(0), m_colcnt(0), m_size(1),
          s1(0), s2(0), s3(0), s4(0),
          m_scissor(false),
          m_antialias(true),
          m_forceantialias(false),
          m_stippled(stippled),
          m_blendfunc1(GL_SRC_ALPHA),
          m_blendfunc2(GL_ONE_MINUS_SRC_ALPHA)
    { }
    virtual ~GLBuffer() {}

    void scissor(GLshort x1, GLshort y1, GLshort x2, GLshort y2) {
        s1 = x1;
        s2 = y1;
        s3 = x2;
        s4 = y2;
        m_scissor = true;
    }

    int Max() const { return m_max; }
    int cnt() const { return m_cnt; }
    bool full() const { return m_cnt >= m_max; }
    float size() const { return m_size; }
    int type() const { return m_type; }

    void reset() { m_cnt = 0; }
    void setSize(float f) { m_size = f; }
    void setAntiAlias(bool b) { m_antialias = b; }
    void forceAntiAlias(bool b) { m_forceantialias = b; }
    void setColor(QColor col) { m_color = col; }
    void setBlendFunc(GLuint b1, GLuint b2) { m_blendfunc1 = b1; m_blendfunc2 = b2; }

    virtual void draw() {}

  protected:
    int m_max;
    int m_type;     // type (GL_LINES, GL_QUADS, etc)
    int m_cnt;      // cnt
    int m_colcnt;
    QColor m_color;
    float m_size;
    int s1, s2, s3, s4;
    bool m_scissor;
    bool m_antialias;
    bool m_forceantialias;
    QMutex mutex;
    bool m_stippled;
    GLuint m_blendfunc1, m_blendfunc2;
};

/*! \class GLFloatBuffer
    \brief Holds an OpenGL draw list composed of 32bit GLfloat objects and vertex colors
    */
class GLFloatBuffer : public GLBuffer
{
  public:
    GLFloatBuffer(int max = 2048, int type = GL_LINES, bool stippled = false);
    virtual ~GLFloatBuffer();

    // Add with vertex color(s).
    void add(GLfloat x, GLfloat y, QColor &col);
    void add(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2, QColor &col);
    void add(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2,
             GLfloat x3, GLfloat y3, GLfloat x4, GLfloat y4, QColor &col);
    void quadGrTB(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2,
                  GLfloat x3, GLfloat y3, GLfloat x4, GLfloat y4, QColor &col, QColor &col2);
    void quadGrLR(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2,
                  GLfloat x3, GLfloat y3, GLfloat x4, GLfloat y4, QColor &col, QColor &col2);

    virtual void draw();

  protected:
    GLfloat *buffer;
    GLubyte *colors;
};

#endif // graphs_glbuffer_h
