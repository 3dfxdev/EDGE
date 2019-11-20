//----------------------------------------------------------------------------
//  3DGE MD5 Animation Management
//----------------------------------------------------------------------------
//
//  (C) Isotope Softworks 2014-2015
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
//  TODO: 
//----------------------------------------------------------------------------

#include "system/i_defs.h"
#include "system/i_defs_gl.h"
#include "md5_conv/md5_draw.h"

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

#include "system/i_defs.h"
#include "system/i_defs_gl.h"

#include "../epi/types.h"
#include "../epi/endianess.h"

#include "../src/r_gldefs.h"
#include "../src/r_colormap.h"
#include "../src/r_effects.h"
#include "../src/r_image.h"
#include "../src/r_misc.h"
#include "../src/r_modes.h"
#include "../src/r_state.h"
#include "../src/r_shader.h"
#include "../src/r_units.h"

//64 bit hack!
#ifdef _M_X64
#define SSE2 1
#endif


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

DEF_CVAR(r_md5scale, int, "c", 0);

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
		//I_Debugf("Render model: loading md5anim\n");
	}
	else
	{
		md5_pose_load_identity(model->joints, model->jointcnt, dst);
		//I_Debugf("Render model: loading md5joints\n");
	}
}


void md5_draw_unified_gl(MD5umodel *umd5, epi::mat4_c *jointmats,const epi::mat4_c& model_mat) 
{
	int i;
	MD5model *md5 = &umd5->model;
	
	for(i = 0; i < md5->meshcnt; i++) 
	{

		MD5mesh *msh = md5->meshes + i;
		const image_c *skin_img = msh->tex;
		/*Debug missing model group skins by uncommenting the next two Debugf lines!!!*/

		if (!skin_img)
		{   

			//I_Debugf("R_unifiedMD5: no skin(s) defined in MD5 model: \"%s\"\n", md5);
			I_Debugf("md5draw: No skin(s) found, subbing for DummySkin!\n");
			skin_img = W_ImageForDummySkin();
		}
		//BUG: md5_transform_Verticies_sse() is not functional and will crash 3DGE.
#ifndef __SSE2__  // Visual Studio Version: #ifdef __SSE2__
		md5_transform_vertices(msh, jointmats, vbuff); /// DOES NOT USE SSE
#else
		md5_transform_vertices_sse(msh, jointmats, vbuff); /// uses _SSE for quicker transforms
#endif
		//Lighting render stage. This would be what we would change to render softer lighting on triangles!
		render_md5_direct_triangle_lighting(msh, vbuff,model_mat);
		
	}
}

static void DLIT_CollectLights(mobj_t *mo, void *dataptr) 
{
	mobj_t* data= (mobj_t*)dataptr;

	// dynamic lights do not light themselves up!
	if (mo == data)
		return;
	RGL_AddLight(mo);
}


void MD5_RenderModel(modeldef_c *md, int last_anim, int last_frame,
	int current_anim, int current_frame, float lerp,epi::vec3_c pos,
	epi::vec3_c scale,epi::vec3_c bias,mobj_t *mo)
{
	//When rendering an uninterpolated model, pass -1 for last_anim and pass 1.0f for lerp
	
	SYS_ASSERT(md->modeltype == MODEL_MD5_UNIFIED);
	
	static epi::mat4_c posemats[MD5_MAX_JOINTS+1];

	static MD5jointposebuff jpcur, jplast;
	
	//TODO get previous animfile and make sure previous frame was same model, in case model changes
	//TODO check model and animation have same number of joints
	if (mo->state->framerange == 0) 
	{
		if (last_anim >= 0)
		LoadMD5Animation(&md->md5u->model, last_anim, last_frame, jplast);

		LoadMD5Animation(&md->md5u->model, current_anim, current_frame, jpcur);
	} 
	else 
	{	//No frames and/or STATIC!
		SYS_ASSERT(0);
	}
	
	if (lerp < 1.0)
	md5_pose_lerp(jplast,jpcur,md->md5u->model.jointcnt,lerp,jpcur);

	md5_pose_to_matrix(jpcur, md->md5u->model.jointcnt, posemats);

	epi::mat4_c model_mat;

	//position
	model_mat.SetOrigin(pos);

	//TODO: is there any existing epi::mat4_c rotation/scale code?
	float ang=-ANG_2_FLOAT(mo->GetInterpolatedAngle())*M_PI/180.0f;
	float cos_a=cos(ang);
	float sin_a=sin(ang);

	//TODO: vert angle disabled for now!
	float vertang=ANG_2_FLOAT(mo->GetInterpolatedVertAngle())*M_PI/180.0f;
	//float vertang=0.0f; 
	float cos_va=cos(vertang);
	float sin_va=sin(vertang);

	epi::mat4_c tmp_mat;

	//rotation
	tmp_mat.m[0]=cos_a;
	tmp_mat.m[1]=-sin_a;
	tmp_mat.m[4]=sin_a;
	tmp_mat.m[5]=cos_a;
	model_mat*=tmp_mat;

	//vert rotation
	tmp_mat=epi::mat4_c();
	tmp_mat.m[0]=cos_va;
	tmp_mat.m[2]=sin_va;
	tmp_mat.m[8]=-sin_va;
	tmp_mat.m[10]=cos_va;
	model_mat*=tmp_mat;

	//scale
	tmp_mat=epi::mat4_c();
	tmp_mat.m[0]*=r_md5scale*scale.x;
	tmp_mat.m[5]*=r_md5scale*scale.y;
	tmp_mat.m[10]*=r_md5scale*scale.z;
	model_mat*=tmp_mat;

	//bias (bias is translation after scale/rotation - ie: translation in model-space)
	tmp_mat=epi::mat4_c();
	tmp_mat.SetOrigin(bias);
	model_mat*=tmp_mat;

	// TODO: Maybe make this a CVAR instead. . . .
	short l=CLAMP(0,mo->props->lightlevel+mo->state->bright,255); //
	float r = mo->radius;

	RGL_ClearLights();

	// This clamps to sector lighting.
	RGL_SetAmbientLight(l,l,l);

	P_DynamicLightIterator(mo->x - r, mo->y - r, mo->z,
						   mo->x + r, mo->y + r, mo->z + mo->height,
						   DLIT_CollectLights, mo);


	md5_draw_unified_gl(md->md5u, posemats,model_mat);

	//I_Printf("MD5_Render: %f %f %f\n",mo->x,mo->y,mo->z);

	//glPopMatrix();
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
