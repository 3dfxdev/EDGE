//----------------------------------------------------------------------------
//  EDGE2 MD5 Library
//----------------------------------------------------------------------------
// 
//  Copyright (c) 2015 Isotope SoftWorks and Contributors.
//  Portions (C) GSP, LLC.
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


#ifndef _MD5_DRAW_H_
#define _MD5_DRAW_H_

#include "md5.h"

typedef struct {
	epi::vec3_c pos;
	float padding0;
	epi::vec3_c norm;
	float padding1;
	epi::vec2_c uv;
	float padding2;
	epi::vec3_c tan;
} basevert;

void md5_transform_vertices(MD5mesh *msh, epi::mat4_c *posemats, basevert *dst);
void render_md5_direct_triangle_fullbright(MD5mesh *msh, basevert *vbuff);
void render_md5_direct_triangle_lighting(MD5mesh *msh, basevert *vbuff,const epi::mat4_c& model_mat);


#endif
