//----------------------------------------------------------------------------
//  MD2 Models
//----------------------------------------------------------------------------
//
//  Copyright (c) 2002-2007  The EDGE Team.
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
//  Based on MD2 loading and rendering code (C) 2004 David Henry.
//
//----------------------------------------------------------------------------

#include "i_defs.h"
#include "i_defs_gl.h"

#include "epi/types.h"

#include "r_md2.h"


/*============== FORMAT DEFINITIONS ====================*/


// format uses float pointing values, but to allow for endianness
// conversions they are represented here as unsigned integers.
typedef u32_t f32_t;

#define MD2_IDENTIFIER  "IDP2"

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
	s8_t v[3];
	u8_t light_normal;
} 
raw_md2_vertex_t;

typedef struct
{
	f32_t s, t;
	s32_t vert_index;
}
raw_md2_glcmd_t;

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

static float md2_normals[MD2_NUM_NORMALS][3] =
{
	{-0.525731f,  0.000000f,  0.850651f},
	{-0.442863f,  0.238856f,  0.864188f},
	{-0.295242f,  0.000000f,  0.955423f},
	{-0.309017f,  0.500000f,  0.809017f},
	{-0.162460f,  0.262866f,  0.951056f},
	{ 0.000000f,  0.000000f,  1.000000f},
	{ 0.000000f,  0.850651f,  0.525731f},
	{-0.147621f,  0.716567f,  0.681718f},
	{ 0.147621f,  0.716567f,  0.681718f},
	{ 0.000000f,  0.525731f,  0.850651f},
	{ 0.309017f,  0.500000f,  0.809017f},
	{ 0.525731f,  0.000000f,  0.850651f},
	{ 0.295242f,  0.000000f,  0.955423f},
	{ 0.442863f,  0.238856f,  0.864188f},
	{ 0.162460f,  0.262866f,  0.951056f},
	{-0.681718f,  0.147621f,  0.716567f},
	{-0.809017f,  0.309017f,  0.500000f},
	{-0.587785f,  0.425325f,  0.688191f},
	{-0.850651f,  0.525731f,  0.000000f},
	{-0.864188f,  0.442863f,  0.238856f},
	{-0.716567f,  0.681718f,  0.147621f},
	{-0.688191f,  0.587785f,  0.425325f},
	{-0.500000f,  0.809017f,  0.309017f},
	{-0.238856f,  0.864188f,  0.442863f},
	{-0.425325f,  0.688191f,  0.587785f},
	{-0.716567f,  0.681718f, -0.147621f},
	{-0.500000f,  0.809017f, -0.309017f},
	{-0.525731f,  0.850651f,  0.000000f},
	{ 0.000000f,  0.850651f, -0.525731f},
	{-0.238856f,  0.864188f, -0.442863f},
	{ 0.000000f,  0.955423f, -0.295242f},
	{-0.262866f,  0.951056f, -0.162460f},
	{ 0.000000f,  1.000000f,  0.000000f},
	{ 0.000000f,  0.955423f,  0.295242f},
	{-0.262866f,  0.951056f,  0.162460f},
	{ 0.238856f,  0.864188f,  0.442863f},
	{ 0.262866f,  0.951056f,  0.162460f},
	{ 0.500000f,  0.809017f,  0.309017f},
	{ 0.238856f,  0.864188f, -0.442863f},
	{ 0.262866f,  0.951056f, -0.162460f},
	{ 0.500000f,  0.809017f, -0.309017f},
	{ 0.850651f,  0.525731f,  0.000000f},
	{ 0.716567f,  0.681718f,  0.147621f},
	{ 0.716567f,  0.681718f, -0.147621f},
	{ 0.525731f,  0.850651f,  0.000000f},
	{ 0.425325f,  0.688191f,  0.587785f},
	{ 0.864188f,  0.442863f,  0.238856f},
	{ 0.688191f,  0.587785f,  0.425325f},
	{ 0.809017f,  0.309017f,  0.500000f},
	{ 0.681718f,  0.147621f,  0.716567f},
	{ 0.587785f,  0.425325f,  0.688191f},
	{ 0.955423f,  0.295242f,  0.000000f},
	{ 1.000000f,  0.000000f,  0.000000f},
	{ 0.951056f,  0.162460f,  0.262866f},
	{ 0.850651f, -0.525731f,  0.000000f},
	{ 0.955423f, -0.295242f,  0.000000f},
	{ 0.864188f, -0.442863f,  0.238856f},
	{ 0.951056f, -0.162460f,  0.262866f},
	{ 0.809017f, -0.309017f,  0.500000f},
	{ 0.681718f, -0.147621f,  0.716567f},
	{ 0.850651f,  0.000000f,  0.525731f},
	{ 0.864188f,  0.442863f, -0.238856f},
	{ 0.809017f,  0.309017f, -0.500000f},
	{ 0.951056f,  0.162460f, -0.262866f},
	{ 0.525731f,  0.000000f, -0.850651f},
	{ 0.681718f,  0.147621f, -0.716567f},
	{ 0.681718f, -0.147621f, -0.716567f},
	{ 0.850651f,  0.000000f, -0.525731f},
	{ 0.809017f, -0.309017f, -0.500000f},
	{ 0.864188f, -0.442863f, -0.238856f},
	{ 0.951056f, -0.162460f, -0.262866f},
	{ 0.147621f,  0.716567f, -0.681718f},
	{ 0.309017f,  0.500000f, -0.809017f},
	{ 0.425325f,  0.688191f, -0.587785f},
	{ 0.442863f,  0.238856f, -0.864188f},
	{ 0.587785f,  0.425325f, -0.688191f},
	{ 0.688191f,  0.587785f, -0.425325f},
	{-0.147621f,  0.716567f, -0.681718f},
	{-0.309017f,  0.500000f, -0.809017f},
	{ 0.000000f,  0.525731f, -0.850651f},
	{-0.525731f,  0.000000f, -0.850651f},
	{-0.442863f,  0.238856f, -0.864188f},
	{-0.295242f,  0.000000f, -0.955423f},
	{-0.162460f,  0.262866f, -0.951056f},
	{ 0.000000f,  0.000000f, -1.000000f},
	{ 0.295242f,  0.000000f, -0.955423f},
	{ 0.162460f,  0.262866f, -0.951056f},
	{-0.442863f, -0.238856f, -0.864188f},
	{-0.309017f, -0.500000f, -0.809017f},
	{-0.162460f, -0.262866f, -0.951056f},
	{ 0.000000f, -0.850651f, -0.525731f},
	{-0.147621f, -0.716567f, -0.681718f},
	{ 0.147621f, -0.716567f, -0.681718f},
	{ 0.000000f, -0.525731f, -0.850651f},
	{ 0.309017f, -0.500000f, -0.809017f},
	{ 0.442863f, -0.238856f, -0.864188f},
	{ 0.162460f, -0.262866f, -0.951056f},
	{ 0.238856f, -0.864188f, -0.442863f},
	{ 0.500000f, -0.809017f, -0.309017f},
	{ 0.425325f, -0.688191f, -0.587785f},
	{ 0.716567f, -0.681718f, -0.147621f},
	{ 0.688191f, -0.587785f, -0.425325f},
	{ 0.587785f, -0.425325f, -0.688191f},
	{ 0.000000f, -0.955423f, -0.295242f},
	{ 0.000000f, -1.000000f,  0.000000f},
	{ 0.262866f, -0.951056f, -0.162460f},
	{ 0.000000f, -0.850651f,  0.525731f},
	{ 0.000000f, -0.955423f,  0.295242f},
	{ 0.238856f, -0.864188f,  0.442863f},
	{ 0.262866f, -0.951056f,  0.162460f},
	{ 0.500000f, -0.809017f,  0.309017f},
	{ 0.716567f, -0.681718f,  0.147621f},
	{ 0.525731f, -0.850651f,  0.000000f},
	{-0.238856f, -0.864188f, -0.442863f},
	{-0.500000f, -0.809017f, -0.309017f},
	{-0.262866f, -0.951056f, -0.162460f},
	{-0.850651f, -0.525731f,  0.000000f},
	{-0.716567f, -0.681718f, -0.147621f},
	{-0.716567f, -0.681718f,  0.147621f},
	{-0.525731f, -0.850651f,  0.000000f},
	{-0.500000f, -0.809017f,  0.309017f},
	{-0.238856f, -0.864188f,  0.442863f},
	{-0.262866f, -0.951056f,  0.162460f},
	{-0.864188f, -0.442863f,  0.238856f},
	{-0.809017f, -0.309017f,  0.500000f},
	{-0.688191f, -0.587785f,  0.425325f},
	{-0.681718f, -0.147621f,  0.716567f},
	{-0.442863f, -0.238856f,  0.864188f},
	{-0.587785f, -0.425325f,  0.688191f},
	{-0.309017f, -0.500000f,  0.809017f},
	{-0.147621f, -0.716567f,  0.681718f},
	{-0.425325f, -0.688191f,  0.587785f},
	{-0.162460f, -0.262866f,  0.951056f},
	{ 0.442863f, -0.238856f,  0.864188f},
	{ 0.162460f, -0.262866f,  0.951056f},
	{ 0.309017f, -0.500000f,  0.809017f},
	{ 0.147621f, -0.716567f,  0.681718f},
	{ 0.000000f, -0.525731f,  0.850651f},
	{ 0.425325f, -0.688191f,  0.587785f},
	{ 0.587785f, -0.425325f,  0.688191f},
	{ 0.688191f, -0.587785f,  0.425325f},
	{-0.955423f,  0.295242f,  0.000000f},
	{-0.951056f,  0.162460f,  0.262866f},
	{-1.000000f,  0.000000f,  0.000000f},
	{-0.850651f,  0.000000f,  0.525731f},
	{-0.955423f, -0.295242f,  0.000000f},
	{-0.951056f, -0.162460f,  0.262866f},
	{-0.864188f,  0.442863f, -0.238856f},
	{-0.951056f,  0.162460f, -0.262866f},
	{-0.809017f,  0.309017f, -0.500000f},
	{-0.864188f, -0.442863f, -0.238856f},
	{-0.951056f, -0.162460f, -0.262866f},
	{-0.809017f, -0.309017f, -0.500000f},
	{-0.681718f,  0.147621f, -0.716567f},
	{-0.681718f, -0.147621f, -0.716567f},
	{-0.850651f,  0.000000f, -0.525731f},
	{-0.688191f,  0.587785f, -0.425325f},
	{-0.587785f,  0.425325f, -0.688191f},
	{-0.425325f,  0.688191f, -0.587785f},
	{-0.425325f, -0.688191f, -0.587785f},
	{-0.587785f, -0.425325f, -0.688191f},
	{-0.688191f, -0.587785f, -0.425325f}  
};


