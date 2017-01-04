//----------------------------------------------------------------------------
//  EDGE2 Model Management
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2009  The EDGE2 Team.
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

#include "i_defs.h"
#include "i_defs_gl.h"

#include "e_main.h"
#include "r_image.h"
#include "r_md2.h"
#include "../md5_conv/md5.h"
//#include "r_mdl.h"
#include "r_things.h"
#include "w_model.h"
#include "w_wad.h"

#include "p_local.h"  // mobjlisthead


// Model storage
static modeldef_c **models;
static int nummodels = 0;


modeldef_c::modeldef_c(const char *_prefix) : model(NULL)
{
	strcpy(name, _prefix);
	modeltype = MODEL_MD2;

	for (int i=0; i < MAX_MODEL_SKINS; i++)
		skins[i]=skindef_c();
}

modeldef_c::~modeldef_c()
{
	// FIXME: delete model;

	// TODO: free the skins
}

static void FindModelFrameNames(md2_model_c *md, int model_num)

{
	int missing = 0;

	I_Printf("Finding frame names for model '%sMD2'...\n",
			 ddf_model_names[model_num].c_str());

	for (int stnum = 1; stnum < num_states; stnum++)
	{
		state_t *st = &states[stnum];

		if (st->sprite != model_num)
			continue;

		if (! (st->flags & SFF_Model))
			continue;

		if (! (st->flags & SFF_Unmapped))
			continue;

		SYS_ASSERT(st->model_frame);

		st->frame = MD2_FindFrame(md, st->model_frame);

		if (st->frame >= 0)
		{
			st->flags &= ~SFF_Unmapped;
		}
		else
		{
			missing++;
			I_Printf("-- no such frame '%s'\n", st->model_frame);
		}
	}

	if (missing > 0)
		I_Error("Failed to find %d frames for model '%sMDx' (see EDGE2.LOG)\n",
				missing, ddf_model_names[model_num].c_str());
}


modeldef_c *LoadModelFromLump(int model_num)
{
	const char *basename = ddf_model_names[model_num].c_str();

	modeldef_c *def = new modeldef_c(basename);

	char lumpname[16];
	char skinname[32];

	epi::file_c *f;

	// try MD3 first, then MD2, then MD5
	sprintf(lumpname, "%sMD3", basename);

	if (W_CheckNumForName(lumpname) >= 0) {
		I_Debugf("Loading MD3 model from lump : %s\n", lumpname);

		f = W_OpenLump(lumpname);
		SYS_ASSERT(f);

		def->model = MD3_LoadModel(f);
		def->modeltype = MODEL_MD2;
	} else {
		sprintf(lumpname, "%sMD2", basename);
		if (W_CheckNumForName(lumpname) >= 0) {
			I_Debugf("Loading MD2 model from lump : %s\n", lumpname);
			f = W_OpenLump(lumpname);
			if (! f)
				I_Error("Missing model lump: %s\n", lumpname);

			def->model = MD2_LoadModel(f);
			def->modeltype = MODEL_MD2;
		} else {
			sprintf(lumpname, "%sMD5", basename);
			I_Debugf("Loading MD5 model from lump : %s\n", lumpname);
			f = W_OpenLump(lumpname);
			SYS_ASSERT(f);
			byte *modeltext = f->LoadIntoMemory();
			SYS_ASSERT(modeltext);

			MD5model *md5 = md5_load((char*)modeltext);
			def->modeltype = MODEL_MD5_UNIFIED;
			def->md5u = md5_normalize_model(md5);
			
			md5_free(md5);
			delete[] modeltext;
			
			delete f;
			
			for (int i=0; i < def->md5u->model.meshcnt; i++) {
				char* file_normal=new char[strlen(def->md5u->model.meshes[i].shader)+strlen("_l")+1];
				char* file_specular=new char[strlen(def->md5u->model.meshes[i].shader)+strlen("_s")+1];

				sprintf(file_normal,"%s%s",def->md5u->model.meshes[i].shader,"_l");
				sprintf(file_specular,"%s%s",def->md5u->model.meshes[i].shader,"_s");

				//I_Printf("md5 shader: %s, %s, %s\n",def->md5u->model.meshes[i].shader,file_normal,file_specular);
				def->md5u->model.meshes[i].tex = W_ImageLookup(def->md5u->model.meshes[i].shader, INS_Sprite, ILF_Null);
				//def->md5u->model.meshes[i].tex_normal = W_ImageLookup(file_normal, INS_Sprite, ILF_Null);
				//def->md5u->model.meshes[i].tex_specular = W_ImageLookup(file_specular, INS_Sprite, ILF_Null);

				delete[] file_normal;
				delete[] file_specular;
			}
			
			return def;
		}
	}

	SYS_ASSERT(def->model);

	// close the lump
	delete f;

	for (int i=0; i < MAX_MODEL_SKINS; i++)
	{
		sprintf(skinname, "%sSKN%d", basename, i);
		def->skins[i].img = W_ImageLookup(skinname, INS_Sprite, ILF_Null);
		sprintf(skinname,"%sNRM%d",basename,i);
		def->skins[i].norm = W_ImageLookup(skinname, INS_Sprite, ILF_Null);
		sprintf(skinname,"%sSPC%d",basename,i);
		def->skins[i].spec= W_ImageLookup(skinname, INS_Sprite, ILF_Null);
	}

	// need at least one skin
	if (! def->skins[1].img)
		I_Error("Missing model skin: %sSKN1\n", basename);

	FindModelFrameNames(def->model, model_num);

	return def;
}


