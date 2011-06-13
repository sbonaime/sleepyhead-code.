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


//#include <fontconfig/fontconfig.h>
#include <wx/log.h>
//#include <assert.h>
#include <stdio.h>
//#include <wchar.h>
#include "font-manager.h"


wchar_t *
wcsdup( const wchar_t *string )
{
    wchar_t * result;
    assert( string );
    result = (wchar_t *) malloc( (wcslen(string) + 1) * sizeof(wchar_t) );
    wcscpy( result, string );
    return result;
}

FontManager *manager_instance=NULL;

FontManager::FontManager()
{
    m_atlas = new TextureAtlas(512, 512);
    m_fonts.clear();
    m_cache = wcsdup( L" " );
/*
        self->cache = wcsdup( L" !\"#$%&'()*+,-./0123456789:;<=>?"
                              L"@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_"
                              L"`abcdefghijklmnopqrstuvwxyz{|}~" );
*/
}

FontManager::~FontManager()
{
    for (vector<TextureFont *>::iterator f=m_fonts.begin();f!=m_fonts.end();f++) {
        delete *f;
    }
    delete m_atlas;

    if (m_cache) {
        free(m_cache);
    }
}

TextureFont * FontManager::GetFromFilename(const wxString & filename, const float size)
{
    size_t i;
    TextureFont *font;

    for (i=0; i<m_fonts.size();++i ) {
        font = m_fonts[i];
        if (((font->m_filename == filename)) && (font->m_size == size)) {
            return font;
        }
    }
    font = new TextureFont(m_atlas,filename, size);

    if (font) {
        font->CacheGlyphs(m_cache);
        m_fonts.push_back(font);
        return font;
    }
    return NULL;
}


TextureFont * FontManager::GetFromDescription(const wxString & family, const float size, const int bold, const int italic)
{
    TextureFont *font;
    wxString filename=MatchDescription(family, size, bold, italic);

    if (filename.IsEmpty()) {
        return NULL;
    }
    font = GetFromFilename(filename, size);
    return font;
}

TextureFont * FontManager::GetFromMarkup(const TextMarkup *markup)
{
    assert( markup );

    return GetFromDescription(markup->family, markup->size, markup->bold, markup->italic );
}


const wxString & FontManager::MatchDescription(const wxString & family, const float size, const int bold, const int italic)
{
    static wxString filename=wxEmptyString;
    /*int weight = FC_WEIGHT_REGULAR;
    int slant = FC_SLANT_ROMAN;
    if (bold) {
        weight = FC_WEIGHT_BOLD;
    }
    if (italic) {
        slant = FC_SLANT_ITALIC;
    }
    FcInit();
    FcPattern *pattern = FcPatternCreate();
    FcPatternAddDouble(pattern, FC_SIZE, size);
    FcPatternAddInteger(pattern, FC_WEIGHT, weight);
    FcPatternAddInteger(pattern, FC_SLANT, slant);

    char *t=strdup(family.mb_str());
    FcPatternAddString(pattern, FC_FAMILY, (FcChar8*)t);
    free(t);
    FcConfigSubstitute(0, pattern, FcMatchPattern);
    FcDefaultSubstitute(pattern);
    FcResult result;
    FcPattern *match = FcFontMatch(0, pattern, &result);
    FcPatternDestroy(pattern);

    if (!match ) {
        wxLogError(wxT("fontconfig error: Could not match family '")+family+wxT("'"));
        return filename;
    } else {
        FcValue value;
        FcResult result = FcPatternGet(match, FC_FILE, 0, &value);
        if (result) {
            wxLogError(wxT("fontconfig error: Could not match family '")+family+wxT("'"));
        } else {
            filename = wxString((char *)(value.u.s),wxConvUTF8);
        }
    }
    FcPatternDestroy(match); */
    return filename;
}

const wchar_t * FontManager::GetCache()
{
    return m_cache;
}

void FontManager::SetCache(const wchar_t * cache)
{
    assert( cache );

    if (m_cache) {
        free(m_cache);
    }
    m_cache=wcsdup(cache);
}
