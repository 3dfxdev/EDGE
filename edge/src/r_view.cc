//----------------------------------------------------------------------------
//  EDGE View Bitmap systems
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2001  The EDGE Team.
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
#include "r_plane.h"
#include "r_things.h"
#include "z_zone.h"

// internal variables showing the biggest dimensions so far, just to keep some
// common temp arrays big enough for all the viewbitmaps.
int max_vb_h = 0;
int max_vb_w = 0;

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
// R_DestroyViewBitmap
//
// Destroys the view bitmap and all its components (including views and aspects)
//
void R_DestroyViewBitmap(viewbitmap_t * vb)
{
  while (vb->views)
    R_DestroyView(vb->views);
  while (vb->aspects)
    R_DestroyAspect(vb->aspects);

  V_EmptyScreen(&vb->screen);

  Z_Free(vb->baseylookup);
  Z_Free(vb->basecolumnofs);

  R_DestroyCallbackList(&vb->frame_start);
  R_DestroyCallbackList(&vb->frame_end);

  // just to avoid strange bugs...
  if (vb == curviewbmp)
    curviewbmp = NULL;

  Z_Free(vb);
}

//
// R_CreateViewBitmap
//
// Creates a viewbitmap with the given dimensions. If p is NULL, a standalone
// screen will be created and used as destination. Otherwise a subscreen to
// p, starting at (x,y), will be created and used.
//
viewbitmap_t *R_CreateViewBitmap(int width, int height, int bytepp, screen_t * p, int x, int y)
{
  viewbitmap_t *vb;
  int i;

  vb = Z_ClearNew(viewbitmap_t, 1);

  if (p)
  {
    if (bytepp != p->bytepp)
      I_Error("R_CreateViewBitmap: bytepp different in vb and parent");
    V_InitSubScreen(&vb->screen, p, x, y, width, height);
  }
  else
    V_InitScreen(&vb->screen, width, height, bytepp);

  vb->views = NULL;
  vb->aspects = NULL;
  vb->baseylookup = Z_New(byte *, SCREENHEIGHT);  //!!! -AJA- hack !
  vb->basecolumnofs = Z_New(int, SCREENWIDTH);

  for (i = 0; i < SCREENHEIGHT; i++)
  {
    vb->baseylookup[i] = vb->screen.data + i * vb->screen.pitch;
  }
  for (i = 0; i < SCREENWIDTH; i++)
  {
    vb->basecolumnofs[i] = i * bytepp;
  }

  // some global arrays have to be as big as the largest viewbitmap.
  if (width > max_vb_w)
  {
    max_vb_w = width;
  }

  if (height > max_vb_h)
  {
    max_vb_h = height;

    Z_Resize(spanstart, int, height);
    Z_Clear(spanstart, int, height);
  }

  return vb;
}

//
// R_DestroyAspect
//
// Destroys the aspect and removes all references to it.
// Does not destroy any views, but all child views get their aspect info
// cleared.
void R_DestroyAspect(aspect_t * a)
{
  aspect_t *a2;

  // no view should be related to this aspect
  while (a->views)
    R_ViewClearAspect(a->views);

  // Unlink the aspect from its parent's aspect list
  if (a == a->parent->aspects)
    a->parent->aspects = a->next;
  else
  {
    for (a2 = a->parent->aspects; a2->next; a2 = a2->next)
      if (a2->next == a)
      {
        a2->next = a->next;
        break;
      }
    if (!a2->next)
    {  // didn't find v in its parent's list

      I_Error("R_DestroyAspect: Aspect doesn't exist in parent's aspect list");
    }
  }

  Z_Free(a->basextoviewangle);
  Z_Free(a->baseviewangletox);
  Z_Free(a->basedistscale);
  Z_Free(a->baseyslope);
  Z_Free(a);
}

