//----------------------------------------------------------------------------
//  EDGE Layer Rendering System
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
//
//  See the file "docs/layersys.txt" for a complete description of the
//  layer system.
//
//  -AJA- 2000/06/13: Started work on this file.
//

#include "i_defs.h"
#include "r_layers.h"

#include "dm_state.h"
#include "m_bbox.h"
#include "m_fixed.h"
#include "r_data.h"
#include "r_main.h"
#include "z_zone.h"


#ifdef USE_LAYERS


layer_t *layer_list = NULL;


layer_t ** solid_rects = NULL;
int num_solid_rects = 0;
int max_solid_rects = 0;


static void ResizeLayerList(layer_t *list, int x1, int y1, int x2, int y2);



//
// R_LayerInit
//
// Initialise the layer system.

boolean_t R_LayerInit(void)
{
  // nothing to do
  return true;
}


//
// R_LayerNew
//
// Create a new layer.

layer_t *R_LayerNew(int depth, int x1, int y1, int x2, int y2,
    layerflags_e flags, void *private)
{
  layer_t *layer;

  DEV_ASSERT2(x1 >= 0);
  DEV_ASSERT2(y1 >= 0);
  DEV_ASSERT2(x2 >= x1);
  DEV_ASSERT2(y2 >= y1);
  DEV_ASSERT2(depth >= 0);

  layer = Z_ClearNew(layer_t, 1);

  layer->x1 = x1;
  layer->y1 = y1;
  layer->x2 = x2;
  layer->y2 = y2;

  layer->depth = depth;
  layer->flags = flags;
  layer->private = private;

  return layer;
}


//
// R_LayerDestroy
//
// Destroy a layer.  The layer *must* not be in a list somewhere.  Any
// child layers are also destroyed (by recursively calling this
// routine).  If the `private' field is not NULL, Z_Free() will be
// called on it.

void R_LayerDestroy(layer_t *layer)
{
  if (layer->private)
    Z_Free(layer->private);
  
  while (layer->children)
  {
    layer_t *cur = layer->children;
    layer->children = cur->next;

    R_LayerDestroy(cur);
  }

  Z_Free(layer);
}


//
// R_LayerAdd
//
// Add a layer to the parent layer, which can be NULL for the top
// level layer list.  The layers in the list are depth sorted, higher
// values occur later in the list.

void R_LayerAdd(layer_t *parent, layer_t *layer)
{
  layer_t ** parent_list = &layer_list;

  if (layer->parent)
    parent_list = &layer->parent->children;

  if (!(*parent_list) || (*parent_list)->depth >= layer->depth)
  {
    // add to head

    layer->next = (*parent_list);
    layer->prev = NULL;

    if (*parent_list)
      (*parent_list)->prev = layer;
    
    (*parent_list) = layer;
  }
  else
  {
    // link in after some other node

    layer_t *cur = (*parent_list);

    DEV_ASSERT2(cur);

    while (cur && cur->next && cur->next->depth < layer->depth)
      cur = cur->next;
    
    layer->next = cur->next;
    layer->prev = cur;

    if (cur->next)
      cur->next->prev = layer;
    
    cur->next = layer;
  }

  M_DirtyRegion(layer->x1, layer->y1, layer->x2, layer->y2);
}


//
// R_LayerRemove
//
// Remove the layer from it's parent list.

void R_LayerRemove(layer_t *layer)
{
  layer_t ** parent_list = &layer_list;

  if (layer->parent)
    parent_list = &layer->parent->children;

  if (layer->next)
    layer->next->prev = layer->prev;
  
  if (layer->prev)
    layer->prev->next = layer->next;
  else
    (*parent_list) = layer->next;
  
  layer->next = NULL;
  layer->prev = NULL;

  M_DirtyRegion(layer->x1, layer->y1, layer->x2, layer->y2);
}


//
// R_LayerEnable
//
// Enable the layer, making it active and thus will be drawn and can
// receive events.  Updates the dirty matrix appropriately.

void R_LayerEnable(layer_t *layer)
{
  layer->flags |= LAYF_Active;

  M_DirtyRegion(layer->x1, layer->y1, layer->x2, layer->y2);
}


//
// R_LayerDisable
//
// Disable the layer, making it non-active.  It won't be drawn after
// this nor receive any more events.  Updates the dirty matrix
// appropriately.

void R_LayerDisable(layer_t *layer)
{
  layer->flags &= ~LAYF_Active;

  M_DirtyRegion(layer->x1, layer->y1, layer->x2, layer->y2);
}


//
// R_LayerChangeBounds
//
// Change the position and/or size of the layer on the screen.
// Updates the dirty matrix appropriately.  Causes all children layers
// to be resized.

