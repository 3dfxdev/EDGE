//----------------------------------------------------------------------------
//  EDGE Packed Data Support Code
//  WAD, EPK, PAK, ZIP, PK3/PK7, WL6, PhysFS Handler
//----------------------------------------------------------------------------
//
//  Copyright (c) 1999-2021  The EDGE Team.
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
// This file contains various levels of support for using sprites and
// flats directly from a PWAD as well as some minor optimisations for
// patches. Because there are some PWADs that do arcane things with
// sprites, it is possible that this feature may not always work (at
// least, not until I become aware of them and support them) and so
// this feature can be turned off from the command line if necessary.
//
// -MH- 1998/03/04
//

#include "system/i_defs.h"

#include <limits.h>

#include <list>

#include "../epi/endianess.h"
#include "../epi/file.h"
#include "../epi/file_sub.h"
#include "../epi/filesystem.h"
#include "../epi/math_md5.h"
#include "../epi/path.h"
#include "../epi/str_format.h"
#include "../epi/utility.h"

#include "../ddf/main.h"
#include "../ddf/anim.h"
#include "../ddf/colormap.h"
#include "ddf/flat.h"
#include "../ddf/font.h"
#include "../ddf/image.h"
#include "../ddf/style.h"
#include "../ddf/switch.h"

#include "dm_data.h"
#include "dm_defs.h"
#include "dm_state.h"
#include "dm_structs.h"
#include "dstrings.h"
#include "e_main.h"
#include "e_search.h"
#include "l_deh.h"
#include "l_ajbsp.h"
#include "m_misc.h"
#include "r_image.h"
#include "rad_trig.h"
#include "vm_coal.h"
#include "games/wolf3d/wlf_local.h"
#include "games/wolf3d/wlf_rawdef.h"
#include "w_wad.h"
#include "z_zone.h"

#ifdef HAVE_PHYSFS
#include <physfs.h>
#endif

extern void CreatePlaypal(); //Wolfenstein 3D
extern void CreateROTTpal(); // Rise of the Triad
//extern std::string iwad_base;

// -KM- 1999/01/31 Order is important, Languages are loaded before sfx, etc...
typedef struct ddf_reader_s
{
	const char *name;
	const char *print_name;
	bool(*func)(void *data, int size);
}
ddf_reader_t;

static ddf_reader_t DDF_Readers[] =
{
	{ "DDFLANG", "Languages",  DDF_ReadLangs },
	{ "DDFSFX",  "Sounds",     DDF_ReadSFX },
	{ "DDFCOLM", "ColourMaps", DDF_ReadColourMaps },  // -AJA- 1999/07/09.
	{ "DDFIMAGE","Images",     DDF_ReadImages },      // -AJA- 2004/11/18
	{ "DDFFONT", "Fonts",      DDF_ReadFonts },       // -AJA- 2004/11/13
	{ "DDFSTYLE","Styles",     DDF_ReadStyles },      // -AJA- 2004/11/14
	{ "DDFATK",  "Attacks",    DDF_ReadAtks },
	{ "DDFWEAP", "Weapons",    DDF_ReadWeapons },
	{ "DDFTHING","Things",     DDF_ReadThings },
	{ "DDFPLAY", "Playlists",  DDF_ReadMusicPlaylist },
	{ "DDFLINE", "Lines",      DDF_ReadLines },
	{ "DDFSECT", "Sectors",    DDF_ReadSectors },
	{ "DDFSWTH", "Switches",   DDF_ReadSwitch },
	{ "DDFANIM", "Anims",      DDF_ReadAnims },
	{ "DDFGAME", "Games",      DDF_ReadGames },
	{ "DDFLEVL", "Levels",     DDF_ReadLevels },
	{ "DDFFLAT", "Flats",     DDF_ReadFlat },
	{ "RSCRIPT", "RadTrig",    RAD_ReadScript }       // -AJA- 2000/04/21.
};

#define NUM_DDF_READERS  (int)(sizeof(DDF_Readers) / sizeof(ddf_reader_t))

#define LANG_READER  0
#define COLM_READER  2
#define IMAG_READER  4
#define SWTH_READER  12
#define ANIM_READER  13
#define GAME_READER  14
#define RTS_READER   16

#define NULL_INDEX		(0xffffffff)

class data_file_c
{
public:
	const char *file_name;

	// type of file (FLKIND_XXX)
	int kind;

	// file object
	epi::file_c *file;

	// lists for sprites, flats, patches (stuff between markers)
	epi::u32array_c sprite_lumps;
	epi::u32array_c flat_lumps;
	epi::u32array_c patch_lumps;
	epi::u32array_c colmap_lumps;
	epi::u32array_c tx_lumps;
	epi::u32array_c hires_lumps;
	//epi::u32array_c shader_lumps;
	epi::u32array_c lbm_lumps;
	epi::u32array_c rottpic_lumps;
	epi::u32array_c rottraw_flats;

	// level markers and skin markers
	epi::u32array_c level_markers;
	epi::u32array_c skin_markers;
	//epi::u32array_c rottraw_markers;

	// ddf lump list
	int ddf_lumps[NUM_DDF_READERS];

	// texture information
	wadtex_resource_c wadtex;

	// Wolfenstein texture information (basic)
	raw_vswap_t vv;

	// Rise of the Triad LBM (only used for 5 images)
	int lbm_pic;

	int pic_rott;

	// Rise of the Triad RAW (only used for FLRCL flats and PLANE/TRILOGO)
	int raw_flats;

	// DeHackEd support
	int deh_lump;

	// COAL scripts
	int coal_huds;
	int coal_api;

	// GLSL Shaders
	int shader_files;

	// ROQ Cinematics
	int roq_videos;

	// BOOM stuff
	int animated, switches;

	// rottpic LPIC RAW
	//int rott_lpic;

	// raw LBM lumps!
	//int lbm_lumps;

	// file containing the GL nodes for the levels in this WAD.
	// -1 when none (usually when this WAD has no levels, but also
	// temporarily before a new GWA files has been built and added).
	int companion_gwa;

	//int rottpic_lumps;

	// MD5 hash of the contents of the WAD directory.
	// This is used to disambiguate cached GWA/HWA filenames.
	epi::md5hash_c dir_hash;

public:
	data_file_c(const char *_fname, int _kind, epi::file_c* _file) :
		file_name(_fname), kind(_kind), file(_file),
		sprite_lumps(), flat_lumps(), patch_lumps(),
		colmap_lumps(), tx_lumps(), hires_lumps(),
		lbm_lumps(), rottpic_lumps(), rottraw_flats(),
		level_markers(), skin_markers(),
		wadtex(), vv(), lbm_pic(-1), pic_rott(-1), raw_flats(-1), deh_lump(-1), coal_huds(-1),
		coal_api(-1), shader_files(-1), roq_videos(-1), animated(-1), switches(-1),
		companion_gwa(-1), dir_hash()
	{
		file_name = strdup(_fname);

		for (int d = 0; d < NUM_DDF_READERS; d++)
			ddf_lumps[d] = -1;
	}

	~data_file_c()
	{
		free((void*)file_name);
	}
};

static std::vector<data_file_c *> data_files;

// Raw filenames
class raw_filename_c
{
public:
	std::string filename;
	int kind;

public:
	raw_filename_c(const char *_name, int _kind) :
		filename(_name), kind(_kind)
	{ }

	~raw_filename_c()
	{ }
};

static std::list<raw_filename_c *> wadfiles;
//static std::list<raw_filename_c *> r_pak_fp;
static std::list<raw_filename_c *> wl6files;

typedef enum
{
	LMKIND_Normal = 0,  // fallback value
	LMKIND_Marker = 3,  // X_START, X_END, S_SKIN, level name
	LMKIND_WadTex = 6,  // palette, pnames, texture1/2
	LMKIND_DDFRTS = 10, // DDF, RTS, DEHACKED lump
	LMKIND_TX = 14,
	LMKIND_Colmap = 15,
	LMKIND_Flat = 16,
	LMKIND_Sprite = 17,
	LMKIND_Patch = 18,
	LMKIND_HiRes = 19,
	LMKIND_Shaders = 20,
	LMKIND_ROQ = 21,
	LMKIND_LBM = 22,
	LMKIND_PIC = 23, //imsrc_rottpic
	LMKIND_RAWFLATS = 24
}
lump_kind_e;

typedef struct
{
	char name[10];
	std::string fullname;
	int position;
	int size;

	// file number (an index into data_files[]).
	short file;

	// value used when sorting.  When lumps have the same name, the one
	// with the highest sort_index is used (W_CheckNumForName).  This is
	// closely related to the `file' field, with some tweaks for
	// handling GWA files (especially dynamic ones).
	short sort_index;

	// one of the LMKIND values.  For sorting, this is the least
	// significant aspect (but still necessary).
	short kind;
#ifdef HAVE_PHYSFS
	// pathname for PHYSFS file - wasteful, but no biggy on a PC
	char path[256];

#endif
}
lumpinfo_t;

// Create the start and end markers

//
//  GLOBALS
//

// Location of each lump on disk.
static std::vector<lumpinfo_t> lumpinfo;
static int *lumpmap = NULL;
int numlumps;

#define LUMP_MAP_CMP(a) (strncmp(lumpinfo[lumpmap[a]].name, buf, 8))


typedef struct lumpheader_s
{
#ifdef DEVELOPERS
	static const int LUMPID = (int)0xAC45197e;

	int id;  // Should be LUMPID
#endif

			 // number of users.
	int users;

	// index in lumplookup
	int lumpindex;
	struct lumpheader_s *next, *prev;
}
lumpheader_t;

static lumpheader_t **lumplookup;
static lumpheader_t lumphead;

// number of freeable bytes in cache (excluding headers).
// Used to decide how many bytes we should flush.
static int cache_size = 0;

// the first datafile which contains a PLAYPAL lump
static int palette_datafile = -1;
// the last datafile which contains a PLAYPAL lump
static int palette_lastfile = -1;

bool modpalette = false;

// Sprites & Flats
static bool within_sprite_list;
static bool within_flat_list;
static bool within_patch_list;
static bool within_lbm_list;
static bool within_rottpic_list; //lpic_t
static bool within_rawflats_list;
static bool within_colmap_list;
static bool within_tex_list;
static bool within_hires_list;

static int num_paks = 0;

static int zmarker;

byte *W_ReadLumpAlloc(int lump, int *length);

//
// Is the name a sprite list start flag?
// If lax syntax match, fix up to standard syntax.
//
static bool IsS_START(char *name)
{
	if (strncmp(name, "GUNSTART", 8) == 0) //|| (strncmp(name, "SHAPSTOP", 8) == 0))
	{
		//fix up flag to standard syntax
	   // Note: strncpy will pad will nulls
		I_Printf("ROTT: GUNSTART Weapon Sprites...\n");
		strncpy(name, "S_START", 8);
		return 1;
	}

	if (strncmp(name, "SS_START", 8) == 0)
	{
		// fix up flag to standard syntax
		// Note: strncpy will pad will nulls
		strncpy(name, "S_START", 8);
		return 1;
	}

	return (strncmp(name, "S_START", 8) == 0);
}

//
// Is the name a sprite list end flag?
// If lax syntax match, fix up to standard syntax.
//
static bool IsS_END(char *name)
{

	if (strncmp(name, "PAL", 8) == 0) //|| (strncmp(name, "SHAPSTOP", 8) == 0))
	{
		//fix up flag to standard syntax
	   // Note: strncpy will pad will nulls
		I_Printf("ROTT: End of Weapon Sprites (stops at entry PAL)...\n");
		strncpy(name, "S_END", 8);
		return 1;
	}

	if (strncmp(name, "SS_END", 8) == 0)
	{
		// fix up flag to standard syntax
		strncpy(name, "S_END", 8);
		return 1;
	}

	return (strncmp(name, "S_END", 8) == 0);
}

//
// Is the name a flat list start flag?
// If lax syntax match, fix up to standard syntax.
//
static bool IsF_START(char *name)
{
#if 1
	if (strncmp(name, "WALLSTRT", 8) == 0)
	{
		// fix up flag to standard syntax
		I_Printf("ROTT: WALLSTRT -> flats\n");
		strncpy(name, "F_START", 8);
		return 1;
	}

	if (strncmp(name, "ANIMSTRT", 8) == 0)
	{
		// fix up flag to standard syntax
		I_Printf("ROTT: WALLSTRT -> flats\n");
		strncpy(name, "F_START", 8);
		return 1;
	}

	//Even though it says patches, it is technically a flat
	if (strncmp(name, "DOORSTRT", 8) == 0)
	{
		I_Printf("ROTT: DOORSTRT -> flats\n");
		// fix up flag to standard syntax
		strncpy(name, "F_START", 8);
		return 1;
	}

	//Even though it says patches, it is technically a flat
	if (strncmp(name, "EXITSTRT", 8) == 0)
	{
		I_Printf("ROTT: EXITSTRT -> flats\n");
		// fix up flag to standard syntax
		strncpy(name, "F_START", 8);
		return 1;
	}

	//Even though it says patches, it is technically a flat
	if (strncmp(name, "ELEVSTRT", 8) == 0)
	{
		I_Printf("ROTT: ELEVSTRT -> flats\n");
		// fix up flag to standard syntax
		strncpy(name, "F_START", 8);
		return 1;
	}

	//Even though it says patches, it is technically a flat
	if (strncmp(name, "SIDESTRT", 8) == 0)
	{
		I_Printf("ROTT: SIDESTRT -> flats\n");
		// fix up flag to standard syntax
		strncpy(name, "F_START", 8);
		return 1;
	}

#endif // 0

	if (strncmp(name, "FF_START", 8) == 0)// || (strncmp(name, "UPDNSTRT", 8) == 0))
	{
		// fix up flag to standard syntax
		strncpy(name, "F_START", 8);
		return 1;
	}

	if (strncmp(name, "UPDNSTRT", 9) == 0)// || (strncmp(name, "UPDNSTRT", 8) == 0))
	{
		// fix up flag to standard syntax
		I_Printf("Found UPDNSTRT!!\n");
		strncpy(name, "F_START", 8);
		return 1;
	}

	return (strncmp(name, "F_START", 8) == 0);
}

//
// Is the name a flat list end flag?
// If lax syntax match, fix up to standard syntax.
//
static bool IsF_END(char *name)
{
#if 1
	if (strncmp(name, "WALLSTOP", 8) == 0)
	{
		// fix up flag to standard syntax
		I_Printf("ROTT: WALLSTOP -> flats\n");
		strncpy(name, "FF_END", 8);
		return 1;
	}

	if (strncmp(name, "ANIMSTOP", 8) == 0)
	{
		// fix up flag to standard syntax
		I_Printf("ROTT: ANIMSTOP -> flats\n");
		strncpy(name, "FF_END", 8);
		return 1;
	}

	//Even though it says patches, it is technically a flat
	if (strncmp(name, "DOORSTOP", 8) == 0)
	{
		I_Printf("ROTT: DOORSTOP -> flats\n");
		// fix up flag to standard syntax
		strncpy(name, "FF_END", 8);
		return 1;
	}

	//Even though it says patches, it is technically a flat
	if (strncmp(name, "EXITSTOP", 8) == 0)
	{
		I_Printf("ROTT: EXITSTOP -> flats\n");
		// fix up flag to standard syntax
		strncpy(name, "FF_END", 8);
		return 1;
	}

	//Even though it says patches, it is technically a flat
	if (strncmp(name, "ELEVSTOP", 8) == 0)
	{
		I_Printf("ROTT: ELEVSTOP -> flats\n");
		// fix up flag to standard syntax
		strncpy(name, "FF_END", 8);
		return 1;
	}

	//Even though it says patches, it is technically a flat
	if (strncmp(name, "SIDESTOP", 8) == 0)
	{
		I_Printf("ROTT: SIDESTOP -> flats\n");
		// fix up flag to standard syntax
		strncpy(name, "FF_END", 8);
		return 1;
	}

#endif // 0

	if (strncmp(name, "FINDRPAL", 8) == 0)
	{
		// fix up flag to standard syntax
		I_Printf("ROTT: FINDRPAL -> flats\n");
		strncpy(name, "F_END", 8);
		return 1;
	}

	if (strncmp(name, "FF_END", 8) == 0)// || (strncmp(name, "UPDNSTOP", 8) == 0))
	{
		// fix up flag to standard syntax
		strncpy(name, "F_END", 8);
		return 1;
	}

	if (strncmp(name, "UPDNSTOP", 8) == 0)// || (strncmp(name, "UPDNSTRT", 8) == 0))
	{
		// fix up flag to standard syntax
		I_Printf("Found UPDNSTOP...\n");
		strncpy(name, "F_END", 8);
		return 1;
	}

	return (strncmp(name, "F_END", 8) == 0);
}
//
// Is the name a patch list start flag?
// If lax syntax match, fix up to standard syntax.
//
static bool IsP_START(char *name)
{
#if 1
	if (strncmp(name, "SHAPSTART", 8) == 0)//TODO: V666 https://www.viva64.com/en/w/v666/ Consider inspecting third argument of the function 'strncmp'. It is possible that the value does not correspond with the length of a string which was passed with the second argument.
	{
		//fix up flag to standard syntax
		//Note: strncpy will pad will nulls
		strncpy(name, "PP_START", 8);
		return 1;
	}
	if (strncmp(name, "MASKSTRT", 8) == 0)
	{
		I_Printf("ROTT: MASKSTRT -> Patches\n");
		// fix up flag to standard syntax
		strncpy(name, "PP_START", 8);
		return 1;
	}

	if (strncmp(name, "SKYSTOP", 8) == 0)
	{
		// fix up flag to standard syntax
		I_Printf("ROTT: SKYSTOP -> Patches\n");
		strncpy(name, "PP_START", 8);
		return 1;
	}
#endif // 0


	if (strncmp(name, "PP_START", 8) == 0)
	{
		// fix up flag to standard syntax
		strncpy(name, "P_START", 8);
		return 1;
	}

	return (strncmp(name, "P_START", 8) == 0);
}

