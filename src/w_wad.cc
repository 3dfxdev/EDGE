//----------------------------------------------------------------------------
//  EDGE WAD Support Code
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2008  The EDGE Team.
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

#include "i_defs.h"

#include <limits.h>

#include <list>

#include "epi/endianess.h"
#include "epi/file.h"
#include "epi/file_sub.h"
#include "epi/filesystem.h"
#include "epi/math_md5.h"
#include "epi/path.h"
#include "epi/str_format.h"
#include "epi/utility.h"

#include "ddf/anim.h"
#include "ddf/attack.h"
#include "ddf/colormap.h"
#include "ddf/font.h"
#include "ddf/game.h"
#include "ddf/level.h"
#include "ddf/image.h"
#include "ddf/language.h"
#include "ddf/playlist.h"
#include "ddf/sfx.h"
#include "ddf/style.h"
#include "ddf/switch.h"
#include "ddf/thing.h"
#include "ddf/weapon.h"

#include "dm_data.h"
#include "dm_defs.h"
#include "dm_state.h"
#include "dm_structs.h"
#include "dstrings.h"
#include "e_main.h"
#include "e_search.h"
#include "l_deh.h"
#include "l_glbsp.h"
#include "m_misc.h"
#include "rad_trig.h"
#include "w_wad.h"
#include "z_zone.h"

// -KM- 1999/01/31 Order is important, Languages are loaded before sfx, etc...
typedef struct ddf_reader_s
{
	const char *name;
	const char *print_name;
	bool (* func)(void *data, int size);
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
	{ "RSCRIPT", "Scripts",    RAD_ReadScript }       // -AJA- 2000/04/21.
};

#define NUM_DDF_READERS  (int)(sizeof(DDF_Readers) / sizeof(ddf_reader_t))

#define LANG_READER  0
#define COLM_READER  2
#define SWTH_READER  12
#define ANIM_READER  13
#define RTS_READER   16

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

	// level markers and skin markers
	epi::u32array_c level_markers;
	epi::u32array_c skin_markers;

	// ddf lump list
	int ddf_lumps[NUM_DDF_READERS];

	// texture information
	wadtex_resource_c wadtex;

	// DeHackEd support
	int deh_lump;

	// BOOM stuff
	int animated, switches;

	// file containing the GL nodes for the levels in this WAD.
	// -1 when none (usually when this WAD has no levels, but also
	// temporarily before a new GWA files has been built and added).
	int companion_gwa;

	// MD5 hash of the contents of the WAD directory.
	// This is used to disambiguate cached GWA/HWA filenames.
	epi::md5hash_c dir_hash;

	// if this WAD contains a palette, then this will hold a
	// copy of it (as soon as something requests it).
	byte *palette_data;

public:
	data_file_c(const char *_fname, int _kind, epi::file_c* _file) :
		file_name(_fname), kind(_kind), file(_file),
		sprite_lumps(), flat_lumps(), patch_lumps(),
		colmap_lumps(), tx_lumps(),
		level_markers(), skin_markers(),
		wadtex(), deh_lump(-1), animated(-1), switches(-1),
		companion_gwa(-1), dir_hash(),
		palette_data(NULL)
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


typedef enum
{
	LMKIND_Normal = 0,  // fallback value
	LMKIND_Marker = 3,  // X_START, X_END, S_SKIN, level name
	LMKIND_WadTex = 6,  // palette, pnames, texture1/2
	LMKIND_DDFRTS = 10, // DDF, RTS, DEHACKED lump
	LMKIND_TX     = 14,
	LMKIND_Colmap = 15,
	LMKIND_Flat   = 16,
	LMKIND_Sprite = 17,
	LMKIND_Patch  = 18
}
lump_kind_e;

