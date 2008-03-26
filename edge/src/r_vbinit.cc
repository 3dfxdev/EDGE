//----------------------------------------------------------------------------
//  EDGE View Bitmap Initialisation code
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2008  The EDGE Team.
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
//  Based on the DOOM source code, released by Id Software under the
//  following copyright:
//
//    Copyright (C) 1993-1996 by id Software, Inc.
//
//----------------------------------------------------------------------------
//
// Here are all the composition mode dependent initialisation routines and
// callbacks for viewbitmaps and its substructs (ie views and cameras).
//

#include "i_defs.h"
#include "r_vbinit.h"

#include "con_defs.h"
#include "dm_state.h"
#include "e_player.h"
#include "p_mobj.h"
#include "r_misc.h"
#include "r_state.h"
#include "r_view.h"
#include "r_colormap.h"
#include "r_modes.h"
#include "z_zone.h"

/***** Camera routines *****/
typedef struct
{
	camera_t *c;
	mobj_t *mo;
}
camera_start_stdobj_t;


static void CameraFrameInit_StdObject(void *data)
{
	camera_start_stdobj_t *o = (camera_start_stdobj_t*) data;
	camera_t *c;
	mobj_t *mo;

	mo = o->mo;
	c = o->c;

	c->view_obj = mo;

	viewx = mo->x;
	viewy = mo->y;
	viewz = mo->z + mo->height * 9 / 10;

	viewangle = mo->angle;
	viewvertangle = mo->vertangle;
	viewsubsector = mo->subsector;

	view_props = R_PointGetProps(viewsubsector, viewz);
}

void R_InitCamera_StdObject(camera_t * c, mobj_t * mo)
{
	camera_start_stdobj_t *o;

	o = Z_New(camera_start_stdobj_t, 1);
	o->c = c;
	o->mo = mo;
	R_AddStartCallback(&c->frame_start, CameraFrameInit_StdObject, o, Z_Free);
}



static void CameraFrameInit_StdPlayer(void *data)
{
	camera_t *c = (camera_t*)data;
	player_t *player = players[displayplayer];

	c->view_obj = player->mo;

	viewx = player->mo->x;
	viewy = player->mo->y;
	viewz = player->mo->z + player->viewz;
	viewangle = player->mo->angle;

	viewsubsector = player->mo->subsector;
	viewvertangle = player->mo->vertangle + M_ATan(player->kick_offset);
	view_props = R_PointGetProps(viewsubsector, viewz);

	if (! level_flags.mlook)
		viewvertangle = 0;

	// No heads above the ceiling
	if (viewz > player->mo->ceilingz - 2)
		viewz = player->mo->ceilingz - 2;

	// No heads below the floor, please
	if (viewz < player->mo->floorz + 2)
		viewz = player->mo->floorz + 2;
}

void R_InitCamera_StdPlayer(camera_t * c)
{
	R_AddStartCallback(&c->frame_start, CameraFrameInit_StdPlayer, c, NULL);
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