//
// Is the name a patch list end flag?
// If lax syntax match, fix up to standard syntax.
//
static bool IsP_END(char *name)
{


	if (strncmp(name, "SHAPSTOP", 8) == 0)
	{
		// fix up flag to standard syntax
		strncpy(name, "PP_END", 8);
		return 1;
	}

	if (strncmp(name, "MASKSTOP", 8) == 0)
	{
		I_Printf("ROTT: MASKSTRT -> Patches\n");
		// fix up flag to standard syntax
		strncpy(name, "PP_END", 8);
		return 1;
	}

	if (strncmp(name, "PP_END", 8) == 0) //TODO: V666 https://www.viva64.com/en/w/v666/ Consider inspecting third argument of the function 'strncmp'. It is possible that the value does not correspond with the length of a string which was passed with the second argument.
	{
		// fix up flag to standard syntax
		strncpy(name, "P_END", 8);
		return 1;
	}

	return (strncmp(name, "P_END", 8) == 0); //TODO: V666 https://www.viva64.com/en/w/v666/ Consider inspecting third argument of the function 'strncmp'. It is possible that the value does not correspond with the length of a string which was passed with the second argument.
}

//
// Is the name a colourmap list start/end flag?
//
static bool IsC_START(char *name)
{
#if 0
	if (strncmp(name, "COLORMAP", 8) == 0)
	{
		I_Printf("ROTT: COLORMAP -> C_START\n");
		// fix up flag to standard syntax
		strncpy(name, "C_START", 8);
		return 1;
	}
#endif // 0

	return (strncmp(name, "C_START", 8) == 0);
}

static bool IsC_END(char *name)
{
#if 0
	if (strncmp(name, "ORNGMAP", 8) == 0)
	{
		I_Printf("ROTT: ORNGMAP -> C_END\n");
		// fix up flag to standard syntax
		strncpy(name, "C_END", 8);
		return 1;
	}
#endif
	return (strncmp(name, "C_END", 8) == 0); //TODO: V666 https://www.viva64.com/en/w/v666/ Consider inspecting third argument of the function 'strncmp'. It is possible that the value does not correspond with the length of a string which was passed with the second argument.
}

//
// Is the name a texture list start/end flag?
//
static bool IsTX_START(char *name)
{
	return (strncmp(name, "TX_START", 8) == 0);
}

static bool IsTX_END(char *name)
{
	return (strncmp(name, "TX_END", 8) == 0); //TODO: V666 https://www.viva64.com/en/w/v666/ Consider inspecting third argument of the function 'strncmp'. It is possible that the value does not correspond with the length of a string which was passed with the second argument.
}

//
// Is the name a high-resolution start/end flag?
//
static bool IsHI_START(char *name)
{
	return (strncmp(name, "HI_START", 8) == 0);
}

static bool IsHI_END(char *name)
{
	return (strncmp(name, "HI_END", 8) == 0); //TODO: V666 https://www.viva64.com/en/w/v666/ Consider inspecting third argument of the function 'strncmp'. It is possible that the value does not correspond with the length of a string which was passed with the second argument.
}

// This is ugly, but this will list every single ROTT RAW picture (lpic_t) so they can be sorted properly
static bool IsRottPic(char *name)
{
	return (strncmp(name, "AMMO18", 8) == 0 ||
		strncmp(name, "AMMO1C", 8) == 0 ||
		strncmp(name, "AMMO2B", 8) == 0 ||
		strncmp(name, "AMMO2C", 8) == 0 ||
		strncmp(name, "AMMO3B", 8) == 0 ||
		strncmp(name, "AMMO3C", 8) == 0 ||
		strncmp(name, "AMMO4B", 8) == 0 ||
		strncmp(name, "AMMO4C", 8) == 0 ||
		strncmp(name, "AMMO5B", 8) == 0 ||
		strncmp(name, "AMMO5C", 8) == 0 ||
		strncmp(name, "AMMO6B", 8) == 0 ||
		strncmp(name, "AMMO6C", 8) == 0 ||
		strncmp(name, "AMMO7B", 8) == 0 ||
		strncmp(name, "AMMO7C", 8) == 0 ||
		strncmp(name, "AMMO8B", 8) == 0 ||
		strncmp(name, "AMMO8C", 8) == 0 ||
		strncmp(name, "AMMO9B", 8) == 0 ||
		strncmp(name, "AMMO9C", 8) == 0 ||
		strncmp(name, "AMMO10B", 8) == 0 ||
		strncmp(name, "AMMO10C", 8) == 0 ||
		strncmp(name, "ARMORP", 8) == 0 ||
		strncmp(name, "BACKTILE", 8) == 0 ||
		strncmp(name, "BATTP", 8) == 0 ||
		strncmp(name, "BOTNPIC1", 8) == 0 ||
		strncmp(name, "BOTNPIC2", 8) == 0 ||
		strncmp(name, "BOTNPIC3", 8) == 0 ||
		strncmp(name, "BOTNPIC4", 8) == 0 ||
		strncmp(name, "BOTNPIC5", 8) == 0 ||
		strncmp(name, "BOTOPIC1", 8) == 0 ||
		strncmp(name, "BOTOPIC2", 8) == 0 ||
		strncmp(name, "BOTOPIC3", 8) == 0 ||
		strncmp(name, "BOTOPIC4", 8) == 0 ||
		strncmp(name, "BOTOPIC5", 8) == 0 ||
		strncmp(name, "BOTPIC0", 8) == 0 ||
		strncmp(name, "BOTPIC1", 8) == 0 ||
		strncmp(name, "BOTPIC2", 8) == 0 ||
		strncmp(name, "BOTPIC3", 8) == 0 ||
		strncmp(name, "BOTPIC4", 8) == 0 ||
		strncmp(name, "BOTPIC5", 8) == 0 ||
		strncmp(name, "BOTTBAR", 8) == 0 ||
		strncmp(name, "DEMO", 8) == 0 ||
		strncmp(name, "DGMODEP", 8) == 0 ||
		strncmp(name, "DSTRIP", 8) == 0 ||
		strncmp(name, "ELMODEP", 8) == 0 ||
		strncmp(name, "ERASE", 8) == 0 ||
		strncmp(name, "ERASEB", 8) == 0 ||
		strncmp(name, "FFMODEP", 8) == 0 ||
		strncmp(name, "FIREP", 8) == 0 ||
		strncmp(name, "GASMKP", 8) == 0 ||
		strncmp(name, "GDMODEP", 8) == 0 ||
		strncmp(name, "HEALTH1B", 8) == 0 ||
		strncmp(name, "HEALTH1C", 8) == 0 ||
		strncmp(name, "HEALTH2B", 8) == 0 ||
		strncmp(name, "HEALTH2C", 8) == 0 ||
		strncmp(name, "HEALTH3B", 8) == 0 ||
		strncmp(name, "HEALTH3C", 8) == 0 ||
		strncmp(name, "INF_B", 8) == 0 ||
		strncmp(name, "INF_C", 8) == 0 ||
		strncmp(name, "KEY1", 8) == 0 ||
		strncmp(name, "KEY2", 8) == 0 ||
		strncmp(name, "KEY3", 8) == 0 ||
		strncmp(name, "KEY4", 8) == 0 ||
		strncmp(name, "KILNUM0", 8) == 0 ||
		strncmp(name, "KILNUM1", 8) == 0 ||
		strncmp(name, "KILNUM2", 8) == 0 ||
		strncmp(name, "KILNUM3", 8) == 0 ||
		strncmp(name, "KILNUM4", 8) == 0 ||
		strncmp(name, "KILNUM5", 8) == 0 ||
		strncmp(name, "KILNUM6", 8) == 0 ||
		strncmp(name, "KILNUM7", 8) == 0 ||
		strncmp(name, "KILNUM8", 8) == 0 ||
		strncmp(name, "KILNUM9", 8) == 0 ||
		strncmp(name, "LFNUM0", 8) == 0 ||
		strncmp(name, "LFNUM1", 8) == 0 ||
		strncmp(name, "LFNUM2", 8) == 0 ||
		strncmp(name, "LFNUM3", 8) == 0 ||
		strncmp(name, "LFNUM4", 8) == 0 ||
		strncmp(name, "LFNUM5", 8) == 0 ||
		strncmp(name, "LFNUM6", 8) == 0 ||
		strncmp(name, "LFNUM7", 8) == 0 ||
		strncmp(name, "LFNUM8", 8) == 0 ||
		strncmp(name, "LFNUM9", 8) == 0 ||
		strncmp(name, "LVNUM0", 8) == 0 ||
		strncmp(name, "LVNUM1", 8) == 0 ||
		strncmp(name, "LVNUM2", 8) == 0 ||
		strncmp(name, "LVNUM3", 8) == 0 ||
		strncmp(name, "LVNUM4", 8) == 0 ||
		strncmp(name, "LVNUM5", 8) == 0 ||
		strncmp(name, "LVNUM6", 8) == 0 ||
		strncmp(name, "LVNUM7", 8) == 0 ||
		strncmp(name, "LVNUM8", 8) == 0 ||
		strncmp(name, "LVNUM9", 8) == 0 ||
		strncmp(name, "MAN1", 8) == 0 ||
		strncmp(name, "MAN2", 8) == 0 ||
		strncmp(name, "MAN3", 8) == 0 ||
		strncmp(name, "MAN4", 8) == 0 ||
		strncmp(name, "MAN5", 8) == 0 ||
		strncmp(name, "MINUS", 8) == 0 ||
		strncmp(name, "NEGMAN1", 8) == 0 ||
		strncmp(name, "NEGMAN2", 8) == 0 ||
		strncmp(name, "NEGMAN3", 8) == 0 ||
		strncmp(name, "NEGMAN4", 8) == 0 ||
		strncmp(name, "NEGMAN5", 8) == 0 ||
		strncmp(name, "NEWG1", 8) == 0 ||
		strncmp(name, "NEWG2", 8) == 0 ||
		strncmp(name, "NEWG3", 8) == 0 ||
		strncmp(name, "NEWG4", 8) == 0 ||
		strncmp(name, "NEWG5", 8) == 0 ||
		strncmp(name, "NEWG6", 8) == 0 ||
		strncmp(name, "NEWG7", 8) == 0 ||
		strncmp(name, "NEWG8", 8) == 0 ||
		strncmp(name, "NEWG9", 8) == 0 ||
		strncmp(name, "NEWG10", 8) == 0 ||
		strncmp(name, "NEWG11", 8) == 0 ||
		strncmp(name, "NEWG12", 8) == 0 ||
		strncmp(name, "PAUSED", 8) == 0 ||
		strncmp(name, "PLAYER1", 8) == 0 ||
		strncmp(name, "PLAYER2", 8) == 0 ||
		strncmp(name, "PLAYER3", 8) == 0 ||
		strncmp(name, "PLAYER4", 8) == 0 ||
		strncmp(name, "PLAYER5", 8) == 0 ||
		strncmp(name, "QUIT01", 8) == 0 ||
		strncmp(name, "QUIT02", 8) == 0 ||
		strncmp(name, "QUIT03", 8) == 0 ||
		strncmp(name, "QUIT04", 8) == 0 ||
		strncmp(name, "QUIT05", 8) == 0 ||
		strncmp(name, "QUIT06", 8) == 0 ||
		strncmp(name, "QUIT07", 8) == 0 ||
		strncmp(name, "QUITPIC", 8) == 0 ||
		strncmp(name, "SCNUM0", 8) == 0 ||
		strncmp(name, "SCNUM1", 8) == 0 ||
		strncmp(name, "SCNUM2", 8) == 0 ||
		strncmp(name, "SCNUM3", 8) == 0 ||
		strncmp(name, "SCNUM4", 8) == 0 ||
		strncmp(name, "SCNUM5", 8) == 0 ||
		strncmp(name, "SCNUM6", 8) == 0 ||
		strncmp(name, "SCNUM7", 8) == 0 ||
		strncmp(name, "SCNUM8", 8) == 0 ||
		strncmp(name, "SCNUM9", 8) == 0 ||
		strncmp(name, "SMALLTRI", 8) == 0 ||
		strncmp(name, "SMMODEP", 8) == 0 ||
		strncmp(name, "STAT_BAR", 8) == 0 ||
		strncmp(name, "T_BAR", 8) == 0 ||
		strncmp(name, "T_BLNK", 8) == 0 ||
		strncmp(name, "T_ENEMY", 8) == 0 ||
		strncmp(name, "T_FRIEND", 8) == 0 ||
		strncmp(name, "T_KCOUNT", 8) == 0 ||
		strncmp(name, "T_KILPER", 8) == 0 ||
		strncmp(name, "T_NAME", 8) == 0 ||
		strncmp(name, "T_PERKIL", 8) == 0 ||
		strncmp(name, "T_SCORE", 8) == 0 ||
		strncmp(name, "T_SUICID", 8) == 0 ||
		strncmp(name, "TEAMNPIC", 8) == 0 ||
		strncmp(name, "TEAMPIC", 8) == 0 ||
		strncmp(name, "TMNUM0", 8) == 0 ||
		strncmp(name, "TMNUM1", 8) == 0 ||
		strncmp(name, "TMNUM2", 8) == 0 ||
		strncmp(name, "TMNUM3", 8) == 0 ||
		strncmp(name, "TMNUM4", 8) == 0 ||
		strncmp(name, "TMNUM5", 8) == 0 ||
		strncmp(name, "TMNUM6", 8) == 0 ||
		strncmp(name, "TMNUM7", 8) == 0 ||
		strncmp(name, "TMNUM8", 8) == 0 ||
		strncmp(name, "TMNUM9", 8) == 0 ||
		strncmp(name, "TNUMB",  8) == 0 ||
		strncmp(name, "TOPPIC1", 8) == 0 ||
		strncmp(name, "TOPPIC2", 8) == 0 ||
		strncmp(name, "TOPPIC3", 8) == 0 ||
		strncmp(name, "TOPPIC4", 8) == 0 ||
		strncmp(name, "TOPPIC5", 8) == 0 ||
		strncmp(name, "TRI1PIC", 8) == 0 ||
		strncmp(name, "TRI2PIC", 8) == 0 ||
		strncmp(name, "WAIT",	 8) == 0 ||
		strncmp(name, "WINDOW1", 8) == 0 ||
		strncmp(name, "WINDOW2", 8) == 0 ||
		strncmp(name, "WINDOW3", 8) == 0 ||
		strncmp(name, "WINDOW4", 8) == 0 ||
		strncmp(name, "WINDOW5", 8) == 0 ||
		strncmp(name, "WINDOW6", 8) == 0 ||
		strncmp(name, "WINDOW7", 8) == 0 ||
		strncmp(name, "WINDOW8", 8) == 0 ||
		strncmp(name, "WINDOW9", 8) == 0);
}

static bool IsRawROTT(const char *name)
{
	return (strncmp(name, "FLRCL1", 8) == 0 ||
		strncmp(name, "FLRCL2", 8) == 0 ||
		strncmp(name, "FLRCL3", 8) == 0 ||
		strncmp(name, "FLRCL4", 8) == 0 ||
		strncmp(name, "FLRCL5", 8) == 0 ||
		strncmp(name, "FLRCL6", 8) == 0 ||
		strncmp(name, "FLRCL7", 8) == 0 ||
		strncmp(name, "FLRCL8", 8) == 0 ||
		strncmp(name, "FLRCL9", 8) == 0 ||
		strncmp(name, "FLRCL10", 8) == 0 ||
		strncmp(name, "FLRCL11", 8) == 0 ||
		strncmp(name, "FLRCL12", 8) == 0 ||
		strncmp(name, "FLRCL13", 8) == 0 ||
		strncmp(name, "FLRCL14", 8) == 0 ||
		strncmp(name, "FLRCL15", 8) == 0 ||
		strncmp(name, "FLRCL16", 8) == 0);
}
//
// Is the name a dummy sprite/flat/patch marker ?
//
static bool IsDummySF(const char *name)
{
	return (strncmp(name, "S1_START", 8) == 0 ||
		strncmp(name, "S2_START", 8) == 0 ||
		strncmp(name, "S3_START", 8) == 0 ||
		strncmp(name, "F1_START", 8) == 0 ||
		strncmp(name, "F2_START", 8) == 0 ||
		strncmp(name, "F3_START", 8) == 0 ||
		strncmp(name, "P1_START", 8) == 0 ||
		strncmp(name, "P2_START", 8) == 0 ||
		strncmp(name, "P3_START", 8) == 0 ||
		strncmp(name, "GUNSTART", 8) == 0 ||
		strncmp(name, "WALLSTRT", 8) == 0 ||
		//strncmp(name, "WALLSTOP", 8) == 0 ||
		//strncmp(name, "GUNSTOP", 7) == 0  ||
		strncmp(name, "MASKSTRT", 8) == 0 ||
		//strncmp(name, "MASKSTOP", 8) == 0 ||
		strncmp(name, "SKYSTART", 8) == 0 ||
		//strncmp(name, "SKYSTOP", 7) == 0  ||
		strncmp(name, "ADLSTART", 8) == 0 ||
		strncmp(name, "DIGISTRT", 8) == 0);
		//strncmp(name, "DIGISTOP", 8) == 0);
}