void R_LayerChangeBounds(layer_t *layer, int x1, int y1, int x2, int y2)
{
  if (layer->flags & LAYF_Active)
    M_DirtyRegion(layer->x1, layer->y1, layer->x2, layer->y2);

  layer->x1 = x1;
  layer->y1 = y1;
  layer->x2 = x2;
  layer->y2 = y2;

  if (layer->flags & LAYF_Active)
    M_DirtyRegion(layer->x1, layer->y1, layer->x2, layer->y2);

  // resize children, recursivly

  ResizeLayerList(layer->children, x1, y1, x2, y2);
}


//
// R_LayerChangeFlags
//
// Change the layer's flags, especially the Solid one, updating the
// the dirty matrix appropriately.  Cannot be used to set the Active
// flag, use the Enable/Disable functions for that.

void R_LayerChangeFlags(layer_t *layer, layerflags_e flags)
{
  if (layer->flags & LAYF_Active)
  {
    if ((layer->flags ^ flags) & LAYF_Solid)
    {
      M_DirtyRegion(layer->x1, layer->y1, layer->x2, layer->y2);
    }
  }

  layer->flags = (layer->flags & LAYF_Active) | (flags & ~LAYF_Active);
}


//
// R_LayerChangeContents
//
// Signals that the contents of the layer has changed.  Updates the
// dirty matrix appropriately.

void R_LayerChangeContents(layer_t *layer)
{
  if (layer->flags & LAYF_Active)
    M_DirtyRegion(layer->x1, layer->y1, layer->x2, layer->y2);
}


//
// R_LayerClipRectToSolids
//
// Tests the given rectangle (in x1/y1/x2/y2) against all the solid
// rectangles _before_ the index, shrinking the given rectangle where
// possible.  Returns true if it was totally occluded.  Can only be
// called by "Drawer" functions or by DL_Query().

boolean_t R_LayerClipRectToSolids(int solid_index, 
    int *x1, int *y1, int *x2, int *y2)
{
  DEV_ASSERT2(solid_index >= 0);

  for (solid_index--; solid_index >= 0; solid_index--)
  {
    int sx1 = solid_rects[solid_index]->x1;
    int sy1 = solid_rects[solid_index]->y1;
    int sx2 = solid_rects[solid_index]->x2;
    int sy2 = solid_rects[solid_index]->y2;
    
    int ocode = 0;

    // no overlap ?
    if ((*x2) < sx1 || (*x1) > sx2 || (*y2) < sy1 || (*y1) > sy2)
      continue;
    
    if ((*x1) <= sx1 && (*x2) >= sx2)
      ocode |= 1;

    if ((*y1) <= sy1 && (*y2) >= sy2)
      ocode |= 2;

    switch (ocode)
    {
      case 0:
        // only partially covered in X or Y
        break;
      
      case 3:
        // totally covered
        return true;
      
      case 1:
        // convered in X, only partially in Y
        if ((*y1) <= sy1)
        {
          (*y1) = sy2 + 1;
        }
        else if ((*y2) >= sy2)
        {
          (*y2) = sy1 - 1;
        }

        DEV_ASSERT2((*y2) >= (*y1));
        break;
    
      case 2:
        // convered in Y, only partially in X
        if ((*x1) <= sx1)
        {
          (*x1) = sx2 + 1;
        }
        else if ((*x2) >= sx2)
        {
          (*x2) = sx1 - 1;
        }

        DEV_ASSERT2((*x2) >= (*x1));
        break;

#ifdef DEVELOPERS
      default:
        I_Error("Bad ocode (%d) in R_LayerClipRectToSolids\n", ocode);
#endif 
    }
  }

  return false;
}


//------------------------------------------------------------------------


static boolean_t ListenLayersRecursive(layer_t *list, event_t *ev)
{
  // move to end
  while (list && list->next)
    list = list->next;
  
  for (; list; list = list->prev)
  {
    if (! (list->flags & LAYF_Active))
      continue;

    if (list->children)
    {
      if (ListenLayersRecursive(list->children, ev))
        return true;
    }
    
    // do this layer _after_ children (since conceptually this layer
    // is underneath all of the children layers).
 
    if (list->Listener)
    {
      if ((*list->Listener)(list, ev))
        return true;
    }
  }

  return false;
}


//
// R_ListenLayers
//
// Traverses the layer tree from highest to lowest and passes the
// event to each active layer, exiting early if one of the layers
// "eats" the event.  Returns true if a layer ate the event, otherwise
// false.

boolean_t R_ListenLayers(event_t *ev)
{
  return ListenLayersRecursive(layer_list, ev);
}


static void ResizeLayerList(layer_t *list, int x1, int y1, int x2, int y2)
{
  for (; list; list = list->next)
  {
    DEV_ASSERT2(list->Resizer);

    (* list->Resizer)(list, x1, y1, x2, y2);
  }
}


