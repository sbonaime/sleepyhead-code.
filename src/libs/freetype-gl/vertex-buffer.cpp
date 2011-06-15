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
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "vertex-buffer.h"




VertexBuffer::VertexBuffer(char *_format )
{
    __init(_format);
}
void VertexBuffer::__init(char *_format)
{
    size_t i, index = 0;
    int stride = 0;
    GLvoid *pointer = 0;

    format = strdup(_format);
    char *p = format-1;

    for (i=0; i<MAX_VERTEX_ATTRIBUTE; ++i) {
        attributes[i] = 0;
    }

    while (p && (index < MAX_VERTEX_ATTRIBUTE)) {
        p++;
        VertexAttribute *attribute = VertexAttribute::Parse(p);
        p = (char *)(strchr(p, ':'));
        attribute->m_pointer = pointer;
        stride += attribute->m_size*GL_TYPE_SIZE(attribute->m_type);
        pointer=(unsigned char *)pointer+ attribute->m_size*GL_TYPE_SIZE(attribute->m_type);
        attributes[index] = attribute;
        index++;
    }

    for (i=0; i<index; ++i) {
        attributes[i]->m_stride=stride;
    }

    vertices = vector_new( stride );
    vertices_id  = 0;

    indices = vector_new(sizeof(GLuint));
    indices_id  = 0;
    dirty = true;
}

VertexBuffer::VertexBuffer(char * format, size_t vcount, void *_vertices, size_t icount, GLuint *_indices)
{
    vector_resize(vertices, vcount );
    assert(vertices->size == vcount);
    memcpy(vertices->items, _vertices, vcount * vertices->item_size );

    vector_resize(indices, icount );
    assert(indices->size == icount);
    memcpy(indices->items, _indices, icount * indices->item_size );

    dirty = true;
}

VertexBuffer::~VertexBuffer()
{
    vector_delete(vertices);
    vertices = 0;
    if (vertices_id ) {
        glDeleteBuffers( 1, &vertices_id );
    }
    vertices_id = 0;

    vector_delete(indices);
    indices = 0;
    if (indices_id) {
        glDeleteBuffers( 1, &vertices_id );
    }
    indices_id = 0;
    if (format) {
        free(format);
    }
    format = 0;
    dirty = false;
}


void VertexBuffer::Upload()
{
    assert(glGenBuffers!=NULL); // glewInit must be called or this will die on windows
    if (!vertices_id) {
        glGenBuffers(1, &vertices_id);
    }
    if (!indices_id) {
        glGenBuffers(1, &indices_id );
    }
    glBindBuffer(GL_ARRAY_BUFFER, vertices_id);
    glBufferData(GL_ARRAY_BUFFER, vertices->size * vertices->item_size, vertices->items, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices->size * indices->item_size, indices->items, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}


void VertexBuffer::Clear()
{
    vector_clear(indices);
    vector_clear(vertices);
    dirty = true;
}

void VertexBuffer::Render(GLenum mode, char *what)
{
    unsigned i;
    int j;

    if (!vertices->size) {
        return;
    }

    if (dirty) {
        Upload();
        dirty=false;
    }

    glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
    glBindBuffer(GL_ARRAY_BUFFER, vertices_id);
    for (i=0; i<strlen(what); ++i) {
        char ctarget = what[i];
        GLuint target = GL_VERTEX_ATTRIBUTE_TARGET(ctarget);
        for (j=0; j<MAX_VERTEX_ATTRIBUTE; ++j) {
            VertexAttribute *attribute = attributes[j];
            if (attribute == 0) {
                break;
            } else if (attribute->m_target == target) {
                (*(attribute->enable))(attribute);
                break;
            }
        }
    }
    if (indices->size) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_id);
        glDrawElements(mode, indices->size, GL_UNSIGNED_INT, 0);
    } else {
        glDrawArrays(mode, 0, vertices->size);
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glPopClientAttrib();
}

void VertexBuffer::PushBackIndex(GLuint index)
{
    dirty = true;
    vector_push_back(indices, &index);
}

void VertexBuffer::PushBackVertex (void *vertex)
{
    dirty = true;
    vector_push_back(vertices, vertex);
}

