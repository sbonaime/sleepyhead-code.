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
 * =========================================================================

   This source is based on the article by Jukka Jylänki :
   "A Thousand Ways to Pack the Bin - A Practical Approach to
    Two-Dimensional Rectangle Bin Packing", February 27, 2010.

   More precisely, this is an implementation of the Skyline Bottom-Left
   algorithm based on C++ sources provided by Jukka Jylänki at:
   http://clb.demon.fi/files/RectangleBinPack/

   ========================================================================= */
#ifndef __TEXTURE_ATLAS_H__
#define __TEXTURE_ATLAS_H__

#include <wx/gdicmn.h>
#include <vector>

using namespace std;

struct Node {
    Node(int _x,int _y,int _width) { x=_x; y=_y; width=_width; };
    int x, y, width;
};

class TextureAtlas {
public:
    TextureAtlas(int width, int height);
    ~TextureAtlas();
    void Upload();
    wxRect GetRegion(int width, int height);
    void SetRegion(int x, int y, int width, int height, unsigned char *data, int stride);

    int m_width, m_height, m_used;

    int Fit(int index, int width, int height);
    void Merge();

    vector<Node *> m_nodes;
    unsigned int m_texid;
    unsigned char *m_data;
};



#endif /* __TEXTURE_ATLAS_H__ */