/*============== EDGE REPRESENTATION ====================*/

class md2_vertex_c
{
public:
	float x, y, z;

	short normal_idx;

public:
	 md2_vertex_c() { }
	~md2_vertex_c() { }
};

class md2_frame_c
{
public:
///---	float scale[3];
///---	float translate[3];

	md2_vertex_c *verts;
};

class md2_point_c
{
public:
	// index into frame-specific vertex array
	int vertex_idx;

	float skin_s, skin_t;
};

class md2_strip_c
{
public:
	// either GL_TRIANGLE_STRIP or GL_TRIANGLE_FAN
	GLenum mode;

	// number of points in this strip / fan
	int count;

	// index to the first point (within md2_model_c::points).
	// All points for the strip are contiguous in that array.
	int first;
};


class md2_model_c
{
public:
	int num_frames;
	int num_strips;
	int num_points;

	md2_frame_c *frames;
	md2_point_c *points;
	md2_strip_c *strips;

public:
	md2_model_c()
	{ }

	~md2_model_c()
	{ /* !!!! FIXME */ }
};


/*============== LOADING CODE ====================*/

md2_model_c *MD2_LoadModel(epi::file_c *f)
{
	raw_md2_header_t header;

	/* read header */
	f->Read(&header, sizeof (raw_md2_header_t));

	if ((header.ident != 844121161) ||
		(header.version != 8))
	{
		I_Error("MD2_LoadModel: bad header or version!");
		return NULL;
	}

	/* memory allocation */
	mdl->texcoords = (md2_texCoord_t *)malloc (sizeof (md2_texCoord_t) * mdl->header.num_st);
	mdl->frames = (md2_frame_t *)malloc (sizeof(md2_frame_t) * mdl->header.num_frames);
	mdl->glcmds = (int *)malloc (sizeof (int) * mdl->header.num_glcmds);

	/* read model data */
	fseek (fp, mdl->header.offset_st, SEEK_SET);
	fread (mdl->texcoords, sizeof (md2_texCoord_t), mdl->header.num_st, fp);

	fseek (fp, mdl->header.offset_glcmds, SEEK_SET);
	fread (mdl->glcmds, sizeof (int), mdl->header.num_glcmds, fp);

	/* read frames */
	fseek (fp, mdl->header.offset_frames, SEEK_SET);
	for (i = 0; i < mdl->header.num_frames; ++i)
	{
		/* memory allocation for vertices of this frame */
		mdl->frames[i].verts = (md2_vertex_t *)
			malloc (sizeof (md2_vertex_t) * mdl->header.num_vertices);

		/* read frame data */
		fread (mdl->frames[i].scale, sizeof (vec3_t), 1, fp);
		fread (mdl->frames[i].translate, sizeof (vec3_t), 1, fp);
		fread (mdl->frames[i].name, sizeof (char), 16, fp);
		fread (mdl->frames[i].verts, sizeof (md2_vertex_t), mdl->header.num_vertices, fp);
	}

	return 1
}


