//----------------------------------------------------------------------------
//  EDGE View Bitmap Initialisation code
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2005  The EDGE Team.
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
#include "r_main.h"
#include "r_state.h"
#include "r_view.h"
#include "v_colour.h"
#include "v_res.h"
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
	viewangle = mo->angle;
	viewvertangle = mo->vertangle;
	viewsubsector = mo->subsector;

	if (mo->player)
	{
		// -AJA- NOTE: CameraFrameInit_StdPlayer is used instead !!

		viewz = mo->player->viewz;
	}
	else
	{
		viewz = mo->z + mo->height * 9 / 10;
	}

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
	viewangle = player->mo->angle;
	viewz = player->viewz;

	viewsubsector = player->mo->subsector;
	viewvertangle = player->mo->vertangle + M_ATan(player->kick_offset);
	view_props = R_PointGetProps(viewsubsector, viewz);

	if (! level_flags.mlook)
		viewvertangle = 0;
}


void R_InitCamera_StdPlayer(camera_t * c)
{
	R_AddStartCallback(&c->frame_start, CameraFrameInit_StdPlayer, c, NULL);
}

typedef struct
{
	angle_t offs;
}
camera_start_viewoffs_t;

static void CameraFrameInit_ViewOffs(void *data)
{
	camera_start_viewoffs_t *o = (camera_start_viewoffs_t*)data;

	viewangle += o->offs;
}

void R_InitCamera_ViewOffs(camera_t * c, angle_t offs)
{
	camera_start_viewoffs_t *data;

	R_InitCamera_StdPlayer(c);

	data = Z_New(camera_start_viewoffs_t, 1);
	data->offs = offs;

	R_AddStartCallback(&c->frame_start, CameraFrameInit_ViewOffs, data, Z_Free);
}

