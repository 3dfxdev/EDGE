#ifndef _MD5_DRAW_H_
#define _MD5_DRAW_H_

#include "md5.h"

typedef struct {
	epi::vec3_c pos;
	float padding0;
	epi::vec3_c norm;
	float padding1;
	epi::vec2_c uv;
} basevert;

void md5_transform_vertices(MD5mesh *msh, epi::mat4_c *posemats, basevert *dst);
void render_md5_direct_triangle_fullbright(MD5mesh *msh, basevert *vbuff);
void render_md5_direct_triangle_lighting(MD5mesh *msh, basevert *vbuff);


#endif
