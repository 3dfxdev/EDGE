//----------------------------------------------------------------------------
//  EDGE Layer Rendering System
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
//
//  See the file "docs/layersys.txt" for a complete description of the
//  layer system.
//
//  -AJA- 2000/06/13: Started work on this file.
//

#ifndef __R_LAYERS__
#define __R_LAYERS__


//
// Flags/Properties
//

typedef enum
{
  // Whether this layer is active or not.  Only active layers get
  // drawn and receive events.
  
  LAYF_Active = 0x0001,
  
  // The layer is completely solid, nothing inside the layer bounding
  // rectangle will show through.
  
  LAYF_Solid = 0x0002,

  // The layer is Volatile, which means the contents are always
  // changing.  Camera views are examples of volatile layers.  Use
  // this for "Non-Clippable" layers as well
  
  LAYF_Volatile = 0x0004
}
layerflags_e;


//
// LAYER itself
//

typedef struct layer_s
{
	// Link in list.

	struct layer_s *next;
	struct layer_s *prev;

	// Parent layer, or NULL if this is a top level layer.

	struct layer_s *parent;

	// Child list (often empty).  All children layers are considered to
	// sit above their parent.

	struct layer_s *children;

	// Depth.  Higher layers are drawn later than lower ones.  Multiple
	// layers of the same depth are allowed, but the results are
	// undefined if they overlap.

	int depth;

	// Rectangular bounds of layer on the screen.  (x,y) coordinates are
	// absolute.  Coordinates are inclusive.

	int x1, y1, x2, y2;

	// Various flags/properties for the layer.

	layerflags_e flags;

	// Draw routine.  Can be NULL (for listen-only layers).  Doesn't
	// need to handle child layers, they will be called automatically.
	// All drawing MUST be clipped to the current clipping rectangle (in
	// the clip_* fields).

	void (* Drawer)(struct layer_s *layer);

	// Event handler.  Can be NULL (for draw-only layers).  Doesn't need
	// to handle child layers, they will be called automatically.

	bool (* Listener)(struct layer_s *layer, event_t *ev);

	// Resize routine, called whenever the parent layer's bounding box
	// changes.  For top level layers, this means whenever the user
	// selects a different video mode.  Must not be NULL !  The routine
	// must call R_LayerChangeBounds() on itself with the new size; this
	// will take care of resizing any children layers.  The parameters
	// are the parent layer bounds.

	void (* Resizer)(struct layer_s *layer, int x1, int y1, int x2, int y2);

	// Private pointer, store any layer-specific information here.  Must
	// be allocated with new or similiar if not NULL.

	void *priv;

	// --- stuff private to layer system itself ---

	// Layer is not visible (e.g. completely covered by a higher layer
	// that is solid, or lies totally outside parent's bounding box).

	bool invisible;

	// Index into stack array of solid rectangles.  All solid rects
	// strictly _before_ this value can be used for clipping.

	int solid_index;

	// Current clipping rectangle (based on parent and current layer
	// dimensions).  Only valid when Drawer() is called, and the draw
	// function MUST clip to this rectangle. 

	int clip_x1, clip_y1, clip_x2, clip_y2;

	// Visible size of the layer (assuming `invisible' field is false)
	// once clipped to the solid rectangles.  No need to draw anything
	// outside of this rectangle, e.g. a rendering window can skip
	// columns and spans the lie totally outside.  Only valid when
	// Drawer() is called.  This rectangle is guaranteed to exist
	// completely within the clip rectangle.

	int vis_x1, vis_y1, vis_x2, vis_y2;
}
layer_t;


extern layer_t *layer_list;


//
// SOLID RECTS
//

extern layer_t ** solid_rects;
extern int num_solid_rects;


//
// INTERFACE
//

bool R_LayerInit(void);

layer_t *R_LayerNew(int depth, int x1, int y1, int x2, int y2, layerflags_e flags, void *priv);
void R_LayerDestroy(layer_t *layer);

void R_LayerAdd(layer_t *parent, layer_t *layer);
void R_LayerRemove(layer_t *layer);

void R_LayerEnable(layer_t *layer);
void R_LayerDisable(layer_t *layer);

void R_LayerChangeBounds(layer_t *layer, int x1, int y1, int x2, int y2);
void R_LayerChangeFlags(layer_t *layer, layerflags_e flags);
void R_LayerChangeContents(layer_t *layer);

bool R_LayerClipRectToSolids(int solid_index,
    int *x1, int *y1, int *x2, int *y2);

void R_DrawLayers(void);
bool R_ListenLayers(event_t *ev);
void R_ResizeLayers(void);


#endif  // __R_LAYERS__