//
// R_ResizeLayers
//
// Called whenever the video mode changes, allowing each layer to
// move/resize itself.

void R_ResizeLayers(void)
{
  // mark whole screen as dirty
  M_DirtyMatrix();

  ResizeLayerList(layer_list, 0, 0, SCREENWIDTH-1, SCREENHEIGHT-1);
}


//------------------------------------------------------------------------
//
// NOTE: `DL' is short for DrawLayers...
//


static void DL_Init(void)
{
  num_solid_rects = 0;
}


static void DL_CleanUp(void)
{
  // nothing to do
}


// DL_Query
//
// Returns true if a 100% blocking layer was reached.  Coordinates are
// the parent clipping rectangle.
//
static boolean_t DL_Query(layer_t *list, int x1, int y1, int x2, int y2)
{
  // move to end
  while (list && list->next)
    list = list->next;
  
  // move backward (i.e. downward) through list

  for (; list; list = list->prev)
  {
    list->solid_index = -1;

    if (! (list->flags & LAYF_Active))
      continue;
    
    // check if this layer totally outside of parent
    if (list->x2 < x1 || list->x1 > x2 ||
        list->y2 < y1 || list->y1 > y2)
    {
      list->invisible = true;
      continue;
    }

    // determine intersection between this layer & parent clip
    list->clip_x1 = MAX(list->x1, x1);
    list->clip_y1 = MAX(list->y1, y1);
    list->clip_x2 = MIN(list->x2, x2);
    list->clip_y2 = MIN(list->y2, y2);

    DEV_ASSERT2(list->clip_x2 >= list->clip_x1);
    DEV_ASSERT2(list->clip_y2 >= list->clip_y1);

    list->vis_x1 = list->clip_x1;
    list->vis_y1 = list->clip_y1;
    list->vis_x2 = list->clip_x2;
    list->vis_y2 = list->clip_y2;

    list->invisible = R_LayerClipRectToSolids(num_solid_rects, 
        &list->vis_x1, &list->vis_y1, &list->vis_x2, &list->vis_y2);

    if (list->invisible)
      continue;

    // query children layers, recursively

    if (DL_Query(list->children, list->clip_x1, list->clip_y1,
                 list->clip_x2, list->clip_y2))
    {
      // early out, mark all lower layers (including this one !) as
      // invisible.

      for (; list; list=list->prev)
        list->invisible = true;
      
      return true;
    }

    if (list->flags & LAYF_Volatile)
      M_DirtyRegion(list->x1, list->y1, list->x2, list->y2);

    // solid rectangle stuff

    list->solid_index = num_solid_rects;

    if (list->flags & LAYF_Solid)
    {
      // check if layer covers whole screen

      if (list->x1 <= 0 && list->y1 <= 0 &&
          list->x2 >= SCREENWIDTH-1 && 
          list->y2 >= SCREENHEIGHT-1)
      {
        // early out, mark all lower layers (but not this one !) as
        // invisible.

        for (list=list->prev; list; list=list->prev)
          list->invisible = true;
        
        return true;
      }

      // add this layer into array of solid rectangles

      num_solid_rects++;

      if (num_solid_rects > max_solid_rects)
      {
        Z_Resize(solid_rects, layer_t *, num_solid_rects);

        while (max_solid_rects < num_solid_rects)
          solid_rects[max_solid_rects++] = NULL;
      }

      solid_rects[num_solid_rects-1] = list;
    }
  }

  return false;
}


static void DL_Draw(layer_t *list)
{
  // move forward (i.e. upward) through list

  for (; list; list=list->next)
  {
    if (! (list->flags & LAYF_Active))
      continue;

    if (list->invisible)
      continue;
    
    DEV_ASSERT2(list->solid_index >= 0);

    if (list->Drawer)
    {
      (* list->Drawer)(list);
    }

    // draw children layers, recursively.  Called here since
    // conceptually the children layers are above their parent.

    DL_Draw(list->children);
  }
}


//
// R_DrawLayers
//
// Traverse the layer tree and draws everything that needs to be
// drawn.  When the Drawer() functions in each layer are called, the
// dirty matrix must be used to clip any drawing to the clean areas
// (unless flagged with LAYF_Volatile).  Drawing must also be clipped
// to the layer's current clip rectangle.  Drawing can be optionally
// clipped to the solid areas too, for improved performance.
//
void R_DrawLayers(void)
{
  DL_Init();
  DL_Query(layer_list, 0, 0, SCREENWIDTH-1, SCREENHEIGHT-1);
  DL_Draw(layer_list);
  DL_CleanUp();
}

#endif // USE_LAYERS
