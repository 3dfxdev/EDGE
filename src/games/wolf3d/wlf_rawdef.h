
/* WOLF RAW STRUCTS */

#ifndef __WF_RAWDEF_H__
#define __WF_RAWDEF_H__

//!!!!!! FIXME:
#if 0
#define GAT_PACK  __attribute__ ((packed))

#endif // 0
//
// structure of the VSWAP file.
//
typedef struct raw_vswap_s
{
	u16_t total_chunks;
	u16_t first_sprite;
	u16_t first_sound;
}
raw_vswap_t;

typedef struct raw_chunk_s
{
	u32_t offset;
	u16_t length;
}
raw_chunk_t;

typedef struct raw_shape_s
{
	u16_t leftpix, rightpix;
	u16_t offsets[64];
}
raw_shape_t;

//
// structure of the MAPHEAD file.
//
typedef struct raw_maphead_s
{
  // tag used for RLE_Expand when decompressing the data.
  // (always 0xABCD from what I've seen -- AJA).
  u16_t rlew_tag;

  // offset into GAMEMAPS file where each map header is found.  The
  // list of maps ends when <= 0 is encountered.
  u32_t offsets[100];// GAT_PACK;
}
raw_maphead_t;

//
// structure of the GAMEMAPS file.
//
typedef struct raw_gamemap_s
{
  // offset into file where the plane starts
  u32_t plane_offset[3];

  // actual (uncompressed) length of plane data
  u16_t plane_length[3];

  // size of map (always 64 by 64 it seems)
  u16_t width, height;

  // name of map
  char name[16];
}
raw_gamemap_t;

//
// structure of the VGAGRAPH file.
//
typedef struct huffnode_s
{
	u16_t bit0, bit1;
}
huffnode_t;

typedef struct raw_picsize_s
{
	s16_t width, height;
}
raw_picsize_t;

#endif // __WF_RAWDEF_H__
