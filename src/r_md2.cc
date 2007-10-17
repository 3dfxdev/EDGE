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
#include "epi/endianess.h"

#include "r_md2.h"
#include "r_gldefs.h"
#include "r_misc.h"
#include "r_state.h"
#include "r_units.h"
#include "m_math.h"

#include "dm_state.h"  //!!!! game_dir

#include "m_misc.h"  //!!!! M_OpenComposedEPIFile

extern int leveltime; //!!!!


// #define DEBUG_MD2_LOAD  1


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
	s32_t num_vertices;  // per frame
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
	u8_t x, y, z;
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

//	raw_md2_vertex_t verts[1];  /* variable sized */
} 
raw_md2_frame_t;

typedef struct
{
	char name[64];
}
raw_md2_skin_t;


/* ---- normals ---- */

#define MD2_NUM_NORMALS  162

static vec3_t md2_normals[MD2_NUM_NORMALS] =
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
	md2_model_c(int _nframe, int _npoint, int _nstrip) :
		num_frames(_nframe), num_points(_npoint),
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
	if ( /* (header.ident != 844121161) || */
		(header.version != 8))
	{
		I_Error("MD2_LoadModel: bad header or version!");
		return NULL; /* NOT REACHED */
	}

	int num_frames = EPI_LE_S32(header.num_frames);
	int num_points = 0;
	int num_strips = 0;

	/* PARSE GLCMDS */

	int num_glcmds = EPI_LE_S32(header.num_glcmds);

	s32_t *glcmds = new s32_t[num_glcmds];

	f->Seek(EPI_LE_S32(header.ofs_glcmds), epi::file_c::SEEKPOINT_START);
	f->Read(glcmds, num_glcmds * sizeof(s32_t));

	for (int aa = 0; aa < num_glcmds; aa++)
		glcmds[aa] = EPI_LE_S32(glcmds[aa]);

	// determine total number of strips and points
	for (i = 0; i < num_glcmds && glcmds[i] != 0; )
	{
		int count = glcmds[i++];

		if (count < 0)
			count = -count;

		num_strips += 1;
		num_points += count;

		i += count*3;
	}

	L_WriteDebug("MODEL INFO:\n");
	L_WriteDebug("  frames:%d  points:%d  strips: %d\n",
			num_frames, num_points, num_strips);

	md2_model_c *md = new md2_model_c(num_frames, num_points, num_strips);

	md->verts_per_frame = EPI_LE_S32(header.num_vertices);

	L_WriteDebug("  verts_per_frame:%d  glcmds:%d\n", md->verts_per_frame, num_glcmds);

	// convert glcmds into strips and points
	md2_strip_c *strip = md->strips;
	md2_point_c *point = md->points;

	for (i = 0; i < num_glcmds && glcmds[i] != 0; )
	{
		int count = glcmds[i++];

		SYS_ASSERT(strip < md->strips + md->num_strips);
		SYS_ASSERT(point < md->points + md->num_points);

		strip->mode = (count < 0) ? GL_TRIANGLE_FAN : GL_TRIANGLE_STRIP;

		if (count < 0)
			count = -count;

		strip->count = count;
		strip->first = point - md->points;

		strip++;

		for (; count > 0; count--, point++, i += 3)
		{
			float *f_ptr = (float *) &glcmds[i];

			point->skin_s   = f_ptr[0];
			point->skin_t   = 1.0 - f_ptr[1];
			point->vert_idx = glcmds[i+2];

			SYS_ASSERT(point->vert_idx >= 0);
			SYS_ASSERT(point->vert_idx < md->verts_per_frame);
		}
	}

	SYS_ASSERT(strip == md->strips + md->num_strips);
	SYS_ASSERT(point == md->points + md->num_points);

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

#ifdef DEBUG_MD2_LOAD
		L_WriteDebug("  __FRAME_%d__\n", i);
		L_WriteDebug("    scale: %1.2f, %1.2f, %1.2f\n", scale[0], scale[1], scale[2]);
		L_WriteDebug("    translate: %1.2f, %1.2f, %1.2f\n", translate[0], translate[1], translate[2]);
#endif

		f->Read(raw_verts, md->verts_per_frame * sizeof(raw_md2_vertex_t));

		md->frames[i].vertices = new md2_vertex_c[md->verts_per_frame];

		for (int v = 0; v < md->verts_per_frame; v++)
		{
			raw_md2_vertex_t *raw_V  = raw_verts + v;
			md2_vertex_c     *good_V = md->frames[i].vertices + v;

			good_V->x = (int)raw_V->x * scale[0] + translate[0];
			good_V->y = (int)raw_V->y * scale[1] + translate[1];
			good_V->z = (int)raw_V->z * scale[2] + translate[2];

#ifdef DEBUG_MD2_LOAD
			L_WriteDebug("    __VERT_%d__\n", v);
			L_WriteDebug("      raw: %d,%d,%d\n", raw_V->x, raw_V->y, raw_V->z);
			L_WriteDebug("      normal: %d\n", raw_V->light_normal);
			L_WriteDebug("      good: %1.2f, %1.2f, %1.2f\n", good_V->x, good_V->y, good_V->z);
#endif
			good_V->normal_idx = raw_V->light_normal;
			
			SYS_ASSERT(good_V->normal_idx >= 0);
			SYS_ASSERT(good_V->normal_idx < MD2_NUM_NORMALS);
		}
	}

	delete[] raw_verts;

	return md;
}


