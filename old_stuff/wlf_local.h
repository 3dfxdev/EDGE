
/* WOLF LOCAL DEFS */

#ifndef __WOLF_LOCAL_H__
#define __WOLF_LOCAL_H__

#include "wf_rawdef.h"
#include "r_defs.h"

#include "epi/basicimage.h"

// --- globals ---

#define WLFMAP_W  64
#define WLFMAP_H  64

extern u16_t *wlf_map_tiles;
extern u16_t *wlf_obj_tiles;


// ---- wf_colors ----

extern const byte wolf_palette[256*3];


// ---- wf_util ----

void WF_Carmack_Expand(const byte *source, byte *dest, int length);
void WF_RLEW_Expand(const u16_t *source, u16_t *dest, int length, u16_t rlew_tag);
void WF_Huff_Expand(const byte *source, int src_len, byte *dest, int dest_len, 
				 const huffnode_t *table);


// ---- wf_maps ----

void WF_InitMaps(void);

void WF_LoadMap(int map_num);
void WF_FreeMap(void);


// ---- wf_bsp ----

void WF_BuildBSP(void);


// ---- wf_setup ----

vertex_t *WF_GetVertex(int vx, int vy);


// ---- wf_vswap ----

void WF_VSwapOpen(void);

epi::basicimage_c *WF_VSwapLoadWall(int index);
epi::basicimage_c *WF_VSwapLoadSprite(int index);

// ---- wf_graph ----

void WF_GraphicsOpen(void);

epi::basicimage_c *WF_GraphLoadPic(int chunk);


// ---- other shite ----

void P_SpawnMapThing(const mobjtype_c *info,
					  float x, float y, float z,
					  angle_t angle, int options, int tag);


#endif // __WOLF_LOCAL_H__
