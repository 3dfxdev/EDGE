//----------------------------------------------------------------------------
//  EDGE View Bitmap systems
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

#include "i_defs.h"
#include "r_view.h"

#include "dm_state.h"
#include "m_math.h"
#include "r_data.h"
#include "r_main.h"
#include "r_things.h"
#include "z_zone.h"

//
// Destroys a list of callbacks
//
void R_DestroyCallbackList(callback_t ** list)
{
	callback_t *c, *tmp;

	for (c = *list; c; c = tmp)
	{
		if (c->kill_data)
			c->kill_data(c->data);
		tmp = c->next;
		Z_Free(c);
	}
	*list = NULL;
}

//
// Executes all the callbacks in the list.
//
void R_CallCallbackList(callback_t * list)
{
	for (; list; list = list->next)
		if (list->f)
			list->f(list->data);
}

//
// Adds a callback to the end of *list. Use this for start_frame lists.
//
void R_AddStartCallback(callback_t ** list, void (*f) (void *), void *data, void (*kill_data) (void *))
{
	callback_t *c;
	callback_t dummy;

	dummy.next = *list;
	dummy.prev = NULL;

	// find the last item in the list
	for (c = &dummy; c->next; c = c->next)
		;

	// link in the new element after the last one (i.e. after c)
	c->next = Z_New(callback_t, 1);
	if (c == &dummy)
	{
		c->next->prev = NULL;
		*list = c->next;
	}
	else
		c->next->prev = c;
	c = c->next;
	c->next = NULL;

	// init the new item
	c->f = f;
	c->data = data;
	c->kill_data = kill_data;
}

//
// Adds a callback to the start of *list. Use this for end_frame lists.
//
void R_AddEndCallback(callback_t ** list, void (*f) (void *), void *data, void (*kill_data) (void *))
{
	callback_t *c;

	c = Z_New(callback_t, 1);
	c->f = f;
	c->data = data;
	c->kill_data = kill_data;
	c->prev = NULL;
	c->next = *list;
	if (*list)
		(*list)->prev = c;
	*list = c;
}

//
// R_DestroyCamera
//
void R_DestroyCamera(camera_t * c)
{
	R_DestroyCallbackList(&c->frame_start);
	R_DestroyCallbackList(&c->frame_end);

	Z_Free(c);
}

//
// R_CreateCamera
//
// Just creates an empty camera. You must add a start_frame routine manually.
camera_t *R_CreateCamera(void)
{
	camera_t *c;

	c = Z_ClearNew(camera_t, 1);

	return c;
}
