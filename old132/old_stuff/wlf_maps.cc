
/* WOLF MAPS */

#include "i_defs.h"

#include <stdio.h>

#include <vector>

#include "epi/endianess.h"

#include "wf_local.h"


//!!!!  const char *wlf_extension = "wl6";


u16_t *wlf_map_tiles;
u16_t *wlf_obj_tiles;


static int map_count;

static raw_maphead_t map_head;


static inline void FROM_LE_U16(u16_t& val)
{
	val = EPI_LE_U16(val);
}

static inline void FROM_LE_U32(u32_t& val)
{
	val = EPI_LE_U32(val);
}


static void MapsReadHeaders()
{
	FILE *fp = fopen("MAPHEAD.WL6", "rb");
	if (!fp)
		I_Error("Missing MAPHEAD.WL6 file!\n");

	map_count = 0;

	memset(&map_head, 0, sizeof(map_head));

	if (fread(&map_head, 1, sizeof(map_head), fp) < 6)
		throw "MapsReadHeaders: fucked";

	map_head.rlew_tag = EPI_LE_U16(map_head.rlew_tag);

	for (int i=0; i < 100; i++)
	{
		if (map_head.offsets[i] == 0)
			break;

		FROM_LE_U32(map_head.offsets[i]);

// fprintf(stderr, "Header offset[%d] = %d\n", i, map_head.offsets[i]);
		map_count++;
	}

	fclose(fp);

	if (map_count == 0)
		throw "MapsReadHeaders: no maps!";
	
fprintf(stderr, "Num MAPS : %d\n", map_count);
}


//------------------------------------------------------------------------

static void LoadMapInfo(FILE *fp, int offset, raw_gamemap_t *gmp)
{
fprintf(stderr, "LoadMapInfo: offset = %d\n", offset);
	// move to correct position in file
	fseek(fp, offset, SEEK_SET);

fprintf(stderr, "Reading %d bytes...\n", sizeof(raw_gamemap_t));
	// FIXME: check error
	fread(gmp, sizeof(raw_gamemap_t), 1, fp);

	FROM_LE_U32(gmp->plane_offset[0]);
	FROM_LE_U32(gmp->plane_offset[1]);
	FROM_LE_U32(gmp->plane_offset[2]);

	FROM_LE_U16(gmp->plane_length[0]);
	FROM_LE_U16(gmp->plane_length[1]);
	FROM_LE_U16(gmp->plane_length[2]);

	FROM_LE_U16(gmp->width);
	FROM_LE_U16(gmp->height);

fprintf(stderr, "Planes: %d, %d, %d\n",
gmp->plane_offset[0], gmp->plane_offset[1], gmp->plane_offset[2]);
fprintf(stderr, "Lengths: %d, %d, %d\n",
gmp->plane_length[0], gmp->plane_length[1], gmp->plane_length[2]);

	// ensure string is terminated
	gmp->name[16] = 0;
}

static u16_t *LoadMapPlane(FILE *fp, int offset, int complen, int width, int height)
{
	int expected = width * height * 2;

	// seek to correct position in file
fprintf(stderr, "Seeking to %d\n", offset);
	fseek(fp, offset, SEEK_SET);

	byte *buf1 = new byte[complen];

fprintf(stderr, "Reading %d bytes...\n", complen);
	// !!!! FIXME check error/underrun
	fread(buf1, complen, 1, fp);

	int size2 = buf1[0] | ((int) buf1[1] << 8);
// CHECK STUFF !!!
	
	byte *buf2 = new byte[size2];

	WF_Carmack_Expand(buf1+2, buf2, size2);

	delete[] buf1;

	int size_r = buf2[0] | ((int) buf2[1] << 8);

	SYS_ASSERT(size_r == expected);

	u16_t *result = new u16_t[expected / 2];

	WF_RLEW_Expand((u16_t *)(buf2 + 2), result, expected, map_head.rlew_tag);

	delete[] buf2;

	// flip plane upsidedown
	for (int y=0; y < height/2; y++)
	for (int x=0; x < width; x++)
	{
		u16_t tmp = result[y * width + x];

		result[y * width + x] = result[(height-1-y) * width + x];
		result[(height-1-y) * width + x] = tmp;
	}

	return result;
}


void WF_LoadMap(int map_num)
{
	SYS_ASSERT(map_num >= 0);
	SYS_ASSERT(map_num < map_count);
	SYS_ASSERT(map_head.offsets[map_num] > 0);

	// Read gamefile
	FILE *fp = fopen("GAMEMAPS.WL6", "rb");

	if (! fp)
	{
		// try alternate name: MAPTEMP
		fp = fopen("MAPTEMP.WL6", "rb");
	}

	if (! fp)
		throw "WLF_LoadMap: missing GAMEMAPS file !";

	// load in info
	raw_gamemap_t gmp;

	LoadMapInfo(fp, map_head.offsets[map_num], &gmp);

	// load in plane #0 (MAP)
	wlf_map_tiles = LoadMapPlane(fp, gmp.plane_offset[0],
			gmp.plane_length[0], gmp.width, gmp.height);

	// load in plane #1 (OBJECTS)
	wlf_obj_tiles = LoadMapPlane(fp, gmp.plane_offset[1],
			gmp.plane_length[1], gmp.width, gmp.height);

	L_WriteDebug("------>\n");

	for (int y=63; y >= 0; y--)
	{
		for (int x=0; x < 64; x++)
		{
			short M = wlf_map_tiles[y*64+x];
			short J = wlf_obj_tiles[y*64+x];

			if (J != 0)
				L_WriteDebug("%c", 97+(J%26));
			else if (M < 64)
				L_WriteDebug("%c", 65+(M%26));
			else
				L_WriteDebug(" ");
		}

		L_WriteDebug("\n");
	}

	L_WriteDebug("<------\n");
}

void WF_FreeMap(void)
{
	// FIXME
}

void WF_InitMaps(void)
{
	MapsReadHeaders();

//	WF_LoadMap(0);  // !!!! TEST
}