void VertexBuffer::PushBackIndices(GLuint *_indices, size_t count)
{
    dirty = true;
    vector_push_back_data(indices, _indices, count );
}

void VertexBuffer::PushBackVertices(void *_vertices,size_t count)
{
    dirty = true;
    vector_push_back_data(vertices, _vertices, count);
}

void VertexBuffer::InsertIndices(size_t index, GLuint *_indices, size_t count)
{
    dirty = true;
    vector_insert_data( indices, index, _indices, count);
}

void VertexBuffer::InsertVertices(size_t index, void *_vertices, size_t count)
{
    size_t i;

    assert(vertices);
    assert(index < vertices->size+1 );

    dirty = true;
    for (i=0; i<indices->size-index; ++i) {
        if( *(GLuint *)(vector_get(indices, i )) > index ) {
            *(GLuint *)(vector_get(indices, i )) += index;
        }
    }
    vector_insert_data(vertices, index, _vertices, count);
}

// These are callbacks.
void VertexAttributePositionEnable(VertexAttribute *attr)
{
    glEnableClientState(attr->m_target);
    glVertexPointer(attr->m_size, attr->m_type, attr->m_stride, attr->m_pointer);
}

void VertexAttributeNormalEnable(VertexAttribute *attr)
{
    glEnableClientState(attr->m_target);
    glNormalPointer(attr->m_type, attr->m_stride, attr->m_pointer);
}

void VertexAttributeColorEnable(VertexAttribute *attr)
{
    glEnableClientState(attr->m_target);
    glColorPointer(attr->m_size, attr->m_type, attr->m_stride, attr->m_pointer);
}


void VertexAttributeTexCoordEnable(VertexAttribute *attr)
{
    glEnableClientState(attr->m_target);
    glTexCoordPointer(attr->m_size, attr->m_type, attr->m_stride, attr->m_pointer);
}

void VertexAttributeFogCoordEnable(VertexAttribute *attr)
{
    glEnableClientState(attr->m_target );
    glFogCoordPointer(attr->m_type, attr->m_stride, attr->m_pointer);
}

void VertexAttributeEdgeFlagEnable(VertexAttribute *attr)
{
    glEnableClientState(attr->m_target);
    glEdgeFlagPointer(attr->m_stride, attr->m_pointer);
}

void VertexAttributeSecondaryColorEnable(VertexAttribute *attr)
{
    glEnableClientState(attr->m_target);
    glSecondaryColorPointer(attr->m_size, attr->m_type, attr->m_stride, attr->m_pointer);
}

VertexAttribute *VertexAttribute::Parse(char *format)
{
    char *p = strpbrk ( format, "vcntfse" );
    if (p != 0) {
        int size;
        char ctarget, ctype;
        sscanf( p, "%c%d%c", &ctarget, &size, &ctype );
        GLenum type = GL_TYPE( ctype );
        GLenum target = GL_VERTEX_ATTRIBUTE_TARGET( ctarget );
        return new VertexAttribute(target, size, type, 0, 0);
    } else {
    }
    return NULL;
}



