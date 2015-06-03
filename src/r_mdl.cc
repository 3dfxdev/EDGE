
/*
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    Half-Life Model Renderer (Experimental) Copyright (C) 2001 James 'Ender' Brown [ender@quakesrc.org] This program is
    free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.
    This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
    warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
    details. You should have received a copy of the GNU General Public License along with this program; if not, write
    to the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. fromquake.h -

	render.c - apart from calculations (mostly range checking or value conversion code is a mix of standard Quake 1
	meshing, and vertex deforms. The rendering loop uses standard Quake 1 drawing, after SetupBones deforms the vertex.
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



  Also, please note that it won't do all hl models....
  Nor will it work 100%
 */
/* 
#include "i_defs.h"
#include "i_defs_gl.h"

#include "../epi/types.h"
#include "../epi/endianess.h" */

#include "r_mdl.h"
//#include "r_md2.h"
/* #include "r_gldefs.h"
#include "r_colormap.h"
#include "r_effects.h"
#include "r_image.h"
#include "r_misc.h"
#include "r_modes.h"
#include "r_state.h"
#include "r_shader.h"
#include "r_units.h"
#include "p_blockmap.h"
#include "m_math.h" */


#define MAX_BONES 128


//matrix3x4 transform_matrix[MAX_BONES];	/* Vertex transformation matrix */

//void GL_Draw_HL_AliasFrame(short *order, vec3_t *transformed, float tex_w, float tex_h);
//void above need referencing somewhere else?



//3DGE COPY FRAMES TO DDF (UNUSED)
/* static const char *CopyFrameName16(char *name16)
{
	char *str = new char[20];
//memcpy(str, frm->name, 16);
	memcpy(str, name16, 16);

	// ensure it is NUL terminated
	str[16] = 0;

	return str;
} */

/*
 =======================================================================================================================
    Mod_LoadHLModel - read in the model's constituent parts
 =======================================================================================================================
 */
mdl_model_c *MDL_LoadModel(epi::file_c *f)
{
    /*~~*/
    int i;
	
	mdl_header_t header;

	/* read header */
	f->Read(&header, sizeof (mdl_header_t));

	int version = EPI_LE_S32(header.version);
	
	I_Debugf("MODEL IDENT: [%c%c%c%c] VERSION: %d",
			 header.ident[0], header.ident[1],
			 header.ident[2], header.ident[3], version);
	
	
	if (strncmp(header.ident, MDL_IDENTIFIER, 10) != 0)
	{
		I_Error("MDL_LoadModel: lump is not an MDL model!");
		return NULL; /* NOT REACHED */
	}
			
	if (version != MDL_VERSION)
	{
		I_Error("MDL_LoadModel: strange version!");
		return NULL; /* NOT REACHED */
	}
	
		// if (strncmp(header.ident, numcontrollers > MAX_BONE_CONTROLLERS)
	// {
		// I_Error("Cannot load model %s - too many controllers %i\n");//, mod->name, header->numcontrollers);
		// return NULL; /* NOT REACHED */
		//return false;
	// }/* 
	//if (strncmp(header.ident, MAX_BONES);
	
	//	I_Error("Cannot load model %s - bones %i\n");//, mod->name, header->numbones);
	//	return NULL; /* NOT REACHED */
		//return false;
	//}
	
	int num_frames = EPI_LE_S32(header.numseq);
//	int num_points = 0;
//	int num_strips = 0;
//

	//int					start, end, total;
    /*~~*/


	//checksum the model

	/* PARSE GLCMDS */

/* 	int num_glcmds = EPI_LE_S32(header.num_glcmds);

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

	I_Debugf("  frames:%d  points:%d  strips: %d\n",
			num_frames, num_points, num_strips);

	mdl_model_c *md = new mdl_model_c(num_frames, num_points, num_strips);

	md->verts_per_frame = EPI_LE_S32(header.num_vertices); */

//	I_Debugf("  verts_per_frame:%d  glcmds:%d\n", md->verts_per_frame, num_glcmds);
	
	
	// convert glcmds into strips and points
/* 	mod_strip_c *strip = md->strips;
	mod_point_c *point = md->points;

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
	} */

	//SYS_ASSERT(strip == md->strips + md->num_strips);
	//SYS_ASSERT(point == md->points + md->num_points);

	//delete[] glcmds;
	
	
	/* PARSE FRAMES */
	
}
	/*
	//start = Hunk_LowMark ();


	//load the model into hunk
	//model = Hunk_Alloc(sizeof(hlmodelcache_t)); REMEMBER hlmodelcache_t!!

	//header = Hunk_Alloc(com_filesize);
	//memcpy(header, buffer, com_filesize); */


	// if (header->version != 10)
	// {
		// I_Printf(CON_ERROR "Cannot load model %s - unknown version %i\n", mod->name, header->version);
		// Hunk_FreeToLowMark(start);
		// return false;
	// }
