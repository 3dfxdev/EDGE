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


#ifndef _MD5_ANIM_H_
#define _MD5_ANIM_H_

#include "md5.h"

#include "../epi/epi.h"
#include "../epi/math_vector.h"
#include "../epi/math_matrix.h"
#include "../epi/math_quaternion.h"

//md5anim format
#define MD5_POSE_X_POS 1
#define MD5_POSE_Y_POS 2
#define MD5_POSE_Z_POS 4
#define MD5_POSE_X_ROT 8
#define MD5_POSE_Y_ROT 16
#define MD5_POSE_Z_ROT 32

typedef struct MD5Bone_s {
	char parentidx;
	char flags;
	short startidx;
	char name[MAX_MD5_NAME];
} MD5hierarchy;

typedef struct {
	fvec3_t min,max;
} MD5bounds;

typedef struct {
	fvec3_t pos;
	quat3_t rot;
} MD5baseframe;

typedef struct {
	float framerate;
	
	int framecnt;
	MD5bounds *bounds;	//number of bounds is framecnt
	
	int jointcnt;
	MD5hierarchy *hierarchy; //number of heirarchies us jointcnt
	MD5baseframe *baseframe;	//number of baseframe elements is jointcnt
	
	int animatedcomponentcnt;
	float *framecomponents;	//component x of frame y is accessed by framecomponents[y*animatedcomponentcnt+x]
	
	int version;
	char commandline[MAX_MD5_NAME];
} MD5animation;

//rendering internal
typedef struct {
	epi::vec3_c pos;
	int pad;
	epi::quat_c rot;
} MD5jointpose;

typedef MD5jointpose MD5jointposebuff[MD5_MAX_JOINTS];

void md5_pose_load_identity(MD5joint *joints, int jointcnt, MD5jointposebuff dst);
void md5_pose_load(MD5animation *anim, int frame, MD5jointpose *dst);
void md5_pose_to_matrix(MD5jointpose *src, int jointcnt, epi::mat4_c *dst);
void md5_pose_lerp(MD5jointposebuff from, MD5jointposebuff to, int jointcnt, float blend, MD5jointposebuff dst);

MD5animation * md5_load_anim(char *md5text);

#endif
