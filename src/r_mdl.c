
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

#include "i_defs.h"
#include "i_defs_gl.h"

#include "../epi/types.h"
#include "../epi/endianess.h"

#include "r_mdl.h"
#include "r_gldefs.h"
#include "r_colormap.h"
#include "r_effects.h"
#include "r_image.h"
#include "r_misc.h"
#include "r_modes.h"
#include "r_state.h"
#include "r_shader.h"
#include "r_units.h"
#include "p_blockmap.h"
#include "m_math.h"

void QuaternionGLMatrix(float x, float y, float z, float w, vec4_t *GLM)
{
    GLM[0][0] = 1 - 2 * y * y - 2 * z * z;
    GLM[1][0] = 2 * x * y + 2 * w * z;
    GLM[2][0] = 2 * x * z - 2 * w * y;
    GLM[0][1] = 2 * x * y - 2 * w * z;
    GLM[1][1] = 1 - 2 * x * x - 2 * z * z;
    GLM[2][1] = 2 * y * z + 2 * w * x;
    GLM[0][2] = 2 * x * z + 2 * w * y;
    GLM[1][2] = 2 * y * z - 2 * w * x;
    GLM[2][2] = 1 - 2 * x * x - 2 * y * y;
}

/*
 =======================================================================================================================
    QuaternionGLAngle - Convert a GL angle to a quaternion matrix
 =======================================================================================================================
 */
void QuaternionGLAngle(const vec3_t angles, vec4_t quaternion)
{
    /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
    float	yaw = angles[2] * 0.5;
    float	pitch = angles[1] * 0.5;
    float	roll = angles[0] * 0.5;
    float	siny = sin(yaw);
    float	cosy = cos(yaw);
    float	sinp = sin(pitch);
    float	cosp = cos(pitch);
    float	sinr = sin(roll);
    float	cosr = cos(roll);
    /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

    quaternion[0] = sinr * cosp * cosy - cosr * sinp * siny;
    quaternion[1] = cosr * sinp * cosy + sinr * cosp * siny;
    quaternion[2] = cosr * cosp * siny - sinr * sinp * cosy;
    quaternion[3] = cosr * cosp * cosy + sinr * sinp * siny;
}

#define MAX_BONES 128



matrix3x4 transform_matrix[MAX_BONES];	/* Vertex transformation matrix */

void GL_Draw_HL_AliasFrame(short *order, vec3_t *transformed, float tex_w, float tex_h);
//void above need referencing somewhere else?


/*============== FORMAT DEFINITIONS ====================*/


// format uses float pointing values, but to allow for endianness
// conversions they are represented here as unsigned integers.
typedef struct
{
    //int		filetypeid;	//IDSP
	char ident[4];
	
	//int		version;	//10
	s32_t version;
	
    //char	name[64];
    int		filesize;
    vec3_t	unknown3[5];
    int		unknown4;
    int		numbones;
    int		boneindex;
    int		numcontrollers;
    int		controllerindex;
    int		unknown5[2];
    int		numseq;
    int		seqindex;
    int		unknown6;
    int		seqgroups;
    int		numtextures;
    int		textures;
    int		unknown7[3];
    int		skins;
    int		numbodyparts;
    int		bodypartindex;
    int		unknown9[8];
} mdl_header_t;




/*
 =======================================================================================================================
    Mod_LoadHLModel - read in the model's constituent parts
 =======================================================================================================================
 */
