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
    #include <GL/glut.h>
#endif
//#include <stdlib.h>
#include <limits.h>
#include "texture-atlas.h"

#define max(a,b) (a)>(b)?(a):(b)
#define min(a,b) (a)<(b)?(a):(b)


/* ========================================================================= */
/*                                                                           */
/* ========================================================================= */
TextureAtlas::TextureAtlas(int width, int height)
:m_used(0),m_width(width),m_height(height),m_texid(0)
{
    Node *node=new Node(0,0,m_width);
    m_nodes.push_back(node);
    m_data = (unsigned char *) calloc(m_width * m_height, sizeof(unsigned char));
    // Think I have to use calloc here..
    //data=new unsigned char [width*height];
}

/* ========================================================================= */
/*                                                                           */
/* ========================================================================= */
TextureAtlas::~TextureAtlas()
{
    if (m_texid) {
        glDeleteTextures(1, &m_texid);
    }

    // delete [] data;
    free(m_data);
    for (vector<Node *>::iterator n=m_nodes.begin();n!=m_nodes.end();n++) {
        delete *n;
    }
}


/* ========================================================================= */
/*                                                                           */
/* ========================================================================= */
void TextureAtlas::Upload()
{
    if (!m_texid) {
        glGenTextures( 1, &m_texid );
    }
    glBindTexture(GL_TEXTURE_2D, m_texid );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, m_width, m_height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, m_data );
}



/* ========================================================================= */
/*                                                                           */
/* ========================================================================= */
void TextureAtlas::SetRegion(int x, int y, int width, int height, unsigned char *data, int stride)
{
    assert( x < m_width);
    assert( (x + width) <= m_width);
    assert( y < m_height);
    assert( (y + height) <= m_height);
    size_t i;
    size_t charsize = sizeof(char);

    for (i=0; i<height; ++i) {
        memcpy( m_data + ((y+i) * m_width+x) * charsize,
                data+(i*stride) * charsize, width  * charsize );
    }
}



/* ========================================================================= */
/*                                                                           */
/* ========================================================================= */
int TextureAtlas::Fit(int index, int width, int height)
{
    Node *node = m_nodes[index];
    int x = node->x, y, width_left = width;
	size_t i = index;

	if ((x + width) > m_width) {
		return -1;
    }
	y = node->y;

	while (width_left > 0) {
        node = m_nodes[i];
		y = max( y, node->y );
		if ((y + height) > m_height) {
			return -1;
        }
		width_left -= node->width;
		++i;
	}
	return y;
}



/* ========================================================================= */
/*                                                                           */
/* ========================================================================= */
void TextureAtlas::Merge()
{
    Node *node, *next;
    int i;

	for (i=0; i<m_nodes.size()-1; ++i)
    {
        node = m_nodes[i];
        next = m_nodes[i+1];

		if (node->y == next->y) {
			node->width += next->width;
			int j=i+1;
			m_nodes.erase(m_nodes.begin()+(i+1));
			--i;
		}
    }
}



/* ========================================================================= */
/*                                                                           */
/* ========================================================================= */
wxRect TextureAtlas::GetRegion(int width, int height)
{
	int y, best_height, best_width, best_index;
    Node *node, *prev;
    wxRect region(0,0,width,height);

    size_t i;

    best_height = INT_MAX;
    best_index  = -1;
    best_width = INT_MAX;

	for (i=0; i<m_nodes.size(); ++i ) {
        y = Fit(i, width, height);
		if (y >= 0) {
            node = m_nodes[i];
			if ((y + height < best_height) || (y + height == best_height && node->width < best_width)) {
				best_height = y + height;
				best_index = i;
				best_width = node->width;
				region.x = node->x;
				region.y = y;
			}
        }
    }

	if (best_index == -1) {
        region.x = -1;
        region.y = -1;
        region.width = 0;
        region.height = 0;
        return region;
    }

    node = new Node(region.x,region.y+height,width);
    m_nodes.insert(m_nodes.begin()+best_index,node);

    for(i = best_index+1; i < m_nodes.size(); ++i) {
        node = m_nodes[i];
        prev = m_nodes[i-1];

        if (node->x < (prev->x + prev->width)) {
            int shrink = prev->x + prev->width - node->x;
            node->x += shrink;
            node->width -= shrink;
            if (node->width <= 0) {
                m_nodes.erase(m_nodes.begin()+i);
                --i;
            } else {
                break;
            }
        } else {
            break;
        }
    }
    Merge();
    m_used += width * height;
    return region;
}