//
// R_AspectChangeY
//
// Changes y_distunit and y angle of an aspect.
// fakefocusslope is the slope of the angle which would be focus if true 3D
// would be used.
void R_AspectChangeY(aspect_t * a, float y_distunit, float fakefocusslope)
{
  float dy;
  int i;

  if (a->y_distunit != y_distunit || a->fakefocusslope != fakefocusslope)
  {
    a->fakefocusslope = fakefocusslope;

    a->focusyfrac = ((float)a->maxheight * (fakefocusslope + a->topslope)) /
                     (float)((a->topslope-a->bottomslope))-(float)0.5;

    a->y_distunit = y_distunit;

    for (i = 0; i < a->maxheight; i++)
    {
      dy = fabs((float)i - (a->focusyfrac + 0.5));
      // -ES- 1999/10/17 Prevent from crash by using dist*4 as slope instead
      // of Inf if dy is very small
      // (gives an angle of 89.9 instead of 90 in normal 320x200).
      if (dy <= 0.25)
        a->baseyslope[i] = y_distunit * 4;
      else
        a->baseyslope[i] = y_distunit / dy;
    }
  }
}

void R_AspectChangeX(aspect_t * a, float x_distunit, float focusxfrac)
{
  int i;
  int x;
  int t;
  angle_t ang;
  float maxleftslope, minrightslope;
  float cosadj;

  a->x_distunit = x_distunit;
  a->focusxfrac = focusxfrac;

  maxleftslope = focusxfrac / x_distunit;
  minrightslope = (focusxfrac - (float)a->maxwidth) / x_distunit;

  // Use tangent table to generate viewangletox:
  //  viewangletox will give the next greatest x
  //  after the view angle.
  for (i = 0; i < FINEANGLES / 2; i++)
  {
    ang = (i << ANGLETOFINESHIFT) - ANG90;

    if (i == 0 || M_Tan(ang) < minrightslope)
      t = a->maxwidth + 1;
    else if (M_Tan(ang) > maxleftslope)
      t = -1;
    else
    {
      t = (int)(focusxfrac - M_Tan(ang) * x_distunit + 1.0);

      if (t < -1)
        t = -1;
      else if (t > a->maxwidth + 1)
        t = a->maxwidth + 1;
    }
    a->baseviewangletox[i] = t;
  }

  // Scan viewangletox[] to generate xtoviewangle[]:
  //  xtoviewangle will give the smallest view angle
  //  that maps to x. 
  for (x = 0; x <= a->maxwidth; x++)
  {
    i = 0;
    while (a->baseviewangletox[i] > x)
      i++;
    a->basextoviewangle[x] = (i << ANGLETOFINESHIFT) - ANG90;
  }

  for (i = 0; i < a->maxwidth; i++)
  {
    cosadj = (float)fabs(M_Cos(a->basextoviewangle[i]));
    a->basedistscale[i] =  (float)(1.0 / cosadj);
  }
}

//
// R_CreateAspect
//
// Creates an aspect_t with the given dimensions, and links it into the
// parent's list.
aspect_t *R_CreateAspect(viewbitmap_t * parent, float x_distunit,
    float y_distunit,float focusxfrac,
    float topslope,float bottomslope, int maxwidth, int maxheight)
{
  aspect_t *a;

  a = Z_New(aspect_t, 1);

  a->maxwidth = maxwidth;
  a->maxheight = maxheight;

  if (maxwidth > parent->screen.width || maxheight > parent->screen.height)
    I_Error("R_CreateAspect: aspect's max size larger than parent!");

  // X STUFF
  a->baseviewangletox = Z_New(int, FINEANGLES / 2);
  a->basextoviewangle = Z_New(angle_t, maxwidth + 1);
  a->basedistscale = Z_New(float, maxwidth);

  R_AspectChangeX(a, x_distunit, focusxfrac);

  // Y STUFF
  a->topslope = topslope;
  a->bottomslope = bottomslope;
  a->fakefocusslope = (float)M_PI;  // ChangeY is called w slope==0, so it has to be different.

  a->baseyslope = Z_New(float, a->maxheight);

  R_AspectChangeY(a, y_distunit, 0);

  a->views = NULL;

  a->next = parent->aspects;
  parent->aspects = a;
  a->parent = parent;

  return a;
}