// RISE OF THE TRIAD SPECIAL MARKERS

// Make sure to set all ROTT flats (anything 64x64) into the FLATS namespace
static bool IsROTTFlat_Start(const char *name)
{
	return (strncmp(name, "WALLSTRT", 8) == 0 ||
		strncmp(name, "ANIMSTRT", 8) == 0 ||
		strncmp(name, "DOORSTRT", 8) == 0 ||
		strncmp(name, "EXITSTRT", 8) == 0 ||
		strncmp(name, "ELEVSTRT", 8) == 0 ||
		strncmp(name, "SIDESTRT", 8) == 0);// ||
		//strncmp(name, "UPDNSTRT", 8) == 0);

	return (strncmp(name, "FF_START", 8) == 0);
}

static bool IsROTTFlat_End(const char *name)
{
	return (strncmp(name, "WALLSTOP", 8) == 0 ||
		strncmp(name, "ANIMSTOP", 8) == 0 ||
		strncmp(name, "DOORSTOP", 8) == 0 ||
		strncmp(name, "EXITSTOP", 8) == 0 ||
		strncmp(name, "ABVWSTRT", 8) == 0 ||
		strncmp(name, "ELEVSTOP", 8) == 0 ||
		strncmp(name, "SIDESTOP", 8) == 0);// ||
		//strncmp(name, "UPDNSTOP", 8) == 0);

	return (strncmp(name, "FF_END", 8) == 0);
}

#if 0
static bool IsROTTFLRCL(const char *name)
{
	return 	(strncmp(name, "FLRCL1", 8) == 0 ||
		strncmp(name, "FLRCL2", 8) == 0 ||
		strncmp(name, "FLRCL3", 8) == 0 ||
		strncmp(name, "FLRCL4", 8) == 0 ||
		strncmp(name, "FLRCL5", 8) == 0 ||
		strncmp(name, "FLRCL6", 8) == 0 ||
		strncmp(name, "FLRCL7", 8) == 0 ||
		strncmp(name, "FLRCL8", 8) == 0 ||
		strncmp(name, "FLRCL9", 8) == 0 ||
		strncmp(name, "FLRCL10", 8) == 0 ||
		strncmp(name, "FLRCL11", 8) == 0 ||
		strncmp(name, "FLRCL12", 8) == 0 ||
		strncmp(name, "FLRCL13", 8) == 0 ||
		strncmp(name, "FLRCL14", 8) == 0 ||
		strncmp(name, "FLRCL15", 8) == 0 ||
		strncmp(name, "FLRCL16", 8) == 0);

	return (strncmp(name, "F_"))
}
#endif // 0



//
// Is the name a skin specifier ?
//
static bool IsSkin(const char *name)
{
	return (strncmp(name, "S_SKIN", 6) == 0);
}


static inline bool IsGL_Prefix(const char *name)
{
	return name[0] == 'G' && name[1] == 'L' && name[2] == '_';
}

//
// W_GetTextureLumps
//
void W_GetTextureLumps(int file, wadtex_resource_c *res)
{
	SYS_ASSERT(0 <= file && file < (int)data_files.size());
	SYS_ASSERT(res);

	data_file_c *df = data_files[file];

	res->palette = df->wadtex.palette;
	res->pnames = df->wadtex.pnames;
	res->texture1 = df->wadtex.texture1;
	res->texture2 = df->wadtex.texture2;

	// find an earlier PNAMES lump when missing.
	// Ditto for palette.

	if (res->texture1 >= 0 || res->texture2 >= 0)
	{
		int cur;

		for (cur = file; res->pnames == -1 && cur > 0; cur--)
			res->pnames = data_files[cur]->wadtex.pnames;

		for (cur = file; res->palette == -1 && cur > 0; cur--)
			res->palette = data_files[cur]->wadtex.palette;
	}
}

//
// W_GetWolfTextureLumps
//
void W_GetWolfTextureLumps(int file, raw_vswap_t *res)
{
	I_Printf("WOLF: Getting Wolf Texture Lumps...\n");
	WF_VSwapOpen();

	SYS_ASSERT(0 <= file && file < (int)data_files.size());
	SYS_ASSERT(res);

	data_file_c *df = data_files[file];
}

//
// SortLumps
//
// Create the lumpmap[] array, which is sorted by name for fast
// searching.  It also makes sure that entries with the same name all
// refer to the same lump (prefering lumps in later WADs over those in
// earlier ones).
//
// -AJA- 2000/10/14: simplified.
//
static void SortLumps(void)
{
	int i;

	Z_Resize(lumpmap, int, numlumps);

	for (i = 0; i < numlumps; i++)
		lumpmap[i] = i;

	// sort it, primarily by increasing name, secondly by decreasing
	// file number, thirdly by the lump type.

#define CMP(a, b)  \
	(strncmp(lumpinfo[a].name, lumpinfo[b].name, 8) < 0 ||    \
	 (strncmp(lumpinfo[a].name, lumpinfo[b].name, 8) == 0 &&  \
	  (lumpinfo[a].sort_index > lumpinfo[b].sort_index ||     \
	   (lumpinfo[a].sort_index == lumpinfo[b].sort_index &&   \
		lumpinfo[a].kind > lumpinfo[b].kind))))
	QSORT(int, lumpmap, numlumps, CUTOFF);
#undef CMP

#if 0
	for (i = 1; i < numlumps; i++)
	{
		int a = lumpmap[i - 1];
		int b = lumpmap[i];

		if (strncmp(lumpinfo[a].name, lumpinfo[b].name, 8) == 0)
			lumpmap[i] = lumpmap[i - 1];
	}
#endif
}

//
// SortSpriteLumps
//
// Put the sprite list in sorted order (of name), required by
// R_InitSprites (speed optimisation).
//
static void SortSpriteLumps(data_file_c *df)
{
	if (df->sprite_lumps.GetSize() < 2)
		return;

#define CMP(a, b) (strncmp(lumpinfo[a].name, lumpinfo[b].name, 8) < 0)
	QSORT(int, df->sprite_lumps, df->sprite_lumps.GetSize(), CUTOFF);
#undef CMP

#if 0 // DEBUGGING
	{
		int i, lump;

		for (i = 0; i < f->sprite_num; i++)
		{
			lump = df->sprite_lumps[i];

			I_Debugf("Sorted sprite %d = lump %d [%s]\n", i, lump,
				lumpinfo[lump].name);
		}
	}
#endif
}

//
// LUMP BASED ROUTINES.
//

//
// W_AddFile
//
// All files are optional, but at least one file must be
//  found (PWAD, if all required lumps are present).
// Files with a .wad extension are wadlink files
//  with multiple lumps.
// Other files are single lumps with the base filename
//  for the lump name.
//

#if 0
static void FreeLump(lumpheader_t *h)
{
	int lumpnum = h->lumpindex;

	cache_size -= W_LumpLength(lumpnum);
#ifdef DEVELOPERS
	if (h->id != lumpheader_s::LUMPID)
		I_Error("FreeLump: id != LUMPID");
	h->id = 0;
	if (h == NULL)
		I_Error("FreeLump: NULL lump");
	if (h->users)
		I_Error("FreeLump: lump %d has %d users!", lumpnum, h->users);
	if (lumplookup[lumpnum] != h)
		I_Error("FreeLump: Internal error, lump %d", lumpnum);
#endif
	lumplookup[lumpnum] = NULL;
	h->prev->next = h->next;
	h->next->prev = h->prev;
	Z_Free(h);
}
#endif

//
// MarkAsCached
//
static void MarkAsCached(lumpheader_t *item)
{
#ifdef DEVELOPERS
	if (item->id != lumpheader_s::LUMPID)
		I_Error("MarkAsCached: id != LUMPID");
	if (!item)
		I_Error("MarkAsCached: lump %d is NULL", item->lumpindex);
	if (item->users)
		I_Error("MarkAsCached: lump %d has %d users!", item->lumpindex, item->users);
	if (lumplookup[item->lumpindex] != item)
		I_Error("MarkAsCached: Internal error, lump %d", item->lumpindex);
#endif

	cache_size += W_LumpLength(item->lumpindex);
}

