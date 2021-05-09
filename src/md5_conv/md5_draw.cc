//----------------------------------------------------------------------------
//  EDGE MD5 Library
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

#ifdef WIN32
#include <windows.h>
#include "../system/i_defs_gl.h"
#endif

#include "md5_draw.h"
#include "md5_anim.h"

#include "../system/i_defs.h"
#include "../system/i_defs_gl.h"

#include "../../epi/types.h"
#include "../../epi/endianess.h"

#include "../r_gldefs.h"
#include "../r_colormap.h"
#include "../r_effects.h"
#include "../r_image.h"
#include "../r_misc.h"
#include "../r_modes.h"
#include "../r_state.h"
#include "../r_shader.h"
#include "../r_units.h"

//#pragma optimize( "", off )  

#define PREMULTIPLY 1
 
#if defined (__arm__) || defined (__aarch64__)
#include "sse2neon.h"
#else
#include <xmmintrin.h>
#endif

// TODO: Fatal Crash using MD5 models & SSE2 code:: ERROR CODE IS HERE, IN SSE2 CODE!  Line 76.
__m128 m4x4v_colSSE(const __m128 cols[4], const __m128 v) 
{
	__m128 u1 = _mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 0, 0, 0));
	__m128 u2 = _mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 1, 1, 1));
	__m128 u3 = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 2, 2, 2));
	__m128 u4 = _mm_shuffle_ps(v, v, _MM_SHUFFLE(3, 3, 3, 3));

	__m128 prod1 = _mm_mul_ps(u1, cols[0]);
	__m128 prod2 = _mm_mul_ps(u2, cols[1]); //<--- this line produces the error! Exception thrown at 0x0012F167 
	__m128 prod3 = _mm_mul_ps(u3, cols[2]);
	__m128 prod4 = _mm_mul_ps(u4, cols[3]);

	return _mm_add_ps(_mm_add_ps(prod1, prod2), _mm_add_ps(prod3, prod4));
}



void md5_transform_vertices_sse(MD5mesh *msh, epi::mat4_c *posemats, basevert *dst) 
{
	epi::mat4_c *mats = posemats + 1;
	MD5vertex *vs = msh->verts;
	MD5weight *ws = msh->weights;

	int i,j;
	
	for(i = 0; i < msh->vertcnt; i++)
	{
		MD5vertex *v = vs + i;
		basevert *cv = dst + i;

		__m128 pos = _mm_set1_ps(0);
		__m128 norm = _mm_set1_ps(0);
		
		MD5weight *w = ws + v->firstweight;
		for(j = 0; j < v->weightcnt; j++) 
		{
			__m128 wpos = _mm_setr_ps(w[j].pos[0], w[j].pos[1], w[j].pos[2], 1);
			__m128 wnorm = _mm_setr_ps(w[j].normal[0],w[j].normal[1], w[j].normal[2], 0);
			wpos = m4x4v_colSSE((__m128*)&mats[w[j].jointidx], wpos);
			wnorm = m4x4v_colSSE((__m128*)&mats[w[j].jointidx], wnorm);
			
			__m128 weight = _mm_set1_ps(w[j].weight);
			pos = _mm_mul_ps(_mm_add_ps(wpos,pos),weight);
			norm = _mm_mul_ps(_mm_add_ps(wnorm,norm),weight);
		}

		_mm_store_ps((float*)&cv->pos, pos);
		_mm_store_ps((float*)&cv->norm, norm);
	}

	// non-SSE2 "fix" below (?)
	//void md5_transform_vertices(MD5mesh *msh, epi::mat4_c *posemats, basevert *dst); //FIXME: temporary workaround to the SSE version crashing the system somehow
}


