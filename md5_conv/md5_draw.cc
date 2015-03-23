#define _STDCALL_SUPPORTED
#define _M_IX86
#define GLUT_DISABLE_ATEXIT_HACK

#define COMMON_NO_LOADING

#include <windows.h>
#include <C:\hyperedge\Dream3DGE\PC136\md5_conv\GL\glut.h>
//#include <GL/glut.h>
//#include "glut.h"
#include <GL/gl.h>
#include <GL/glu.h>
#include <unistd.h>

#include <stdio.h>
#include <assert.h>
#include <math.h>

#include "md5_draw.h"
#include "md5_anim.h"

#define PREMULTIPLY 0

#if 0
#include <xmmintrin.h>

__m128 m4x4v_colSSE(const __m128 cols[4], const __m128 v) {
	__m128 u1 = _mm_shuffle_ps(v,v, _MM_SHUFFLE(0,0,0,0));
	__m128 u2 = _mm_shuffle_ps(v,v, _MM_SHUFFLE(1,1,1,1));
	__m128 u3 = _mm_shuffle_ps(v,v, _MM_SHUFFLE(2,2,2,2));
	__m128 u4 = _mm_shuffle_ps(v,v, _MM_SHUFFLE(3,3,3,3));

	__m128 prod1 = _mm_mul_ps(u1, cols[0]);
	__m128 prod2 = _mm_mul_ps(u2, cols[1]);
	__m128 prod3 = _mm_mul_ps(u3, cols[2]);
	__m128 prod4 = _mm_mul_ps(u4, cols[3]);

	return _mm_add_ps(_mm_add_ps(prod1, prod2), _mm_add_ps(prod3, prod4));
}

void md5_transform_vertices_sse(MD5mesh *msh, epi::mat4_c *posemats, basevert *dst) {
	epi::mat4_c *mats = posemats + 1;
	MD5vertex *vs = msh->verts;
	MD5weight *ws = msh->weights;
	int i,j;
	
	for(i = 0; i < msh->vertcnt; i++) {
		MD5vertex *v = vs + i;
		basevert *cv = dst + i;

		__m128 pos = _mm_set1_ps(0);
		__m128 norm = _mm_set1_ps(0);
		
		MD5weight *w = ws + v->firstweight;
		for(j = 0; j < v->weightcnt; j++) {
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
}
#endif

void md5_transform_vertices(MD5mesh *msh, epi::mat4_c *posemats, basevert *dst) {
	epi::mat4_c *mats = posemats + 1;
	MD5vertex *vs = msh->verts;
	MD5weight *ws = msh->weights;
	int i,j;
	
	for(i = 0; i < msh->vertcnt; i++) {
		MD5vertex *v = vs + i;
		basevert *cv = dst + i;
		
		cv->pos = epi::vec3_c(0,0,0);
		cv->norm = epi::vec3_c(0,0,0);
		cv->uv = epi::vec2_c(v->uv[0],v->uv[1]);
		
		MD5weight *w = ws + v->firstweight;
		for(j = 0; j < v->weightcnt; j++) {
#if PREMULTIPLY
			cv->pos += (mats[w[j].jointidx] * epi::vec4_c(w[j].pos,w[j].weight)).Get3D();
			cv->norm += (mats[w[j].jointidx] * epi::vec4_c(w[j].normal,0)).Get3D();
#else
			cv->pos += mats[w[j].jointidx] * epi::vec3_c(w[j].pos) * w[j].weight;
			cv->norm += (mats[w[j].jointidx] * epi::vec4_c(w[j].normal,0)).Get3D() * w[j].weight;
#endif
		}
		//cv->norm.MakeUnit();
	}
}

void render_md5_direct_triangle_fullbright(MD5mesh *msh, basevert *vbuff) {
	glColor3f(1,1,1);
	
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	
	glVertexPointer(3, GL_FLOAT, sizeof(vbuff[0]), &vbuff[0].pos);
	glTexCoordPointer(2, GL_FLOAT, sizeof(vbuff[0]), &vbuff[0].uv);
	
	glDrawElements(GL_TRIANGLES, msh->tricnt * 3, GL_UNSIGNED_SHORT, (const GLvoid *)msh->tris);
	
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

void render_md5_direct_triangle_lighting(MD5mesh *msh, basevert *vbuff) {
	glEnable(GL_LIGHTING);	
	glEnable(GL_LIGHT0);
	
	glColor3f(1,1,1);
	
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
	
	glDisable(GL_LIGHTING);	
	glDisable(GL_LIGHT0);
}


