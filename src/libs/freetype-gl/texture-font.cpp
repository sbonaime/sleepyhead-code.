/* =========================================================================
 * Freetype GL - An ansi C OpenGL Freetype engine
 * Platform:    Any
 * API version: 1.0
 * WWW:         http://www.loria.fr/~rougier/freetype-gl
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 * ========================================================================= */
#include <ft2build.h>
#include FT_FREETYPE_H
#include <wx/log.h>
//#include <stdint.h>
//#include <stdlib.h>
//#include <stdio.h>
//#include <assert.h>
#include <math.h>
#include "texture-font.h"

#undef __FTERRORS_H__
#define FT_ERRORDEF( e, v, s )  { e, s },
#define FT_ERROR_START_LIST     {
#define FT_ERROR_END_LIST       { 0, 0 } };
const struct {
    int          code;
    const char*  message;
} FT_Errors[] =
#include FT_ERRORS_H

TextureFont::TextureFont(TextureAtlas * atlas,const wxString & filename, const float size)
:m_atlas(atlas),m_filename(filename),m_size(size),m_gamma(1.5)
{
}

TextureFont::~TextureFont()
{
    for (vector<TextureGlyph *>::iterator g=m_glyphs.begin();g!=m_glyphs.end();g++) {
        delete *g;
    }
}

void TextureFont::GenerateKerning()
{
    size_t i, j, k, count;
    FT_Library   library;
    FT_Face      face;
    FT_UInt      glyph_index, prev_index;
    TextureGlyph *glyph, *prev_glyph;
    FT_Vector    kerning;

    if (!LoadFace(&library, m_filename, m_size, &face)) {
        return;
    }

    for( i=0; i<m_glyphs.size(); ++i ) {
        glyph = m_glyphs[i];
        if (glyph->m_kerning) {
            free(glyph->m_kerning);
            glyph->m_kerning = 0;
            glyph->m_kerning_count = 0;
        }

        // Count how many kerning pairs we need
        count = 0;
        glyph_index = FT_Get_Char_Index(face, glyph->m_charcode);
        for (j=0; j<m_glyphs.size(); ++j) {
            prev_glyph = m_glyphs[i];
            prev_index = FT_Get_Char_Index(face, prev_glyph->m_charcode);
            FT_Get_Kerning(face, prev_index, glyph_index, FT_KERNING_UNSCALED, &kerning);
            if (kerning.x != 0.0) {
                count++;
            }
        }

        // No kerning necessary
        if (!count) {
            continue;
        }
        // Store kerning pairs
        glyph->m_kerning = (KerningPair *) malloc(count * sizeof(KerningPair));
        glyph->m_kerning_count = count;
        k = 0;
        for (j=0; j<m_glyphs.size(); ++j) {
            prev_glyph = m_glyphs[i];
            prev_index = FT_Get_Char_Index(face, prev_glyph->m_charcode);
            FT_Get_Kerning(face, prev_index, glyph_index, FT_KERNING_UNSCALED, &kerning);
            if (kerning.x != 0.0) {
                glyph->m_kerning[k].charcode = prev_glyph->m_charcode;
                glyph->m_kerning[k].kerning = kerning.x / 64.0f;
                ++k;
            }
        }
    }

    FT_Done_FreeType( library );
}