//
// ViewRemoveFromVBList
//
// Removes the view from its parent viewbitmap_t's list.
static void ViewRemoveFromVBList(view_t * v)
{
  view_t *v2;

  if (v == v->parent->views)
    v->parent->views = v->vbnext;
  else
  {
    bool error = true;

    for (v2 = v->parent->views; v2->vbnext; v2 = v2->vbnext)
      if (v2->vbnext == v)
      {
        v2->vbnext = v->vbnext;
        error = false;
        break;
      }
    if (error)
    {  // didn't find v in its parent's list

      I_Error("ViewRemoveFromVBList: View doesn't exist in parent's view list");
    }
  }
}

//
// R_ViewClearAspect
//
// Removes all aspect info from a view, and removes it from the aspect's list.
// The view can't be used again until you give it a new aspect
// (through R_ViewSetAspect)
void R_ViewClearAspect(view_t * v)
{
  view_t *v2;

  if (v->aspect == NULL)
    I_Error("R_ViewClearAspect: No aspect to clear!");

  if (v == v->aspect->views)
    v->aspect->views = v->anext;
  else
  {
    bool error = true;

    for (v2 = v->aspect->views; v2->anext; v2 = v2->anext)
      if (v2->anext == v)
      {
        v2->anext = v->anext;
        error = false;
        break;
      }
    if (error)
    {  // didn't find v in the aspect's list

      I_Error("R_ViewClearAspect: View doesn't exist in aspect's view list");
    }
  }

  // destroy all aspect related stuff
  v->xtoviewangle = NULL;
  Z_Free(v->viewangletox);
  v->viewangletox = NULL;
  v->yslope = NULL;

  v->aspect = NULL;
}

//
// R_DestroyView
//
// Destroys the view and removes it from any lists that refer to it.
//
void R_DestroyView(view_t * v)
{
  // First unlink v from all the view lists
  ViewRemoveFromVBList(v);
  if (v->aspect)
    R_ViewClearAspect(v);

  V_EmptyScreen(&v->screen);

  R_DestroyCallbackList(&v->frame_start);
  R_DestroyCallbackList(&v->frame_start);

  // just to avoid strange bugs...
  if (v == curview)
    curview = NULL;

  Z_Free(v);
}

//
// SetVBPos
//
// Selects where within the viewbitmap the view should be drawn.
static void ViewSetVBXPos(view_t * v, int vbx)
{
  V_MoveSubScreen(&v->screen, 0, 0); //!!!! vbx, v->screen.y);
  v->columnofs = v->parent->basecolumnofs + 0; //!!!! vbx;
}

//
// ViewSetVBYPos
//
// Selects where within the viewbitmap the view should be drawn.
static void ViewSetVBYPos(view_t * v, int vby)
{
  V_MoveSubScreen(&v->screen, 0, 0); //!!!! v->screen.x, vby);
  v->ylookup = v->parent->baseylookup + 0; //!!!! vby;
}

//
// R_ViewSetAspectPos
//
// Selects which part of the aspect the view should render, and the width of
// the rendering.
void R_ViewSetAspectXPos(view_t * v, int ax, int width)
{
  aspect_t *a = v->aspect;
  int i;

  if (ax + width > a->maxwidth)
    I_Error("R_ViewSetAspectXPos: aspect width/height overflow!");

  V_ResizeScreen(&v->screen, width, v->screen.height, v->screen.bytepp);

  v->aspect_x = ax;

  // Create viewangletox, based on aspect's baseviewangletox
  i = 0;
  while (a->baseviewangletox[i] - ax > width)
  {
    v->viewangletox[i] = width;
    i++;
  }
  while (a->baseviewangletox[i] - ax > 0)
  {
    v->viewangletox[i] = a->baseviewangletox[i] - ax;
    i++;
  }
  while (i < FINEANGLES / 2)
  {
    v->viewangletox[i] = 0;
    i++;
  }
  // xtoviewangle is just a part of aspect's basextoviewangle
  v->xtoviewangle = a->basextoviewangle + ax;
  // same with distscale
  v->distscale = a->basedistscale + ax;
}