void md5_transform_vertices(MD5mesh *msh, epi::mat4_c *posemats, basevert *dst) 
{
	epi::mat4_c *mats = posemats + 1;
	MD5vertex *vs = msh->verts;
	MD5weight *ws = msh->weights;
	int i,j;
	
	for(i = 0; i < msh->vertcnt; i++) 
	{
		MD5vertex *v = vs + i;
		basevert *cv = dst + i;
		
		cv->pos = epi::vec3_c(0,0,0);
		cv->norm = epi::vec3_c(0,0,0);
		cv->tan = epi::vec3_c(0,0,0);
		cv->uv = epi::vec2_c(v->uv[0],v->uv[1]);
		
		MD5weight *w = ws + v->firstweight;
		for(j = 0; j < v->weightcnt; j++) 
		{
#if PREMULTIPLY
			cv->pos += (mats[w[j].jointidx] * epi::vec4_c(w[j].pos,w[j].weight)).Get3D();
			cv->norm += (mats[w[j].jointidx] * epi::vec4_c(w[j].normal,0)).Get3D();
			cv->tan += (mats[w[j].jointidx] * epi::vec4_c(w[j].tan,0)).Get3D();
#else
			cv->pos += mats[w[j].jointidx] * epi::vec3_c(w[j].pos) * w[j].weight;
			cv->norm += (mats[w[j].jointidx] * epi::vec4_c(w[j].normal,0)).Get3D() * w[j].weight;
			cv->tan += (mats[w[j].jointidx] * epi::vec4_c(w[j].tan,0)).Get3D() * w[j].weight;
#endif
		}
		//cv->norm.MakeUnit();
	}
}

void render_md5_direct_triangle_fullbright(MD5mesh *msh, basevert *vbuff) 
{
	glColor3f(1,1,1);
	
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	
	glVertexPointer(3, GL_FLOAT, sizeof(vbuff[0]), &vbuff[0].pos);
	glTexCoordPointer(2, GL_FLOAT, sizeof(vbuff[0]), &vbuff[0].uv);
	
	glDrawElements(GL_TRIANGLES, msh->tricnt * 3, GL_UNSIGNED_SHORT, (const GLvoid *)msh->tris);
	
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

void render_md5_direct_triangle_lighting(MD5mesh *msh, basevert *vbuff,const epi::mat4_c& model_mat) 
{
	/*
	glEnable(GL_LIGHTING);	
	glEnable(GL_LIGHT0);
	glColor3f(1,1,1);
	*/
	/*
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	
	glVertexPointer(3, GL_FLOAT, sizeof(vbuff[0]), &vbuff[0].pos);
	glTexCoordPointer(2, GL_FLOAT, sizeof(vbuff[0]), &vbuff[0].uv);
	glNormalPointer(GL_FLOAT, sizeof(vbuff[0]), &vbuff[0].norm);
	
	glDrawElements(GL_TRIANGLES, msh->tricnt * 3, GL_UNSIGNED_SHORT, (const GLvoid *)msh->tris);
	
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	*/


	/*
	glDisable(GL_LIGHTING);	
	glDisable(GL_LIGHT0);
	*/

	GLuint tex=W_ImageCache(msh->tex ? msh->tex : W_ImageForDummySkin());

	local_gl_vert_t * glvert = RGL_BeginUnit(GL_TRIANGLES, msh->tricnt*3, ENV_SKIP_RGB, tex,ENV_NONE, 0, 0, BL_NONE);

	for(int i=0;i<msh->tricnt;i++) 

	{
		for(int t=0;t<3;t++) 
		{
			local_gl_vert_t *dest = glvert + i*3+t;
			md5_vert_idx id=msh->tris[i].vidx[t];

			epi::vec3_c pos=model_mat*vbuff[id].pos;
			epi::vec3_c norm=(model_mat*epi::vec4_c(vbuff[id].norm,0)).Get3D();
			epi::vec3_c tan=(model_mat*epi::vec4_c(vbuff[id].tan,0)).Get3D();

			dest->rgba[0]=1;
			dest->rgba[1]=1;
			dest->rgba[2]=1;
			dest->rgba[3]=1;

			dest->pos.x=pos.x;
			dest->pos.y=pos.y;
			dest->pos.z=pos.z;

			dest->normal.x=norm.x;
			dest->normal.y=norm.y;
			dest->normal.z=norm.z;

			dest->tangent.x=tan.x;
			dest->tangent.y=tan.y;
			dest->tangent.z=tan.z;

			dest->texc[0].x=vbuff[id].uv.x;
			dest->texc[0].y=vbuff[id].uv.y;
			dest->edge=false;
		}
	}

	RGL_EndUnit(msh->tricnt*3);

}


