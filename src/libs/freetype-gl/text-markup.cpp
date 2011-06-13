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
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "font-manager.h"
#include "text-markup.h"

TextMarkup::TextMarkup()
{
    wxColor black(0,0,0,255);
    wxColor white(255,255,255,255);


    family = wxString("monotype");
    italic = 0;
    bold   = 0;
    size   = 16;
    rise   = 0;
    spacing= 0;

    foreground_color    = black;
    background_color    = white;
    underline           = 0;
    underline_color     = black;
    overline            = 0;
    overline_color      = black;
    strikethrough       = 0;
    strikethrough_color = black;
}

TextMarkup::~TextMarkup()
{
}


