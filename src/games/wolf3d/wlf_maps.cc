/* WOLF MAPS */

#include "../../system/i_defs.h"

#include <vector>
#include <map>

#include "../../../epi/epi.h"
#include "../../../epi/file.h"
#include "../../../epi/filesystem.h"
#include "../../../epi/endianess.h"
#include "../../../epi/math_crc.h"

#include "../../../ddf/main.h"
#include "../../../ddf/colormap.h"

#include "wlf_local.h"
#include "wlf_rawdef.h"

#include "../../dm_defs.h"
#include "../../dm_data.h"
#include "../../dm_state.h"
#include "../../dm_structs.h"
#include "../../e_main.h"
#include "../../g_game.h"
#include "../../l_glbsp.h"
#include "../../e_player.h"
#include "../../m_argv.h"
#include "../../m_bbox.h"
#include "../../m_misc.h"
#include "../../m_random.h"
#include "../../p_local.h"
#include "../../p_setup.h" //<--- Anything defined here already should be modified here, and commented out in this file!
#include "../../r_defs.h"
#include "../../r_gldefs.h"
#include "../../r_sky.h"
#include "../../s_sound.h"
#include "../../s_music.h"
#include "../../sv_main.h"
#include "../../r_image.h"
#include "../../w_texture.h"
#include "../../w_wad.h"
#include "../../z_zone.h"
#include "../../vm_coal.h"

#if 0
namespace
{
	const char *wlf_extension = "wlf_maphead.wl6";
	const char *wlf_maptemp = "wlf_maptemp.wl6";
}
#endif // 0


u16_t *wlf_map_tiles;
u16_t *wlf_obj_tiles;

static int map_count;

static raw_maphead_t map_head;

static inline void FROM_LE_U16(u16_t& val)
{
	val = EPI_LE_U16(val); //TODO: V570 https://www.viva64.com/en/w/v570/ The 'val' variable is assigned to itself.
}

static inline void FROM_LE_U32(u32_t& val)
{
	val = EPI_LE_U32(val); //TODO: V570 https://www.viva64.com/en/w/v570/ The 'val' variable is assigned to itself.
}

bool MapsReadHeaders()
{
	FILE *fp = fopen("MAPHEAD.WL6", "rb");
	if (!fp)
		I_Error("Missing MAPHEAD.WL6 file!\n");

	//unsigned int map_count = (raw_gamemap_t->plane_length() - 2) / 4;
	map_count = 0;

	memset(&map_head, 0, sizeof(map_head));

	if (fread(&map_head, 1, sizeof(map_head), fp) < 6) //TODO: V1004 https://www.viva64.com/en/w/v1004/ The 'fp' pointer was used unsafely after it was verified against nullptr. Check lines: 72, 80.
		throw "MapsReadHeaders: fucked"; 
	//TODO: V773 https://www.viva64.com/en/w/v773/ The exception was thrown without closing the file referenced by the 'fp' handle. A resource leak is possible.

	map_head.rlew_tag = EPI_LE_U16(map_head.rlew_tag); 
	//TODO: V570 https://www.viva64.com/en/w/v570/ The 'map_head.rlew_tag' variable is assigned to itself.

	for (int i = 0; i < 100; i++)
	{
		if (map_head.offsets[i] == 0)
			break;

		FROM_LE_U32(map_head.offsets[i]);

		I_Printf("Header offset[%d] = %d\n", i, map_head.offsets[i]);
		map_count++;
	}

	fclose(fp);

	if (map_count == 0)
		I_Error("MapsReadHeaders: no maps!");

	I_Printf("Number of maps found : %d\n", map_count);
}

//------------------------------------------------------------------------

static void LoadMapInfo(FILE *fp, int offset, raw_gamemap_t *gmp)
{
	I_Printf("LoadMapInfo: offset = %d\n", offset);

	// move to correct position in file
	fseek(fp, offset, SEEK_SET);

	I_Printf("Reading %lu bytes...\n", sizeof(raw_gamemap_t));
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

	I_Printf("Planes: %d, %d, %d\n",
		gmp->plane_offset[0], gmp->plane_offset[1], gmp->plane_offset[2]);
	I_Printf("Lengths: %d, %d, %d\n",
		gmp->plane_length[0], gmp->plane_length[1], gmp->plane_length[2]);

	// ensure string is terminated
	gmp->name[16] = 0; 
	//TODO: V557 https://www.viva64.com/en/w/v557/ Array overrun is possible. The '16' index is pointing beyond array bound.
}

