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
#ifndef __TEXT_MARKUP_H__
#define __TEXT_MARKUP_H__

#include <wx/colour.h>


class TextMarkup {
public:
    TextMarkup();
    ~TextMarkup();
    const wxString & GetFamily() { return family; };
    void SetFamily(wxString fam) { family=fam; };
    int GetItalic() { return italic; };
    void SetItalic(int i) { italic=i; };
    int GetBold() { return bold; };
    void SetBold(int i) { bold=i; };
    float GetSize() { return size; };
    void SetSize(float f) { size=f; };
    float GetRise() { return rise; };
    void SetRise(float f) { rise=f; };
    float GetSpacing() { return spacing; };
    void SetSpacing(float f) { spacing=f; };
    wxColor GetForegroundColor() { return foreground_color; };
    void SetForegroundColor(wxColor c) { foreground_color=c; };
    wxColor GetBackgroundColor() { return background_color; };
    void SetBackgroundColor(wxColor c) { background_color=c; };
    int GetOutline() { return outline; };
    void SetOutline(int i) { outline=i; };
    wxColor GetOutlineColor() { return outline_color; };
    void SetOutlineColor(wxColor c) { outline_color=c; };
    int GetUnderline() { return underline; };
    void SetUnderline(int i) { underline=i; };
    wxColor GetUnderlineColor() { return underline_color; };
    void SetUnderlineColor(wxColor c) { underline_color=c; };
    int GetOverline() { return overline; };
    void SetOverline(int i) { overline=i; };
    wxColor GetOverlineColor() { return overline_color; };
    void SetOverlineColor(wxColor c) { overline_color=c; };
    int GetStrikethrough() { return strikethrough; };
    void SetStrikethrough(int i) { strikethrough=i; };
    wxColor GetStrikethroughColor() { return strikethrough_color; };
    void SetStrikethroughColor(wxColor c) { strikethrough_color=c; };


    wxString     family;
    float         size;
    int           bold;
    int           italic;
    float         rise;
    float         spacing;
    wxColor       foreground_color;
    wxColor       background_color;
    int           outline;
    wxColor       outline_color;
    int           underline;
    wxColor       underline_color;
    int           overline;
    wxColor       overline_color;
    int           strikethrough;
    wxColor       strikethrough_color;
};

#endif /* __TEXT_MARKUP_H__ */