VertexAttribute::VertexAttribute(GLenum target,GLint size,GLenum type,GLsizei stride,GLvoid *pointer)
{
    switch(target ) {
    case GL_VERTEX_ARRAY:
        assert( size > 1 );
        enable = (void(*)(void *)) VertexAttributePositionEnable;
        break;

    case GL_NORMAL_ARRAY:
        assert (size == 3);
        assert( (type == GL_BYTE) || (type == GL_SHORT) ||
                (type == GL_INT)  || (type == GL_FLOAT) ||
                (type == GL_DOUBLE) );
        enable = (void(*)(void *)) VertexAttributeNormalEnable;
        break;

    case GL_COLOR_ARRAY:
        assert( (size == 3) || (size == 4) );
        assert( (type == GL_BYTE)  || (type == GL_UNSIGNED_BYTE)  ||
                (type == GL_SHORT) || (type == GL_UNSIGNED_SHORT) ||
                (type == GL_INT)   || (type == GL_UNSIGNED_INT)   ||
                (type == GL_FLOAT) || (type == GL_DOUBLE) );
        enable = (void(*)(void *)) VertexAttributeColorEnable;
        break;

    case GL_TEXTURE_COORD_ARRAY:
        assert( (type == GL_SHORT) || (type == GL_INT) ||
                (type == GL_FLOAT) || (type == GL_DOUBLE) );
        enable = (void(*)(void *)) VertexAttributeTexCoordEnable;
        break;

    case GL_FOG_COORD_ARRAY:
        assert( size == 2 );
        assert( (type == GL_FLOAT) || (type == GL_DOUBLE) );
        enable = (void(*)(void *)) VertexAttributeFogCoordEnable;
        break;

    case GL_EDGE_FLAG_ARRAY:
        assert( size == 1 );
        assert( type == GL_BOOL );
        enable = (void(*)(void *)) VertexAttributeEdgeFlagEnable;
        break;

    case GL_SECONDARY_COLOR_ARRAY:
        assert( size == 3 );
        assert( (type == GL_BYTE)  || (type == GL_UNSIGNED_BYTE)  ||
                (type == GL_SHORT) || (type == GL_UNSIGNED_SHORT) ||
                (type == GL_INT)   ||  (type == GL_UNSIGNED_INT)  ||
                (type == GL_FLOAT) || (type == GL_DOUBLE) );
        enable = (void(*)(void *)) VertexAttributeSecondaryColorEnable;
        break;

    default:
        enable = 0;
        break;
    }

    m_target  = target;
    m_size    = size;
    m_type    = type;
    m_stride  = stride;
    m_pointer = pointer;
}


GLenum GL_TYPE(char ctype)
{
    switch( ctype )
    {
    case 'b': return GL_BYTE;
    case 'B': return GL_UNSIGNED_BYTE;
    case 's': return GL_SHORT;
    case 'S': return GL_UNSIGNED_SHORT;
    case 'i': return GL_INT;
    case 'I': return GL_UNSIGNED_INT;
    case 'f': return GL_FLOAT;
    case 'd': return GL_DOUBLE;
    default:  return 0;
    }
}



GLenum GL_VERTEX_ATTRIBUTE_TARGET(char ctarget)
{
    switch(ctarget)
    {
    case 'v': return GL_VERTEX_ARRAY;
    case 'n': return GL_NORMAL_ARRAY;
    case 'c': return GL_COLOR_ARRAY;
    case 't': return GL_TEXTURE_COORD_ARRAY;
    case 'f': return GL_FOG_COORD_ARRAY;
    case 's': return GL_SECONDARY_COLOR_ARRAY;
    case 'e': return GL_EDGE_FLAG_ARRAY;
    default:  return 0;
    }
}



GLuint GL_TYPE_SIZE(GLenum gtype)
{
    switch(gtype)
    {
    case GL_BOOL:           return sizeof(GLboolean);
    case GL_BYTE:           return sizeof(GLbyte);
    case GL_UNSIGNED_BYTE:  return sizeof(GLubyte);
    case GL_SHORT:          return sizeof(GLshort);
    case GL_UNSIGNED_SHORT: return sizeof(GLushort);
    case GL_INT:            return sizeof(GLint);
    case GL_UNSIGNED_INT:   return sizeof(GLuint);
    case GL_FLOAT:          return sizeof(GLfloat);
    case GL_DOUBLE:         return sizeof(GLdouble);
    default:                return 0;
    }
}



char * GL_TYPE_STRING(GLenum gtype)
{
    switch(gtype)
    {
    case GL_BOOL:           return (char *)"GL_BOOL";
    case GL_BYTE:           return (char *)"GL_BYTE";
    case GL_UNSIGNED_BYTE:  return (char *)"GL_UNSIGNED_BYTE";
    case GL_SHORT:          return (char *)"GL_SHORT";
    case GL_UNSIGNED_SHORT: return (char *)"GL_UNSIGNED_SHORT";
    case GL_INT:            return (char *)"GL_INT";
    case GL_UNSIGNED_INT:   return (char *)"GL_UNSIGNED_INT";
    case GL_FLOAT:          return (char *)"GL_FLOAT";
    case GL_DOUBLE:         return (char *)"GL_DOUBLE";
    default:                return (char *)"GL_VOID";
    }
}
