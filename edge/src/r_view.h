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
#include "z_zone.h"

typedef struct view_s view_t;
typedef struct viewbitmap_s viewbitmap_t;
typedef struct aspect_s aspect_t;
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

//
// A viewbitmap is a screen on which one or more views can be projected.
//
// -ES- 1999/07/31 Use the screen_t system.
struct viewbitmap_s
{
  screen_t screen;

  // Linked list of the views that can be drawn to this bitmap.
  view_t *views;

  // Linked list of the aspects that can be used with this bitmap.
  aspect_t *aspects;

  // Array of size (height), containing the addresses of each line.
  // the views' ylookups will point to one of the elements.
  byte **baseylookup;

  // Array of size (width), where element x contains x*BPP (currently).
  // the views' columnoffsets will point to one of these elements.
  int *basecolumnofs;

  // If these aren't null, they will be called at the start/end of each frame.
  callback_t *frame_start;
  callback_t *frame_end;
};

//
// aspect_s
//
// Contains the precalculated tables used when rendering a view with the
// specified aspect ratio etc.
//
// FIXME: Split up aspect_s into separate x and y?
struct aspect_s
{
  // the maximal width/height for views using this aspect.
  int maxwidth;
  int maxheight;

  // X RELATED STUFF

  // These show the tables for maxwidth.
  // The tables used in view_t are based on these ones, but can do some small
  // changes if not maxwidth is used.
  int *baseviewangletox;
  angle_t *basextoviewangle;
  float *basedistscale;
  float x_distunit;
  float focusxfrac;

  // Y RELATED STUFF

  float *baseyslope;
  float y_distunit;
  float focusyfrac;

  // the slope of the real focus is 0, this is the slope which we pretend to be focus.
  // ie. the slope that normally is in the middle of the screen.
  float fakefocusslope;
  // topslope & bottomslope show the offset to fakefocusslope.
  float topslope;
  float bottomslope;

  // GENERAL STUFF

  // list of views that use this aspect. These will be updated if the aspect
  // is changed.
  view_t *views;

  // aspects are atm viewbitmap specific.
  viewbitmap_t *parent;

  // next in parent's list
  aspect_t *next;
};

//
// VIEW STRUCT
//
// A view is an area on a viewbitmap, on which a view is projected.
struct view_s
{
  // The memory area this view is drawn to. A subscreen of the parent
  // viewbitmap's screen.
  screen_t screen;

  byte **ylookup;
  int *columnofs;

  // aspect related
  int *viewangletox;
  angle_t *xtoviewangle;
  float *distscale;
  int aspect_x;

  int aspect_y;
  float *yslope;

#define VRF_PSPR (1)
#define VRF_VIEW (2)
  unsigned char renderflags;  // flag variable telling what to render here

  // Lists of routines that will be called at the start/end of each frame.
  callback_t *frame_start;
  callback_t *frame_end;

  // Views with high priority are drawn on top on low-prioritised.
  // Conventions:
  // A normal view has priority 0.
  // A player sprite view has priority 100.
  int priority;

  camera_t *camera;

  aspect_t *aspect;
  view_t *anext;  // the next view in the aspect's list

  viewbitmap_t *parent;
  view_t *vbnext;  // the next view in the parent viewbitmap's list

};

extern viewbitmap_t *screenvb;

extern view_t *curview;
extern viewbitmap_t *curviewbmp;

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

extern void R_DestroyViewBitmap(viewbitmap_t * view);
extern viewbitmap_t *R_CreateViewBitmap(int width, int height, int bytepp, screen_t * p, int x, int y);

extern void R_DestroyAspect(aspect_t * a);
extern aspect_t *R_CreateAspect(viewbitmap_t * parent,
    float x_distunit, float y_distunit,
    float focusxfrac,
    float topslope, float bottomslope,
    int maxwidth, int maxheight);

extern void R_ViewSetAspectXPos(view_t * v, int ax, int width);
extern void R_AspectChangeX(aspect_t * a, float x_distunit, float focusxslope);
extern void R_AspectChangeY(aspect_t * a, float y_distunit, float fakefocusslope);
extern void R_ViewSetXPosition(view_t * v, int vbx, int ax, int width);
extern void R_ViewSetYPosition(view_t * v, int vby, int ay, int width);
extern void R_ViewSetAspect(view_t * v, aspect_t * a);

extern void R_ViewClearAspect(view_t * v);
extern void R_DestroyView(view_t * v);
extern view_t *R_CreateView(viewbitmap_t * parent, aspect_t * aspect, int x, int y, camera_t * camera, unsigned char flags, int priority);

extern void R_SetActiveViewBitmap(viewbitmap_t * vb);
extern void R_SetActiveView(view_t * v);

extern void R_RenderViewBitmap(viewbitmap_t * vb);

void R_DestroyCamera(camera_t * c);
camera_t *R_CreateCamera(void);

#endif