//
// AddLump/AddLumpEx
//
static void AddLumpEx(data_file_c *df, int lump, int pos, int size, int file,
	int sort_index, const char *name, bool allow_ddf, const char *path)
{
	int j;
	lumpinfo_t *lump_p = &lumpinfo[lump];
	image_c *rim;

	lump_p->position = pos;
	lump_p->size = size;
	lump_p->file = file;
	lump_p->sort_index = sort_index;
	lump_p->kind = LMKIND_Normal;

	Z_StrNCpy(lump_p->name, name, 8);

	for (size_t i=0;i<strlen(lump_p->name);i++) 
		lump_p->name[i] = toupper(lump_p->name[i]);

	// strip any extension
	for (j = 7; j >= 0; j--)
		if (lump_p->name[j] == '.')
		{
			lump_p->name[j] = 0;
			break;
		}

	Z_StrNCpy(lump_p->path, path, 255);

//#ifdef DEVELOPERS
//	I_Debugf("AddLumpEx: %p, %d, %d, %d, %d, %d, %s, %d, %s\n",
//		df, lump, pos, size, file, sort_index, lump_p->name, allow_ddf, lump_p->path);
//#endif

	// -- handle special names --

	if ((strncmp(lump_p->name, "PLAYPAL", 8) == 0) || (strncmp(lump_p->name, "PAL", 4) == 0))
	{
		lump_p->kind = LMKIND_WadTex;
		df->wadtex.palette = lump;
		if (palette_datafile < 0)
			palette_datafile = file;
		palette_lastfile = file;
		return;
	}

	// -CA- 10.29.18:
	// UGLY HACKS INBOUND! Basically this looks up a table of entries from the wad since ROTT doesn't always
	// have matching start and end markers. TODO: in the markers code, make an end marker -1 of current entry. Eh,
	// since they are scattered all over the place, this will take some thought if we want it done nicer.

	else if ((strncmp(lump_p->name, "BOOTBLOD", 8) == 0) ||
		(strncmp(lump_p->name, "IMFREE", 8) == 0) ||
		(strncmp(lump_p->name, "BOOTNORM", 8) == 0) ||
		 (strncmp(lump_p->name, "DEADBOSS", 8) == 0))
	{
		lump_p->kind = LMKIND_LBM;
		SYS_ASSERT(rim->source_type == IMSRC_ROTTLBM);
		df->lbm_pic = lump;
		return;
	}


	else if ((strncmp(lump_p->name, "AMMO18", 8) == 0) ||
		(strncmp(name, "AMMO1C", 8) == 0) ||
		(strncmp(name, "AMMO2B", 8) == 0) ||
		(strncmp(name, "AMMO2C", 8) == 0) ||
		(strncmp(name, "AMMO3B", 8) == 0) ||
		(strncmp(name, "AMMO3C", 8) == 0) ||
		(strncmp(name, "AMMO4B", 8) == 0) ||
		(strncmp(name, "AMMO4C", 8) == 0) ||
		(strncmp(name, "AMMO5B", 8) == 0) ||
		(strncmp(name, "AMMO5C", 8) == 0) ||
		(strncmp(name, "AMMO6B", 8) == 0) ||
		(strncmp(name, "AMMO6C", 8) == 0) ||
		(strncmp(name, "AMMO7B", 8) == 0) ||
		(strncmp(name, "AMMO7C", 8) == 0) ||
		(strncmp(name, "AMMO8B", 8) == 0) ||
		(strncmp(name, "AMMO8C", 8) == 0) ||
		(strncmp(name, "AMMO9B", 8) == 0) ||
		(strncmp(name, "AMMO9C", 8) == 0) ||
		(strncmp(name, "AMMO10B", 8) == 0) ||
		(strncmp(name, "AMMO10C", 8) == 0) ||
		(strncmp(name, "ARMORP", 8) == 0) ||
		(strncmp(name, "BACKTILE", 8) == 0) ||
		(strncmp(name, "BATTP", 8) == 0) ||
		(strncmp(name, "BOTNPIC1", 8) == 0) ||
		(strncmp(name, "BOTNPIC2", 8) == 0) ||
		(strncmp(name, "BOTNPIC3", 8) == 0) ||
		(strncmp(name, "BOTNPIC4", 8) == 0) ||
		(strncmp(name, "BOTNPIC5", 8) == 0) ||
		(strncmp(name, "BOTOPIC1", 8) == 0) ||
		(strncmp(name, "BOTOPIC2", 8) == 0) ||
		(strncmp(name, "BOTOPIC3", 8) == 0) ||
		(strncmp(name, "BOTOPIC4", 8) == 0) ||
		(strncmp(name, "BOTOPIC5", 8) == 0) ||
		(strncmp(name, "BOTPIC0", 8) == 0) ||
		(strncmp(name, "BOTPIC1", 8) == 0) ||
		(strncmp(name, "BOTPIC2", 8) == 0) ||
		(strncmp(name, "BOTPIC3", 8) == 0) ||
		(strncmp(name, "BOTPIC4", 8) == 0) ||
		(strncmp(name, "BOTPIC5", 8) == 0) ||
		(strncmp(name, "BOTTBAR", 8) == 0) ||
		(strncmp(name, "DEMO", 8) == 0) ||
		(strncmp(name, "DGMODEP", 8) == 0) ||
		(strncmp(name, "DSTRIP", 8) == 0) ||
		(strncmp(name, "ELMODEP", 8) == 0) ||
		(strncmp(name, "ERASE", 8) == 0) ||
		(strncmp(name, "ERASEB", 8) == 0) ||
		(strncmp(name, "FFMODEP", 8) == 0) ||
		(strncmp(name, "FIREP", 8) == 0) ||
		(strncmp(name, "GASMKP", 8) == 0) ||
		(strncmp(name, "GDMODEP", 8) == 0) ||
		(strncmp(name, "HEALTH1B", 8) == 0) ||
		(strncmp(name, "HEALTH1C", 8) == 0) ||
		(strncmp(name, "HEALTH2B", 8) == 0) ||
		(strncmp(name, "HEALTH2C", 8) == 0) ||
		(strncmp(name, "HEALTH3B", 8) == 0) ||
		(strncmp(name, "HEALTH3C", 8) == 0) ||
		(strncmp(name, "INF_B", 8) == 0) ||
		(strncmp(name, "INF_C", 8) == 0) ||
		(strncmp(name, "KEY1", 8) == 0) ||
		(strncmp(name, "KEY2", 8) == 0) ||
		(strncmp(name, "KEY3", 8) == 0) ||
		(strncmp(name, "KEY4", 8) == 0) ||
		(strncmp(name, "KILNUM0", 8) == 0) ||
		(strncmp(name, "KILNUM1", 8) == 0) ||
		(strncmp(name, "KILNUM2", 8) == 0) ||
		(strncmp(name, "KILNUM3", 8) == 0) ||
		(strncmp(name, "KILNUM4", 8) == 0) ||
		(strncmp(name, "KILNUM5", 8) == 0) ||
		(strncmp(name, "KILNUM6", 8) == 0) ||
		(strncmp(name, "KILNUM7", 8) == 0) ||
		(strncmp(name, "KILNUM8", 8) == 0) ||
		(strncmp(name, "KILNUM9", 8) == 0) ||
		(strncmp(name, "LFNUM0", 8) == 0) ||
		(strncmp(name, "LFNUM1", 8) == 0) ||
		(strncmp(name, "LFNUM2", 8) == 0) ||
		(strncmp(name, "LFNUM3", 8) == 0) ||
		(strncmp(name, "LFNUM4", 8) == 0) ||
		(strncmp(name, "LFNUM5", 8) == 0) ||
		(strncmp(name, "LFNUM6", 8) == 0) ||
		(strncmp(name, "LFNUM7", 8) == 0) ||
		(strncmp(name, "LFNUM8", 8) == 0) ||
		(strncmp(name, "LFNUM9", 8) == 0) ||
		(strncmp(name, "LVNUM0", 8) == 0) ||
		(strncmp(name, "LVNUM1", 8) == 0) ||
		(strncmp(name, "LVNUM2", 8) == 0) ||
		(strncmp(name, "LVNUM3", 8) == 0) ||
		(strncmp(name, "LVNUM4", 8) == 0) ||
		(strncmp(name, "LVNUM5", 8) == 0) ||
		(strncmp(name, "LVNUM6", 8) == 0) ||
		(strncmp(name, "LVNUM7", 8) == 0) ||
		(strncmp(name, "LVNUM8", 8) == 0) ||
		(strncmp(name, "LVNUM9", 8) == 0) ||
		(strncmp(name, "MAN1", 8) == 0) ||
		(strncmp(name, "MAN2", 8) == 0) ||
		(strncmp(name, "MAN3", 8) == 0) ||
		(strncmp(name, "MAN4", 8) == 0) ||
		(strncmp(name, "MAN5", 8) == 0) ||
		(strncmp(name, "MINUS", 8) == 0) ||
		(strncmp(name, "NEGMAN1", 8) == 0) ||
		(strncmp(name, "NEGMAN2", 8) == 0) ||
		(strncmp(name, "NEGMAN3", 8) == 0) ||
		(strncmp(name, "NEGMAN4", 8) == 0) ||
		(strncmp(name, "NEGMAN5", 8) == 0) ||
		(strncmp(name, "NEWG1", 8) == 0) ||
		(strncmp(name, "NEWG2", 8) == 0) ||
		(strncmp(name, "NEWG3", 8) == 0) ||
		(strncmp(name, "NEWG4", 8) == 0) ||
		(strncmp(name, "NEWG5", 8) == 0) ||
		(strncmp(name, "NEWG6", 8) == 0) ||
		(strncmp(name, "NEWG7", 8) == 0) ||
		(strncmp(name, "NEWG8", 8) == 0) ||
		(strncmp(name, "NEWG9", 8) == 0) ||
		(strncmp(name, "NEWG10", 8) == 0) ||
		(strncmp(name, "NEWG11", 8) == 0) ||
		(strncmp(name, "NEWG12", 8) == 0) ||
		(strncmp(name, "PAUSED", 8) == 0) ||
		(strncmp(name, "PLAYER1", 8) == 0) ||
		(strncmp(name, "PLAYER2", 8) == 0) ||
		(strncmp(name, "PLAYER3", 8) == 0) ||
		(strncmp(name, "PLAYER4", 8) == 0) ||
		(strncmp(name, "PLAYER5", 8) == 0) ||
		(strncmp(name, "QUIT01", 8) == 0) ||
		(strncmp(name, "QUIT02", 8) == 0) ||
		(strncmp(name, "QUIT03", 8) == 0) ||
		(strncmp(name, "QUIT04", 8) == 0) ||
		(strncmp(name, "QUIT05", 8) == 0) ||
		(strncmp(name, "QUIT06", 8) == 0) ||
		(strncmp(name, "QUIT07", 8) == 0) ||
		(strncmp(name, "QUITPIC", 8) == 0) ||
		(strncmp(name, "SCNUM0", 8) == 0) ||
		(strncmp(name, "SCNUM1", 8) == 0) ||
		(strncmp(name, "SCNUM2", 8) == 0) ||
		(strncmp(name, "SCNUM3", 8) == 0) ||
		(strncmp(name, "SCNUM4", 8) == 0) ||
		(strncmp(name, "SCNUM5", 8) == 0) ||
		(strncmp(name, "SCNUM6", 8) == 0) ||
		(strncmp(name, "SCNUM7", 8) == 0) ||
		(strncmp(name, "SCNUM8", 8) == 0) ||
		(strncmp(name, "SCNUM9", 8) == 0) ||
		(strncmp(name, "SMALLTRI", 8) == 0) ||
		(strncmp(name, "SMMODEP", 8) == 0) ||
		(strncmp(name, "STAT_BAR", 8) == 0) ||
		(strncmp(name, "T_BAR", 8) == 0) ||
		(strncmp(name, "T_BLNK", 8) == 0) ||
		(strncmp(name, "T_ENEMY", 8) == 0) ||
		(strncmp(name, "T_FRIEND", 8) == 0) ||
		(strncmp(name, "T_KCOUNT", 8) == 0) ||
		(strncmp(name, "T_KILPER", 8) == 0) ||
		(strncmp(name, "T_NAME", 8) == 0) ||
		(strncmp(name, "T_PERKIL", 8) == 0) ||
		(strncmp(name, "T_SCORE", 8) == 0) ||
		(strncmp(name, "T_SUICID", 8) == 0) ||
		(strncmp(name, "TEAMNPIC", 8) == 0) ||
		(strncmp(name, "TEAMPIC", 8) == 0) ||
		(strncmp(name, "TMNUM0", 8) == 0) ||
		(strncmp(name, "TMNUM1", 8) == 0) ||
		(strncmp(name, "TMNUM2", 8) == 0) ||
		(strncmp(name, "TMNUM3", 8) == 0) ||
		(strncmp(name, "TMNUM4", 8) == 0) ||
		(strncmp(name, "TMNUM5", 8) == 0) ||
		(strncmp(name, "TMNUM6", 8) == 0) ||
		(strncmp(name, "TMNUM7", 8) == 0) ||
		(strncmp(name, "TMNUM8", 8) == 0) ||
		(strncmp(name, "TMNUM9", 8) == 0) ||
		(strncmp(name, "TNUMB", 8) == 0) ||
		(strncmp(name, "TOPPIC1", 8) == 0) ||
		(strncmp(name, "TOPPIC2", 8) == 0) ||
		(strncmp(name, "TOPPIC3", 8) == 0) ||
		(strncmp(name, "TOPPIC4", 8) == 0) ||
		(strncmp(name, "TOPPIC5", 8) == 0) ||
		(strncmp(name, "TRI1PIC", 8) == 0) ||
		(strncmp(name, "TRI2PIC", 8) == 0) ||
		(strncmp(name, "WAIT", 8) == 0) ||
		(strncmp(name, "WINDOW1", 8) == 0) ||
		(strncmp(name, "WINDOW2", 8) == 0) ||
		(strncmp(name, "WINDOW3", 8) == 0) ||
		(strncmp(name, "WINDOW4", 8) == 0) ||
		(strncmp(name, "WINDOW5", 8) == 0) ||
		(strncmp(name, "WINDOW6", 8) == 0) ||
		(strncmp(name, "WINDOW7", 8) == 0) ||
		(strncmp(name, "WINDOW8", 8) == 0) ||
		(strncmp(name, "WINDOW9", 8) == 0))
	{
			lump_p->kind = LMKIND_PIC;
			SYS_ASSERT(rim->source_type == IMSRC_rottpic);
			df->pic_rott = lump;
			return;
	}

#if 0
					else if ((strncmp(lump_p->name, "FLRCL1", 8) == 0) ||
					(strncmp(lump_p->name, "FLRCL2", 8) == 0) ||
					(strncmp(lump_p->name, "FLRCL3", 8) == 0) ||
					(strncmp(lump_p->name, "FLRCL4", 8) == 0) ||
					(strncmp(lump_p->name, "FLRCL5", 8) == 0) ||
					(strncmp(lump_p->name, "FLRCL6", 8) == 0) ||
					(strncmp(lump_p->name, "FLRCL7", 8) == 0) ||
					(strncmp(lump_p->name, "FLRCL8", 8) == 0) ||
					(strncmp(lump_p->name, "FLRCL9", 8) == 0) ||
					(strncmp(lump_p->name, "FLRCL10", 8) == 0) ||
					(strncmp(lump_p->name, "FLRCL11", 8) == 0) ||
					(strncmp(lump_p->name, "FLRCL12", 8) == 0) ||
					(strncmp(lump_p->name, "FLRCL13", 8) == 0) ||
					(strncmp(lump_p->name, "FLRCL14", 8) == 0) ||
					(strncmp(lump_p->name, "FLRCL15", 8) == 0) ||
					(strncmp(lump_p->name, "FLRCL16", 8) == 0))// ||
					//(strncmp(lump_p->name, "PLANE",   8) == 0) ||
					//(strncmp(lump_p->name, "TRILOGO", 8) == 0) ||
					//(strncmp(lump_p->name, "AP_WRLD", 8) == 0))
	{
	lump_p->kind = LMKIND_Flat;
	SYS_ASSERT(rim->source_type == IMSRC_ROTTRaw128x128);
	df->raw_flats = lump;
	return;
	}
#endif // 0


	else if (strncmp(lump_p->name, "PNAMES", 8) == 0)
	{
		lump_p->kind = LMKIND_WadTex;
		df->wadtex.pnames = lump;
		return;
	}
	else if (strncmp(lump_p->name, "TEXTURE1", 8) == 0)
	{
		lump_p->kind = LMKIND_WadTex;
		df->wadtex.texture1 = lump;
		return;
	}
	else if (strncmp(lump_p->name, "TEXTURE2", 8) == 0)
	{
		lump_p->kind = LMKIND_WadTex;
		df->wadtex.texture2 = lump;
		return;
	}
	else if (strncmp(lump_p->name, "DEHACKED", 8) == 0)
	{
		lump_p->kind = LMKIND_DDFRTS;
		df->deh_lump = lump;
		return;
	}
	else if (strncmp(lump_p->name, "COALAPI", 8) == 0)
	{
		lump_p->kind = LMKIND_DDFRTS;
		df->coal_api = lump;
		return;
	}
	else if (strncmp(lump_p->name, "COALHUDS", 8) == 0)
	{
		lump_p->kind = LMKIND_DDFRTS;
		df->coal_huds = lump;
		return;
	}

	else if (strncmp(lump_p->name, "ANIMATED", 8) == 0)
	{
		lump_p->kind = LMKIND_DDFRTS;
		df->animated = lump;
		return;
	}
	else if (strncmp(lump_p->name, "SWITCHES", 8) == 0)
	{
		lump_p->kind = LMKIND_DDFRTS;
		df->switches = lump;
		return;
	}

	// -KM- 1998/12/16 Load DDF/RSCRIPT file from wad or pak.
	if (allow_ddf)
	{
		for (j = 0; j < NUM_DDF_READERS; j++)
		{
			if (strncmp(lump_p->name, DDF_Readers[j].name, 8) == 0)
			{
				lump_p->kind = LMKIND_DDFRTS;
				df->ddf_lumps[j] = lump;
				return;
			}
		}
	}

	if (IsSkin(lump_p->name))
	{
		lump_p->kind = LMKIND_Marker;
		df->skin_markers.Insert(lump);
		return;
	}

	// -- handle sprite, flat & patch lists --

	if (IsS_START(lump_p->name))
	{
		lump_p->kind = LMKIND_Marker;
		within_sprite_list = true;
		return;
	}
	else if (IsS_END(lump_p->name))
	{
		if (!within_sprite_list)
			I_Warning("Unexpected S_END marker in wad.\n");

		lump_p->kind = LMKIND_Marker;
		within_sprite_list = false;
		return;
	}
	else if (IsF_START(lump_p->name))
	{
		lump_p->kind = LMKIND_Marker;
		within_flat_list = true;
		return;
	}
	else if (IsROTTFlat_Start(lump_p->name))
	{
		I_Printf("Parsing ROTT Flat Lumps by ROTTFlat_Start array\n");
		lump_p->kind = LMKIND_Marker;
		within_flat_list = true;
		return;
	}
	else if (IsRawROTT(lump_p->name))
	{
		lump_p->kind = LMKIND_Flat;
		within_flat_list = true;
		return;
	}

	else if (IsROTTFlat_End(lump_p->name))
	{
		if (!within_flat_list)
			I_Warning("Unexpected F_END marker in DARKWAR.wad (check ROTTFLat_End!).\n");
		lump_p->kind = LMKIND_Marker;
		within_flat_list = false;
		return;
	}
	else if (IsF_END(lump_p->name))
	{
		if (!within_flat_list)
			I_Warning("Unexpected F_END marker in wad.\n");
		lump_p->kind = LMKIND_Marker;
		within_flat_list = false;
		return;
	}

	else if (IsP_START(lump_p->name))
	{
		lump_p->kind = LMKIND_Marker;
		within_patch_list = true;
		return;
	}
	//else if (IsROTTMask_Start(lump_p->name))
	//{
	//	I_Printf("Parsing ROTT Masked Lumps by array\n");
	//	lump_p->kind = LMKIND_Marker;
	//	within_patch_list = true;
	//	return;
	//}
	else if (IsP_END(lump_p->name))
	{
		if (!within_patch_list)
			I_Warning("Unexpected P_END marker in wad.\n");

		lump_p->kind = LMKIND_Marker;
		within_patch_list = false;
		return;
	}
	//else if (IsROTTMask_End(lump_p->name))
	//{
	//	I_Printf("Parsing ROTT Masked Lumps by array\n");
	//	lump_p->kind = LMKIND_Marker;
	//	within_patch_list = false;
	//	return;
	//}
	else if (IsC_START(lump_p->name))
	{
		lump_p->kind = LMKIND_Marker;
		within_colmap_list = true;
		return;
	}
	else if (IsC_END(lump_p->name))
	{
		if (!within_colmap_list)
			I_Warning("Unexpected C_END marker in wad.\n");

		lump_p->kind = LMKIND_Marker;
		within_colmap_list = false;
		return;
	}
	else if (IsTX_START(lump_p->name))
	{
		lump_p->kind = LMKIND_Marker;
		within_tex_list = true;
		return;
	}
	else if (IsTX_END(lump_p->name))
	{
		if (!within_tex_list)
			I_Warning("Unexpected TX_END marker in wad.\n");

		lump_p->kind = LMKIND_Marker;
		within_tex_list = false;
		return;
	}
	else if (IsHI_START(lump_p->name))
	{
		lump_p->kind = LMKIND_Marker;
		within_hires_list = true;
		return;
	}
	else if (IsHI_END(lump_p->name))
	{
		if (!within_hires_list)
			I_Warning("Unexpected HI_END marker in wad.\n");

		lump_p->kind = LMKIND_Marker;
		within_hires_list = false;
		return;
	}

	else if (IsRottPic(lump_p->name))
	{
		lump_p->kind = LMKIND_PIC;
		within_rottpic_list = true;
		return;
	}

	//else if (IsRawROTT(lump_p->name))
	//{
	//	lump_p->kind = LMKIND_RAWFLATS;
	//	within_flat_list = true;
	//	return;
	//}

	// ignore zero size lumps or dummy markers
	if (lump_p->size > 0 && !IsDummySF(lump_p->name))
	{

		if (within_sprite_list)
		{
			lump_p->kind = LMKIND_Sprite;
			df->sprite_lumps.Insert(lump);
		}

		if (within_flat_list)
		{
			lump_p->kind = LMKIND_Flat;
			df->flat_lumps.Insert(lump);
		}

		if (within_patch_list)
		{
			lump_p->kind = LMKIND_Patch;
			df->patch_lumps.Insert(lump);
		}
    
		if (within_colmap_list)
		{
			lump_p->kind = LMKIND_Colmap;
			df->colmap_lumps.Insert(lump);
		}

		if (within_tex_list)
		{
			lump_p->kind = LMKIND_TX;
			df->tx_lumps.Insert(lump);
		}

		if (within_hires_list)
		{
			lump_p->kind = LMKIND_HiRes;
			df->hires_lumps.Insert(lump);
		}

		if (within_rottpic_list)// || (lump_p->size == 64008))
		{
			lump_p->kind = LMKIND_PIC;
			df->rottpic_lumps.Insert(lump);
		}

		if (within_rawflats_list)
		{
			lump_p->kind = LMKIND_RAWFLATS;
			df->rottraw_flats.Insert(lump);
		}
	}
}

static void AddLump(data_file_c *df, int lump, int pos, int size, int file,
	int sort_index, const char *name, bool allow_ddf)
{
	AddLumpEx(df, lump, pos, size, file, sort_index, name, allow_ddf, "");
}

//
// CheckForLevel
//
// Tests whether the current lump is a level marker (MAP03, E1M7, etc).
// Because EDGE supports arbitrary names (via DDF), we look at the
// sequence of lumps _after_ this one, which works well since their
// order is fixed (e.g. THINGS is always first).
//
static void CheckForLevel(data_file_c *df, int lump, const char *name,
	const raw_wad_entry_t *raw, int remaining)
{
	// we only test four lumps (it is enough), but fewer definitely
	// means this is not a level marker.
	if (remaining < 4)
		return;

	if (strncmp(raw[1].name, "THINGS", 8) == 0 &&
		strncmp(raw[2].name, "LINEDEFS", 8) == 0 &&
		strncmp(raw[3].name, "SIDEDEFS", 8) == 0 &&
		strncmp(raw[4].name, "VERTEXES", 8) == 0)
	{
		if (strlen(name) > 5)
		{
			I_Warning("Level name '%s' is too long !!\n", name);
			return;
		}

		// check for duplicates (Slige sometimes does this)
		for (int L = 0; L < df->level_markers.GetSize(); L++)
		{
			if (strcmp(lumpinfo[df->level_markers[L]].name, name) == 0)
			{
				I_Warning("Duplicate level '%s' ignored.\n", name);
				return;
			}
		}

		df->level_markers.Insert(lump);
		return;
	}

	// handle GL nodes here too

	if (strncmp(raw[1].name, "GL_VERT", 8) == 0 &&
		strncmp(raw[2].name, "GL_SEGS", 8) == 0 &&
		strncmp(raw[3].name, "GL_SSECT", 8) == 0 &&
		strncmp(raw[4].name, "GL_NODES", 8) == 0)
	{
		df->level_markers.Insert(lump);
		return;
	}
}

