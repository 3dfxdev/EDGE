//----------------------------------------------------------------------------
//  EDGE Wolfenstein/Rise of the Triad Local Definitions
//----------------------------------------------------------------------------
//
//  Copyright (c) 1999-2018  The EDGE Team.
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
//  Based on the Wolfenstein and DOOM source codes, released by Id Software 
//  under the following copyright:
//
//    Copyright (C) 1993-1996 by id Software, Inc.
//
//  Based on the Rise of the Triad source codes, release by Apogee Software
//
//    Copyright (C) 1994-1995 Apogee Software, Ltd.
//
//----------------------------------------------------------------------------

#ifndef __WOLF_LOCAL_H__
#define __WOLF_LOCAL_H__

#include "wlf_rawdef.h"
#include "../../r_defs.h"

#include "../../../epi/image_data.h"
#include "../../../epi/math_vector.h"

// --- globals ---

#define WLFMAP_W  64
#define WLFMAP_H  64

extern u16_t *wlf_map_tiles;
extern u16_t *wlf_obj_tiles;


// ---- wf_colors ----

extern const byte wolf_palette[256*3];
extern const byte rott_palette[256 * 3];
void CreatePlaypal(void);
void CreateROTTpal(void);



// ---- wf_util ----

void WF_Carmack_Expand(const byte *source, byte *dest, int length);
void WF_RLEW_Expand(const u16_t *source, u16_t *dest, int length, u16_t rlew_tag);
void WF_Huff_Expand(const byte *source, int src_len, byte *dest, int dest_len, const huffnode_s *table);


// ---- wf_maps ----
void TinyBSP(void);
void WF_BuildBSP(void);
void WF_InitMaps(void);

void WF_LoadMap(int map_num);
void WF_FreeMap(void);

void WF_SetupLevel(void);

// ---- wf_setup ----

vertex_t *WF_GetVertex(int vx, int vy);

// ---- wf_vswap ----
//class graph_info_c vga_info;
void WF_VSwapOpen(void);

epi::image_data_c *WF_VSwapLoadWall(int index);
epi::image_data_c *WF_VSwapLoadSprite(int index);

// ---- wf_graph ----

void WF_GraphicsOpen(void);

epi::image_data_c *WF_GraphLoadPic(int chunk);


#endif // __WOLF_LOCAL_H__