/*============== MODEL RENDERING ====================*/


typedef struct
{
	md2_model_c *model;

	int frame;
	int strip;

	float R, G, B;
	float x, y, z;

	bool is_weapon;

	// scaling
	float xy_scale;
	float  z_scale;

	// mlook vectors
	vec2_t kx_mat;
	vec2_t kz_mat;

	// rotation vectors
	vec2_t rx_mat;
	vec2_t ry_mat;

	mobj_t *mo;
}
model_coord_data_t;


static void ModelCoordFunc(void *d, int v_idx,
		vec3_t *pos, float *rgb, vec2_t *texc,
		vec3_t *normal, vec3_t *lit_pos)
{
	const model_coord_data_t *data = (model_coord_data_t *)d;

	const md2_model_c *md = data->model;

	const md2_frame_c *frame = & md->frames[data->frame];
	const md2_strip_c *strip = & md->strips[data->strip];

	SYS_ASSERT(strip->first + v_idx >= 0);
	SYS_ASSERT(strip->first + v_idx < md->num_points);

	const md2_point_c *point = &md->points[strip->first + v_idx];
	const md2_vertex_c *vert = &frame->vertices[point->vert_idx];

	float x1 = vert->x * data->xy_scale;
	float y1 = vert->y * data->xy_scale;
	float z1 = vert->z * data-> z_scale;

	float x2 = x1 * data->kx_mat.x + z1 * data->kx_mat.y;
	float z2 = x1 * data->kz_mat.x + z1 * data->kz_mat.y;
    float y2 = y1;

	pos->x = data->x + x2 * data->rx_mat.x + y2 * data->rx_mat.y;
	pos->y = data->y + x2 * data->ry_mat.x + y2 * data->ry_mat.y;
	pos->z = data->z + z2;

	rgb[0] = data->R;
	rgb[1] = data->G;
	rgb[2] = data->B;

	texc->Set(point->skin_s, point->skin_t);

	float nx = md2_normals[vert->normal_idx].x;
	float ny = md2_normals[vert->normal_idx].y;
	float nz = md2_normals[vert->normal_idx].z;

	float nx2 = nx * data->kx_mat.x + nz * data->kx_mat.y;
	float nz2 = nx * data->kz_mat.x + nz * data->kz_mat.y;
	float ny2 = ny;

	normal->x = nx2 * data->rx_mat.x + ny2 * data->rx_mat.y;
	normal->y = nx2 * data->ry_mat.x + ny2 * data->ry_mat.y;
	normal->z = nz2;

	if (false) /// NORMAL LIGHTING
	{
//		float vx = data->mo->x - viewx;
//		float vy = data->mo->y - viewy;
//		float vz = data->mo->z - viewz;
//
//		float v_dist = sqrt(vx*vx + vy*vy + vz*vz);
//
//		vx /= v_dist;
//		vy /= v_dist;
//		vz /= v_dist;

		float vx = 0;
		float vy = 1;
		float vz = 0;

		vx *= normal->x;
		vy *= normal->y;
		vz *= normal->z;

		float n_dist = vx + vy + vz;

		n_dist = 0.5 - n_dist * 0.5;
		n_dist = CLAMP(n_dist, 0, 1);

		rgb[0] = n_dist;
		rgb[1] = n_dist;
		rgb[2] = n_dist;
	}

	*lit_pos = *pos;
}


void MD2_RenderModel(md2_model_c *md, GLuint skin_tex, int frame,
		             bool is_weapon, mobj_t *mo,
					 region_properties_t *props)
{
	// check if frame is valid
	if (frame < 0 || frame >= md->num_frames)
	{
I_Debugf("Render model: bad frame %d\n", frame);
		return;
	}

	int fuzzy = (mo->flags & MF_FUZZY);

	float trans = fuzzy ? FUZZY_TRANS : mo->visibility;

	int blending = BL_CullBack | (trans < 0.99 ? BL_Alpha : 0);


	model_coord_data_t data;

	data.mo = mo;
	data.model = md;
	data.frame = frame;

	data.R = fuzzy ? 0 : 1;
	data.G = fuzzy ? 0 : 1;
	data.B = fuzzy ? 0 : 1;

	data.x = mo->x;
	data.y = mo->y;
	data.z = mo->z + 24;

	if (is_weapon)
	{
		data.x = viewx + viewcos * 1;
		data.y = viewy + viewsin * 1;
		data.z = viewz + 1;
	}

	data.is_weapon = is_weapon;

	data.xy_scale = 1.0;
	data. z_scale = 1.0;

	M_Angle2Matrix(is_weapon ? ~mo->vertangle : 0, &data.kx_mat, &data.kz_mat);

	M_Angle2Matrix(~ mo->angle, &data.rx_mat, &data.ry_mat);

	R_ColmapPipe_AdjustLight(mo->state->bright);

	/* draw the model */
	for (int i = 0; i < md->num_strips; i++)
	{
		data.strip = i;

  		R_RunPipeline(md->strips[i].mode, md->strips[i].count,
  				      skin_tex, trans, blending, PIPEF_NONE, //!!!!!
  					  &data, (pipeline_coord_func_t) ModelCoordFunc);
	}

	R_ColmapPipe_AdjustLight(0);
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