//
// HasInternalGLNodes
//
// Determine if WAD file contains GL nodes for every level.
//
static bool HasInternalGLNodes(data_file_c *df, int datafile)
{
	int levels = 0;
	int glnodes = 0;

	for (int L = 0; L < df->level_markers.GetSize(); L++)
	{
		int lump = df->level_markers[L];

		if (IsGL_Prefix(lumpinfo[lump].name))
			continue;

		levels++;

		char glmarker[16];

		sprintf(glmarker, "GL_%s", lumpinfo[lump].name);

		for (int M = L + 1; M < df->level_markers.GetSize(); M++)
		{
			int lump2 = df->level_markers[M];

			if (strcmp(lumpinfo[lump2].name, glmarker) == 0)
			{
				glnodes++;
				break;
			}
		}
	}

	I_Debugf("Levels %d, Internal GL nodes %d\n", levels, glnodes);

	return levels == glnodes;
}

static void ComputeFileMD5hash(epi::md5hash_c& hash, epi::file_c *file)
{
	int length = file->GetLength();

	if (length <= 0)
		return;

	byte *buffer = new byte[length];

	// TODO: handle Read failure
	file->Read(buffer, length);

	hash.Compute(buffer, length);

	delete[] buffer;
}

static bool FindCacheFilename(std::string& out_name,
	const char *filename, data_file_c *df,
	const char *extension)
{
	std::string wad_dir;
	std::string hash_string;
	std::string local_name;
	std::string cache_name;

	// Get the directory which the wad is currently stored
	wad_dir = epi::PATH_GetDir(filename);

	// Hash string used for files in the cache directory
	hash_string = epi::STR_Format("-%02X%02X%02X-%02X%02X%02X",
		df->dir_hash.hash[0], df->dir_hash.hash[1],
		df->dir_hash.hash[2], df->dir_hash.hash[3],
		df->dir_hash.hash[4], df->dir_hash.hash[5]);

	// Determine the full path filename for "local" (same-directory) version
	local_name = epi::PATH_GetBasename(filename);
	local_name += (".");
	local_name += (extension);

	local_name = epi::PATH_Join(wad_dir.c_str(), local_name.c_str());

	// Determine the full path filename for the cached version
	cache_name = epi::PATH_GetBasename(filename);
	cache_name += (hash_string);
	cache_name += (".");
	cache_name += (extension);

	cache_name = epi::PATH_Join(cache_dir.c_str(), cache_name.c_str());

	I_Debugf("FindCacheFilename: local_name = '%s'\n", local_name.c_str());
	I_Debugf("FindCacheFilename: cache_name = '%s'\n", cache_name.c_str());

	// Check for the existance of the local and cached dir files
	bool has_local = epi::FS_Access(local_name.c_str(), epi::file_c::ACCESS_READ);
	bool has_cache = epi::FS_Access(cache_name.c_str(), epi::file_c::ACCESS_READ);

	// If both exist, use the local one.
	// If neither exist, create one in the cache directory.

	// Check whether the waddir gwa is out of date
	//if (has_local)
	//	has_local = (L_CompareFileTimes(filename, local_name.c_str()) <= 0);

	// Check whether the cached gwa is out of date
	//if (has_cache)
	//	has_cache = (L_CompareFileTimes(filename, cache_name.c_str()) <= 0);

	I_Debugf("FindCacheFilename: has_local=%s  has_cache=%s\n",
		has_local ? "YES" : "NO", has_cache ? "YES" : "NO");

	if (has_local)
	{
		out_name = local_name;
		return true;
	}
	else if (has_cache)
	{
		out_name = cache_name;
		return true;
	}

	// Neither is valid so create one in the cached directory
	out_name = cache_name;
	return false;
}

//
// File_Info
//
typedef struct
{
	data_file_c *dfile;
	const char *name;
	int kind;
	int index;
	int dfindex;
	bool is_named = true;
} file_info_t;

static PHYSFS_EnumerateCallbackResult TopLevel(void *userData, const char *origDir, const char *fname);

static bool Check_isDirectory(const char *fname)
{
	PHYSFS_Stat fstat;
	PHYSFS_stat(fname, &fstat);
	return fstat.filetype == PHYSFS_FILETYPE_DIRECTORY ? true : false;
}

static PHYSFS_EnumerateCallbackResult LumpNamespace(void* userData, const char* origDir, const char* fname)
{
#ifdef HAVE_PHYSFS
	file_info_t* user_data = (file_info_t*)userData;
	char path[256];
	strcpy(path, origDir);
	strcat(path, "/");
	strcat(path, fname);

	I_Debugf("  LumpNamespace: processing %s\n", path);

	if (Check_isDirectory(path))
	{
		// subdirectory... recurse
		PHYSFS_enumerate(path, LumpNamespace, userData);
		return PHYSFS_ENUM_OK;
	}

	//switch( tolower(iwad_base.c_str()))
#if 0
	if (game_mode_doom)
	{
		if (stricmp(fname, "doom") == 0)
		{
			// recurse rott subdirectory to TopLevel
			PHYSFS_enumerate(path, LumpNamespace, userData);
			return PHYSFS_ENUM_OK;
		}
	}

	else if (game_mode_doom2)
	{
		if (stricmp(fname, "doom2") == 0)
		{
			// recurse rott subdirectory to TopLevel
			PHYSFS_enumerate(path, LumpNamespace, userData);
			return PHYSFS_ENUM_OK;
		}
	}
#endif // 0

	if (wolf3d_mode)
	{
		if (stricmp(fname, "wolf3d") == 0)
		{
			// recurse wolf3d subdirectory to TopLevel
			PHYSFS_enumerate(path, LumpNamespace, userData);
			return PHYSFS_ENUM_OK;
		}
	}
	else if (rott_mode)
	{
		if (stricmp(fname, "rott") == 0)
		{
			// recurse rott subdirectory to TopLevel
			PHYSFS_enumerate(path, LumpNamespace, userData);
			return PHYSFS_ENUM_OK;
		}
	}
	//else
		// add lump
		I_Debugf("    opening lump %s\n", fname);
	PHYSFS_File *file = PHYSFS_openRead(path);
	if (!file)
		return PHYSFS_ENUM_OK; // couldn't open file - skip
	int length = PHYSFS_fileLength(file);
	PHYSFS_close(file);

	I_Debugf("    adding lump %s\n", fname);
	numlumps++;
	lumpinfo.resize(numlumps);
	AddLumpEx(user_data->dfile, numlumps - 1, 0, length,
		user_data->dfindex, user_data->index, user_data->is_named ? fname : "INTRNLMP", 1, path);
#endif
	return PHYSFS_ENUM_OK;
}

static PHYSFS_EnumerateCallbackResult WadNamespace(void *userData, const char *origDir, const char *fname)
{
#ifdef HAVE_PHYSFS
	file_info_t *user_data = (file_info_t *)userData;
	raw_wad_header_t header;
	raw_wad_entry_t entry;
	char tname[10];
	char path[256];
	strcpy(path, origDir);
	strcat(path, "/");
	strcat(path, fname);

	I_Printf("  WadNamespace: processing %s\n", path);

	if (Check_isDirectory(path))
	{
		// subdirectory... recurse
		PHYSFS_enumerate(path, WadNamespace, userData);
		return PHYSFS_ENUM_OK;
	}

	// open wad lump
	I_Debugf("    opening wad %s\n", fname);
	PHYSFS_File *file = PHYSFS_openRead(path);
	if (!file)
		return PHYSFS_ENUM_OK; // couldn't open file - skip

	PHYSFS_readBytes(file, &header, sizeof(raw_wad_header_t));

	
	if ((strncmp(header.identification, "IWAD", 4) != 0) && (!wolf3d_mode))
	{
		// Homebrew levels?
		if (strncmp(header.identification, "PWAD", 4) != 0)
		{
			I_Error("Wad file %s doesn't have IWAD or PWAD id\n", fname);
		}
	}

	header.num_entries = EPI_LE_S32(header.num_entries);
	header.dir_start = EPI_LE_S32(header.dir_start);
	PHYSFS_seek(file, header.dir_start);

	// loop over wad directory entries
	for (int i = 0; i < (int)header.num_entries; i++)
	{
		PHYSFS_readBytes(file, &entry, sizeof(raw_wad_entry_t));
		int pos = EPI_LE_S32(entry.pos);
		int size = EPI_LE_S32(entry.size);
		int marker = 0;
		Z_StrNCpy(tname, entry.name, 8);
		// check for ExMy/MAPxx - some map wads have bad map marker lump
		if (strncmp(tname, "MAP", 3) == 0)
		{
			Z_StrNCpy(tname, fname, 5);
			marker = 1;
		}
		else if ((tname[0] == 'E') && (tname[2] == 'M'))
		{
			Z_StrNCpy(tname, fname, 4);
			marker = 1;
		}
		if (strncmp(tname, "TEXTMAP", 7) == 0)
		{
			I_Debugf("    found UDMF map file\n");
			zmarker = 1;
		}
		I_Debugf("    adding wad lump %s\n", tname);
		numlumps++;
		lumpinfo.resize(numlumps);
		AddLumpEx(user_data->dfile, numlumps - 1, pos, size,
			user_data->dfindex, user_data->index, tname, 0, path);

		if (marker)
		{
			// insert level marker
			data_file_c *df = user_data->dfile;

			// check for duplicates (Slige sometimes does this)
			for (int L = 0; L < df->level_markers.GetSize(); L++)
			{
				if (strcmp(lumpinfo[df->level_markers[L]].name, tname) == 0)
				{
					I_Warning("Duplicate level '%s' ignored.\n", tname);
					PHYSFS_close(file);
					return PHYSFS_ENUM_OK;
				}
			}

			df->level_markers.Insert(numlumps - 1);
		}
	}

	PHYSFS_close(file);
#endif
	return PHYSFS_ENUM_OK;
}

static PHYSFS_EnumerateCallbackResult ScriptNamespace(void *userData, const char *origDir, const char *fname)
{
#ifdef HAVE_PHYSFS
	file_info_t *user_data = (file_info_t *)userData;
	//char check[16];
	char path[256];
	strcpy(path, origDir);
	strcat(path, "/");
	strcat(path, fname);

	I_Printf("ScriptNamespace: processing %s\n", path);

	if (Check_isDirectory(path))
	{
		I_Printf("ScriptNamespace: found subdirectory %s\n", fname);

		// check for supported namespace directory
		if (wolf3d_mode)
		{
			if ((stricmp(fname, "wolf3d") == 0) || (stricmp(fname, "wolf3d_ddf") == 0))
			{
				// recurse wolf3d subdirectory to TopLevel
				PHYSFS_enumerate(path, TopLevel, userData);
			}
		}
		else if (rott_mode)
		{
			if ((stricmp(fname, "rott") == 0) || (stricmp(fname, "rott_ddf") == 0))
			{
				I_Printf("Rise of the Triad: enumerate PAKdir...\n");
				// recurse rott subdirectory to TopLevel
				PHYSFS_enumerate(path, TopLevel, userData);
			}
		}
		else if (heretic_mode)
		{
			if ((stricmp(fname, "heretic") == 0) || (stricmp(fname, "heretic_ddf") == 0))
			{
				// recurse heretic subdirectory to TopLevel
				PHYSFS_enumerate(path, TopLevel, userData);
			}
		}
		else if (game_mode_hacx)
		{
			if ((stricmp(fname, "hacx") == 0) || (stricmp(fname, "hacx_ddf") == 0))
			{
				// recurse HACX subdirectory to TopLevel
				PHYSFS_enumerate(path, TopLevel, userData);
			}
		}
		else // default to doom mode
		{
			if ((stricmp(fname, "doom") == 0) || (stricmp(fname, "doom_ddf") == 0))
			{
				// recurse doom subdirectory to TopLevel
				PHYSFS_enumerate(path, TopLevel, userData);
			}
		}

		return PHYSFS_ENUM_OK;
	}

	// check if add global lump - TODO
	if (1)
	{
		// add global lump
		I_Printf("  checking global lump %s\n", fname);
		PHYSFS_File *file = PHYSFS_openRead(path);
		if (!file)
			return PHYSFS_ENUM_OK; // couldn't open file - skip
		int length = PHYSFS_fileLength(file);
		PHYSFS_close(file);

		I_Printf("  adding global lump %s\n", fname);
		numlumps++;
		lumpinfo.resize(numlumps);
		AddLumpEx(user_data->dfile, numlumps - 1, 0, length,
			user_data->dfindex, user_data->index, fname, 1, path);
	}
#endif
	return PHYSFS_ENUM_OK;
}

