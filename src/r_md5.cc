//----------------------------------------------------------------------------
//  3DGE 2.0 MD5 Animation Management
//----------------------------------------------------------------------------
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

#include "i_defs.h"
#include "i_defs_gl.h"
#include "../md5_conv/md5_draw.h"

#include "dm_data.h"
#include "dm_defs.h"
#include "dm_state.h"
#include "p_local.h"
#include "r_image.h"
#include "w_model.h"
#include "r_draw.h"
//#include "w_model.h"
#include "r_md5.h"
#include "w_wad.h"

#define MAX_ANIMATION_NAME_LENGTH 11
typedef struct md5_animation_handle_s
{
	char lumpname[MAX_ANIMATION_NAME_LENGTH+1];
	MD5animation *animation;
}
md5_animation_handle_t;

static int num_animations = 0;
static md5_animation_handle_t animations[R_MAX_MD5_ANIMATIONS];

static basevert vbuff[10000];

cvar_c r_md5scale;

short R_LoadMD5AnimationName(const char * lumpname)
{
	if (lumpname == NULL)
		return -1;
	
	// Check if already loaded
	int i;
	for (i=0; i < num_animations; i++)
	{
		if (strcmp(lumpname, animations[i].lumpname) == 0)
			return i;
	}
	
	// Load animation
	SYS_ASSERT(strlen(lumpname) < MAX_ANIMATION_NAME_LENGTH);
	SYS_ASSERT(num_animations < R_MAX_MD5_ANIMATIONS);
	
	// If animation does not exist, return negative animation
	if (W_CheckNumForName2(lumpname) == -1)
		return -1;
	
	epi::file_c *f = W_OpenLump(lumpname);
	if (f == NULL)
		return -1;
	
	byte *animtext = f->LoadIntoMemory();
	SYS_ASSERT(animtext);
	
	strcpy(animations[num_animations].lumpname, lumpname);
	animations[num_animations].animation = md5_load_anim((char*)animtext);
	
	SYS_ASSERT(animations[num_animations].animation);
	
	delete[] animtext;		
	delete f;
	
	return num_animations++;
}

MD5animation * R_GetMD5Animation(short animation_num)
{
	if (0 <= animation_num && animation_num < num_animations)
		return animations[animation_num].animation;
	return NULL;
}

static void LoadMD5Animation(MD5model *model, short animfile, int frame, MD5jointposebuff dst)
{
	MD5animation *anim = R_GetMD5Animation(animfile);
	if (anim != NULL && 0 <= frame && frame < anim->framecnt )
	{
		md5_pose_load(anim, frame, dst);
	}
	else
	{
		md5_pose_load_identity(model->joints, model->jointcnt, dst);
	}
}

void md5_draw_unified_gl(MD5umodel *umd5, epi::mat4_c *jointmats) {
	int i;
	MD5model *md5 = &umd5->model;
	
	for(i = 0; i < md5->meshcnt; i++) {
		MD5mesh *msh = md5->meshes + i;
		const image_c *skin_img = msh->tex;
		if (! skin_img)
		{
			I_Debugf("Render model: no skin \"%s\"\n", msh->shader);
			skin_img = W_ImageForDummySkin();
		}
		glBindTexture(GL_TEXTURE_2D, W_ImageCache(skin_img));
		
		
		md5_transform_vertices(msh, jointmats, vbuff);
		render_md5_direct_triangle_lighting(msh, vbuff);
		
	}
}

//TODO add skin_img support
void MD5_RenderModel(modeldef_c *md, int last_anim, int last_frame,
	int current_anim, int current_frame, float lerp, float x, float y, float z, mobj_t *mo)
{
	//when rendering a uninterpolated model, pass -1 for last_anim and pass 1.0f for lerp
	
	SYS_ASSERT(md->modeltype == MODEL_MD5_UNIFIED);
	
	static epi::mat4_c posemats[MD5_MAX_JOINTS+1];
	static MD5jointposebuff jpcur, jplast;
	
	//TODO get previous animfile and make sure previous frame was same model, in case model changes
	//TODO check model and animation have same number of joints
	if (mo->state->framerange == 0) {
		if (last_anim >= 0)
			LoadMD5Animation(&md->md5u->model, last_anim, last_frame, jplast);
		LoadMD5Animation(&md->md5u->model, current_anim, current_frame, jpcur);
	} else {
		SYS_ASSERT(0);
	}
	
	if (lerp < 1.0)
		md5_pose_lerp(jplast,jpcur,md->md5u->model.jointcnt,lerp,jpcur);
	md5_pose_to_matrix(jpcur, md->md5u->model.jointcnt, posemats);
	
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	
	glPushMatrix();
	glTranslatef(x, y, z);
	glRotatef(90.0f - ANG_2_FLOAT(mo->GetInterpolatedAngle()), 0.0f, 0.0f, 1.0f);
	glScalef(r_md5scale.d,r_md5scale.d,r_md5scale.d);
	
	md5_draw_unified_gl(md->md5u, posemats);
	
	glPopMatrix();
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
