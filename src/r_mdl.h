//----------------------------------------------------------------------------
//  Half-Life Models (MDL)
//----------------------------------------------------------------------------
//
//  Copyright (c) 2015 Isotope SoftWorks
//  Based on FTEQuake's implementation
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
 

#include "../epi/file.h"

#include "r_defs.h"
#include "p_mobj.h"

// opaque handle for rest of the engine
class mdl_model_c;

mdl_model_c *MDL_LoadModel(epi::file_c *f); 


/*============== FORMAT DEFINITIONS ====================*/
// format uses float pointing values, but to allow for endianness
// conversions they are represented here as unsigned integers.
typedef u32_t f32_t;
#define MDL_IDENTIFIER  "IDSP"
#define MDL_VERSION     10
// format uses float pointing values, but to allow for endianness
// conversions they are represented here as unsigned integers.
typedef struct
{
    int		filetypeid;	//IDSP
	char ident[10];
	
	int		version;	//10
	s32_t version;
	
    char	name[64];
    int		filesize;
    vec3_t	unknown3[5];
    int		unknown4;
    s32_t		numbones;
    s32_t		boneindex;
    s32_t		numcontrollers;
    s32_t		controllerindex;
    s32_t		unknown5[2];
    s32_t	    numseq;
    s32_t		seqindex;
    s32_t		unknown6;
    s32_t		seqgroups;
    s32_t		numtextures;
    s32_t		textures;
    s32_t		unknown7[3];
    s32_t		skins;
    s32_t		numbodyparts;
    s32_t		bodypartindex;
    s32_t	unknown9[8];
} mdl_header_t;

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
//void	R_DrawHLModel(entity_t	*curent);
//*mod to *md
/* void MDL_RenderModel(mdl_model_c *md, const image_c * skin_img, bool is_weapon,
		             int frame1, int frame2, float lerp,
		             float x, float y, float z, mobj_t *mo,
					 region_properties_t *props,
					 float scale, float aspect, float bias);

void MDL_RenderModel_2D(mdl_model_c *md, const image_c * skin_img, int frame,
		                float x, float y, float xscale, float yscale,
		                const mobjtype_c *info); */
