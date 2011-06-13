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
#ifndef __TEXTURE_GLYPH_H__
#define __TEXTURE_GLYPH_H__

#include <wchar.h>
#include "text-markup.h"
#include "vertex-buffer.h"

typedef struct {
    wchar_t   charcode;
    float     kerning;
} KerningPair;

typedef struct {
    float x,y;
} Pen;

typedef struct {
    int x, y, z;
    float u, v;
    float r, g, b, a;
} TextureGlyphVertex;


class TextureGlyph {
public:
    TextureGlyph();
    ~TextureGlyph();
    void Render(TextMarkup * markup, Pen * pen );
    void AddToVertexBuffer( VertexBuffer * buffer, const TextMarkup * markup, Pen * pen );
    float GetKerning(wchar_t charcode);

    wchar_t       m_charcode;
    int           m_width, m_height;
    int           m_offset_x, m_offset_y;
    int           m_advance_x, m_advance_y;
    float         m_u0, m_v0, m_u1, m_v1;
    KerningPair * m_kerning;
    size_t        m_kerning_count;
};


#endif /* __TEXTURE_GLYPH_H__ */