// -------------------------------------------------------------------------
int TextureFont::CacheGlyphs(wchar_t * charcodes)
{
    size_t i, x, y, width, height;
    FT_Library    library;
    FT_Error      error;
    FT_Face       face;
    FT_GlyphSlot  slot;
    FT_UInt       glyph_index;
    TextureGlyph *glyph;
    wxRect  region;
    unsigned char c;
    size_t        missed = 0;
    width  = m_atlas->m_width;
    height = m_atlas->m_height;

    if (!LoadFace( &library, m_filename, m_size, &face)){
        return wcslen(charcodes);
    }

    // Load each glyph
    for (i=0; i<wcslen(charcodes); ++i) {
        glyph_index = FT_Get_Char_Index(face, charcodes[i]);
        error = FT_Load_Glyph( face, glyph_index, FT_LOAD_RENDER | FT_LOAD_TARGET_LIGHT );
        if (error) {
            wxLogError(wxString::Format(wxT("FT_Error (line%d, code 0x%02x) : "),__LINE__,FT_Errors[error].code)+wxString(FT_Errors[error].message,wxConvUTF8));
            FT_Done_FreeType(library);
            return wcslen(charcodes)-i;
        }
        slot = face->glyph;

        // Gamma correction (sort of)
        for( x=0; x<slot->bitmap.width; ++x ) {
            for( y=0; y<slot->bitmap.rows; ++y ){
                c = *(unsigned char *)(slot->bitmap.buffer + y*slot->bitmap.pitch + x );
                c = (unsigned char) ( pow(float(c/255.0), float(m_gamma)) * 255);
                *(unsigned char *)(slot->bitmap.buffer + y*slot->bitmap.pitch + x ) = c;
            }
        }
        region = m_atlas->GetRegion(slot->bitmap.width, slot->bitmap.rows);
        if (region.x < 0) {
            missed++;
            continue;
        }
        m_atlas->SetRegion(region.x, region.y, slot->bitmap.width, slot->bitmap.rows, slot->bitmap.buffer, slot->bitmap.pitch );

        glyph = new TextureGlyph();
        glyph->m_charcode = charcodes[i];
        glyph->m_kerning  = 0;
        glyph->m_width    = slot->bitmap.width;
        glyph->m_height   = slot->bitmap.rows;
        glyph->m_offset_x = slot->bitmap_left;
        glyph->m_offset_y = slot->bitmap_top;
        glyph->m_u0       = region.x/(float)width;
        glyph->m_v0       = region.y/(float)height;
        glyph->m_u1       = (region.x + glyph->m_width)/(float)width;
        glyph->m_v1       = (region.y + glyph->m_height)/(float)height;

        // Discard hinting to get advance
        FT_Load_Glyph(face, glyph_index, FT_LOAD_RENDER | FT_LOAD_NO_HINTING);
        slot = face->glyph;
        glyph->m_advance_x    = slot->advance.x/64.0;
        glyph->m_advance_y    = slot->advance.y/64.0;

        m_glyphs.push_back(glyph);
    }
    FT_Done_FreeType( library );
    m_atlas->Upload();
    GenerateKerning();
    return missed;
}

// -------------------------------------------------------------------------
TextureGlyph * TextureFont::GetGlyph(wchar_t charcode)
{
    size_t i;
    static wchar_t *buffer = 0;
    TextureGlyph *glyph;

    assert(m_atlas);

    // Check if charcode has been already loaded
    for (i=0; i<m_glyphs.size(); ++i) {
        glyph = m_glyphs[i];
        if (glyph->m_charcode == charcode) {
            return glyph;
        }
    }

    // If not, load it
    if (!buffer) {
        buffer = (wchar_t *)calloc( 2, sizeof(wchar_t));
    }
    buffer[0] = charcode;

    if (CacheGlyphs(buffer )==0) {
        return *(m_glyphs.rbegin());
    }
    return NULL;
}



// -------------------------------------------------------------------------
int TextureFont::LoadFace(FT_Library * library, const wxString & filename, const float size, FT_Face * face)
{
    size_t hres = 1;
    FT_Error error;
    FT_Matrix matrix = { (int)((1.0/hres) * 0x10000L),
                         (int)((0.0)      * 0x10000L),
                         (int)((0.0)      * 0x10000L),
                         (int)((1.0)      * 0x10000L) };

    // Initialize library
    error = FT_Init_FreeType( library );
    if (error)
    {
        wxLogError(wxString::Format(wxT("FT_Error (code 0x%02x) : "),FT_Errors[error].code)+wxString(FT_Errors[error].message,wxConvUTF8));
        return 0;
    }

    // Load face
    error = FT_New_Face( *library, filename.mb_str(), 0, face );
    if (error) {
        wxLogError(wxString::Format(wxT("FT_Error (line%d, code 0x%02x) : "),__LINE__,FT_Errors[error].code)+wxString(FT_Errors[error].message,wxConvUTF8));
        FT_Done_FreeType(*library);
        return 0;
    }

    // Select charmap
    error = FT_Select_Charmap( *face, FT_ENCODING_UNICODE );
    if (error) {
        wxLogError(wxString::Format(wxT("FT_Error (line%d, code 0x%02x) : "),__LINE__,FT_Errors[error].code)+wxString(FT_Errors[error].message,wxConvUTF8));
        FT_Done_FreeType( *library );
        return 0;
    }

    // Set char size
    error=FT_Set_Char_Size( *face, size*64, 0, 72*hres, 72 );
    // error = FT_Set_Char_Size( *face, size*64, 0, 72, 72 );
    if (error) {
        wxLogError(wxString::Format(wxT("FT_Error (line%d, code 0x%02x) : "),__LINE__,FT_Errors[error].code)+wxString(FT_Errors[error].message,wxConvUTF8));
        FT_Done_FreeType( *library );
        return 0;
    }

    // Set transform matrix
    FT_Set_Transform(*face, &matrix, NULL);

    return 1;
}

