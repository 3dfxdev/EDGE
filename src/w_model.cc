//----------------------------------------------------------------------------
//  EDGE Model Management
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2007  The EDGE Team.
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

#include "e_main.h"
#include "e_search.h"
#include "r_image.h"
#include "r_things.h"
#include "w_model.h"
#include "w_wad.h"

#include "p_local.h"  // mobjlisthead


// Model storage
static md2_model_c **models;
static int nummodels = 0;


modeldef_c::modeldef_c(const char *_prefix) : model(NULL)
{
	strcpy(name, _prefix);

	for (int i=0; i < MAX_MODEL_SKINS; i++)
		skins[i] = NULL;
}

modeldef_c::~modeldef_c();


md2_model_c *LoadModelFromLump(int model_num)
{
	char lumpname[16];

	sprintf(lumpname, "%sMD2", ddf_model_names[model_num].c_str());


}


void W_InitModels(void)
{
	nummodels = (int)ddf_model_names.size();

	SYS_ASSERT(nummodels >= 1);  // at least SPR_NULL

	I_Printf("W_InitModels: Setting up\n");

	models = new md2_model_c * [nummodels];

	for (int i=0; i < nummodels; i++)
		models[i] = NULL;
}


md2_model_c *W_GetModel(int model_num)
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
	// TODO
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