//
// ViewSetAspectPos
//
// Selects which part of the aspect the view should render, and the height of
// the rendering.
static void ViewSetAspectYPos(view_t * v, int ay, int height)
{
  if (ay + height > v->aspect->maxheight)
    I_Error("R_ViewSetYPos: aspect width/height overflow!");

  v->aspect_y = ay;

  // don't update if height hasn't changed
  if (height != v->screen.height)
  {
    V_ResizeScreen(&v->screen, v->screen.width, height, v->screen.bytepp);
  }

  v->yslope = v->aspect->baseyslope + ay;
}

//
// R_ViewSetYPosition
//
// Sets the position of the view within both the viewbitmap and aspect
void R_ViewSetYPosition(view_t * v, int vby, int ay, int height)
{
  // just calls the two helper functions. Do them in an order that suits the
  // range checking
  if (vby + v->screen.height > v->parent->screen.height)
  {
    ViewSetAspectYPos(v, ay, height);
    ViewSetVBYPos(v, vby);
  }
  else
  {
    ViewSetVBYPos(v, vby);
    ViewSetAspectYPos(v, ay, height);
  }
}

//
// R_ViewSetXPosition
//
// Sets the position of the view within both the viewbitmap and aspect
void R_ViewSetXPosition(view_t * v, int vbx, int ax, int width)
{
  // just calls the two helper functions. Do them in an order that suits the
  // range checking
  if (vbx + v->screen.width > v->parent->screen.width)
  {
    R_ViewSetAspectXPos(v, ax, width);
    ViewSetVBXPos(v, vbx);
  }
  else
  {
    ViewSetVBXPos(v, vbx);
    R_ViewSetAspectXPos(v, ax, width);
  }
}

//
// ViewSetPosition
//
// Sets the position of the view within both the viewbitmap and aspect
static void ViewSetPosition(view_t * v, int vbx, int vby, int ax, int ay, int width, int height)
{
  R_ViewSetXPosition(v, vbx, ax, width);
  R_ViewSetYPosition(v, vby, ay, height);
}

//
// R_ViewSetAspect
//
// Sets the view's aspect to a, and initialises the view for the aspect's
// maximum size.
void R_ViewSetAspect(view_t * v, aspect_t * a)
{
  if (v->aspect)
    R_ViewClearAspect(v);

  v->viewangletox = Z_New(int, FINEANGLES / 2);

  v->anext = a->views;
  a->views = v;
  v->aspect = a;

  // view dimensions are initially set to the maximal possible
  ViewSetPosition(v, v->screen.x, v->screen.y, 0, 0, a->maxwidth, a->maxheight);
}

//
// R_CreateView
//
// Creates a view. x and y show the smallest possible coords for the
// upper-left corner (they can grow later). The view will be initialised for
// the aspect's maximal dimensions.
//
view_t *R_CreateView(viewbitmap_t * parent, aspect_t * aspect, int x, int y, camera_t * camera, unsigned char flags, int priority)
{
  view_t *v, *v1;

  if (x < 0 || y < 0)
    I_Error("R_CreateView: x or y is below zero!");

  v = Z_ClearNew(view_t, 1);

  // init width and height to 0, and let R_ViewAspectChange set them properly.
  V_InitSubScreen(&v->screen, &parent->screen, x, y, 0, 0);

  v->priority = priority;
  v->parent = parent;

  // High priority views are drawn last to overwrite the others, and
  // are therefore in the tail of the view list.
  if (parent->views == NULL || parent->views->priority >= priority)
  {
    v->vbnext = parent->views;
    parent->views = v;
  }
  else
  {
    // Seach through the list for the first view whose priority is greater or
    // equal to the new view's
    for (v1 = parent->views; v1->vbnext && v1->vbnext->priority < priority; v1 = v1->vbnext) ;
    // Link it in there
    v->vbnext = v1->vbnext;
    v1->vbnext = v;
  }

  v->camera = camera;
  v->renderflags = flags;

  R_ViewSetAspect(v, aspect);

  return v;
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
