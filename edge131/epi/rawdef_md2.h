//----------------------------------------------------------------------------
//  MD2 defines, raw on-disk layout
//----------------------------------------------------------------------------
//
//  Copyright (c) 2002-2005  The EDGE Team.
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
//
//  Based on "qfiles.h" and "anorms.h" from the GPL'd quake 2 source
//  release.  Copyright (C) 1997-2001 Id Software, Inc.
//
//----------------------------------------------------------------------------

#ifndef __EPI_RAWDEF_MD2_H__
#define __EPI_RAWDEF_MD2_H__

#include "types.h"

namespace epi
{

// format uses float pointing values, but to allow for endianness
// conversions they are represented here as unsigned integers.
typedef u32_t f32_t;

#define EPI_MD2_IDENT  "IDP2"

typedef struct
{
	char ident[4];
	s32_t version;

	s32_t skin_width;
	s32_t skin_height;
	s32_t frame_size;

	s32_t num_skins;
	s32_t num_xyz;
	s32_t num_st;
	s32_t num_tris;
	s32_t num_glcmds;
	s32_t num_frames;

	s32_t ofs_skins;
	s32_t ofs_st;
	s32_t ofs_tris;
	s32_t ofs_frames;
	s32_t ofs_glcmds;  
	s32_t ofs_end;
} 
raw_md2_header_t;

typedef struct
{
	u16_t s, t;
} 
raw_md2_texcoord_t;

typedef struct 
{
	u16_t index_xyz[3];
	u16_t index_st[3];
} 
raw_md2_triangle_t;

typedef struct
{
	u8_t v[3];
	u8_t light_normal;
} 
raw_md2_vertex_t;

typedef struct
{
	f32_t s, t;
	s32_t vert_index;
}
raw_md2_gl_vertex_t;

typedef struct
{
	f32_t scale[3];
	f32_t translate[3];

	char name[16];

	raw_md2_vertex_t verts[1];  /* variable sized */
} 
raw_md2_frame_t;

typedef struct
{
	char name[64];
}
raw_md2_skin_t;


/* ---- normals ---- */

#define MD2_NUM_NORMALS  162

extern float md2_normals[MD2_NUM_NORMALS][3];

}  // namespace epi

#endif /* __EPI_RAWDEF_MD2_H__ */