extern char loadname[];
qboolean Mod_LoadHLModel (model_t *mod, void *buffer)
{
    /*~~*/
    int i;

	hlmodelcache_t *model;
	hlmdl_header_t *header;
	hlmdl_header_t *texheader;
	hlmdl_tex_t	*tex;
	hlmdl_bone_t	*bones;
	hlmdl_bonecontroller_t	*bonectls;
	texid_t *texnums;

	int					start, end, total;
    /*~~*/


	//checksum the model

	if (mod->engineflags & MDLF_DOCRC)
	{
		unsigned short crc;
		qbyte *p;
		int len;
		char st[40];

		QCRC_Init(&crc);
		for (len = com_filesize, p = buffer; len; len--, p++)
			QCRC_ProcessByte(&crc, *p);

		sprintf(st, "%d", (int) crc);
		Info_SetValueForKey (cls.userinfo[0],
			(mod->engineflags & MDLF_PLAYER) ? pmodel_name : emodel_name,
			st, sizeof(cls.userinfo[0]));

		if (cls.state >= ca_connected)
		{
			CL_SendClientCommand(true, "setinfo %s %d",
				(mod->engineflags & MDLF_PLAYER) ? pmodel_name : emodel_name,
				(int)crc);
		}
	}

	start = Hunk_LowMark ();


	//load the model into hunk
	model = Hunk_Alloc(sizeof(hlmodelcache_t));

	header = Hunk_Alloc(com_filesize);
	memcpy(header, buffer, com_filesize);


	if (header->version != 10)
	{
		Con_Printf(CON_ERROR "Cannot load model %s - unknown version %i\n", mod->name, header->version);
		Hunk_FreeToLowMark(start);
		return false;
	}

	if (header->numcontrollers > MAX_BONE_CONTROLLERS)
	{
		Con_Printf(CON_ERROR "Cannot load model %s - too many controllers %i\n", mod->name, header->numcontrollers);
		Hunk_FreeToLowMark(start);
		return false;
	}
	if (header->numbones > MAX_BONES)
	{
		Con_Printf(CON_ERROR "Cannot load model %s - too many bones %i\n", mod->name, header->numbones);
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
		header->numtextures = texheader->numtextures;

	tex = (hlmdl_tex_t *) ((qbyte *) texheader + texheader->textures);
    bones = (hlmdl_bone_t *) ((qbyte *) header + header->boneindex);
    bonectls = (hlmdl_bonecontroller_t *) ((qbyte *) header + header->controllerindex);


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

	model->header = (char *)header - (char *)model;
	model->texheader = (char *)texheader - (char *)model;
	model->textures = (char *)tex - (char *)model;
	model->bones = (char *)bones - (char *)model;
	model->bonectls = (char *)bonectls - (char *)model;

	texnums = Hunk_Alloc(texheader->numtextures*sizeof(model->texnums));
	model->texnums = (char *)texnums - (char *)model;
    for(i = 0; i < texheader->numtextures; i++)
    {
        texnums[i] = GL_LoadTexture8Pal24("", tex[i].w, tex[i].h, (qbyte *) texheader + tex[i].offset, (qbyte *) texheader + tex[i].w * tex[i].h + tex[i].offset, IF_NOALPHA|IF_NOGAMMA);
    }


//
// move the complete, relocatable alias model to the cache
//
	end = Hunk_LowMark ();
	total = end - start;

	mod->type = mod_halflife;

	Cache_Alloc (&mod->cache, total, mod->name);
	if (!mod->cache.data)
		return false;
	memcpy (mod->cache.data, model, total);

	Hunk_FreeToLowMark (start);
	return true;
}


int HLMod_FrameForName(model_t *mod, char *name)
{
	int i;
	hlmdl_header_t *h;
	hlmdl_sequencelist_t *seqs;
	hlmodelcache_t *mc;
	if (!mod || mod->type != mod_halflife)
		return -1;	//halflife models only, please

	mc = Mod_Extradata(mod);

	h = (hlmdl_header_t *)((char *)mc + mc->header);
	seqs = (hlmdl_sequencelist_t*)((char*)h+h->seqindex);

	for (i = 0; i < h->numseq; i++)
	{
		if (!strcmp(seqs[i].name, name))
			return i;
	}
	return -1;
}

int HLMod_BoneForName(model_t *mod, char *name)
{
	int i;
	hlmdl_header_t *h;
	hlmdl_bone_t *bones;
	hlmodelcache_t *mc;
	if (!mod || mod->type != mod_halflife)
		return -1;	//halflife models only, please

	mc = Mod_Extradata(mod);

	h = (hlmdl_header_t *)((char *)mc + mc->header);
	bones = (hlmdl_bone_t*)((char*)h+h->boneindex);

	for (i = 0; i < h->numbones; i++)
	{
		if (!strcmp(bones[i].name, name))
			return i+1;
	}
	return 0;
}

/*
 =======================================================================================================================
    HL_CalculateBones - calculate bone positions - quaternion+vector in one function
 =======================================================================================================================

  note, while ender may be proud of this function, it lacks the fact that interpolating eular angles is not as acurate as interpolating quaternions.
  it is faster though.
 */
void HL_CalculateBones
(
    int				offset,
    int				frame,
	float			lerpfrac,
    vec4_t			adjust,
    hlmdl_bone_t	*bone,
    hlmdl_anim_t	*animation,
    float			*destination
)
{
    /*~~~~~~~~~~*/
    int		i;
    vec3_t	angle;
	float lerpifrac = 1-lerpfrac;
	float t;
    /*~~~~~~~~~~*/

    /* For each vector */
    for(i = 0; i < 3; i++)
    {
        /*~~~~~~~~~~~~~~~*/
        int o = i + offset;        /* Take the value offset - allows quaternion & vector in one function */
        /*~~~~~~~~~~~~~~~*/

        angle[i] = bone->value[o];	/* Take the bone value */

        if(animation->offset[o] != 0)
        {
            /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
            int					tempframe = frame;
            hlmdl_animvalue_t	*animvalue = (hlmdl_animvalue_t *) ((qbyte *) animation + animation->offset[o]);
            /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

            /* find values including the required frame */
            while(animvalue->num.total <= tempframe)
            {
                tempframe -= animvalue->num.total;
                animvalue += animvalue->num.valid + 1;
            }
            if(animvalue->num.valid > tempframe)
            {
                if(animvalue->num.valid > (tempframe + 1))
				{
					//we can lerp that
                    t = animvalue[tempframe + 1].value * lerpifrac + lerpfrac * animvalue[tempframe + 2].value;
				}
                else
                    t = animvalue[animvalue->num.valid].value;
                angle[i] = bone->value[o] + t * bone->scale[o];
            }
            else
            {
                if(animvalue->num.total < tempframe + 1)
                {
                    angle[i] +=
                        (animvalue[animvalue->num.valid].value * lerpifrac +
                         lerpfrac * animvalue[animvalue->num.valid + 2].value) *
                        bone->scale[o];
                }
                else
                {
                    angle[i] += animvalue[animvalue->num.valid].value * bone->scale[o];
                }
            }
        }

        if(bone->bonecontroller[o] != -1)
		{	/* Add the programmable offset. */
            angle[i] += adjust[bone->bonecontroller[o]];
        }
    }

    if(offset < 3)
    {
        VectorCopy(angle, destination);			/* Just a standard vector */
    }
    else
    {
        QuaternionGLAngle(angle, destination);	/* A quaternion */
    }
}

/*
 =======================================================================================================================
    HL_CalcBoneAdj - Calculate the adjustment values for the programmable controllers
 =======================================================================================================================
 */
void HL_CalcBoneAdj(hlmodel_t *model)
{
    /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
    int						i;
    float					value;
    hlmdl_bonecontroller_t	*control = (hlmdl_bonecontroller_t *)
                                      ((qbyte *) model->header + model->header->controllerindex);
    /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

    for(i = 0; i < model->header->numcontrollers; i++)
    {
        /*~~~~~~~~~~~~~~~~~~~~~*/
        int j = control[i].index;
        /*~~~~~~~~~~~~~~~~~~~~~*/

        if(control[i].type & 0x8000)
        {
            value = model->controller[j] + control[i].start;
        }
        else
        {
            value = (model->controller[j]+1)*0.5;	//shifted to give a valid range between -1 and 1, with 0 being mid-range.
            if(value < 0)
                value = 0;
            else if(value > 1.0)
                value = 1.0;
            value = (1.0 - value) * control[i].start + value * control[i].end;
        }

        /* Rotational controllers need their values converted */
        if(control[i].type >= 0x0008 && control[i].type <= 0x0020)
            model->adjust[i] = M_PI * value / 180;
        else
            model->adjust[i] = value;
    }
}

/*
 =======================================================================================================================
    HL_SetupBones - determine where vertex should be using bone movements
 =======================================================================================================================
 */
void QuaternionSlerp( const vec4_t p, vec4_t q, float t, vec4_t qt );
void HL_SetupBones(hlmodel_t *model, int seqnum, int firstbone, int lastbone, float subblendfrac, float frametime)
{
    /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
    int						i;
    float					matrix[3][4];
    static vec3_t			positions[2];
    static vec4_t			quaternions[2], blended;

	int frame;

    hlmdl_sequencelist_t	*sequence = (hlmdl_sequencelist_t *) ((qbyte *) model->header + model->header->seqindex) +
										 ((unsigned int)seqnum>=model->header->numseq?0:seqnum);
    hlmdl_sequencedata_t	*sequencedata = (hlmdl_sequencedata_t *)
                                         ((qbyte *) model->header + model->header->seqgroups) +
                                         sequence->seqindex;
    hlmdl_anim_t			*animation = (hlmdl_anim_t *)
                                ((qbyte *) model->header + sequencedata->data + sequence->index);
    /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	frametime *= sequence->timing;
	if (frametime < 0)
		frametime = 0;

	frame = (int)frametime;
	frametime -= frame;

	if (!sequence->numframes)
		return;
    if(frame >= sequence->numframes)
	{
		if (sequence->loop)
			frame %= sequence->numframes;
		else
			frame = sequence->numframes-1;
	}

	if (lastbone > model->header->numbones)
		lastbone = model->header->numbones;



    HL_CalcBoneAdj(model);	/* Deal with programmable controllers */

	/*FIXME:this is useless*/
	/*
    if(sequence->motiontype & 0x0001)
		positions[sequence->motionbone][0] = 0.0;
    if(sequence->motiontype & 0x0002)
		positions[sequence->motionbone][1] = 0.0;
    if(sequence->motiontype & 0x0004)
		positions[sequence->motionbone][2] = 0.0;
		*/

	/*
	this is hellish.
	a hl model blends:
		4 controllers (on a player, it seems each one of them twists a separate bone in the chest)
		a mouth (not used on players)
		its a sequence (to be smooth we need to blend between two frames in the sequence)
		up to four source animations (ironically used to pitch up/down)
		alternate sequence (walking+firing)
		frame2 (quake expectations.)

		this is madness, quite frankly.

		luckily...
		controllers and mouth control the entire thing. they should be interpolated outside, and have no affect on blending here
		alternate sequences replace. we can just call this function twice (so long as bone ranges are incremental).
		autoanimating sequence is handled inside HL_CalculateBones (sequences are weird and it has to be handled there anyway)

		this means we only have sources and alternate frames left to cope with.

		FIXME: we don't handle frame2.
	*/

	if (sequence->hasblendseq>1)
	{
		if (subblendfrac < 0)
			subblendfrac = 0;
		if (subblendfrac > 1)
			subblendfrac = 1;
		for(i = firstbone; i < lastbone; i++)
		{
			HL_CalculateBones(0, frame, frametime, model->adjust, model->bones + i, animation + i, positions[0]);
			HL_CalculateBones(3, frame, frametime, model->adjust, model->bones + i, animation + i, quaternions[0]);

			HL_CalculateBones(3, frame, frametime, model->adjust, model->bones + i, animation + i + model->header->numbones, quaternions[1]);

			QuaternionSlerp(quaternions[0], quaternions[1], subblendfrac, blended);
			QuaternionGLMatrix(blended[0], blended[1], blended[2], blended[3], matrix);
			matrix[0][3] = positions[0][0];
			matrix[1][3] = positions[0][1];
			matrix[2][3] = positions[0][2];

			/* If we have a parent, take the addition. Otherwise just copy the values */
			if(model->bones[i].parent>=0)
			{
				R_ConcatTransforms(transform_matrix[model->bones[i].parent], matrix, transform_matrix[i]);
			}
			else
			{
				memcpy(transform_matrix[i], matrix, 12 * sizeof(float));
			}
		}

	}
	else
	{
		for(i = firstbone; i < lastbone; i++)
		{
			/*
			 * There are two vector offsets in the structure. The first seems to be the
			 * positions of the bones, the second the quats of the bone matrix itself. We
			 * convert it inside the routine - Inconsistant, but hey.. so's the whole model
			 * format.
			 */
			HL_CalculateBones(0, frame, frametime, model->adjust, model->bones + i, animation + i, positions[0]);
			HL_CalculateBones(3, frame, frametime, model->adjust, model->bones + i, animation + i, quaternions[0]);

			QuaternionGLMatrix(quaternions[0][0], quaternions[0][1], quaternions[0][2], quaternions[0][3], matrix);
			matrix[0][3] = positions[0][0];
			matrix[1][3] = positions[0][1];
			matrix[2][3] = positions[0][2];

			/* If we have a parent, take the addition. Otherwise just copy the values */
			if(model->bones[i].parent>=0)
			{
				R_ConcatTransforms(transform_matrix[model->bones[i].parent], matrix, transform_matrix[i]);
			}
			else
			{
				memcpy(transform_matrix[i], matrix, 12 * sizeof(float));
			}
		}
	}
}

/*
 =======================================================================================================================
    R_Draw_HL_AliasModel - main drawing function
 =======================================================================================================================
 */
void R_DrawHLModel(entity_t	*curent)
{
    /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	hlmodelcache_t *modelc = Mod_Extradata(curent->model);
	hlmodel_t model;
    int						b, m, v;
    short					*skins;
	int bgroup, cbone, lastbone;
	float mat[16];
    /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	//general model
	model.header	= (hlmdl_header_t *)			((char *)modelc + modelc->header);
	model.texheader	= (hlmdl_header_t *)			((char *)modelc + modelc->texheader);
	model.textures	= (hlmdl_tex_t *)				((char *)modelc + modelc->textures);
	model.bones		= (hlmdl_bone_t *)				((char *)modelc + modelc->bones);
	model.bonectls	= (hlmdl_bonecontroller_t *)	((char *)modelc + modelc->bonectls);
	model.texnums	= (texid_t *)					((char *)modelc + modelc->texnums);

    skins = (short *) ((qbyte *) model.texheader + model.texheader->skins);

	if (!model.texheader->numtextures)
	{
		Con_DPrintf("model with no textures: %s\n", curent->model->name);
		return;
	}

	for (b = 0; b < MAX_BONE_CONTROLLERS; b++)
		model.controller[b] = curent->framestate.bonecontrols[b];

	GL_TexEnv(GL_MODULATE);

	if (curent->shaderRGBAf[3]<1)
	{
		qglEnable(GL_BLEND);
	}
	else
	{
		qglDisable(GL_BLEND);
	}
	qglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

//	Con_Printf("%s %i\n", sequence->name, sequence->unknown1[0]);

    qglPushMatrix();

	{
		vec3_t difuse, ambient, ldir;
		cl.worldmodel->funcs.LightPointValues(cl.worldmodel, curent->origin, difuse, ambient, ldir);
		qglColor4f(difuse[0]/255+ambient[0]/255, difuse[1]/255+ambient[1]/255, difuse[2]/255+ambient[2]/255, curent->shaderRGBAf[3]);
	}

    R_RotateForEntity (mat, curent, curent->model);
	qglLoadMatrixf(mat);

	cbone = 0;
	for (bgroup = 0; bgroup < FS_COUNT; bgroup++)
	{
		lastbone = curent->framestate.g[bgroup].endbone;
		if (bgroup == FS_COUNT-1)
			lastbone = model.header->numbones;
		if (cbone >= lastbone)
			continue;
		HL_SetupBones(&model, curent->framestate.g[bgroup].frame[0], cbone, lastbone, (curent->framestate.g[bgroup].subblendfrac+1)*0.5, curent->framestate.g[bgroup].frametime[0]);	/* Setup the bones */
		cbone = lastbone;
	}

    /* Manipulate each mesh directly */
    for(b = 0; b < model.header->numbodyparts; b++)
    {
        /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
        hlmdl_bodypart_t	*bodypart = (hlmdl_bodypart_t *) ((qbyte *) model.header + model.header->bodypartindex) +
                                     b;
        int					bodyindex = (0 / bodypart->base) % bodypart->nummodels;
        hlmdl_model_t		*amodel = (hlmdl_model_t *) ((qbyte *) model.header + bodypart->modelindex) + bodyindex;
        qbyte				*bone = ((qbyte *) model.header + amodel->vertinfoindex);
        vec3_t				*verts = (vec3_t *) ((qbyte *) model.header + amodel->vertindex);
        vec3_t				transformed[2048];

//		vec3_t				*norms = (vec3_t *) ((qbyte *) model.header + amodel->unknown3[2]);
//		vec3_t				transformednorms[2048];
        /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/


        for(v = 0; v < amodel->numverts; v++)			// Transform per the matrix
		{
            VectorTransform(verts[v], (void *)transform_matrix[bone[v]], transformed[v]);
//			glVertex3fv(verts[v]);
//			glVertex3f(	verts[v][0]+10*verts[v][0],
//						verts[v][1]+10*verts[v][1],
//						verts[v][2]+10*verts[v][2]);
		}

		//Need to work out what we have!
		//raw data appears to be unit vectors
		//transformed gives some points on the skeleton.
		//what's also weird is that the meshes use these up!
/*		glDisable(GL_TEXTURE_2D);
		glBegin(GL_LINES);
		for(v = 0; v < amodel->unknown3[0]; v++)			// Transform per the matrix
		{
			VectorTransform(norms[v], transform_matrix[bone[v]], transformednorms[v]);
			glVertex3fv(transformednorms[v]);
			glVertex3f(	transformednorms[v][0]+10*transformednorms[v][0],
						transformednorms[v][1]+10*transformednorms[v][1],
						transformednorms[v][2]+10*transformednorms[v][2]);
		}
		glEnd();
		glEnable(GL_TEXTURE_2D);
*/


        /* Draw each mesh */
        for(m = 0; m < amodel->nummesh; m++)
        {
            /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
            hlmdl_mesh_t	*mesh = (hlmdl_mesh_t *) ((qbyte *) model.header + amodel->meshindex) + m;
            float			tex_w;
            float			tex_h;
            /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

			{
				tex_w = 1.0f / model.textures[skins[mesh->skinindex]].w;
				tex_h = 1.0f / model.textures[skins[mesh->skinindex]].h;
				GL_LazyBind(0, GL_TEXTURE_2D, model.texnums[skins[mesh->skinindex]], false);
			}

            GL_Draw_HL_AliasFrame((short *) ((qbyte *) model.header + mesh->index), transformed, tex_w, tex_h);
        }
    }

    qglPopMatrix();

	GL_TexEnv(GL_REPLACE);
}

/*
 =======================================================================================================================
    GL_Draw_HL_AliasFrame - clip and draw all triangles
 =======================================================================================================================
 */
void GL_Draw_HL_AliasFrame(short *order, vec3_t *transformed, float tex_w, float tex_h)
{
    /*~~~~~~~~~~*/
    int count = 0;
    /*~~~~~~~~~~*/

//	int c_tris=0;
//	int c_verts=0;
//	int c_chains=0;

    for(;;)
    {
        count = *order++;	/* get the vertex count and primitive type */
        if(!count) break;	/* done */

        if(count < 0)
        {
            count = -count;
            qglBegin(GL_TRIANGLE_FAN);
        }
        else
		{
            qglBegin(GL_TRIANGLE_STRIP);
		}
//		c_tris += count-2;
//		c_chains++;

        do
        {
            /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
            float	*verts = transformed[order[0]];
            /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

            /* texture coordinates come from the draw list */
            qglTexCoord2f(order[2] * tex_w, order[3] * tex_h);
            order += 4;

            qglVertex3fv(verts);
//			c_verts++;
        } while(--count);

        qglEnd();
    }
}

#endif
