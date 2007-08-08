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
#include "epi/endianness.h"

#include "r_md2.h"

extern int leveltime; //!!!!


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

	s32_t verts_per_frame;

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
	s8_t x, y, z;
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
	{ -0.525731f,  0.000000f,  0.850651f },
	{ -0.442863f,  0.238856f,  0.864188f },
	{ -0.295242f,  0.000000f,  0.955423f },
	{ -0.309017f,  0.500000f,  0.809017f },
	{ -0.162460f,  0.262866f,  0.951056f },
	{  0.000000f,  0.000000f,  1.000000f },
	{  0.000000f,  0.850651f,  0.525731f },
	{ -0.147621f,  0.716567f,  0.681718f },
	{  0.147621f,  0.716567f,  0.681718f },
	{  0.000000f,  0.525731f,  0.850651f },
	{  0.309017f,  0.500000f,  0.809017f },
	{  0.525731f,  0.000000f,  0.850651f },
	{  0.295242f,  0.000000f,  0.955423f },
	{  0.442863f,  0.238856f,  0.864188f },
	{  0.162460f,  0.262866f,  0.951056f },
	{ -0.681718f,  0.147621f,  0.716567f },
	{ -0.809017f,  0.309017f,  0.500000f },
	{ -0.587785f,  0.425325f,  0.688191f },
	{ -0.850651f,  0.525731f,  0.000000f },
	{ -0.864188f,  0.442863f,  0.238856f },
	{ -0.716567f,  0.681718f,  0.147621f },
	{ -0.688191f,  0.587785f,  0.425325f },
	{ -0.500000f,  0.809017f,  0.309017f },
	{ -0.238856f,  0.864188f,  0.442863f },
	{ -0.425325f,  0.688191f,  0.587785f },
	{ -0.716567f,  0.681718f, -0.147621f },
	{ -0.500000f,  0.809017f, -0.309017f },
	{ -0.525731f,  0.850651f,  0.000000f },
	{  0.000000f,  0.850651f, -0.525731f },
	{ -0.238856f,  0.864188f, -0.442863f },
	{  0.000000f,  0.955423f, -0.295242f },
	{ -0.262866f,  0.951056f, -0.162460f },
	{  0.000000f,  1.000000f,  0.000000f },
	{  0.000000f,  0.955423f,  0.295242f },
	{ -0.262866f,  0.951056f,  0.162460f },
	{  0.238856f,  0.864188f,  0.442863f },
	{  0.262866f,  0.951056f,  0.162460f },
	{  0.500000f,  0.809017f,  0.309017f },
	{  0.238856f,  0.864188f, -0.442863f },
	{  0.262866f,  0.951056f, -0.162460f },
	{  0.500000f,  0.809017f, -0.309017f },
	{  0.850651f,  0.525731f,  0.000000f },
	{  0.716567f,  0.681718f,  0.147621f },
	{  0.716567f,  0.681718f, -0.147621f },
	{  0.525731f,  0.850651f,  0.000000f },
	{  0.425325f,  0.688191f,  0.587785f },
	{  0.864188f,  0.442863f,  0.238856f },
	{  0.688191f,  0.587785f,  0.425325f },
	{  0.809017f,  0.309017f,  0.500000f },
	{  0.681718f,  0.147621f,  0.716567f },
	{  0.587785f,  0.425325f,  0.688191f },
	{  0.955423f,  0.295242f,  0.000000f },
	{  1.000000f,  0.000000f,  0.000000f },
	{  0.951056f,  0.162460f,  0.262866f },
	{  0.850651f, -0.525731f,  0.000000f },
	{  0.955423f, -0.295242f,  0.000000f },
	{  0.864188f, -0.442863f,  0.238856f },
	{  0.951056f, -0.162460f,  0.262866f },
	{  0.809017f, -0.309017f,  0.500000f },
	{  0.681718f, -0.147621f,  0.716567f },
	{  0.850651f,  0.000000f,  0.525731f },
	{  0.864188f,  0.442863f, -0.238856f },
	{  0.809017f,  0.309017f, -0.500000f },
	{  0.951056f,  0.162460f, -0.262866f },
	{  0.525731f,  0.000000f, -0.850651f },
	{  0.681718f,  0.147621f, -0.716567f },
	{  0.681718f, -0.147621f, -0.716567f },
	{  0.850651f,  0.000000f, -0.525731f },
	{  0.809017f, -0.309017f, -0.500000f },
	{  0.864188f, -0.442863f, -0.238856f },
	{  0.951056f, -0.162460f, -0.262866f },
	{  0.147621f,  0.716567f, -0.681718f },
	{  0.309017f,  0.500000f, -0.809017f },
	{  0.425325f,  0.688191f, -0.587785f },
	{  0.442863f,  0.238856f, -0.864188f },
	{  0.587785f,  0.425325f, -0.688191f },
	{  0.688191f,  0.587785f, -0.425325f },
	{ -0.147621f,  0.716567f, -0.681718f },
	{ -0.309017f,  0.500000f, -0.809017f },
	{  0.000000f,  0.525731f, -0.850651f },
	{ -0.525731f,  0.000000f, -0.850651f },
	{ -0.442863f,  0.238856f, -0.864188f },
	{ -0.295242f,  0.000000f, -0.955423f },
	{ -0.162460f,  0.262866f, -0.951056f },
	{  0.000000f,  0.000000f, -1.000000f },
	{  0.295242f,  0.000000f, -0.955423f },
	{  0.162460f,  0.262866f, -0.951056f },
	{ -0.442863f, -0.238856f, -0.864188f },
	{ -0.309017f, -0.500000f, -0.809017f },
	{ -0.162460f, -0.262866f, -0.951056f },
	{  0.000000f, -0.850651f, -0.525731f },
	{ -0.147621f, -0.716567f, -0.681718f },
	{  0.147621f, -0.716567f, -0.681718f },
	{  0.000000f, -0.525731f, -0.850651f },
	{  0.309017f, -0.500000f, -0.809017f },
	{  0.442863f, -0.238856f, -0.864188f },
	{  0.162460f, -0.262866f, -0.951056f },
	{  0.238856f, -0.864188f, -0.442863f },
	{  0.500000f, -0.809017f, -0.309017f },
	{  0.425325f, -0.688191f, -0.587785f },
	{  0.716567f, -0.681718f, -0.147621f },
	{  0.688191f, -0.587785f, -0.425325f },
	{  0.587785f, -0.425325f, -0.688191f },
	{  0.000000f, -0.955423f, -0.295242f },
	{  0.000000f, -1.000000f,  0.000000f },
	{  0.262866f, -0.951056f, -0.162460f },
	{  0.000000f, -0.850651f,  0.525731f },
	{  0.000000f, -0.955423f,  0.295242f },
	{  0.238856f, -0.864188f,  0.442863f },
	{  0.262866f, -0.951056f,  0.162460f },
	{  0.500000f, -0.809017f,  0.309017f },
	{  0.716567f, -0.681718f,  0.147621f },
	{  0.525731f, -0.850651f,  0.000000f },
	{ -0.238856f, -0.864188f, -0.442863f },
	{ -0.500000f, -0.809017f, -0.309017f },
	{ -0.262866f, -0.951056f, -0.162460f },
	{ -0.850651f, -0.525731f,  0.000000f },
	{ -0.716567f, -0.681718f, -0.147621f },
	{ -0.716567f, -0.681718f,  0.147621f },
	{ -0.525731f, -0.850651f,  0.000000f },
	{ -0.500000f, -0.809017f,  0.309017f },
	{ -0.238856f, -0.864188f,  0.442863f },
	{ -0.262866f, -0.951056f,  0.162460f },
	{ -0.864188f, -0.442863f,  0.238856f },
	{ -0.809017f, -0.309017f,  0.500000f },
	{ -0.688191f, -0.587785f,  0.425325f },
	{ -0.681718f, -0.147621f,  0.716567f },
	{ -0.442863f, -0.238856f,  0.864188f },
	{ -0.587785f, -0.425325f,  0.688191f },
	{ -0.309017f, -0.500000f,  0.809017f },
	{ -0.147621f, -0.716567f,  0.681718f },
	{ -0.425325f, -0.688191f,  0.587785f },
	{ -0.162460f, -0.262866f,  0.951056f },
	{  0.442863f, -0.238856f,  0.864188f },
	{  0.162460f, -0.262866f,  0.951056f },
	{  0.309017f, -0.500000f,  0.809017f },
	{  0.147621f, -0.716567f,  0.681718f },
	{  0.000000f, -0.525731f,  0.850651f },
	{  0.425325f, -0.688191f,  0.587785f },
	{  0.587785f, -0.425325f,  0.688191f },
	{  0.688191f, -0.587785f,  0.425325f },
	{ -0.955423f,  0.295242f,  0.000000f },
	{ -0.951056f,  0.162460f,  0.262866f },
	{ -1.000000f,  0.000000f,  0.000000f },
	{ -0.850651f,  0.000000f,  0.525731f },
	{ -0.955423f, -0.295242f,  0.000000f },
	{ -0.951056f, -0.162460f,  0.262866f },
	{ -0.864188f,  0.442863f, -0.238856f },
	{ -0.951056f,  0.162460f, -0.262866f },
	{ -0.809017f,  0.309017f, -0.500000f },
	{ -0.864188f, -0.442863f, -0.238856f },
	{ -0.951056f, -0.162460f, -0.262866f },
	{ -0.809017f, -0.309017f, -0.500000f },
	{ -0.681718f,  0.147621f, -0.716567f },
	{ -0.681718f, -0.147621f, -0.716567f },
	{ -0.850651f,  0.000000f, -0.525731f },
	{ -0.688191f,  0.587785f, -0.425325f },
	{ -0.587785f,  0.425325f, -0.688191f },
	{ -0.425325f,  0.688191f, -0.587785f },
	{ -0.425325f, -0.688191f, -0.587785f },
	{ -0.587785f, -0.425325f, -0.688191f },
	{ -0.688191f, -0.587785f, -0.425325f }  
};