void W_InitModels(void)
{
	nummodels = (int)ddf_model_names.size();

	SYS_ASSERT(nummodels >= 1);  // at least SPR_NULL

	I_Printf("W_InitModels: Setting up\n");

	models = new modeldef_c * [nummodels];

	for (int i=0; i < nummodels; i++)
		models[i] = NULL;
}


modeldef_c *W_GetModel(int model_num)
{
	// model_num comes from the 'sprite' field of state_t, and
	// is also an index into ddf_model_names vector.

	SYS_ASSERT(model_num > 0);
	SYS_ASSERT(model_num < nummodels);

	if (! models[model_num])
	{
		models[model_num] = LoadModelFromLump(model_num);
	}

	return models[model_num];
}


void W_PrecacheModels(void)
{
	if (nummodels <= 0)
		return;

	byte *model_present = new byte[nummodels];
	memset(model_present, 0, nummodels);

	// mark all monsters (etc) in the level
	for (mobj_t * mo = mobjlisthead ; mo ; mo = mo->next)
	{
		SYS_ASSERT(mo->state);

		if (! (mo->state->flags & SFF_Model))
			continue;

		int model = mo->state->sprite;

		if (model >= 1 && model < nummodels)
			model_present[model] = 1;
	}

	// mark all weapons
	for (int k = 1 ; k < num_states ; k++)
	{
		if ((states[k].flags & (SFF_Weapon | SFF_Model)) != (SFF_Weapon | SFF_Model))
			continue;

		int model = states[k].sprite;

		if (model >= 1 && model < nummodels)
			model_present[model] = 1;
	}

	for (int i = 1 ; i < nummodels ; i++)  // ignore SPR_NULL
	{
		if (model_present[i])
		{
			I_Debugf("Precaching model: %s\n", ddf_model_names[i].c_str());

			modeldef_c *def = W_GetModel(i);

			// precache skins too
			if (def)
			{
				for (int n = 0 ; n < 10 ; n++)
				{
					if(def->skins[n].img) W_ImagePreCache(def->skins[n].img);
					if(def->skins[n].norm) W_ImagePreCache(def->skins[n].norm);
					if(def->skins[n].spec) W_ImagePreCache(def->skins[n].spec);
				}
			}
		}
	}

	delete[] model_present;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
