// ============================================================================
// Freetype GL - An ansi C OpenGL Freetype engine
// Platform:    Any
// API version: 1.0
// WWW:         http://www.loria.fr/~rougier/freetype-gl
// C++ Conversion by Mark Watkins
// ----------------------------------------------------------------------------
// Copyright (c) 2011 Nicolas P. Rougier <Nicolas.Rougier@inria.fr>
//
// This program is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the
// Free Software Foundation, either version 3 of the License, or (at your
// option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
// Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program. If not, see <http://www.gnu.org/licenses/>.
// ============================================================================
#ifndef __TEXTURE_FONT_H__
#define __TEXTURE_FONT_H__
#include <ft2build.h>
#include FT_FREETYPE_H
#include "vector.h"
#include "texture-atlas.h"
#include "texture-glyph.h"


class TextureFont {
public:
    TextureFont(TextureAtlas *,const wxString & filename, const float size);
    virtual ~TextureFont();

    TextureGlyph * GetGlyph(wchar_t charcode);
    int CacheGlyphs(wchar_t * charcodes);
    int LoadFace(FT_Library *library, const wxString & filename, const float size, FT_Face * face);

    void GenerateKerning();

    vector <TextureGlyph *> m_glyphs;
    TextureAtlas * m_atlas;
    wxString       m_filename;
    int            m_bold;
    int            m_italic;
    float          m_size;
    float          m_gamma;
};


#endif /* __TEXTURE_FONT_H__ */

