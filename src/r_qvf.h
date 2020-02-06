//----------------------------------------------------------------------------
//  EDGE2 OpenGL Rendering (Vertex formats)
//----------------------------------------------------------------------------
//
//  Copyright (c) 2020  The EDGE2 Team.
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//----------------------------------------------------------------------------

#ifndef __RGL_QVF_H__
#define __RGL_QVF_H__

#include "system/i_defs_gl.h"
#include <stddef.h>

enum RQVertexFormatSpecialAttr {
    VFATTR_POSITION = -1,
    VFATTR_TEXCOORD = -3,
    VFATTR_TEXCOORD2 = -4,
    VFATTR_COLOR = -6,
    VFATTR_NORMAL = -5,
    VFATTR_FINAL = -2,
};

struct RQVertexFormat {
    GLint attribute;
    GLint size;
    GLenum type;
    GLboolean normalized;
    GLsizei stride;
    ptrdiff_t offset;
};

struct RQVertex3f {
    float x, y, z;
    static RQVertexFormat format[];
};

struct RQVertex3fTextured {
    float x, y, z;
    float u, v;
    static RQVertexFormat format[];
};

struct RQVertex3fSprite {
    float x, y, z;
    float u0, v0;
    float u1, v1;
    float r, g, b, a;
    float nx, ny, nz;
    float tx, ty, tz;
    static RQVertexFormat format[];
};


#endif