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
    inline void init_buffer() {
        if(qbb_buffer == 0) {
            glGenBuffers(1, &qbb_buffer);
        }
    }

    inline RQImmBuffer(RQVertexFormat *format, bool initialize_buffer = true) : format(format) {

    }
    inline ~RQImmBuffer() {}


    inline void add(const T &vertex) {
        data.push_back(vertex);
    }
    inline void draw(unsigned int mode) {
        init_buffer();
        glBindBuffer(GL_ARRAY_BUFFER, qbb_buffer);
        glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(T), data.data(), GL_STREAM_DRAW);
        RQVertexFormat *formatNode = this->format;
        while(formatNode->attribute != VFATTR_FINAL) {
            if(formatNode->attribute == VFATTR_POSITION) {
                glVertexPointer(formatNode->size, formatNode->type, formatNode->stride, (const void *) formatNode->offset);
                glEnableClientState(GL_VERTEX_ARRAY);
            } else if(formatNode->attribute == VFATTR_TEXCOORD) {
                glClientActiveTexture(GL_TEXTURE0);
                glTexCoordPointer(formatNode->size, formatNode->type, formatNode->stride, (const void *)formatNode->offset);
                glEnableClientState(GL_TEXTURE_COORD_ARRAY);
            } else if(formatNode->attribute == VFATTR_TEXCOORD2) {
                glClientActiveTexture(GL_TEXTURE1);
                glTexCoordPointer(formatNode->size, formatNode->type, formatNode->stride, (const void *)formatNode->offset);
                glEnableClientState(GL_TEXTURE_COORD_ARRAY);
            } else if(formatNode->attribute == VFATTR_COLOR) {
                glColorPointer(formatNode->size, formatNode->type, formatNode->stride, (const void *)formatNode->offset);
                glEnableClientState(GL_COLOR_ARRAY);
            } else if(formatNode->attribute == VFATTR_NORMAL) {
                glNormalPointer(formatNode->type, formatNode->stride, (const void *)formatNode->offset);
                glEnableClientState(GL_NORMAL_ARRAY);
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
                glClientActiveTexture(GL_TEXTURE0);
                glDisableClientState(GL_TEXTURE_COORD_ARRAY);
            } else if(formatNode->attribute == VFATTR_TEXCOORD2) {
                glClientActiveTexture(GL_TEXTURE1);
                glDisableClientState(GL_TEXTURE_COORD_ARRAY);
            } else if(formatNode->attribute == VFATTR_COLOR) {
                glDisableClientState(GL_COLOR_ARRAY);
            } else if(formatNode->attribute == VFATTR_NORMAL) {
                glDisableClientState(GL_NORMAL_ARRAY);
            } else {
                glDisableVertexAttribArray(formatNode->attribute);
            }
            formatNode++;
        }
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        data.clear();
    }
};

#endif