/*============== EDGE REPRESENTATION ====================*/

struct md2_vertex_c
{
	float x, y, z;

	short normal_idx;
};

struct md2_frame_c
{
///---	float scale[3];
///---	float translate[3];

	md2_vertex_c *vertices;
};

struct md2_point_c
{
	float skin_s, skin_t;

	// index into frame's vertex array (md2_frame_c::verts)
	int vert_idx;
};

struct md2_strip_c
{
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
	int num_points;
	int num_strips;

	md2_frame_c *frames;
	md2_point_c *points;
	md2_strip_c *strips;

	int verts_per_frame;

public:
	md2_model_c(int _nframe, int _npoints, int _nstrip) :
		num_frames(_nframes), num_points(_npoints),
		num_strips(_nstrip), verts_per_frame(0)
	{
		frames = new md2_frame_c[num_frames];
		points = new md2_point_c[num_points];
		strips = new md2_strip_c[num_strips];
	}

	~md2_model_c()
	{
		delete[] frames;
		delete[] points;
		delete[] strips;
	}
};


/*============== LOADING CODE ====================*/

md2_model_c *MD2_LoadModel(epi::file_c *f)
{
	int i;

	raw_md2_header_t header;

	/* read header */
	f->Read(&header, sizeof (raw_md2_header_t));

	//!!!! FIXME:
	if ((header.ident != 844121161) ||
		(header.version != 8))
	{
		I_Error("MD2_LoadModel: bad header or version!");
		return NULL;
	}

	int num_frames = EPI_LE_S32(header.num_frames);
	int num_points = 0;
	int num_strips = 0;

	/* PARSE GLCMDS */

	int num_glcmds = EPI_LE_S32(header.num_glcmds);

	sint32_t *glcmds = new sint32_t[num_glcmds];

	f->Seek(EPI_LE_S32(header.ofs_glcmds), epi::file_c::SEEKPOINT_START);
	f->Read(glcmds, sizeof(sint32_t) * num_glcmds);

	// determine total number of strips and points
	for (i = 0; i < num_glcmds && glcmds[i] != 0; )
	{
		int count = EPI_LE_S32(glcmds[i]);

		i++;

		if (count < 0)
			count = -count;

		num_strips += 1;
		num_points += count;

		i += count*3;
	}

	md2_model_c *md = new md2_model_c(num_frames, num_points, num_strips);

	md->verts_per_frame = EPI_LE_S32(header.verts_per_frame);

	// convert glcmds into strips and points
	int strip_idx = 0;
	int point_idx = 0;

	for (i = 0; i < num_glcmds && glcmds[i] != 0; )
	{
		int count = EPI_LE_S32(glcmds[i]);

		i++;

		md2_strip_c *strip = md->strips + strip_idx;
		md2_point_c *point = md->points + point_idx;

		strip->mode = (count < 0) ? GL_TRIANGLE_FAN : GL_TRIANGLE_STRIP;

		if (count < 0)
			count = -count;

		strip->count = count;
		strip->first = point_idx;

		for (; count > 0; count--, point++, i += 3)
		{
			glcmds[i+0] = EPI_LE_U32(glcmds[i+0]);
			glcmds[i+1] = EPI_LE_U32(glcmds[i+1]);
			glcmds[i+2] = EPI_LE_U32(glcmds[i+2]);

			float *f_ptr = (float *) &glcmds[i];

			point->skin_s   = f_ptr[0];
			point->skin_t   = f_ptr[1];
			point->vert_idx = glcmds[i+2];
		}
	}

	delete[] glcmds;

	/* PARSE FRAMES */

	raw_md2_vertex_t *raw_verts = new raw_md2_vertex_t[md->verts_per_frame];

	f->Seek(EPI_LE_S32(header.ofs_frames), epi::file_c::SEEKPOINT_START);

	for (i = 0; i < num_frames; i++)
	{
		raw_md2_frame_t raw_frame;

		f->Read(&raw_frame, sizeof(raw_frame));

		for (int j = 0; j < 3; j++)
		{
			raw_frame.scale[j]     = EPI_LE_U32(raw_frame.scale[j]);
			raw_frame.translate[j] = EPI_LE_U32(raw_frame.translate[j]);
		}

		float *f_ptr = (float *) raw_frame.scale;

		float scale[3];
		float translate[3];

		scale[0] = f_ptr[0];
		scale[1] = f_ptr[1];
		scale[2] = f_ptr[2];

		translate[0] = f_ptr[3];
		translate[1] = f_ptr[4];
		translate[2] = f_ptr[5];

		f->Read(raw_verts, sizeof(raw_md2_vertex_t) * md->verts_per_frame);

		md->frames[i].vertices = new md2_vertex_c[md->verts_per_frame];

		for (int v = 0; v < md->verts_per_frame; v++)
		{
			raw_vertex_t *raw_V  = raw_verts + v;
			md2_vertex_c *good_V = md->frames[i].vertices + v;

			good_V->x = (int)raw_V->x * scale[0] + translate[0];
			good_V->y = (int)raw_V->y * scale[1] + translate[1];
			good_V->z = (int)raw_V->z * scale[2] + translate[2];

			good_V->normal_idx = raw_V->light_normal;
		}
	}

	delete[] raw_verts;

	return md;
}


/*============== MODEL RENDERING ====================*/

void MD2_RenderModel(md2_model_c *md, mobj_t *mo)
{
	int i, *pglcmds;

	int n = leveltime % md->num_frames;

	/* check if n is in a valid range */
	if (n < 0 || n >= md->frames.size())
		return;

	/* enable model's texture */
//!!!!	glBindTexture (GL_TEXTURE_2D, mdl->tex_id);

	/* pglcmds points at the start of the command list */
	pglcmds = mdl->glcmds;

	/* draw the model */
	for (int i = 0; i < md->num_strips; i++)
	{
		glBegin(md->strips[i].mode);

		/* draw each vertex of this strip */

		for (int k = 0; k < md->strips[i].count; k++)
		{
			md2_point_c *point = md->points[md->strips[i].first + k];
			md2_vertex_c *vert = md->frames[n].vertices[point->vert_idx];
				
      		glTexCoord2f(point->skin_s, point->skin_t);

			glNormal3fv(md2_normals[vert->normal_idx]);

			float x = mo->x + vert->x;
			float y = mo->y + vert->y;
			float z = mo->z + vert->z;
				
			glVertex3f(x, y, z);
		}

		glEnd();
	}
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
