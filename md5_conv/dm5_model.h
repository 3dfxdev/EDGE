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


#ifndef _DM5_H_
#define _DM5_H_

#include "md5.h"
#include "md5_anim.h"

typedef float vec2_t[2];
typedef float vec3_t[3];
typedef float vec4_t[4];
typedef vec4_t quat4_t;

typedef struct {
	char jointcnt;	//number of weights per vertex
	char joint[4];	//bone id
	short vertcnt;	//number of vertices
	int shaderid;	//texture id
	
	//pointers to vertex data
	
	//position, thare are vertcnt*jointcnt vectors
	//first vertcnt# of vectors are for the first bone, then the next vertcnt# of vectors are for the second bone, etc
	//given a source vector of (x,y,z,weight), the value stored in the array is (x*weight,y*weight,z*weight,weight)
	vec4_t *pos;
	
	//vertex normal, thare are vertcnt*jointcnt vectors
	//first vertcnt# of vectors are for the first bone, then the next vertcnt# of vectors are for the second bone, etc
	//given a source vector of (x,y,z,weight), the value stored in the array is (x*weight,y*weight,z*weight,0)
	vec4_t *norm;
	
	//uv, there are vertcnt vectors
	vec2_t *uv;
	
	short *strips;	//first short is length of strip, (a length of zero terminates the list) followed by list vertex idxs. repeats
} DM5group;

typedef struct {
	vec3_t pos;
	int parentidx;
	quat4_t rot;
} DM5joint;

#define MAX_DM5_NAME 31
typedef struct {
	short version;	//version of model format
	short vertbuffsize;	//size of required vertex buffer
	int modelsize;	//size of entire model in bytes

	int jointcnt;
	DM5joint *joints;
	
	int groupcnt;
	DM5group *groups;
	
	char (*jointnames)[MAX_DM5_NAME+1];
} DM5model;

#endif