/* 
	if (header->numcontrollers > MAX_BONE_CONTROLLERS)
	{
		I_Printf(CON_ERROR "Cannot load model %s - too many controllers %i\n", mod->name, header->numcontrollers);
		Hunk_FreeToLowMark(start);
		return false;
	}
	if (header->numbones > MAX_BONES)
	{
		I_Printf(CON_ERROR "Cannot load model %s - too many bones %i\n", mod->name, header->numbones);
		Hunk_FreeToLowMark(start);
		return false;
	}

	texheader = NULL;
	if (!header->numtextures)
	{
		char texmodelname[MAX_QPATH];
		COM_StripExtension(mod->name, texmodelname, sizeof(texmodelname));
		//no textures? eesh. They must be stored externally.
		texheader = (hlmdl_header_t*)COM_LoadHunkFile(va("%st.mdl", texmodelname));
		if (texheader)
		{
			if (texheader->version != 10)
				texheader = NULL;
		}
	}

	if (!texheader)
		texheader = header;
	else
		header->numtextures = texheader->numtextures; */

	//tex = (hlmdl_tex_t *) ((byte *) texheader + texheader->textures);
    //bones = (hlmdl_bone_t *) ((byte *) header + header->boneindex);
    //bonectls = (hlmdl_bonecontroller_t *) ((byte *) header + header->controllerindex);


/*	won't work - doesn't know exact sizes.

	header = Hunk_Alloc(sizeof(hlmdl_header_t));
	memcpy(header, (hlmdl_header_t *) buffer, sizeof(hlmdl_header_t));

	tex = Hunk_Alloc(sizeof(hlmdl_tex_t)*header->numtextures);
	memcpy(tex, (hlmdl_tex_t *) buffer, sizeof(hlmdl_tex_t)*header->numtextures);

	bones = Hunk_Alloc(sizeof(hlmdl_bone_t)*header->numtextures);
	memcpy(bones, (hlmdl_bone_t *) buffer, sizeof(hlmdl_bone_t)*header->numbones);

	bonectls = Hunk_Alloc(sizeof(hlmdl_bonecontroller_t)*header->numcontrollers);
	memcpy(bonectls, (hlmdl_bonecontroller_t *) buffer, sizeof(hlmdl_bonecontroller_t)*header->numcontrollers);
*/
/* 
	model->header = (char *)header - (char *)model;
	model->texheader = (char *)texheader - (char *)model;
	model->textures = (char *)tex - (char *)model;
	model->bones = (char *)bones - (char *)model;
	model->bonectls = (char *)bonectls - (char *)model;

	texnums = Hunk_Alloc(texheader->numtextures*sizeof(model->texnums));
	model->texnums = (char *)texnums - (char *)model;
    for(i = 0; i < texheader->numtextures; i++)
    {
        texnums[i] = GL_LoadTexture8Pal24("", tex[i].w, tex[i].h, (byte *) texheader + tex[i].offset, (byte *) texheader + tex[i].w * tex[i].h + tex[i].offset, IF_NOALPHA|IF_NOGAMMA);
    } */