static PHYSFS_EnumerateCallbackResult TopLevel(void *userData, const char *origDir, const char *fname)
{
#ifdef HAVE_PHYSFS
	file_info_t *user_data = (file_info_t *)userData;
	char check[16];
	char path[256];
	strcpy(path, origDir);
	strcat(path, "/");
	strcat(path, fname);


	I_Printf("TopLevel: processing %s\n", path);

	if (Check_isDirectory(path))
	{
		I_Printf("TopLevel: found subdirectory %s\n", fname);

		// check for supported namespace directory
		if (stricmp(fname, "colormaps") == 0)
		{
			// add fake C_START lump
			I_Printf("  adding fake lump C_START\n");
			numlumps++;
			lumpinfo.resize(numlumps);
			AddLump(NULL, numlumps - 1, 0, 0, user_data->dfindex, user_data->index, "C_START", 0);

			// enumerate all entries in the colormaps directory
			PHYSFS_enumerate(path, LumpNamespace, userData);

			// add fake C_END lump
			I_Printf("  adding fake lump C_END\n");
			numlumps++;
			lumpinfo.resize(numlumps);
			AddLump(NULL, numlumps - 1, 0, 0, user_data->dfindex, user_data->index, "C_END", 0);
		}
		else if (stricmp(fname, "flats") == 0)
		{
			// add fake F_START lump
			I_Printf("  adding fake lump F_START\n");
			numlumps++;
			lumpinfo.resize(numlumps);
			AddLump(NULL, numlumps - 1, 0, 0, user_data->dfindex, user_data->index, "F_START", 0);

			// enumerate all entries in the flats directory
			PHYSFS_enumerate(path, LumpNamespace, userData);

			// add fake F_END lump
			I_Printf("  adding fake lump F_END\n");
			numlumps++;
			lumpinfo.resize(numlumps);
			AddLump(NULL, numlumps - 1, 0, 0, user_data->dfindex, user_data->index, "F_END", 0);
		}
		else if (stricmp(fname, "hires") == 0)
		{
			// add fake HI_START lump
			I_Printf("  adding fake lump HI_START\n");
			numlumps++;
			lumpinfo.resize(numlumps);
			AddLump(NULL, numlumps - 1, 0, 0, user_data->dfindex, user_data->index, "HI_START", 0);

			// enumerate all entries in the hires directory
			PHYSFS_enumerate(path, LumpNamespace, userData);

			// add fake HI_END lump
			I_Printf("  adding fake lump HI_END\n");
			numlumps++;
			lumpinfo.resize(numlumps);
			AddLump(NULL, numlumps - 1, 0, 0, user_data->dfindex, user_data->index, "HI_END", 0);
		}
		else if (stricmp(fname, "patches") == 0)
		{
			// add fake P_START lump
			I_Printf("  adding fake lump P_START\n");
			numlumps++;
			lumpinfo.resize(numlumps);
			AddLump(NULL, numlumps - 1, 0, 0, user_data->dfindex, user_data->index, "P_START", 0);

			// enumerate all entries in the flats directory
			PHYSFS_enumerate(path, LumpNamespace, userData);

			// add fake P_END lump
			I_Printf("  adding fake lump P_END\n");
			numlumps++;
			lumpinfo.resize(numlumps);
			AddLump(NULL, numlumps - 1, 0, 0, user_data->dfindex, user_data->index, "P_END", 0);
		}
		else if (stricmp(fname, "sprites") == 0)
		{
			// add fake S_START lump
			I_Printf("  adding fake lump S_START\n");
			numlumps++;
			lumpinfo.resize(numlumps);
			AddLump(NULL, numlumps - 1, 0, 0, user_data->dfindex, user_data->index, "S_START", 0);

			// enumerate all entries in the sprites directory
			PHYSFS_enumerate(path, LumpNamespace, userData);

			// add fake S_END lump
			I_Printf("  adding fake lump S_END\n");
			numlumps++;
			lumpinfo.resize(numlumps);
			AddLump(NULL, numlumps - 1, 0, 0, user_data->dfindex, user_data->index, "S_END", 0);
		}
		else if (stricmp(fname, "textures") == 0)
		{
			// add fake TX_START lump
			I_Printf("  adding fake lump TX_START\n");
			numlumps++;
			lumpinfo.resize(numlumps);
			AddLump(NULL, numlumps - 1, 0, 0, user_data->dfindex, user_data->index, "TX_START", 0);

			// enumerate all entries in the textures directory
			PHYSFS_enumerate(path, LumpNamespace, userData);

			// add fake TX_END lump
			I_Printf("  adding fake lump TX_END\n");
			numlumps++;
			lumpinfo.resize(numlumps);
			AddLump(NULL, numlumps - 1, 0, 0, user_data->dfindex, user_data->index, "TX_END", 0);
		}
		else if (stricmp(fname, "voices") == 0)
		{
			// add fake V_START lump
			I_Printf("  adding fake lump V_START\n");
			numlumps++;
			lumpinfo.resize(numlumps);
			AddLump(NULL, numlumps - 1, 0, 0, user_data->dfindex, user_data->index, "V_START", 0);

			// enumerate all entries in the voices directory
			PHYSFS_enumerate(path, LumpNamespace, userData);

			// add fake V_END lump
			I_Printf("  adding fake lump V_END\n");
			numlumps++;
			lumpinfo.resize(numlumps);
			AddLump(NULL, numlumps - 1, 0, 0, user_data->dfindex, user_data->index, "V_END", 0);
		}
		else if (stricmp(fname, "voxels") == 0)
		{
			// add fake VX_START lump
			I_Printf("  adding fake lump VX_START\n");
			numlumps++;
			lumpinfo.resize(numlumps);
			AddLump(NULL, numlumps - 1, 0, 0, user_data->dfindex, user_data->index, "VX_START", 0);

			// enumerate all entries in the voxels directory
			PHYSFS_enumerate(path, LumpNamespace, userData);

			// add fake VX_END lump
			I_Printf("  adding fake lump VX_END\n");
			numlumps++;
			lumpinfo.resize(numlumps);
			AddLump(NULL, numlumps - 1, 0, 0, user_data->dfindex, user_data->index, "VX_END", 0);
		}
		//Startup IWAD?
#if 0
		else if (stricmp(fname, "startup") == 0)
		{
			// enumerate all entries in the graphics directory
			PHYSFS_enumerate(path, LumpNamespace, userData);
		}
#endif // 0

		else if (stricmp(fname, "graphics") == 0)
		{
			// enumerate all entries in the graphics directory
			PHYSFS_enumerate(path, LumpNamespace, userData);
		}


		//const char* game_extras[] = { "base", "extras", NULL };
		else if (stricmp(fname, "maps") == 0)
		{
			// enumerate all entries in the maps directory
			PHYSFS_enumerate(path, WadNamespace, userData);
		}
		else if (stricmp(fname, "models") == 0)
		{
			// enumerate all entries in the models directory
			PHYSFS_enumerate(path, LumpNamespace, userData);
		}
		else if (stricmp(fname, "music") == 0)
		{
			// enumerate all entries in the music directory
			PHYSFS_enumerate(path, LumpNamespace, userData);
		}
		else if (stricmp(fname, "skins") == 0)
		{
			// enumerate all entries in the skins directory
			PHYSFS_enumerate(path, LumpNamespace, userData);
		}
		else if (stricmp(fname, "soundfonts") == 0)
		{
		// enumerate all entries in the soundfonts directory
		PHYSFS_enumerate(path, LumpNamespace, userData);
		}
		else if (stricmp(fname, "sounds") == 0)
		{
			// enumerate all entries in the sounds directory
			PHYSFS_enumerate(path, LumpNamespace, userData);
		}
		else if (stricmp(fname, "scripts") == 0)
		{
			// enumerate scripts subdirectory
			PHYSFS_enumerate(path, ScriptNamespace, userData);
		}
		else if (stricmp(fname, "shaders") == 0)
		{
			// enumerate scripts subdirectory
			user_data->is_named = false;
			PHYSFS_enumerate(path, LumpNamespace, userData);
			user_data->is_named = true;
		}
		else if ((stricmp(fname, "video") == 0) || (stricmp(fname, "cinematics") == 0))
		{
			// enumerate scripts subdirectory
			PHYSFS_enumerate(path, LumpNamespace, userData);
		}
		else if (wolf3d_mode)
		{	// Checks for Global Wolfenstein3D palette (PLAYPAL) instead of palette-byte translation (from SLADE.pk3)
			if (stricmp(fname, "wolf3d") == 0)
			{
				// enumerate all entries in placebo Wolf3d Directory (inside EDGE.pak)
				PHYSFS_enumerate(path, LumpNamespace, userData);
			}
		}
		else if (rott_mode)
        {
			// Checks for Global ROTT palette (PLAYPAL) instead of palette-byte translation (from SLADE.pk3)
			if (stricmp(fname, "rott") == 0)
				{
					//		// enumerate all entries in placebo ROTT Directory (inside EDGE.pak)
					PHYSFS_enumerate(path, LumpNamespace, userData);
				}
        }
		else if (strncasecmp(fname, "root", 4) == 0)
		{
            {
			// recurse root subdirectory to TopLevel
			PHYSFS_enumerate(path, TopLevel, userData);
            }
		}

		return PHYSFS_ENUM_OK;
	}

	// check for special lumps, like map wad lumps
	strcpy(check, "E1M1.wad");
	for (int i = 1; i < 5; i++)
	{
		check[1] = '0' + i;
		for (int j = 1; j < 10; j++)
		{
			check[3] = '0' + j;
			if (strncmp(fname, check, 8) == 0)
			{
				// process wad lump
				WadNamespace(userData, origDir, fname);
				return PHYSFS_ENUM_OK;
			}
		}
	}
	strcpy(check, "MAP01.wad");
	for (int i = 1; i < 100; i++)
	{
		check[3] = '0' + i / 10;
		check[4] = '0' + i % 10;
		if (strncmp(fname, check, 9) == 0)
		{
			// process wad lump
			WadNamespace(userData, origDir, fname);
			return PHYSFS_ENUM_OK;
		}
	}
	// checking for startup.wad!
	strcpy(check, "startup.wad");
	for (int i = 1; i < 8; i++)
	{
		check[3] = '0' + i / 10;
		check[4] = '0' + i % 10;
		if (strncmp(fname, check, 9) == 0)
		{
			// process wad lump
			WadNamespace(userData, origDir, fname);
			return PHYSFS_ENUM_OK;
		}
	}

	// check if add global lump - TODO
	if (1)
	{
		// add global lump
		I_Printf("  checking global lump %s\n", fname);
		PHYSFS_File *file = PHYSFS_openRead(path);
		if (!file)
			return PHYSFS_ENUM_OK; // couldn't open file - skip
		int length = PHYSFS_fileLength(file);
		PHYSFS_close(file);

		I_Printf("  adding global lump %s\n", fname);
		numlumps++;
		lumpinfo.resize(numlumps);
		AddLumpEx(user_data->dfile, numlumps - 1, 0, length,
			user_data->dfindex, user_data->index, fname, 1, path);
	}
#endif
	return PHYSFS_ENUM_OK;
}

extern bool MapsReadHeaders(); //<--- This makes wlf_maps able to start the header identification



//
// AddFile
//
// -AJA- New `dyn_index' parameter -- this is for adding GWA files
//       which have been built by the GLBSP plugin.  Nothing else is
//       supported, e.g. wads with textures/sprites/DDF/RTS.
//
//       The dyn_index value is -1 for normal (non-dynamic) files,
//       otherwise it is the sort_index for the lumps (typically the
//       file number of the wad which the GWA is a companion for).
//
static void AddFile(const char *filename, int kind, int dyn_index)
{
	int j;
	int length;
	int startlump;

	raw_wad_header_t header;
	raw_wad_entry_t *curinfo;

	/* Wolfenstein 3D headers/entries/etc*/ //TODO

	// reset the sprite/flat/patch list stuff
	within_sprite_list = within_flat_list = false;
	within_patch_list = within_colmap_list = false;
	within_tex_list = within_hires_list = false;
	within_lbm_list = within_rottpic_list = within_rawflats_list = false;

	// handle pak/pk3/pk7 files before default file handling
	if ((kind == FLKIND_PAK) || (kind == FLKIND_PK3) || (kind == FLKIND_PK7) || (kind == FLKIND_EPK))
	{
#ifdef HAVE_PHYSFS
		file_info_t userData;
		char pakdir[16];
		char number[8];

		// make mount name for PHYSFS for this pack file
		strcpy(pakdir, "/pack");
		sprintf(number, "%x", num_paks++);
		strcat(pakdir, number);

		startlump = numlumps;

		int datafile = (int)data_files.size();
		data_file_c *df = new data_file_c(filename, kind, NULL);
		data_files.push_back(df);

		int pakErr = PHYSFS_mount(filename, pakdir, 0); // add to head of temp search list
		if (!pakErr)
			I_Error("Error adding %s to search list\n", filename);

		I_Printf("  Adding %s\n", filename);

		userData.dfile = df;
		userData.name = filename;
		userData.kind = kind;
		userData.index = (dyn_index >= 0) ? dyn_index : datafile;
		userData.dfindex = datafile;

		zmarker = 0;

		PHYSFS_enumerate(pakdir, TopLevel, (void *)&userData);

		SortLumps();
		SortSpriteLumps(df);

		// set up caching
		Z_Resize(lumplookup, lumpheader_t *, numlumps);

		for (j = startlump; j < numlumps; j++)
			lumplookup[j] = NULL;

		// check for unclosed sprite/flat/patch lists
		if (within_sprite_list)
			I_Warning("Missing S_END marker in %s.\n", filename);

		if (within_flat_list)
			I_Warning("Missing F_END marker in %s.\n", filename);

		if (within_patch_list)
			I_Warning("Missing P_END marker in %s.\n", filename);

		if (within_colmap_list)
			I_Warning("Missing C_END marker in %s.\n", filename);

		if (within_tex_list)
			I_Warning("Missing TX_END marker in %s.\n", filename);

		if (within_hires_list)
			I_Warning("Missing HI_END marker in %s.\n", filename);

		// check if pack has non-UDMF level maps
		if (df->level_markers.GetSize() > 0 && !zmarker)
		{
			// yes - check for existing GL node file, or make one
			if (HasInternalGLNodes(df, datafile))
			{
				I_Printf("Using internal GL nodes\n");
				df->companion_gwa = datafile;
			}
			else
			{
				SYS_ASSERT(dyn_index < 0);

				std::string gwa_filename;

				bool exists = FindCacheFilename(gwa_filename, filename, df, EDGEGWAEXT);

				I_Debugf("Actual_GWA_filename: %s\n", gwa_filename.c_str());

				if (!exists)
				{
					I_Printf("Building GL Nodes for: %s\n", filename);

					if (!AJ_BuildNodes(pakdir, gwa_filename.c_str()))
						I_Error("Failed to build GL nodes for: %s\n", filename);
				}
				else
				{
					I_Printf("Using cached GWA file: %s\n", gwa_filename.c_str());
				}

				// Load it.  This recursion bit is rather sneaky,
				// hopefully it doesn't break anything...
				AddFile(gwa_filename.c_str(), FLKIND_GWad, datafile);

				df->companion_gwa = datafile + 1;
			}
		}
		else if (zmarker)
			I_Printf("UDMF levels in this pack\n");
		else
			I_Printf("No levels in this pack\n");

#if 0
		if (wolf3d_mode)
		{
			I_Printf("WOLF: Read levels from MAPHEAD\n");
			WF_InitMaps();

			std::string maphead_filename;
			char base_name[64];
			sprintf(base_name, "GAMEMAPS.%s", datafile, WOLFDATEXT);
			maphead_filename = epi::PATH_Join(cache_dir.c_str(), base_name);

			I_Debugf("Actual_MAPHEAD_filename: %s\n", maphead_filename.c_str());

			AddFile(maphead_filename.c_str(), FLKIND_GAMEMAPS, datafile);

			const char *lump_name = lumpinfo[df->wolf_maphead].name;

			I_Printf("Converting [%s] /?lump/? in: %s\n", lump_name, filename);

			const byte *data = (const byte *)W_CacheLumpNum(df->wolf_maphead);
			int length = W_LumpLength(df->wolf_maphead);

			W_DoneWithLump(data);

			AddFile(maphead_filename.c_str(), FLKIND_MAPHEAD, datafile);
			WF_BuildBSP();
			//WF_SetupLevel();
			//I_Printf("WOLF: Not implemented!\n");
		}

#endif // 0

		// handle DeHackEd patch files
		if (df->deh_lump >= 0)
		{
			std::string hwa_filename;

			char base_name[64];
			sprintf(base_name, "DEH_%04d.%s", datafile, EDGEHWAEXT);

			hwa_filename = epi::PATH_Join(cache_dir.c_str(), base_name);

			I_Debugf("Actual_HWA_filename: %s\n", hwa_filename.c_str());

			const char *lump_name = lumpinfo[df->deh_lump].name;

			I_Printf("Converting [%s] lump in: %s\n", lump_name, filename);

			const byte *data = (const byte *)W_CacheLumpNum(df->deh_lump);
			int length = W_LumpLength(df->deh_lump);

			if (!DH_ConvertLump(data, length, lump_name, hwa_filename.c_str()))
				I_Error("Failed to convert DeHackEd LUMP in: %s\n", filename);

			W_DoneWithLump(data);

			// Load it (using good ol' recursion again).
			AddFile(hwa_filename.c_str(), FLKIND_HWad, -1);
		}


		return;
#else
		I_Error("PHYSFS support not compiled. Attempt to open file %s\n", filename);
#endif
	}

	// open the file and add to directory
	epi::file_c *file = epi::FS_Open(filename, epi::file_c::ACCESS_READ | epi::file_c::ACCESS_BINARY);
	if (file == NULL)
	{
		I_Error("Couldn't open file %s\n", filename);
		return;
	}

	I_Printf("  Adding %s\n", filename);

	startlump = numlumps;

	int datafile = (int)data_files.size();
	data_file_c *df = new data_file_c(filename, kind, file);
	data_files.push_back(df);

	// for RTS scripts, adding the data_file is enough
	if (kind == FLKIND_RTS)
		return;

	// for ROQ, is adding the data_file enough??
	if (kind == FLKIND_ROQ)
	{
		return;
	}

	//First things first: Here, we will call all of the existing functions to open neccesarry Wolf files.
	// Start FLKIND_ enumeration, eventually starting with MAPHEAD/GAMEMAPS -> VGADICT/VGAGRAPH, VSWAP,

#if 0
	if ((kind == FLKIND_VGADICT) && (kind == FLKIND_VGAGRAPH))
	{
		WF_GraphicsOpen();
		// First opens Dictionary, Header, and Sizes. Enumerate into image_c *rim class.
		// compute MD5 hash over wad directory
		//df->dir_hash.Compute((const byte *)fileinfo, length);

		// Fill in lumpinfo
		numlumps += header.num_entries;
		Z_Resize(lumpinfo, lumpinfo_t, numlumps);
	}

	if (kind == FLKIND_VSWAP)
	{
		//I_Printf("WOLF: DETECTED FILEKIND 'WL6'\n!");

		I_Printf("Opening VSWAP...\n");

		vswap.fp = fopen("VSWAP.WL6", "rb");
		raw_vswap_t vv;

		vswap.first_wall = 0;  // implied
		vswap.first_sprite = EPI_LE_U16(vv.first_sprite);
		vswap.first_sound = EPI_LE_U16(vv.first_sound);

		int total_chunks = EPI_LE_U16(vv.total_chunks);

		vswap.num_walls = vswap.first_sprite;
		vswap.num_sprites = vswap.first_sound - vswap.first_sprite;
		vswap.num_sounds = total_chunks - vswap.first_sound;

		file->Read(&total_chunks, sizeof(raw_vswap_t));
		//Now, return the list of stuff here, or we just read in from VSwapOpen() directly.

		vswap.chunks.reserve(total_chunks);

		for (int i = 0; i < total_chunks; i++)
		{
			raw_chunk_t CK;

			if (fread(&CK.offset, 4, 1, vswap.fp) != 1)
				throw "FUCK3";

			CK.offset = EPI_LE_U32(CK.offset);

			vswap.chunks.push_back(CK);
		}

		for (int i = 0; i < total_chunks; i++)
		{
			raw_chunk_t& CK = vswap.chunks[i];

			if (fread(&CK.length, 2, 1, vswap.fp) != 1)
				throw "FUCK4";

			CK.length = EPI_LE_U16(CK.length);

			L_WriteDebug("[%d] : offset %d, length %d\n", i, CK.offset, CK.length);
		}
	}
#endif // 0
	if (kind <= FLKIND_HWad)
	{
		// WAD file
		// TODO: handle Read failure
		file->Read(&header, sizeof(raw_wad_header_t));
		if (strncmp(header.identification, "IWAD", 4) != 0)
		{
			// Homebrew levels?
			if (strncmp(header.identification, "PWAD", 4) != 0)
			{
				I_Error("Wad file %s doesn't have IWAD or PWAD id\n", filename);
			}
		}

		header.num_entries = EPI_LE_S32(header.num_entries);
		header.dir_start = EPI_LE_S32(header.dir_start);

		length = header.num_entries * sizeof(raw_wad_entry_t);

		raw_wad_entry_t *fileinfo = new raw_wad_entry_t[header.num_entries];

		file->Seek(header.dir_start, epi::file_c::SEEKPOINT_START);

		// TODO: handle Read failure
		file->Read(fileinfo, length);

		// compute MD5 hash over wad directory
		df->dir_hash.Compute((const byte *)fileinfo, length);

		// Fill in lumpinfo
		numlumps += header.num_entries;
		lumpinfo.resize(numlumps);

		for (j = startlump, curinfo = fileinfo; j < numlumps; j++, curinfo++)
		{
			AddLump(df, j, EPI_LE_S32(curinfo->pos), EPI_LE_S32(curinfo->size),
				datafile,
				(dyn_index >= 0) ? dyn_index : datafile,
				curinfo->name,
				(kind == FLKIND_PWad) || (kind == FLKIND_HWad));

			if (kind != FLKIND_HWad)
				CheckForLevel(df, j, lumpinfo[j].name, curinfo, numlumps - 1 - j);
		}

		delete[] fileinfo;
	}
	else  /* single lump file */
	{
		char lump_name[32];

		SYS_ASSERT(dyn_index < 0);

		if (kind == FLKIND_DDF)
		{
			DDF_GetLumpNameForFile(filename, lump_name);
		}
		else
		{
			std::string base = epi::PATH_GetBasename(filename);
			if (base.size() > 8)
				I_Error("Filename base of %s >8 chars", filename);

            strcpy(lump_name, base.c_str());
			for (size_t i=0;i<strlen(lump_name);i++) {
				lump_name[i] = toupper(lump_name[i]);
			}
        }

		// calculate MD5 hash over whole file
		ComputeFileMD5hash(df->dir_hash, file);

		// Fill in lumpinfo
		numlumps++;
		lumpinfo.resize(numlumps);

		AddLump(df, startlump, 0,
			file->GetLength(), datafile, datafile,
			lump_name, true);
	}
	I_Debugf("   md5hash = %02x%02x%02x%02x...%02x%02x%02x%02x\n",
		df->dir_hash.hash[0], df->dir_hash.hash[1],
		df->dir_hash.hash[2], df->dir_hash.hash[3],
		df->dir_hash.hash[12], df->dir_hash.hash[13],
		df->dir_hash.hash[14], df->dir_hash.hash[15]);

	SortLumps();
	SortSpriteLumps(df);

	// set up caching
	Z_Resize(lumplookup, lumpheader_t *, numlumps);

	for (j = startlump; j < numlumps; j++)
		lumplookup[j] = NULL;

	// check for unclosed sprite/flat/patch lists
	if (within_sprite_list)
		I_Warning("Missing S_END marker in %s.\n", filename);

	if (within_flat_list)
		I_Warning("Missing F_END marker in %s.\n", filename);

	if (within_patch_list)
		I_Warning("Missing P_END marker in %s.\n", filename);

	if (within_colmap_list)
		I_Warning("Missing C_END marker in %s.\n", filename);

	if (within_tex_list)
		I_Warning("Missing TX_END marker in %s.\n", filename);

	if (within_hires_list)
		I_Warning("Missing HI_END marker in %s.\n", filename);

	// -AJA- 1999/12/25: What did Santa bring EDGE ?  Just some support
	//       for "GWA" files (part of the "GL-Friendly Nodes" specs).

	if (kind <= FLKIND_EWad && df->level_markers.GetSize() > 0)
	{
		if (HasInternalGLNodes(df, datafile))
		{
			df->companion_gwa = datafile;
		}
		else
		{
			SYS_ASSERT(dyn_index < 0);

			std::string gwa_filename;

			bool exists = FindCacheFilename(gwa_filename, filename, df, EDGEGWAEXT);

			I_Debugf("Actual_GWA_filename: %s\n", gwa_filename.c_str());

			if (!exists)
			{
				I_Printf("Building GL Nodes for: %s\n", filename);

				if (!AJ_BuildNodes(filename, gwa_filename.c_str()))
					I_Error("Failed to build GL nodes for: %s\n", filename);
			}

			// Load it.  This recursion bit is rather sneaky,
			// hopefully it doesn't break anything...
			AddFile(gwa_filename.c_str(), FLKIND_GWad, datafile);

			df->companion_gwa = datafile + 1;
		}
	}

	// handle DeHackEd patch files
	if (kind == FLKIND_Deh || df->deh_lump >= 0)
	{
		std::string hwa_filename;

		char base_name[64];
		sprintf(base_name, "DEH_%04d.%s", datafile, EDGEHWAEXT);

		hwa_filename = epi::PATH_Join(cache_dir.c_str(), base_name);

		I_Debugf("Actual_HWA_filename: %s\n", hwa_filename.c_str());

		if (kind == FLKIND_Deh)
		{
			I_Printf("Converting DEH file: %s\n", filename);

			if (!DH_ConvertFile(filename, hwa_filename.c_str()))
				I_Error("Failed to convert DeHackEd patch: %s\n", filename);
		}
		else
		{
			const char *lump_name = lumpinfo[df->deh_lump].name;

			I_Printf("Converting [%s] lump in: %s\n", lump_name, filename);

			const byte *data = (const byte *)W_CacheLumpNum(df->deh_lump);
			int length = W_LumpLength(df->deh_lump);

			if (!DH_ConvertLump(data, length, lump_name, hwa_filename.c_str()))
				I_Error("Failed to convert DeHackEd LUMP in: %s\n", filename);

			W_DoneWithLump(data);
		}

		// Load it (using good ol' recursion again).
		AddFile(hwa_filename.c_str(), FLKIND_HWad, -1);
	}
}

