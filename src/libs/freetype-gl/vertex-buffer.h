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

   ========================================================================= */
#ifndef __VERTEX_BUFFER_H__
#define __VERTEX_BUFFER_H__

#define GL_GLEXT_PROTOTYPES

#if defined(__APPLE__)
    #include <Glut/glut.h>
#elif defined(__WXMSW__)
#define GLEW_STATIC
//#include <GL/freeglut_ext.h>
#include <GL/glew.h>
#include <GL/wglew.h>
#else

//#include <GL/glut.h>
//#include <GL/gl.h>
    #include <GL/gl.h>
    #include <GL/glext.h>

#endif

#include "freetype-gl/vector.h"  // grrr.. nasty c programmers.. go learn a real OOPL.

#define MAX_VERTEX_ATTRIBUTE 8

class VertexAttribute;

//Generic vertex buffer.
class VertexBuffer
{
public:
    VertexBuffer(char * format);
    VertexBuffer(char * format, size_t vcount, void *vertices, size_t icount, GLuint * indices);
    ~VertexBuffer();

    void Draw(const char * format, const GLenum mode, const void * vertices, const size_t count );
    void DrawIndexed(const char * format, const GLenum mode, const void * vertices, const size_t vcount, const GLuint * indices, const size_t icount);
    void Render(GLenum mode, char * what);
    void Upload();
    void Clear();
    void PushBackIndex(GLuint index );
    void PushBackVertex(void *vertex);
    void PushBackIndices (GLuint *indices, size_t count);
    void PushBackVertices(void *vertices, size_t count );
    void InsertIndices (size_t index, GLuint *indices, size_t count);
    void InsertVertices(size_t index, void *_vertices, size_t count);

    void AddVertices(size_t index, void *vertices, size_t count);

    char * format;      // Format of the vertex buffer.
    Vector * vertices;  // Vector of vertices.
    GLuint vertices_id; // GL idendity of the vertices buffer.
    Vector * indices;   // Vector of indices.
    GLuint indices_id;  // GL idendity of the indices buffer.
    bool dirty;         // Whether the vertex buffer needs to be uploaded to GPU memory.

    VertexAttribute *attributes[MAX_VERTEX_ATTRIBUTE]; // Array of attributes.

protected:
    void __init(char *format);
};

// Generic vertex attribute.
class VertexAttribute
{
public:
    VertexAttribute( GLenum target, GLint size, GLenum type, GLsizei stride, GLvoid *pointer );
    static VertexAttribute *Parse(char *format);

    GLenum m_target;      // a client-side capability.
    GLint m_size;         // Number of component.
    GLenum m_type;        // Data type.
    GLsizei m_stride;     // Byte offset between consecutive attributes.
    GLvoid * m_pointer;   // Pointer to the first component of the first attribute element in the array.
    void (* enable)(void *); // Pointer to the function that enable this attribute.
};


GLenum GL_TYPE(char ctype);
GLenum GL_VERTEX_ATTRIBUTE_TARGET(char ctarget);
GLuint GL_TYPE_SIZE(GLenum gtype);
char * GL_TYPE_STRING(GLenum gtype);


#endif /* __VERTEX_BUFFER_H__ */
