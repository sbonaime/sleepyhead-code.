/* =========================================================================
 * Freetype GL - A C OpenGL Freetype engine
 * Platform:    Any
 * API version: 1.0
 * WWW:         http://code.google.com/p/freetype-gl/
 * C++ Conversion by Mark Watkins
 * -------------------------------------------------------------------------
 * Copyright (c) 2011 Nicolas P. Rougier <Nicolas.Rougier@inria.fr>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 * ========================================================================= */
#if defined(__APPLE__)
    #include <Glut/glut.h>
#else
    #include <GL/gl.h>
#endif
//#include <assert.h>
//#include <stdlib.h>
#include "texture-glyph.h"



TextureGlyph::TextureGlyph()
:m_width(0),m_height(0),m_offset_x(0),m_offset_y(0),m_advance_x(0),m_advance_y(0)
{
    m_u0=m_v0=m_u1=m_v1 = 0.0;
}

TextureGlyph::~TextureGlyph()
{
}

void TextureGlyph::Render(TextMarkup *markup, Pen *pen)
{
    int x  = pen->x + m_offset_x;
    int y  = pen->y + m_offset_y + markup->rise;
    int w  = m_width;
    int h  = m_height;

    float u0 = m_u0;
    float v0 = m_v0;
    float u1 = m_u1;
    float v1 = m_v1;

    glBegin( GL_TRIANGLES );
    {
        glTexCoord2f(u0, v0); glVertex2i(x, y);
        glTexCoord2f(u0, v1); glVertex2i(x, y-h);
        glTexCoord2f(u1, v1); glVertex2i(x+w, y-h);

        glTexCoord2f(u0, v0); glVertex2i(x, y);
        glTexCoord2f(u1, v1); glVertex2i(x+w, y-h);
        glTexCoord2f(u1, v0); glVertex2i(x+w, y);
    }
    glEnd();

    pen->x += m_advance_x + markup->spacing;
    pen->y += m_advance_y;
}


void TextureGlyph::AddToVertexBuffer(VertexBuffer *buffer, const TextMarkup *markup, Pen *pen)
{
    size_t i;
    int x0  = pen->x + m_offset_x;
    int y0  = pen->y + m_offset_y + markup->rise;
    int x1  = x0 + m_width;
    int y1  = y0 - m_height;
    float u0 = m_u0;
    float v0 = m_v0;
    float u1 = m_u1;
    float v1 = m_v1;

    GLuint index = buffer->vertices->size;
    GLuint indices[] = {index, index+1, index+2,
                        index, index+2, index+3};

    TextureGlyphVertex vertices[] = { { x0,y0,0,  u0,v0,  0,0,0,1 },
                                      { x0,y1,0,  u0,v1,  0,0,0,1 },
                                      { x1,y1,0,  u1,v1,  0,0,0,1 },
                                      { x1,y0,0,  u1,v0,  0,0,0,1 } };
    if (markup) {
        for (i=0; i<4; ++i) {
            vertices[i].r = markup->foreground_color.Red()/256.0;
            vertices[i].g = markup->foreground_color.Green()/256.0;
            vertices[i].b = markup->foreground_color.Blue()/256.0;
            vertices[i].a = markup->foreground_color.Alpha()/256.0;
        }
    }
    buffer->PushBackIndices(indices, 6);
    buffer->PushBackVertices(vertices, 4);

    pen->x += m_advance_x + markup->spacing;
    pen->y += m_advance_y;
}

float TextureGlyph::GetKerning(wchar_t charcode)
{
    size_t i;

    if (!m_kerning ) {
        return 0;
    }

    for (i=0; i<m_kerning_count; ++i)
    {
        if (m_kerning[i].charcode == charcode) {
            return m_kerning[i].kerning;
        }
    }
    return 0;
}
