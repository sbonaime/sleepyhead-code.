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
#ifndef __FONT_MANAGER_H__
#define __FONT_MANAGER_H__

#include <vector>
#include "text-markup.h"
#include "texture-font.h"
#include "texture-atlas.h"


class FontManager {
public:
    FontManager();
    ~FontManager();
    TextureFont * GetFromFilename(const wxString & filename, const float size);
    TextureFont * GetFromDescription(const wxString & family, const float size, const int bold, const int italic);
    TextureFont * GetFromMarkup(const TextMarkup * markup);
    const wxString & MatchDescription(const wxString & family, const float size, const int bold, const int italic);

    const wchar_t * GetCache();
    void SetCache(const wchar_t *cache);

    TextureAtlas * m_atlas;
    vector<TextureFont *> m_fonts;
    wchar_t *      m_cache;
};

#endif /* __FONT_MANAGER_H__ */