static void InitCaches(void)
{
	lumphead.next = lumphead.prev = &lumphead;
}

//
// W_AddRawFilename
//
void W_AddRawFilename(const char *file, int kind)
{
	I_Debugf("Added filename: %s\n", file);

	wadfiles.push_back(new raw_filename_c(file, kind));
}

void WLF_AddRawFilename(const char* file, int kind)
{
	I_Debugf("Added filename: %s\n", file);

	wl6files.push_back(new raw_filename_c(file, kind));
}

//
// W_InitMultipleFiles
//
// Pass a null terminated list of files to use.
// Files with a .wad extension are idlink files with multiple lumps.
// Other files are single lumps with the base filename for the lump name.
// Lump names can appear multiple times.
// The name searcher looks backwards, so a later file
//   does override all earlier ones.
//
void W_InitMultipleFiles(void)
{
	InitCaches();
	// open all the files, load headers, and count lumps
	numlumps = 0;

	// will be realloced as lumps are added
	lumpinfo.clear();

	std::list<raw_filename_c *>::iterator it;

	for (it = wadfiles.begin(); it != wadfiles.end(); it++)
	{
		raw_filename_c *rf = *it;
		AddFile(rf->filename.c_str(), rf->kind, -1);
	}

	if (numlumps == 0)
		I_Warning("W_InitMultipleFiles: no files found!\n");
}

static bool TryLoadExtraLanguage(const char *name)
{
	int lumpnum = W_CheckNumForName(name);

	if (lumpnum < 0)
		return false;

	I_Printf("Loading Languages from %s\n", name);

	int length;
	char *data = (char *)W_ReadLumpAlloc(lumpnum, &length);

	DDF_ReadLangs(data, length);
	delete[] data;

	return true;
}

static bool TryLoadExtraGame(const char *name)
{
	int lumpnum = W_CheckNumForName(name);

	if (lumpnum < 0)
		return false;

	I_Printf("Loading additional graphics from %s\n", name);

	int length;
	char *data = (char *)W_ReadLumpAlloc(lumpnum, &length);

	DDF_ReadGames(data, length);
	delete[] data;

	return true;
}

// MUNDO HACK, but if only fixable by a new wad structure...
// CA - this will be changed to read the HERETIC language file 2.24.2013
static void LoadTntPlutStrings(void)
{
	if (DDF_CompareName(iwad_base.c_str(), "TNT") == 0)
		TryLoadExtraLanguage("TNTLANG");

	if (DDF_CompareName(iwad_base.c_str(), "PLUTONIA") == 0)
		TryLoadExtraLanguage("PLUTLANG");
}

static void LoadTntPlutGame(void)
{
	if (DDF_CompareName(iwad_base.c_str(), "TNT") == 0)
		TryLoadExtraGame("TNTGAME");

	if (DDF_CompareName(iwad_base.c_str(), "PLUTONIA") == 0)
		TryLoadExtraGame("PLUTGAME");
}

void W_ReadDDF(void)
{
	// -AJA- the order here may look strange.  Since DDF files
	// have dependencies between them, it makes more sense to
	// load all lumps of a certain type together (e.g. all
	// DDFSFX lumps before all the DDFTHING lumps).

	for (int d = 0; d < NUM_DDF_READERS; d++)
	{
		if (true)
		{
			I_Printf("Loading external %s\n", DDF_Readers[d].print_name);

			// call read function
			(*DDF_Readers[d].func)(NULL, 0);
		}

		for (int f = 0; f < (int)data_files.size(); f++)
		{
			data_file_c *df = data_files[f];

			// all script files get parsed here
			if (d == RTS_READER && df->kind == FLKIND_RTS)
			{
				I_Printf("Loading RTS script: %s\n", df->file_name);

				RAD_LoadFile(df->file_name);
				continue;
			}

			if (df->kind >= FLKIND_Demo)
				continue;

			if (df->kind == FLKIND_EWad)
			{
				// special handling for TNT and Plutonia
				// edit for HERETIC iwad
				if (d == LANG_READER)
					LoadTntPlutStrings();

				if (d == GAME_READER)
					LoadTntPlutGame();

				continue;
			}

			int lump = df->ddf_lumps[d];

			if (lump >= 0)
			{
				I_Printf("Loading %s from: %s\n", DDF_Readers[d].name, df->file_name);

				int length;
				char *data = (char *)W_ReadLumpAlloc(lump, &length);

				// call read function
				(*DDF_Readers[d].func)(data, length);
				delete[] data;
			}

			// handle Boom's ANIMATED and SWITCHES lumps
			if (d == ANIM_READER && df->animated >= 0)
			{
				I_Printf("Loading ANIMATED from: %s\n", df->file_name);

				int length;
				byte *data = W_ReadLumpAlloc(df->animated, &length);

				DDF_ParseANIMATED(data, length);
				delete[] data;
			}
			if (d == SWTH_READER && df->switches >= 0)
			{
				I_Printf("Loading SWITCHES from: %s\n", df->file_name);

				int length;
				byte *data = W_ReadLumpAlloc(df->switches, &length);

				DDF_ParseSWITCHES(data, length);
				delete[] data;
			}

			// handle BOOM Colourmaps (between C_START and C_END)
			if (d == COLM_READER && df->colmap_lumps.GetSize() > 0)
			{
				for (int i = 0; i < df->colmap_lumps.GetSize(); i++)
				{
					int lump = df->colmap_lumps[i];

					DDF_ColourmapAddRaw(W_GetLumpName(lump), W_LumpLength(lump));
				}
			}
		}

		///		std::string msg_buf(epi::STR_Format(
		///			"Loaded %s %s\n", (d == NUM_DDF_READERS-1) ? "RTS" : "DDF",
		///				DDF_Readers[d].print_name));
		///
		///		E_ProgressMessage(msg_buf.c_str());

		E_LocalProgress(d, NUM_DDF_READERS);
	}
}

void W_ReadCoalLumps(void)
{
	for (int f = 0; f < (int)data_files.size(); f++)
	{
		data_file_c *df = data_files[f];

		if (df->kind > FLKIND_Lump)
			continue;

		if (df->coal_api >= 0)
			VM_LoadLumpOfCoal(df->coal_api);

		if (df->coal_huds >= 0)
			VM_LoadLumpOfCoal(df->coal_huds);
	}
}

epi::file_c *W_OpenLump(int lump)
{
	SYS_ASSERT(0 <= lump && lump < numlumps);

	lumpinfo_t* l = &lumpinfo[lump];

	data_file_c *df = data_files[l->file];

#if _DEBUG
I_Debugf("W_OpenLump: %d(%s)\n", lump, l->name);
#endif

	if (df->file == NULL)
	{
#ifdef HAVE_PHYSFS
		// PHYSFS controlled file
		PHYSFS_File *file = PHYSFS_openRead(l->path);
		SYS_ASSERT(file != NULL);
		return new epi::sub_file_c((epi::file_c*)file, l->position | 0x40000000, l->size);
#else
		SYS_ASSERT(df->file);
#endif
	}

	return new epi::sub_file_c(df->file, l->position, l->size);
}

epi::file_c *W_OpenLump(const char *name)
{
#ifdef _DEBUG
	I_Printf("W_OpenLump: %s\n", name);
#endif
	return W_OpenLump(W_GetNumForName(name));
}

//
// W_GetFileName
//
// Returns the filename of the WAD file containing the given lump, or
// NULL if it wasn't a WAD file (e.g. a pure lump).
//
const char *W_GetFileName(int lump)
{
	SYS_ASSERT(0 <= lump && lump < numlumps);

	lumpinfo_t* l = &lumpinfo[lump];

	data_file_c *df = data_files[l->file];

	if (df->kind >= FLKIND_Lump)
		return NULL;

	return df->file_name;
}

//
// W_GetPaletteForLump
//
// Returns the palette lump that should be used for the given lump
// (presumably an image), otherwise -1 (indicating that the global
// palette should be used).
//
// NOTE: when the same WAD as the lump does not contain a palette,
// there are two possibilities: search backwards for the "closest"
// palette, or simply return -1.  Neither one is ideal, though I tend
// to think that searching backwards is more intuitive.
//
// NOTE 2: the palette_datafile stuff is there so we always return -1
// for the "GLOBAL" palette.
//
int W_GetPaletteForLump(int lump)
{
	SYS_ASSERT(0 <= lump && lump < numlumps);

	// if override palette for mod, return last file with PLAYPAL
	if (modpalette)
	{
		data_file_c *df = data_files[palette_lastfile];

		if (df->wadtex.palette >= 0)
			return df->wadtex.palette;
	}

	int f = lumpinfo[lump].file;

	for (; f > palette_datafile; f--)
	{
		data_file_c *df = data_files[f];

		if (df->kind >= FLKIND_Lump)
			continue;

		if (df->wadtex.palette >= 0)
			return df->wadtex.palette;
	}

	// none found
	return -1;
}

static inline int QuickFindLumpMap(char *buf)
{
	int i;

#define CMP(a)  (LUMP_MAP_CMP(a) < 0)
	BSEARCH(numlumps, i);
#undef CMP

	if (i < 0 || i >= numlumps || LUMP_MAP_CMP(i) != 0)
	{
		// not found (nothing has that name)
		return -1;
	}

	// jump to first matching name
	while (i > 0 && LUMP_MAP_CMP(i - 1) == 0)
		i--;

	return i;
}

//
// W_CheckNumForName
//
// Returns -1 if name not found.
//
// -ACB- 1999/09/18 Added name to error message
//
int W_CheckNumForName2(const char *name)
{
	int i;
	char buf[9];

	for (i = 0; name[i]; i++)
	{
		if (i > 8)
		{
			I_Warning("W_CheckNumForName: Name '%s' longer than 8 chars!\n", name);
			return -1;
		}
		buf[i] = toupper(name[i]);
	}
	buf[i] = 0;

	i = QuickFindLumpMap(buf);

	if (i < 0)
		return -1; // not found

	return lumpmap[i];
}

int W_CheckNumForName3(const char *name)
{
	int i;
	char buf[256];

	for (i = 0; name[i]; i++)
	{
		buf[i] = toupper(name[i]);
	}
	buf[i] = 0;

	i = QuickFindLumpMap(buf);

	if (i < 0)
	{
		I_Printf("W_CheckNumForName3: i is %s\n", name);
		return -1; // not found
	}

	return lumpmap[i];
}

int W_CheckNumForName_GFX(const char *name)
{
	// this looks for a graphic lump, skipping anything which would
	// not be suitable (especially flats and HIRES replacements).

	int i;
	char buf[9];

	for (i = 0; name[i]; i++)
	{
		if (i > 8)
		{
			I_Warning("W_CheckNumForName: Name '%s' longer than 8 chars!\n", name);
			return -1;
		}
		buf[i] = toupper(name[i]);
	}
	buf[i] = 0;

	// search backwards
	for (i = numlumps - 1; i >= 0; i--)
	{
		if (lumpinfo[i].kind == LMKIND_Normal ||
			lumpinfo[i].kind == LMKIND_Sprite ||
			lumpinfo[i].kind == LMKIND_Patch )// ||
			//lumpinfo[i].kind == LMKIND_LBM ||
			//lumpinfo[i].kind == LMKIND_PIC ||
			//lumpinfo[i].kind == LMKIND_RAWFLATS)
		{
			if (strncmp(lumpinfo[i].name, buf, 8) == 0)
				return i;
		}
	}

	return -1; // not found
}

//
// W_GetNumForName
//
// Calls W_CheckNumForName, but bombs out if not found.
//
int W_GetNumForName2(const char *name)
{
	int i;

	if ((i = W_CheckNumForName2(name)) == -1)
		I_Error("W_GetNumForName: \'%.8s\' not found!", name);

	return i;
}

//==========================================================================
//
// W_FindLumpFromPath
//
// Ignore 8 character limit and retrieve lump from path to use that instead.
//
//
//==========================================================================

int W_FindLumpFromPath(const std::string &path)
{
	for (int i = 0; i < numlumps; i++)
	{
		if (lumpinfo[i].path == path)
			return i;
	}
	return -1;
}

