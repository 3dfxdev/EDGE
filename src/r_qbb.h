//----------------------------------------------------------------------------
//  EDGE2 OpenGL Rendering (Buffer building)
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

#ifndef __RGL_QBB_H__
#define __RGL_QBB_H__

#include "system/i_defs_gl.h"
#include "r_qvf.h"
#include <vector>

extern GLuint qbb_buffer;

template <typename T>
class RQImmBuffer {
    std::vector<T> data;
    RQVertexFormat *format;
public:
    inline RQImmBuffer(RQVertexFormat *format) : format(format) {
        if (qbb_buffer == 0) {
            glGenBuffers(1, &qbb_buffer);
        }
    }
    ~RQImmBuffer() {}

    inline void add(const T &vertex) {
        data.push_back(vertex);
    }
    inline void draw(unsigned int mode) {
        glBindBuffer(GL_ARRAY_BUFFER, qbb_buffer);
        glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(T), data.data(), GL_STREAM_DRAW);
        RQVertexFormat *formatNode = this->format;
        while(formatNode->attribute != VFATTR_FINAL) {
            if(formatNode->attribute == VFATTR_POSITION) {
                glVertexPointer(formatNode->size, formatNode->type, formatNode->stride, (const void *) formatNode->offset);
                glEnableClientState(GL_VERTEX_ARRAY);
            } else if(formatNode->attribute == VFATTR_TEXCOORD) {
                glTexCoordPointer(formatNode->size, formatNode->type, formatNode->stride, (const void *)formatNode->offset);
                glEnableClientState(GL_TEXTURE_COORD_ARRAY);
            } else {
                glVertexAttribPointer(formatNode->attribute, formatNode->size, formatNode->type, formatNode->normalized, formatNode->stride, (const void *)formatNode->offset);
                glEnableVertexAttribArray(formatNode->attribute);
            }
            formatNode++;
        }
        glDrawArrays(mode, 0, data.size());
        formatNode = this->format;
        while(formatNode->attribute != VFATTR_FINAL) {
            if(formatNode->attribute == VFATTR_POSITION) {
                glDisableClientState(GL_VERTEX_ARRAY);
            } else if(formatNode->attribute == VFATTR_TEXCOORD) {
                glEnableClientState(GL_TEXTURE_COORD_ARRAY);
            } else {
                glDisableVertexAttribArray(formatNode->attribute);
            }
            formatNode++;
        }
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
};

#endif