typedef struct
{
	char name[10];

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


// the first datafile which contains a PLAYPAL lump
static int palette_datafile = -1;

// Sprites & Flats
bool within_sprite_list;
bool within_flat_list;
bool within_patch_list;
bool within_colmap_list;
bool within_tex_list;

static byte *W_ReadLumpAlloc(int lump, int *length);


//
// Is the name a sprite list start flag?
// If lax syntax match, fix up to standard syntax.
//
static bool IsS_START(char *name)
{
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
	if (strncmp(name, "FF_START", 8) == 0)
	{
		// fix up flag to standard syntax
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
	if (strncmp(name, "FF_END", 8) == 0)
	{
		// fix up flag to standard syntax
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
	if (strncmp(name, "PP_END", 8) == 0)
	{
		// fix up flag to standard syntax
		strncpy(name, "P_END", 8);
		return 1;
	}

	return (strncmp(name, "P_END", 8) == 0);
}

//
// Is the name a colourmap list start/end flag?
//
static bool IsC_START(char *name)
{
	return (strncmp(name, "C_START", 8) == 0);
}

static bool IsC_END(char *name)
{
	return (strncmp(name, "C_END", 8) == 0);
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
	return (strncmp(name, "TX_END", 8) == 0);
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
			strncmp(name, "P3_START", 8) == 0);
}

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

void W_GetTextureLumps(int file, wadtex_resource_c *res)
{
	SYS_ASSERT(0 <= file && file < (int)data_files.size());
	SYS_ASSERT(res);

	data_file_c *df = data_files[file];

	res->palette  = df->wadtex.palette;
	res->pnames   = df->wadtex.pnames;
	res->texture1 = df->wadtex.texture1;
	res->texture2 = df->wadtex.texture2;

	// find an earlier PNAMES lump when missing.
	// Ditto for palette.

	if (res->texture1 >= 0 || res->texture2 >= 0)
	{
		int cur;

		for (cur=file; res->pnames == -1 && cur > 0; cur--)
			res->pnames = data_files[cur]->wadtex.pnames;

		for (cur=file; res->palette == -1 && cur > 0; cur--)
			res->palette = data_files[cur]->wadtex.palette;
	}
}

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
	if (lumpmap)
		delete[] lumpmap;

	lumpmap = new int[numlumps+1];

	int i;

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

	for (i=1; i < numlumps; i++)
	{
		int a = lumpmap[i - 1];
		int b = lumpmap[i];

		if (strncmp(lumpinfo[a].name, lumpinfo[b].name, 8) == 0)
			lumpmap[i] = lumpmap[i - 1];
	}
}

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

#if 0  // DEBUGGING
	{
		int i, lump;
    
		for (i=0; i < f->sprite_num; i++)
		{
			lump = f->sprite_lumps[i];

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


static void AddLump(data_file_c *df, int lump, int pos, int size, int file, 
					int sort_index, const char *name, bool allow_ddf)
{
	int j;
	lumpinfo_t *lump_p = &lumpinfo[lump];
  
	lump_p->position = pos;
	lump_p->size = size;
	lump_p->file = file;
	lump_p->sort_index = sort_index;
	lump_p->kind = LMKIND_Normal;

	Z_StrNCpy(lump_p->name, name, 8);
	strupr(lump_p->name);
 
	// -- handle special names --

	if (strncmp(name, "PLAYPAL", 8) == 0)
	{
		lump_p->kind = LMKIND_WadTex;
		df->wadtex.palette = lump;
		if (palette_datafile < 0)
			palette_datafile = file;
		return;
	}
	else if (strncmp(name, "PNAMES", 8) == 0)
	{
		lump_p->kind = LMKIND_WadTex;
		df->wadtex.pnames = lump;
		return;
	}
	else if (strncmp(name, "TEXTURE1", 8) == 0)
	{
		lump_p->kind = LMKIND_WadTex;
		df->wadtex.texture1 = lump;
		return;
	}
	else if (strncmp(name, "TEXTURE2", 8) == 0)
	{
		lump_p->kind = LMKIND_WadTex;
		df->wadtex.texture2 = lump;
		return;
	}
	else if (strncmp(name, "DEHACKED", 8) == 0)
	{
		lump_p->kind = LMKIND_DDFRTS;
		df->deh_lump = lump;
		return;
	}
	else if (strncmp(name, "ANIMATED", 8) == 0)
	{
		lump_p->kind = LMKIND_DDFRTS;
		df->animated = lump;
		return;
	}
	else if (strncmp(name, "SWITCHES", 8) == 0)
	{
		lump_p->kind = LMKIND_DDFRTS;
		df->switches = lump;
		return;
	}

	// -KM- 1998/12/16 Load DDF/RSCRIPT file from wad.
	if (allow_ddf)
	{
		for (j=0; j < NUM_DDF_READERS; j++)
		{
			if (strncmp(name, DDF_Readers[j].name, 8) == 0)
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
	else if (IsP_END(lump_p->name))
	{
		if (!within_patch_list)
			I_Warning("Unexpected P_END marker in wad.\n");

		lump_p->kind = LMKIND_Marker;
		within_patch_list = false;
		return;
	}
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
	}
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

	if (strncmp(raw[1].name, "THINGS",   8) == 0 &&
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

	if (strncmp(raw[1].name, "GL_VERT",  8) == 0 &&
	    strncmp(raw[2].name, "GL_SEGS",  8) == 0 &&
	    strncmp(raw[3].name, "GL_SSECT", 8) == 0 &&
	    strncmp(raw[4].name, "GL_NODES", 8) == 0)
	{
		df->level_markers.Insert(lump);
		return;
	}
}

//
// Determine if WAD file contains GL nodes for every level.
//
static bool HasInternalGLNodes(data_file_c *df, int datafile)
{
	int levels  = 0;
	int glnodes = 0;
	
	for (int L = 0; L < df->level_markers.GetSize(); L++)
	{
		int lump = df->level_markers[L];

		if (IsGL_Prefix(lumpinfo[lump].name))
			continue;

		levels++;

		char gl_marker[10];

		sprintf(gl_marker, "GL_%s", lumpinfo[lump].name);

		for (int M = L+1; M < df->level_markers.GetSize(); M++)
		{
			int lump2 = df->level_markers[M];

			if (strcmp(lumpinfo[lump2].name, gl_marker) == 0)
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

static bool FindCacheFilename (std::string& out_name,
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
	if (has_local) 
		has_local = (L_CompareFileTimes(filename, local_name.c_str()) <= 0);

	// Check whether the cached gwa is out of date
	if (has_cache) 
		has_cache = (L_CompareFileTimes(filename, cache_name.c_str()) <= 0);

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

	// reset the sprite/flat/patch list stuff
	within_sprite_list = within_flat_list   = false;
	within_patch_list  = within_colmap_list = false;
	within_tex_list    = false;

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
	if (kind == FLKIND_Script)
		return;

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

		for (j=startlump, curinfo=fileinfo; j < numlumps; j++,curinfo++)
		{
			AddLump(df, j, EPI_LE_S32(curinfo->pos), EPI_LE_S32(curinfo->size),
					datafile, 
                    (dyn_index >= 0) ? dyn_index : datafile, 
					curinfo->name,
					(kind == FLKIND_PWad) || (kind == FLKIND_HWad) ||
					(kind == FLKIND_EWad && ! external_ddf));

			if (kind != FLKIND_HWad)
				CheckForLevel(df, j, lumpinfo[j].name, curinfo, numlumps-1 - j);
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
			strupr(lump_name);    // Required to be uppercase
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

			if (! exists)
			{
				I_Printf("Building GL Nodes for: %s\n", filename);

				if (! GB_BuildNodes(filename, gwa_filename.c_str()))
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

		bool exists = FindCacheFilename(hwa_filename, filename, df, EDGEHWAEXT);

		I_Debugf("Actual_HWA_filename: %s\n", hwa_filename.c_str());

		if (! exists)
		{
			if (kind == FLKIND_Deh)
			{
                I_Printf("Converting DEH file: %s\n", filename);

				if (! DH_ConvertFile(filename, hwa_filename.c_str()))
					I_Error("Failed to convert DeHackEd patch: %s\n", filename);
			}
			else
			{
				const char *lump_name = lumpinfo[df->deh_lump].name;

				I_Printf("Converting [%s] lump in: %s\n", lump_name, filename);

				const byte *data = (const byte *)W_LoadLumpNum(df->deh_lump);
				int length = W_LumpLength(df->deh_lump);

				if (! DH_ConvertLump(data, length, lump_name, hwa_filename.c_str()))
					I_Error("Failed to convert DeHackEd LUMP in: %s\n", filename);

				Z_Free((void*)data);
			}
        }

		// Load it (using good ol' recursion again).
		AddFile(hwa_filename.c_str(), FLKIND_HWad, -1);
	}
}


void W_AddRawFilename(const char *file, int kind)
{
	I_Debugf("Added filename: %s\n", file);

    wadfiles.push_back(new raw_filename_c(file, kind));
}

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
		I_Error("W_InitMultipleFiles: no files found!\n");
}

static bool TryLoadExtraLanguage(const char *name)
{
	int lumpnum = W_CheckNumForName(name);

	if (lumpnum < 0)
		return false;

	int length;
	char *data = (char *) W_ReadLumpAlloc(lumpnum, &length);

	DDF_ReadLangs(data, length);
	delete[] data;

	return true;
}

// MUNDO HACK, but if only fixable by a new wad structure...
static void LoadTntPlutStrings(void)
{
	if (strcmp(iwad_base.c_str(), "TNT") == 0)
		TryLoadExtraLanguage("TNTLANG");

	if (strcmp(iwad_base.c_str(), "PLUTONIA") == 0)
		TryLoadExtraLanguage("PLUTLANG");
}


void W_ReadDDF(void)
{
	// -AJA- the order here may look strange.  Since DDF files
	// have dependencies between them, it makes more sense to
	// load all lumps of a certain type together (e.g. all
	// DDFSFX lumps before all the DDFTHING lumps).

	for (int d = 0; d < NUM_DDF_READERS; d++)
	{
		// when external file doesn't exist, use one in EDGE.WAD.
		bool ext_loaded = false;

		if (external_ddf)
		{
			I_Debugf("- Loading external %s\n", DDF_Readers[d].name);

			// call read function
			ext_loaded = (* DDF_Readers[d].func)(NULL, 0);
		}

		for (int f = 0; f < (int)data_files.size(); f++)
		{
			data_file_c *df = data_files[f];

			// all script files get parsed here
			if (d == RTS_READER && df->kind == FLKIND_Script)
			{
				I_Debugf("- Loading RTS script: %s\n", df->file_name);

				RAD_LoadFile(df->file_name);
				continue;
			}

			if (df->kind >= FLKIND_Demo)
				continue;

			if (ext_loaded && df->kind == FLKIND_EWad)
				continue;

			int lump = df->ddf_lumps[d];

			if (lump >= 0)
			{
				I_Debugf("- Loading %s from: %s\n", DDF_Readers[d].name, df->file_name);

				int length;
				char *data = (char *) W_ReadLumpAlloc(lump, &length);

				// call read function
				(* DDF_Readers[d].func)(data, length);
				delete[] data;

				// special handling for TNT and Plutonia
				if (d == LANG_READER && df->kind == FLKIND_EWad)
					LoadTntPlutStrings();
			}

			// handle Boom's ANIMATED and SWITCHES lumps
			if (d == ANIM_READER && df->animated >= 0)
			{
				I_Debugf("- Loading ANIMATED from: %s\n", df->file_name);

				int length;
				byte *data = W_ReadLumpAlloc(df->animated, &length);

				DDF_ParseANIMATED(data, length);
				delete[] data;
			}
			if (d == SWTH_READER && df->switches >= 0)
			{
				I_Debugf("- Loading SWITCHES from: %s\n", df->file_name);

				int length;
				byte *data = W_ReadLumpAlloc(df->switches, &length);

				DDF_ParseSWITCHES(data, length);
				delete[] data;
			}

			// handle BOOM Colourmaps (between C_START and C_END)
			if (d == COLM_READER && df->colmap_lumps.GetSize() > 0)
			{
				for (int i=0; i < df->colmap_lumps.GetSize(); i++)
				{
					int lump = df->colmap_lumps[i];

					DDF_ColourmapAddRaw(W_GetLumpName(lump), W_LumpLength(lump));
				}
			}
		}

		std::string msg_buf(epi::STR_Format(
			"Loaded %s %s\n", (d == NUM_DDF_READERS-1) ? "RTS" : "DDF",
				DDF_Readers[d].print_name));

		E_ProgressMessage(msg_buf.c_str());

		E_LocalProgress(d, NUM_DDF_READERS);
	}
}

epi::file_c *W_OpenLump(int lump)
{
	SYS_ASSERT(0 <= lump && lump < numlumps);

	lumpinfo_t *l = &lumpinfo[lump];

	data_file_c *df = data_files[l->file];

	SYS_ASSERT(df->file);

	return new epi::sub_file_c(df->file, l->position, l->size);
}

epi::file_c *W_OpenLump(const char *name)
{
	return W_OpenLump(W_GetNumForName(name));
}

//
// Returns the filename of the WAD file containing the given lump, or
// NULL if it wasn't a WAD file (e.g. a pure lump).
//
const char *W_GetFileName(int lump)
{
	SYS_ASSERT(0 <= lump && lump < numlumps);

	lumpinfo_t *l = &lumpinfo[lump];

	data_file_c *df = data_files[l->file];

	if (df->kind >= FLKIND_Lump)
		return NULL;
  
	return df->file_name;
}

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
const byte * W_GetPaletteData(int f)
{
	SYS_ASSERT(0 <= f && f < (int)data_files.size());

	if (palette_datafile < 0)
		I_Error("No palettes??\n");

	if (f < palette_datafile)
		f = palette_datafile;

	for (; f >= palette_datafile; f--)
	{
		data_file_c *df = data_files[f];

		if (df->kind >= FLKIND_Lump)
			continue;

		if (df->wadtex.palette >= 0)
		{
			if (! df->palette_data)
				df->palette_data = (byte *) W_LoadLumpNum(df->wadtex.palette);

			return df->palette_data;
		}
	}

	I_Error("W_GetPaletteData INTERNAL ERROR.\n");
	return NULL; /* NOT REACHED */
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
#ifdef DEVELOPERS
		if (i > 8)
			I_Error("W_CheckNumForName: Name '%s' longer than 8 chars!", name);
#endif
		buf[i] = toupper(name[i]);
	}
	buf[i] = 0;

#define CMP(a) (strncmp(lumpinfo[lumpmap[a]].name, buf, 8) < 0)
	BSEARCH(numlumps, i);
#undef CMP

	if (i >= 0 && i < numlumps &&
		strncmp(lumpinfo[lumpmap[i]].name, buf, 8) == 0)
	{
		return lumpmap[i];
	}

	return -1;
}

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

#if 0  // OLD (VERY SLOW) METHOD
	for (int file = data_files.GetSize()-1; file >= 0; file--)
	{
		data_file_c *df = data_files[file];

		// look for patch name
		for (i=0; i < df->patch_lumps.GetSize(); i++)
		{
			if (strncmp(buf, W_GetLumpName(df->patch_lumps[i]), 8) == 0)
				break;
		}

		if (i < df->patch_lumps.GetSize())
			return df->patch_lumps[i];

		// look for sprite name
		for (i=0; i < df->sprite_lumps.GetSize(); i++)
		{
			if (strncmp(buf, W_GetLumpName(df->sprite_lumps[i]), 8) == 0)
				break;
		}

		if (i < df->sprite_lumps.GetSize())
			return df->sprite_lumps[i];

		// check all other lumps
		for (i=0; i < numlumps; i++)
		{
			lumpinfo_t *l = lumpinfo + i;

			if (l->file != file)
				continue;

			if (strncmp(buf, W_GetLumpName(i), 8) == 0)
				break;
		}

		if (i < numlumps)
			return i;
	}
#endif

#define CMP(a) (strncmp(lumpinfo[lumpmap[a]].name, buf, 8) < 0)
	BSEARCH(numlumps, i);
#undef CMP

#define STR_CMP(a) (strncmp(lumpinfo[lumpmap[a]].name, buf, 8))

	if (i < 0 || i >= numlumps || STR_CMP(i) != 0)
	{
		// not found (nothing has that name)
		return -1;
	}

	// jump to last matching name
	while (i+1 < numlumps && STR_CMP(i+1) == 0)
		i++;
	
	for (; i >= 0 && STR_CMP(i) == 0; i--)
	{
		lumpinfo_t *L = &lumpinfo[lumpmap[i]];

		if (L->kind == LMKIND_Patch || L->kind == LMKIND_Sprite ||
			L->kind == LMKIND_Normal)
		{
			// allow LMKIND_Normal to support patches outside of the
			// P_START/END markers.  We especially want to disallow
			// flat and colourmap lumps.
			return lumpmap[i];
		}
	}

	return -1;

#undef STR_CMP
}

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
// Returns the buffer size needed to load the given lump.
//
int W_LumpLength(int lump)
{
	if (lump >= numlumps)
		I_Error("W_LumpLength: %i >= numlumps", lump);

	return lumpinfo[lump].size;
}

//
// Returns the file number containing the sequence, or -1 if not
// found.  Search is from newest wad file to oldest wad file.
//
int W_FindFlatSequence(const char *start, const char *end, 
					   int *s_offset, int *e_offset)
{
	for (int file = (int)data_files.size()-1; file >= 0; file--)
	{
		data_file_c *df = data_files[file];

		// look for start name
		int i;
		for (i=0; i < df->flat_lumps.GetSize(); i++)
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
	data_file_c *df = data_files[L->file];

	// -KM- 1998/07/31 This puts the loading icon in the corner of the screen :-)
	display_disk = true;

    df->file->Seek(L->position, epi::file_c::SEEKPOINT_START);

    int c = df->file->Read(dest, L->size);

	if (c < L->size)
		I_Error("W_ReadLump: only read %i of %i on lump %i", c, L->size, lump);
}

static byte *W_ReadLumpAlloc(int lump, int *length)
{
	*length = W_LumpLength(lump);

	byte *data = new byte[*length + 1];

	W_ReadLump(lump, data);

	data[*length] = 0;

	return data;
}


int W_CacheInfo(int level)
{
	return 0;
}

//
// Returns a copy of the lump (it is your responsibility to free it)
//
void *W_LoadLumpNum(int lump, int *length)
{
	int len = W_LumpLength(lump);

	if (length) *length = len;

	void *p = (void *) Z_Malloc(len);

	W_ReadLump(lump, p);

	return p;
}

void *W_LoadLumpName(const char *name, int *length)
{
	return W_LoadLumpNum(W_GetNumForName2(name), length);
}

const char *W_GetLumpName(int lump)
{
	return lumpinfo[lump].name;
}


static const char *FileKind_Strings[] =
{
	"iwad", "pwad", "edge", "gwa", "hwa",
	"lump", "ddf",  "demo", "scr", "deh",
	"???",  "???",  "???",  "???"
};

static const char *LumpKind_Strings[] =
{
	"normal", "???", "???",
	"marker", "???", "???",
	"wadtex", "???", "???", "???",
	"ddf",    "???", "???", "???",

	"tx", "colmap", "flat", "sprite", "patch",
	"???", "???", "???", "???"
};


void W_ShowLumps(int for_file, const char *match)
{
	I_Printf("Lump list:\n");

	int total = 0;

	for (int i = 0; i < numlumps; i++)
	{
		lumpinfo_t *L = &lumpinfo[i];

		if (for_file >= 1 && L->file != for_file-1)
			continue;

		if (match && *match)
			if (! strstr(L->name, match))
				continue;

		I_Printf(" %4d %-9s %2d %-6s %7d @ 0x%08x\n", 
		         i+1, L->name,
				 L->file+1, LumpKind_Strings[L->kind],
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

		I_Printf(" %2d %-4s %s\n", i+1, FileKind_Strings[df->kind], df->file_name);
	}
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
