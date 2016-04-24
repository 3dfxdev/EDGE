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


#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <setjmp.h>

#include "../epi/epi.h"
#include "../epi/math_quaternion.h"

#include "md5.h"
#include "md5_anim.h"
#include "md5_parse.h"

#define dbo(a...) do { printf(a); fflush(stdout); } while(0)
#define dbl() do { dbo("[%s %i]\n",__FILE__, __LINE__); } while(0)

void I_Error(const char *error,...);

static void md5_load_frames(MD5animation *a) {
	int i,j;
	float *curcomp;
	
	curcomp = a->framecomponents = new float[a->animatedcomponentcnt * a->framecnt]();
	for(i = 0; i < a->framecnt; i++) {
		expect_token("frame", "");
		expect_int();	//should be == i
		expect_token("{", "");
		
		for(j = 0; j < a->animatedcomponentcnt; j++)
			*curcomp++ = expect_float();
		
		expect_token("}", "");
	}
}

static void md5_load_baseframe(MD5animation *a) {
	expect_token("baseframe", "");
	expect_token("{", "");
	
	a->baseframe = new MD5baseframe[a->jointcnt]();
	int i;
	for(i = 0; i < a->jointcnt; i++) {
		expect_paren_vector(3, a->baseframe[i].pos);
		expect_paren_vector(3, a->baseframe[i].rot);
	}
	
	expect_token("}", "");
}

static void md5_load_bounds(MD5animation *a) {
	expect_token("bounds", "");
	expect_token("{", "");
	
	a->bounds = new MD5bounds[a->framecnt]();
	int i;
	for(i = 0; i < a->framecnt; i++) {
		expect_paren_vector(3, a->bounds[i].min);
		expect_paren_vector(3, a->bounds[i].max);
	}
	
	expect_token("}", "");
}

static void md5_load_heirarchy(MD5animation *a) {
	expect_token("hierarchy", "");
	expect_token("{", "");
	
	a->hierarchy = new MD5hierarchy[a->jointcnt]();
	int i;
	for(i = 0; i < a->jointcnt; i++) {
		MD5hierarchy *hierarchy = a->hierarchy + i;
		expect_string(hierarchy->name, sizeof(hierarchy->name));
		hierarchy->parentidx = expect_int();
		hierarchy->flags = expect_int();
		hierarchy->startidx = expect_int();
	}
	
	expect_token("}", "");
}

MD5animation * md5_load_anim(char *md5text) {
	MD5animation *a = NULL;
	start_parse(md5text);
	if (try_parse()) {
		a = new MD5animation[1]();

		expect_token("MD5Version", "");
		a->version = expect_int();
		if (a->version != 10)
			md5_parse_error("expected version 10, but got version %i", a->version);
		
		next_token_no_eof("");
		if (token_equals("commandline")) {
			expect_string(a->commandline, sizeof(a->commandline));
			expect_token("numFrames", "");
		} else if (!token_equals("numFrames")) {
			md5_parse_error("expected numFrames");
		}
		
		//load frames number
		a->framecnt = expect_int();
		
		//load joints number
		expect_token("numJoints", "");
		a->jointcnt = expect_int();
		if (a->jointcnt <= 0)
			md5_parse_error("need to have more than zero joints (found %i)", a->jointcnt);
		if (a->jointcnt >= MD5_MAX_JOINTS)
			md5_parse_error("model has %i joints but max supported is %i", a->jointcnt, MD5_MAX_JOINTS);
		
		//load framerate
		expect_token("frameRate", "");
		a->framerate = expect_float();
		
		//load numanimatedcomponents
		expect_token("numAnimatedComponents", "");
		a->animatedcomponentcnt = expect_int();
		
		//load rest
		md5_load_heirarchy(a);
		md5_load_bounds(a);
		md5_load_baseframe(a);
		md5_load_frames(a);
	} else {
		I_Error("%s\n", get_parse_error());
	}
	end_parse();
	
	return a;
}

void md5_pose_load_identity(MD5joint *joints, int jointcnt, MD5jointposebuff dst) {
	while(jointcnt--) {
		dst->pos = epi::vec3_c(joints->pos);
		dst->rot = epi::quat_c(joints->rot[0], joints->rot[1], joints->rot[2], joints->rot[3]);
		
		//dst->pos = epi::vec3_c();
		//dst->rot = epi::quat_c(0,0,0);
		
		dst++;
		joints++;
	}
}

void md5_pose_lerp(MD5jointposebuff from, MD5jointposebuff to, int jointcnt, float blend, MD5jointposebuff dst) {
	while(jointcnt--) {
		dst->pos = from->pos.Lerp(to->pos, blend);
		dst->rot = from->rot.Slerp(to->rot, blend);
		from++;
		to++;
		dst++;
	}
}

void md5_pose_load(MD5animation *anim, int frame, MD5jointposebuff dst) {
	int jointcnt = anim->jointcnt;
	float *comp = anim->framecomponents + anim->animatedcomponentcnt * frame;
	MD5baseframe *bframe = anim->baseframe;
	MD5hierarchy *hier = anim->hierarchy;
	MD5jointpose *base = dst;
	
	while(jointcnt--) {
		float x,y,z;
		x = bframe->pos[0];
		y = bframe->pos[1];
		z = bframe->pos[2];
		if (hier->flags & MD5_POSE_X_POS)
			x = *comp++;
		if (hier->flags & MD5_POSE_Y_POS)
			y = *comp++;
		if (hier->flags & MD5_POSE_Z_POS)
			z = *comp++;
		dst->pos = epi::vec3_c(x,y,z);
		
		x = bframe->rot[0];
		y = bframe->rot[1];
		z = bframe->rot[2];
		if (hier->flags & MD5_POSE_X_ROT)
			x = *comp++;
		if (hier->flags & MD5_POSE_Y_ROT)
			y = *comp++;
		if (hier->flags & MD5_POSE_Z_ROT)
			z = *comp++;
		dst->rot = epi::quat_c(x,y,z);
		
		if (hier->parentidx >= 0) {
			MD5jointpose *parent = base + hier->parentidx;
			
			dst->pos = parent->rot.Rotate(dst->pos) + parent->pos;
			dst->rot = parent->rot * dst->rot;
		}
		
		bframe++;
		hier++;
		dst++;
	}
}

void md5_pose_to_matrix(MD5jointpose *src, int jointcnt, epi::mat4_c *dst) {
	*dst++ = epi::mat4_c();
	while(jointcnt--) {
		//*dst = mats[joints->parent] * src->rot.ToMat4().SetOrigin(epi::vec3_c(src->pos));
		*dst = src->rot.ToMat4().SetOrigin(epi::vec3_c(src->pos));
		
		dst++;
		src++;
	}
}


