//----------------------------------------------------------------------------
//  EDGE View Bitmap systems
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2004  The EDGE Team.
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

#ifndef __R_VIEW__
#define __R_VIEW__

#include "e_player.h"
#include "m_fixed.h"
#include "m_math.h"
#include "v_screen.h"

typedef struct camera_s camera_t;
typedef struct callback_s callback_t;

// A callback function that is called either at the start or end of a frame,
// specific either to view, viewbitmap or camera. Can be used for other
// purposes too.
struct callback_s
{
	// the actual callback. Could for example be a detail increaser for a view.
	// The parameter is the data field of the callback_t.
	void (*f) (void *);
	// User data: The callback needs info about what to operate on. Typically
	// just a pointer to the view_t (or viewbitmap_t or camera_t).
	// Could also be a pointer to a user defined struct containing that pointer
	// plus some extra user info.
	void *data;
	// Routine that deallocates the data field. Typically Z_Free, if data was
	// allocated with Z_New. Or NULL if nothing was allocated when the
	// callback was created.
	// Some start-of-frame callbacks might want to share data with end-of-frame
	// ones, in that case the destruction should be done in the end-of-frame
	// one, since the start-of-frame list always is destroyed first.
	void (*kill_data) (void *);
	// prev is executed before this callback and next after.
	callback_t *prev, *next;
};

//
// The camera whose view is projected.
//
struct camera_s
{
	// executed at the start of every frame when it's used for rendering,
	// to set some globals
	callback_t *frame_start;
	// Optional, does any necessary cleanup at end of frame
	callback_t *frame_end;

	// If the camera is attached to a player, this points to it. Otherwise
	// it's NULL. It's used for psprite drawing.
	// -AJA- 1999/09/11: Now a pointer to mobj_t instead of player_t.
	mobj_t *view_obj;
};

extern camera_t *camera;
extern mobj_t *background_camera_mo;

// Adds a callback to the end of *list. Use this for start_frame lists.
extern void R_AddStartCallback(callback_t ** list, void (*f) (void *), void *data, void (*kill_data) (void *));

// Adds a callback to the start of *list. Use this for end_frame lists.
extern void R_AddEndCallback(callback_t ** list, void (*f) (void *), void *data, void (*kill_data) (void *));

// Destroys a list of callbacks
extern void R_DestroyCallbackList(callback_t ** list);

// Calls all the callbacks in the list.
extern void R_CallCallbackList(callback_t * list);

void R_DestroyCamera(camera_t * c);
camera_t *R_CreateCamera(void);

#endif