static u16_t *LoadMapPlane(FILE *fp, int offset, int complen, int width, int height)
{
	int expected = width * height * 2;

	// seek to correct position in file
	I_Printf("Seeking to %d\n", offset);
	fseek(fp, offset, SEEK_SET);

	byte *buf1 = new byte[complen];

	I_Printf("Reading %d bytes...\n", complen);
	// !!!! FIXME check error/underrun
	fread(buf1, complen, 1, fp);

	int size2 = buf1[0] | ((int)buf1[1] << 8);
	// CHECK STUFF !!!

	byte *buf2 = new byte[size2];

	WF_Carmack_Expand(buf1 + 2, buf2, size2);

	delete[] buf1;

	int size_r = buf2[0] | ((int)buf2[1] << 8); //TODO: V557 https://www.viva64.com/en/w/v557/ Array overrun is possible. The '1' index is pointing beyond array bound.

	SYS_ASSERT(size_r == expected);

	u16_t *result = new u16_t[expected / 2];

	//mapplanes[plane] = Z_Malloc(expected, &mapplanes[plane]);

	WF_RLEW_Expand((u16_t *)(buf2 + 2), result, expected, map_head.rlew_tag);

	delete[] buf2;

	// flip plane upsidedown
	I_Printf("Flipping plane upsidedown...\n");
	for (int y = 0; y < height / 2; y++)
		for (int x = 0; x < width; x++)
		{
			u16_t tmp = result[y * width + x];

			result[y * width + x] = result[(height - 1 - y) * width + x];
			result[(height - 1 - y) * width + x] = tmp;
		}

	return result;
}

void WF_LoadMap(int map_num)
{
	I_Printf("Wolf: Loading Map  %d\n", map_num);
	SYS_ASSERT(map_num >= 0);
	SYS_ASSERT(map_num < map_count);
	SYS_ASSERT(map_head.offsets[map_num] > 0);

	// Read gamefile
	FILE *fp = fopen("GAMEMAPS.WL6", "rb");

	if (!fp)
	{
		// try alternate name: MAPTEMP
		fp = fopen("MAPTEMP.WL6", "rb");
	}

	if (!fp)
		I_Error("WLF_LoadMap: missing GAMEMAPS file !");

	// load in info
	raw_gamemap_t gmp;

	LoadMapInfo(fp, map_head.offsets[map_num], &gmp); //TODO: V1004 https://www.viva64.com/en/w/v1004/ The 'fp' pointer was used unsafely after it was verified against nullptr. Check lines: 205, 211.

	// load in plane #0 (MAP)
	wlf_map_tiles = LoadMapPlane(fp, gmp.plane_offset[0],
		gmp.plane_length[0], gmp.width, gmp.height);

	// load in plane #1 (OBJECTS)
	wlf_obj_tiles = LoadMapPlane(fp, gmp.plane_offset[1],
		gmp.plane_length[1], gmp.width, gmp.height);

	L_WriteDebug("------>\n");

	for (int y = 63; y >= 0; y--)
	{
		for (int x = 0; x < 64; x++)
		{
			short M = wlf_map_tiles[y * 64 + x];
			short J = wlf_obj_tiles[y * 64 + x];

			if (J != 0)
				L_WriteDebug("%c", 97 + (J % 26));
			else if (M < 64)
				L_WriteDebug("%c", 65 + (M % 26));
			else
				L_WriteDebug(" ");
		}

		L_WriteDebug("\n");
	}

	L_WriteDebug("<------\n");

	return;
}

void WF_FreeMap(void)
{
	// FIXME
}

extern void WF_BuildBSP(void);

void WF_InitMaps(void)
{
	MapsReadHeaders();

	I_Printf("WOLF: MapsReadHeaders successful!\n");

	return;
	//WF_BuildBSP();  // !!!! TEST
}


