
/* WOLF LOCAL DEFS */

#ifndef __WOLF_LOCAL_H__
#define __WOLF_LOCAL_H__

#include "../../../epi/image_data.h"
#include "wlf_rawdef.h"


// --- globals ---

#define WLFMAP_W  64
#define WLFMAP_H  64

extern u16_t *wlf_map_tiles;
extern u16_t *wlf_obj_tiles;


// ---- wf_colors ----

extern const byte wolf_palette[256*3];
void CreatePlaypal(void);


// ---- wf_util ----

void WF_Carmack_Expand(const byte *source, byte *dest, int length);
void WF_RLEW_Expand(const u16_t *source, u16_t *dest, int length, u16_t rlew_tag);
void WF_Huff_Expand(const byte *source, int src_len, byte *dest, int dest_len, const huffnode_s *table);


// ---- wf_maps ----
void TinyBSP(void);
void WF_InitMaps(void);

void WF_LoadMap(int map_num);
void WF_FreeMap(void);

void WF_SetupLevel(void);

// ---- wf_vswap ----
//class graph_info_c vga_info;
void WF_VSwapOpen(void);

epi::image_data_c *WF_VSwapLoadWall(int index);
epi::image_data_c *WF_VSwapLoadSprite(int index);

// ---- wf_graph ----

void WF_GraphicsOpen(void);

epi::image_data_c *WF_GraphLoadPic(int chunk);


#endif // __WOLF_LOCAL_H__