/*============== MODEL RENDERING ====================*/

void MD2_RenderModel(md2_model_c *md, mobj_t *mo)
{
	int i, *pglcmds;

	static int n = -1;
	n = (n + 1) % md->frames.size();

	/* check if n is in a valid range */
	if (n < 0 || n >= md->frames.size())
		return;

	/* enable model's texture */
//!!!!	glBindTexture (GL_TEXTURE_2D, mdl->tex_id);

	/* pglcmds points at the start of the command list */
	pglcmds = mdl->glcmds;

	/* draw the model */
	while ((i = *(pglcmds++)) != 0)
	{
		if (i < 0)
		{
			glBegin(GL_TRIANGLE_FAN);
			i = -i;
		}
		else
		{
			glBegin(GL_TRIANGLE_STRIP);
		}

		/* draw each vertex of this group */

		for (; i > 0; i--, pglcmds += 3)
		{
			md2_gl_vert_t *packet = (md2_gl_vert_t *)pglcmds;
			md2_frame_t *pframe = &mdl->frames[n];
			md2_vertex_t *pvert = &pframe->verts[packet->index];

			/* pass texture coordinates to OpenGL */
//!!!!		glTexCoord2f(packet->s, packet->t);

			/* normal vector */
			glNormal3fv(md2_normals[pvert->normalIndex]);

			GLfloat v[3];

			/* calculate vertex real position */
			v[0] = pvert->v[0] * pframe->scale[0] + pframe->translate[0];
			v[1] = pvert->v[1] * pframe->scale[1] + pframe->translate[1];
			v[2] = pvert->v[2] * pframe->scale[2] + pframe->translate[2];

			v[0] += mo->x;
			v[1] += mo->y;
			v[2] += mo->z;

			glVertex3fv(v);
		}

		glEnd();
	}
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
