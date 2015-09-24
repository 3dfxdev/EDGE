//----------------------------------------------------------------------------
//  EDGE2 Model Management
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2008  The EDGE2 Team.
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

#ifndef __W_MODEL_H__
#define __W_MODEL_H__

#include "../md5_conv/md5.h"
#include "r_defs.h"

class md2_model_c;


#define MAX_MODEL_SKINS  10

#define MODEL_MD2 0
//#define MODEL_MD5 1
#define MODEL_MD5_UNIFIED 2
//#define MODEL_DM5 2

class skindef_c
{
public:

	const image_c *img;
	const image_c *norm;
	const image_c *spec;

	skindef_c() {
		img=0;
		norm=0;
		spec=0;
	}
};

class modeldef_c
{
public:
	// four letter model name (e.g. "TROO").
	char name[6];
	char modeltype;
	
	union {
		md2_model_c *model;
		MD5model *md5;
		MD5umodel *md5u;
	};

	skindef_c skins[MAX_MODEL_SKINS];

public:
	 modeldef_c(const char *_prefix);
	~modeldef_c();
};


/* Functions */

void W_InitModels(void);

void W_PrecacheModels(void);

modeldef_c *W_GetModel(int model_num);

// XXX W_GetModelSkin(int model_num, int skin_num);

#endif // __W_MODEL_H__

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