//==========================================================================
//
// W_FindNameFromPath
//
// Ignore 8 character limit and retrieve lump from its full name to use that instead.
//
//==========================================================================
int W_FindNameFromPath(const char *name)
{
	//std::string fn; //TODO: V808 https://www.viva64.com/en/w/v808/ 'fn' object of 'basic_string' type was created but was not utilized.

	for (int i = 0; i < numlumps; i++)
	{
		if (!lumpinfo[i].fullname.compare(0, strlen(name), name)) //TODO: V814 https://www.viva64.com/en/w/v814/ Decreased performance. The 'strlen' function was called multiple times inside the body of a loop.
			return i;
	}
	return -1;
}


//
// W_GetNumForName
//
// Calls W_FindLumpFromPath, but bombs out if not found.
//
// CA: So far, unused, disabled for Linux as GCC bombs out for some reason. .
#if 0
int W_GetLumpFromPath2(const std::string &path)
{
	int i;

	if ((i = W_FindLumpFromPath(path)) == -1)
		I_Error("W_FindLumpFromPath: \'%s\' not found!", path);

	return i;
}
#endif // 0



//
// W_GetNumForName
//
// Calls W_CheckNumForName, but bombs out if not found.
//
int W_GetNumForFullName2(const char *name)
{
	int i;

	if ((i = W_FindNameFromPath(name)) == -1)
		I_Error("W_GetNumForName: '%s' not found!", name);

	return i;
}

//
// W_CheckNumForTexPatch
//
// Returns -1 if name not found.
//
// -AJA- 2004/06/24: Patches should be within the P_START/P_END markers,
//       so we should look there first.  Also we should never return a
//       flat as a tex-patch.
//
int W_CheckNumForTexPatch(const char *name)
{
	int i;
	char buf[10];

	for (i = 0; name[i]; i++)
	{
#ifdef DEVELOPERS
		if (i > 8)
			I_Error("W_CheckNumForTexPatch: '%s' longer than 8 chars!", name);
#endif
		buf[i] = toupper(name[i]);
	}
	buf[i] = 0;

	i = QuickFindLumpMap(buf);

	if (i < 0)
		return -1;  // not found

	for (; i < numlumps && LUMP_MAP_CMP(i) == 0; i++)
	{
		lumpinfo_t *L = &lumpinfo[lumpmap[i]];

		if (L->kind == LMKIND_Patch || L->kind == LMKIND_Sprite || L->kind == LMKIND_PIC || // L->kind == LMKIND_ROTTPatch || L->kind == LMKIND_ROTTPic ||
			L->kind == LMKIND_Normal)
		{
			// allow LMKIND_Normal to support patches outside of the
			// P_START/END markers.  We especially want to disallow
			// flat and colourmap lumps.
			return lumpmap[i];
		}
	}

	return -1;  // nothing suitable
}

//
// W_VerifyLumpName
//
// Verifies that the given lump number is valid and has the given
// name.
//
// -AJA- 1999/11/26: written.
//
bool W_VerifyLumpName(int lump, const char *name)
{
	if (lump >= numlumps)
		return false;

	return (strncmp(lumpinfo[lump].name, name, 8) == 0);
}

//
// W_LumpLength
//
// Returns the buffer size needed to load the given lump.
//
int W_LumpLength(int lump)
{
	if (lump >= numlumps)
		I_Error("W_LumpLength: %i >= numlumps", lump);

	return lumpinfo[lump].size;
}

//
// W_FindFlatSequence
//
// Returns the file number containing the sequence, or -1 if not
// found.  Search is from newest wad file to oldest wad file.
//
int W_FindFlatSequence(const char *start, const char *end,
	int *s_offset, int *e_offset)
{
	for (int file = (int)data_files.size() - 1; file >= 0; file--)
	{
		data_file_c *df = data_files[file];

		// look for start name
		int i;
		for (i = 0; i < df->flat_lumps.GetSize(); i++)
		{
			if (strncmp(start, W_GetLumpName(df->flat_lumps[i]), 8) == 0)
				break;
		}

		if (i >= df->flat_lumps.GetSize())
			continue;

		(*s_offset) = i;

		// look for end name
		for (i++; i < df->flat_lumps.GetSize(); i++)
		{
			if (strncmp(end, W_GetLumpName(df->flat_lumps[i]), 8) == 0)
			{
				(*e_offset) = i;
				return file;
			}
		}
	}

	// not found
	return -1;
}

//
// Returns NULL for an empty list.
//
epi::u32array_c& W_GetListLumps(int file, lumplist_e which)
{
	SYS_ASSERT(0 <= file && file < (int)data_files.size());

	data_file_c *df = data_files[file];

	switch (which)
	{
	case LMPLST_Sprites: return df->sprite_lumps;
	case LMPLST_Flats:   return df->flat_lumps;
	case LMPLST_Patches: return df->patch_lumps;
	case LMPLST_LBM:     return df->lbm_lumps;
	case LMPLST_LPIC:    return df->rottpic_lumps;
	case LMPLST_RAW:     return df->rottraw_flats; //FLATS!

	default: break;
	}

	I_Error("W_GetListLumps: bad `which' (%d)\n", which);
	return df->sprite_lumps; /* NOT REACHED */
}

int W_GetNumFiles(void)
{
	return (int)data_files.size();
}

int W_GetFileForLump(int lump)
{
	SYS_ASSERT(lump >= 0 && lump < numlumps);

	return lumpinfo[lump].file;
}

//
// Loads the lump into the given buffer,
// which must be >= W_LumpLength().
//
static void W_ReadLump(int lump, void *dest)
{
	if (lump >= numlumps)
		I_Error("W_ReadLump: %i >= numlumps", lump);

	lumpinfo_t *L = &lumpinfo[lump];
#if (DEBUG_LUMPS)
	I_Debugf("W_ReadLump: %d (%s)\n", lump, L->name);
#endif

	data_file_c *df = data_files[L->file];

	// -KM- 1998/07/31 This puts the loading icon in the corner of the screen :-)
	display_disk = true;

	if ((df->kind == FLKIND_PAK) || (df->kind == FLKIND_PK3) || (df->kind == FLKIND_PK7) || (df->kind == FLKIND_EPK))
	{
#ifdef HAVE_PHYSFS
		// do PHSYFS read of data
		PHYSFS_File *handle = PHYSFS_openRead(L->path);
		if (handle)
		{
			PHYSFS_seek(handle, L->position);
			int c = PHYSFS_readBytes(handle, dest, L->size);
			if (c < 1)
				I_Error("W_ReadLump: PHYSFS_readBytes returned %i on lump %i", c, lump);
			PHYSFS_close(handle);
		}
		else
			I_Error("W_ReadLump: PHYSFS_openRead failed on lump %i", lump);
#endif
	}
	else
	{
		df->file->Seek(L->position, epi::file_c::SEEKPOINT_START);
		int c = df->file->Read(dest, L->size);
		if (c < L->size)
			I_Error("W_ReadLump: only read %i of %i on lump %i", c, L->size, lump);
	}
}

// FIXME !!! merge W_ReadLumpAlloc and W_LoadLumpNum into one good function
byte *W_ReadLumpAlloc(int lump, int *length)
{
	//I_Printf("ReadLumpAlloc: %s\n", W_GetLumpName(lump));
	*length = W_LumpLength(lump);

	byte *data = new byte[*length + 1];

	W_ReadLump(lump, data);

	data[*length] = 0;

	return data;
}

//
// W_DoneWithLump
//
void W_DoneWithLump(const void *ptr)
{
	lumpheader_t *h = ((lumpheader_t *)ptr); // Intentional Const Override

#ifdef DEVELOPERS
	if (h == NULL)
		I_Error("W_DoneWithLump: NULL pointer");
	if (h[-1].id != lumpheader_s::LUMPID)
		I_Error("W_DoneWithLump: id != LUMPID");
	if (h[-1].users == 0)
		I_Error("W_DoneWithLump: lump %d has no users!", h[-1].lumpindex);
#endif
	h--;
	h->users--;
	if (h->users == 0)
	{
		// Move the item to the tail.
		h->prev->next = h->next;
		h->next->prev = h->prev;
		h->prev = lumphead.prev;
		h->next = &lumphead;
		h->prev->next = h;
		h->next->prev = h;
		MarkAsCached(h);
	}
}

//
// W_DoneWithLump_Flushable
//
// Call this if the lump probably won't be used for a while, to hint the
// system to flush it early.
//
// Useful if you are creating a cache for e.g. some kind of lump
// conversions (like the sound cache).
//
void W_DoneWithLump_Flushable(const void *ptr)
{
	lumpheader_t *h = ((lumpheader_t *)ptr); // Intentional Const Override

#ifdef DEVELOPERS
	if (h == NULL)
		I_Error("W_DoneWithLump: NULL pointer");
	h--;
	if (h->id != lumpheader_s::LUMPID)
		I_Error("W_DoneWithLump: id != LUMPID");
	if (h->users == 0)
		I_Error("W_DoneWithLump: lump %d has no users!", h->lumpindex);
#endif
	h->users--;
	if (h->users == 0)
	{
		// Move the item to the head of the list.
		h->prev->next = h->next;
		h->next->prev = h->prev;
		h->next = lumphead.next;
		h->prev = &lumphead;
		h->prev->next = h;
		h->next->prev = h;
		MarkAsCached(h);
	}
}

//
// W_CacheLumpNum
//
const void *W_CacheLumpNum2(int lump)
{
	lumpheader_t *h;

#ifdef DEVELOPERS
	if ((unsigned int)lump >= (unsigned int)numlumps)
		I_Error("W_CacheLumpNum: %i >= numlumps", lump);
#endif

	h = lumplookup[lump];

	if (h)
	{
		// cache hit
		if (h->users == 0)
			cache_size -= W_LumpLength(h->lumpindex);
		h->users++;
	}
	else
	{
		// cache miss. load the new item.
		h = (lumpheader_t *)Z_Malloc(sizeof(lumpheader_t) + W_LumpLength(lump));
		lumplookup[lump] = h;
#ifdef DEVELOPERS
		h->id = lumpheader_s::LUMPID;
#endif
		h->lumpindex = lump;
		h->users = 1;
		h->prev = lumphead.prev;
		h->next = &lumphead;
		h->prev->next = h;
		lumphead.prev = h;

		W_ReadLump(lump, (void *)(h + 1));
	}

	return (void *)(h + 1);
}

//
// W_CacheLumpName
//
const void *W_CacheLumpName2(const char *name)
{
	return W_CacheLumpNum2(W_GetNumForName2(name));
}

//
// W_PreCacheLumpNum
//
// Attempts to load lump into the cache, if it isn't already there
//
void W_PreCacheLumpNum(int lump)
{
	W_DoneWithLump(W_CacheLumpNum(lump));
}

//
// W_PreCacheLumpName
//
void W_PreCacheLumpName(const char *name)
{
	W_DoneWithLump(W_CacheLumpName(name));
}

//
// W_CacheInfo
//
int W_CacheInfo(int level)
{
	lumpheader_t *h;
	int value = 0;

	for (h = lumphead.next; h != &lumphead; h = h->next)
	{
		if ((level & 1) && h->users)
			value += W_LumpLength(h->lumpindex);
		if ((level & 2) && !h->users)
			value += W_LumpLength(h->lumpindex);
	}
	return value;
}

//
// W_LoadLumpNum
//
// Returns a copy of the lump (it is your responsibility to free it)
//
void *W_LoadLumpNum(int lump)
{
	void *p;
	const void *cached;
	int length = W_LumpLength(lump);
	p = (void *)Z_Malloc(length);
	cached = W_CacheLumpNum2(lump);
	memcpy(p, cached, length);
	W_DoneWithLump(cached);
	return p;
}

//
// W_LoadLumpName
//
void *W_LoadLumpName(const char *name)
{
	return W_LoadLumpNum(W_GetNumForName2(name));
}

//
// W_GetLumpName
//
const char *W_GetLumpName(int lump)
{
	return lumpinfo[lump].name;
}

const char *W_GetLumpFullName(int lump)
{
	return lumpinfo[lump].fullname.c_str();  //.compare(0, strlen(lump), name)
}

void W_ProcessTX_HI(void)
{
	// Add the textures that occur in between TX_START/TX_END markers

	// TODO: collect names, remove duplicates

	for (int file = 0; file < (int)data_files.size(); file++)
	{
		data_file_c *df = data_files[file];

		for (int i = 0; i < (int)df->tx_lumps.GetSize(); i++)
		{
			int lump = df->tx_lumps[i];
			W_ImageAddTX(lump, W_GetLumpName(lump), false);
		}
	}

	// Add the textures that occur in between HI_START/HI_END markers

	for (int file = 0; file < (int)data_files.size(); file++)
	{
		data_file_c *df = data_files[file];

		for (int i = 0; i < (int)df->hires_lumps.GetSize(); i++)
		{
			int lump = df->hires_lumps[i];
			W_ImageAddTX(lump, W_GetLumpName(lump), true);
		}
	}
}

static const char *FileKind_Strings[] =
{
	"iwad", "pwad", "edge", "gwa", "hwa",
	"lump", "ddf",  "demo", "rts", "deh",
	"pak",  "wl6",  "rtl",  "fp", "vp", "pk3",
	"epk", "pk7"
};


static const char *LumpKind_Strings[] =
{
	"normal", "???", "???", //0-2
	"marker", "???", "???", //3-5
	"wadtex", "???", "???", "???", //6-9
	"ddf",    "???", "???", "???", //10-13

	"tx", "colmap", "flat", "sprite", "patch", //14-18
	"???", "???", "roq", "lbm", "lpic", "rawflats", "???" //24
};

void W_ShowLumps(int for_file, const char *match)
{
	I_Printf("Lump list:\n");

	int total = 0;

	for (int i = 0; i < numlumps; i++)
	{
		lumpinfo_t *L = &lumpinfo[i];

		if (for_file >= 1 && L->file != for_file - 1)
			continue;

		if (match && *match)
			if (!strstr(L->name, match))
				continue;

		I_Printf(" %4d %-9s %2d %-6s %7d @ 0x%08x\n",
			i + 1, L->name,
			L->file + 1, LumpKind_Strings[L->kind],
			L->size, L->position);
		total++;
	}

	I_Printf("Total: %d\n", total);
}

void W_ShowFiles(void)
{
	I_Printf("File list:\n");

	for (int i = 0; i < (int)data_files.size(); i++)
	{
		data_file_c *df = data_files[i];

		I_Printf(" %2d %-4s \"%s\"\n", i + 1, FileKind_Strings[df->kind], df->file_name);
	}
}

// TODO ~CA: Namespace it 
// Lobo
int W_LoboFindSkyImage(int for_file, const char* match)
{
	int total = 0;

	for (int i = 0; i < numlumps; i++)
	{
		lumpinfo_t* L = &lumpinfo[i];

		if (for_file >= 1 && L->file != for_file - 1)
			continue;

		if (match && *match)
			if (!strstr(L->name, match))
				continue;

		switch (L->kind)
		{
		case LMKIND_Patch:
			/*I_Printf(" %4d %-9s %2d %-6s %7d @ 0x%08x\n",
			 i+1, L->name,
			 L->file+1, LumpKind_Strings[L->kind],
			 L->size, L->position); */
			total++;
			break;
		case LMKIND_Normal:
			/*I_Printf(" %4d %-9s %2d %-6s %7d @ 0x%08x\n",
			 i+1, L->name,
			 L->file+1, LumpKind_Strings[L->kind],
			 L->size, L->position); */
			total++;
			break;
		default: //Optional
			continue;
		}
	}

	I_Debugf("FindSkyPatch: file %i,  match %s, count: %i\n", for_file, match, total);

	//I_Printf("Total: %d\n", total);
	return total;
}

// Lobo
bool W_LoboDisableSkybox(const char* ActualSky)
{
	bool TurnOffSkyBox = true;

	//Run through all our loaded files
	for (int m = 0; m < (int)data_files.size(); m++)
	{
		data_file_c* df = data_files[m];

		//we only want pwads
		if (FileKind_Strings[df->kind] == FileKind_Strings[FLKIND_PWad ||FLKIND_EPK ||FLKIND_PK3 ||FLKIND_PK7])
		{
			//I_Printf("Checking skies in %s\n",df->file_name);
			//I_Printf(" %2d %-4s \"%s\"\n", m+1, FileKind_Strings[df->kind], df->file_name);

			//first check if it has a sky box: this overrides normal skies
			int totalskies = W_LoboFindSkyImage(m + 1, ActualSky);
			if (totalskies != 0)
			{	//we have a skybox
				I_Debugf("%s has a skybox\n", df->file_name);
				TurnOffSkyBox = false;
			}
			else
			{
				//3. does it have a patch with "SKY" in the name?
				totalskies = W_LoboFindSkyImage(m + 1, "SKY");
				if (totalskies != 0)
				{	//assume it is a replacement sky
					I_Debugf("%s has %i sky patches\n", df->file_name, totalskies);
					TurnOffSkyBox = true;
				}
			}
		}
	}
	//no sky patches detected
	return TurnOffSkyBox;

}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
