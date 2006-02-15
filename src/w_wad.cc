//----------------------------------------------------------------------------
//  EDGE WAD Support Code
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2005  The EDGE Team.
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
#include "w_wad.h"

#include "ddf_main.h"
#include "ddf_anim.h"
#include "ddf_colm.h"
#include "ddf_font.h"
#include "ddf_image.h"
#include "ddf_style.h"
#include "ddf_swth.h"

#include "dstrings.h"
#include "e_main.h"
#include "e_search.h"
#include "l_deh.h"
#include "l_glbsp.h"
#include "m_misc.h"
#include "rad_trig.h"
#include "z_zone.h"

#include <epi/endianess.h>
#include <epi/files.h>
#include <epi/filesystem.h>
#include <epi/path.h>
#include <epi/strings.h>
#include <epi/utility.h>

#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

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
#define RTS_READER  (NUM_DDF_READERS-1)

class data_file_c
{
public:
	data_file_c(const char *_fname, int _kind, epi::file_c* _file) :
		file_name(_fname), kind(_kind), file(_file), compat_mode(0),
		sprite_lumps(), flat_lumps(), patch_lumps(),
		colmap_lumps(), level_markers(), skin_markers(),
		wadtex(), deh_lump(-1), companion_gwa(-1)
	{
		for (int d = 0; d < NUM_DDF_READERS; d++)
			ddf_lumps[d] = -1;
	}

	const char *file_name;

	// type of file (FLKIND_XXX)
	int kind;

	// file object
    epi::file_c *file;

	// any engine-specific features (WAD_CM_XXX)
	int compat_mode;

	// lists for sprites, flats, patches (stuff between markers)
	epi::u32array_c sprite_lumps;
	epi::u32array_c flat_lumps;
	epi::u32array_c patch_lumps;
	epi::u32array_c colmap_lumps;

	// level markers and skin markers
	epi::u32array_c level_markers;
	epi::u32array_c skin_markers;

	// ddf lump list
	int ddf_lumps[NUM_DDF_READERS];

	// texture information
	wadtex_resource_c wadtex;

	// DeHackEd support
	int deh_lump;

	// file containing the GL nodes for the levels in this WAD.
	// -1 when none (usually when this WAD has no levels, but also
	// temporarily before a new GWA files has been built and added).
	int companion_gwa;
};

class datafile_array_c : public epi::array_c
{
public:
	datafile_array_c() : epi::array_c(sizeof(data_file_c*)) { }
	~datafile_array_c() { Clear(); }

private:
	void CleanupObject(void *obj) { delete *(data_file_c**)obj; }

public:
    // List Management
	int GetSize() { return array_entries; }
	int Insert(data_file_c *c) { return InsertObject((void*)&c); }
	data_file_c* operator[](int idx) { return *(data_file_c**)FetchObject(idx); }
};

// Raw filenames
typedef struct raw_filename_s
{
	epi::strent_c file_name;
	int kind;
}
raw_filename_t;

class raw_filename_container_c : public epi::array_c
{
public:
	raw_filename_container_c() : epi::array_c(sizeof(raw_filename_t*)) { }
	~raw_filename_container_c() { Clear(); }

private:
	void CleanupObject(void *obj)  { delete *(raw_filename_t**)obj; }

public:
    // List Management
	int GetSize() { return array_entries; }

	int Insert(const char *file_name, int kind) 
    { 
        if (file_name == NULL)
            return -1;

        raw_filename_t *rf = new raw_filename_t;
        rf->file_name.Set(file_name);
        rf->kind = kind;

        return InsertObject((void*)&rf); 
    }

	raw_filename_t* operator[](int idx) 
    { 
        return (raw_filename_t*)FetchObject(idx); 
    }
};


typedef enum
{
	LMKIND_Normal = 0,  // fallback value
	LMKIND_Marker = 3,  // X_START, X_END, S_SKIN, level name
	LMKIND_WadTex = 6,  // palette, pnames, texture1/2
	LMKIND_DDFRTS = 10, // DDF, RTS, DEHACKED lump
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
lumpinfo_t *lumpinfo;
static int *lumpmap = NULL;
int numlumps;

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

static datafile_array_c data_files;

// the first datafile which contains a PLAYPAL lump
static int palette_datafile = -1;

// Sprites & Flats
bool within_sprite_list;
bool within_flat_list;
bool within_patch_list;
bool within_colmap_list;

raw_filename_container_c wadfiles;

static void W_ReadLump(int lump, void *dest);

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

static INLINE bool IsGL_Prefix(const char *name)
{
	return name[0] == 'G' && name[1] == 'L' && name[2] == '_';
}

//
// W_GetTextureLumps
//
void W_GetTextureLumps(int file, wadtex_resource_c *res)
{
	DEV_ASSERT2(0 <= file && file < data_files.GetSize());
	DEV_ASSERT2(res);

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

	for (i=1; i < numlumps; i++)
	{
		int a = lumpmap[i - 1];
		int b = lumpmap[i];

		if (strncmp(lumpinfo[a].name, lumpinfo[b].name, 8) == 0)
			lumpmap[i] = lumpmap[i - 1];
	}
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

#if 0  // DEBUGGING
	{
		int i, lump;
    
		for (i=0; i < f->sprite_num; i++)
		{
			lump = f->sprite_lumps[i];

			L_WriteDebug("Sorted sprite %d = lump %d [%s]\n", i, lump,
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

//
// FreeLump
//
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
// AddLump
//
static void AddLump(data_file_c *df, int lump, int pos, int size, int file, 
					int sort_index, const char *name, bool allow_ddf)
{
	int j;
	lumpinfo_t *lump_p = lumpinfo + lump;
  
	lump_p->position = pos;
	lump_p->size = size;
	lump_p->file = file;
	lump_p->sort_index = sort_index;
	lump_p->kind = LMKIND_Normal;

	Z_StrNCpy(lump_p->name, name, 8);
	strupr(lump_p->name);
 
	// -- handle special names --

	if (!strncmp(name, "PLAYPAL", 8))
	{
		lump_p->kind = LMKIND_WadTex;
		df->wadtex.palette = lump;
		if (palette_datafile < 0)
			palette_datafile = file;
		return;
	}
	else if (!strncmp(name, "PNAMES", 8))
	{
		lump_p->kind = LMKIND_WadTex;
		df->wadtex.pnames = lump;
		return;
	}
	else if (!strncmp(name, "TEXTURE1", 8))
	{
		lump_p->kind = LMKIND_WadTex;
		df->wadtex.texture1 = lump;
		return;
	}
	else if (!strncmp(name, "TEXTURE2", 8))
	{
		lump_p->kind = LMKIND_WadTex;
		df->wadtex.texture2 = lump;
		return;
	}
	else if (!strncmp(name, "DEHACKED", 8))
	{
		lump_p->kind = LMKIND_DDFRTS;
		df->deh_lump = lump;
		return;
	}
	
	if (strncmp(name, "SWITCHES", 8) == 0 ||
		strncmp(name, "ANIMATED", 8) == 0 ||
	    strncmp(name, "TRANMAP", 8) == 0)
	{
		df->compat_mode |= WAD_CM_Boom;
	}

	// -KM- 1998/12/16 Load DDF/RSCRIPT file from wad.
	if (allow_ddf)
	{
		for (j=0; j < NUM_DDF_READERS; j++)
		{
			if (!strncmp(name, DDF_Readers[j].name, 8))
			{
				df->compat_mode |= WAD_CM_Edge;

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
	const wad_entry_t *raw, int remaining)
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
// HasInternalGLNodes
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

	L_WriteDebug("Levels %d, Internal GL nodes %d\n", levels, glnodes);

	return levels == glnodes;
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

	wad_header_t header;
	wad_entry_t *curinfo;

	// reset the sprite/flat/patch list stuff
	within_sprite_list = within_flat_list = false;
	within_patch_list  = within_colmap_list = false;

	// open the file and add to directory
    epi::file_c *file = epi::the_filesystem->Open(filename, epi::file_c::ACCESS_READ | epi::file_c::ACCESS_BINARY);
	if (file == NULL)
	{
		I_Error("Couldn't open file %s\n", filename);
		return;
	}

	I_Printf("  Adding %s\n", filename);

	startlump = numlumps;

	int datafile = data_files.GetSize();

	data_file_c *df = new data_file_c(Z_StrDup(filename), kind, file);
	data_files.Insert(df);

	// for RTS scripts, adding the data_file is enough
	if (kind == FLKIND_Script)
		return;

	if (kind <= FLKIND_HWad)
	{
		// WAD file
        file->Read(&header, sizeof(wad_header_t));

		if (strncmp(header.identification, "IWAD", 4))
		{
			// Homebrew levels?
			if (strncmp(header.identification, "PWAD", 4))
			{
				I_Error("Wad file %s doesn't have IWAD or PWAD id\n", filename);
			}
		}

		header.numlumps = EPI_LE_S32(header.numlumps);
		header.infotableofs = EPI_LE_S32(header.infotableofs);

		length = header.numlumps * sizeof(wad_entry_t);

        wad_entry_t *fileinfo = new wad_entry_t[header.numlumps];

        file->Seek(header.infotableofs, epi::file_c::SEEKPOINT_START);
        file->Read(fileinfo, length);

		// Fill in lumpinfo
		numlumps += header.numlumps;
		Z_Resize(lumpinfo, lumpinfo_t, numlumps);

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

		delete [] fileinfo;
	}
	else  /* single lump file */
	{
		char lump_name[32];

		DEV_ASSERT2(dyn_index < 0);

		if (kind == FLKIND_DDF)
        {
			DDF_GetLumpNameForFile(filename, lump_name);
        }
        else
        {
            epi::string_c s = epi::path::GetBasename(filename);
            if (s.GetLength() > 8)
                I_Error("Filename base of %s >8 chars", filename);

            s.ToUpper();    // Required to be uppercase

            strcpy(lump_name, s);
        }

		// Fill in lumpinfo
		numlumps++;
		Z_Resize(lumpinfo, lumpinfo_t, numlumps);

		AddLump(df, startlump, 0, 
                file->GetLength(), datafile, datafile, 
				lump_name, true);
	}

	L_WriteDebug("Compat mode: %s %s\n",
		(df->compat_mode & WAD_CM_Edge) ? "EDGE" : "-",
		(df->compat_mode & WAD_CM_Boom) ? "BOOM" : "-");

	SortLumps();
	SortSpriteLumps(df);

	// set up caching
	Z_Resize(lumplookup, lumpheader_t *, numlumps);

	for (j=startlump; j < numlumps; j++)
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
			DEV_ASSERT2(dyn_index < 0);

            epi::string_c cached_gwa_filename;
            epi::string_c gwa_filename;
            epi::string_c wad_dir;
            
            // Get the directory which the wad is currently stored
            wad_dir = epi::path::GetDir(filename);
            
            // Initially work out the name of the GWA file
            gwa_filename = epi::path::GetBasename(filename);
            gwa_filename.AddString("." EDGEGWAEXT);
            
            // Determine the full path filename for the cached and current directory versions
            cached_gwa_filename = epi::path::Join(cache_dir.GetString(), gwa_filename.GetString());
            gwa_filename = epi::path::Join(wad_dir.GetString(), gwa_filename.GetString());

            // Check for the existance of the waddir and cached dir files
            bool cached_gwa = epi::the_filesystem->Access(cached_gwa_filename.GetString(), epi::file_c::ACCESS_READ);
            bool waddir_gwa = epi::the_filesystem->Access(gwa_filename.GetString(), epi::file_c::ACCESS_READ);

            // If both exist, use one in root. if neither exist create
            // one in the cache directory
 
            // Check whether the waddir gwa is out of date
            if (waddir_gwa) 
                waddir_gwa = (L_CompareFileTimes(filename, gwa_filename.GetString()) <= 0);

            // Check whether the cached gwa is out of date
            if (cached_gwa) 
                cached_gwa = (L_CompareFileTimes(filename, cached_gwa_filename.GetString()) <= 0);

            const char *actual_gwa_filename = NULL;
            
            if (!waddir_gwa && !cached_gwa)
            {
                // Neither is valid so create one in the cached directory
                actual_gwa_filename = cached_gwa_filename.GetString();

				I_Printf("Building GL Nodes for: %s\n", filename);

				if (! GB_BuildNodes(filename, actual_gwa_filename))
					I_Error("Failed to build GL nodes for: %s\n", filename);
            }
            else if (waddir_gwa)
            {
                // The wad directory GWA is valid - use it
                actual_gwa_filename = gwa_filename.GetString();
            }
            else if (cached_gwa)
            {
                // The cached directory GWA is valid - use it
                actual_gwa_filename = cached_gwa_filename.GetString();
            }

			// Load it.  This recursion bit is rather sneaky,
			// hopefully it doesn't break anything...
			AddFile(actual_gwa_filename, FLKIND_GWad, datafile);

			df->companion_gwa = datafile + 1;
		}
	}

	// handle DeHackEd patch files
	if (kind == FLKIND_Deh || df->deh_lump >= 0)
	{
        epi::string_c cached_hwa_filename;
        epi::string_c hwa_filename;
        epi::string_c wad_dir;
            
        // Get the directory which the wad is currently stored
        wad_dir = epi::path::GetDir(filename);
            
        // Initially work out the name of the HWA file
        hwa_filename = epi::path::GetBasename(filename);
        hwa_filename.AddString("." EDGEHWAEXT);
            
        // Determine the full path filename for the cached and current directory versions
        cached_hwa_filename = epi::path::Join(cache_dir.GetString(), hwa_filename.GetString());
        hwa_filename = epi::path::Join(wad_dir.GetString(), hwa_filename.GetString());

        // Check for the existance of the waddir and cached dir files
        bool cached_hwa = epi::the_filesystem->Access(cached_hwa_filename.GetString(), epi::file_c::ACCESS_READ);
        bool waddir_hwa = epi::the_filesystem->Access(hwa_filename.GetString(), epi::file_c::ACCESS_READ);

        // If both exist, use one in root. if neither exist create
        // one in the cache directory
 
        // Check whether the waddir hwa is out of date
        if (waddir_hwa) 
            waddir_hwa = (L_CompareFileTimes(filename, hwa_filename.GetString()) <= 0);

        // Check whether the cached hwa is out of date
        if (cached_hwa) 
            cached_hwa = (L_CompareFileTimes(filename, cached_hwa_filename.GetString()) <= 0);

        const char *actual_hwa_filename = NULL;
        
        if (!waddir_hwa && !cached_hwa)
        {
            // Neither is valid so create one in the cached directory
			if (kind == FLKIND_Deh)
			{
                actual_hwa_filename = cached_hwa_filename.GetString();
				
                I_Printf("Converting DEH file: %s\n", filename);

				if (! DH_ConvertFile(filename, hwa_filename.GetString()))
					I_Error("Failed to convert DeHackEd patch: %s\n", filename);
			}
			else
			{
				const char *lump_name = lumpinfo[df->deh_lump].name;

				I_Printf("Converting [%s] lump in: %s\n", lump_name, filename);

				const byte *data = (const byte *)W_CacheLumpNum(df->deh_lump);
				int length = W_LumpLength(df->deh_lump);

				if (! DH_ConvertLump(data, length, lump_name, hwa_filename.GetString()))
					I_Error("Failed to convert DeHackEd LUMP in: %s\n", filename);

				W_DoneWithLump(data);
			}
        }
        else if (waddir_hwa)
        {
            // The wad directory GWA is valid - use it
            actual_hwa_filename = hwa_filename.GetString();
        }
        else if (cached_hwa)
        {
            // The cached directory GWA is valid - use it
            actual_hwa_filename = cached_hwa_filename.GetString();
        }

		// Load it (using good ol' recursion again).
		AddFile(actual_hwa_filename, FLKIND_HWad, -1);
	}
}

//
// FlushLumpCache
//
// Flushes parts of the cache. High urgency means more is flushed.
static void FlushLumpCache(z_urgency_e urgency)
{
	int bytes_to_free = 0;
	lumpheader_t *h;

	switch (urgency)
	{
		case Z_UrgencyLow: bytes_to_free = cache_size / 16; break;
		case Z_UrgencyMedium: bytes_to_free = cache_size / 8; break;
		case Z_UrgencyHigh: bytes_to_free = cache_size / 2; break;
		case Z_UrgencyExtreme: bytes_to_free = INT_MAX; break;
		default: I_Error("FlushLumpCache: Invalid urgency level %d", urgency);
	}

	for (h = &lumphead; h->next != &lumphead && bytes_to_free > 0;)
	{
		if (h->next->users == 0)
		{
			bytes_to_free -= W_LumpLength(h->next->lumpindex);
			FreeLump(h->next);
		}
		else
			h = h->next;
	}
}

static void InitCaches(void)
{
	lumphead.next = lumphead.prev = &lumphead;
	Z_RegisterCacheFlusher(FlushLumpCache);
}

//
// W_AddRawFilename
//
void W_AddRawFilename(const char *file, int kind)
{
	L_WriteDebug("Added filename: %s\n", file);

    wadfiles.Insert(file, kind);
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
	lumpinfo = NULL;

	for (epi::array_iterator_c it = wadfiles.GetBaseIterator(); it.IsValid(); it++)
    {
        raw_filename_t *r = ITERATOR_TO_TYPE(it, raw_filename_t*);
		AddFile(r->file_name.GetString(), r->kind, -1);
    }

	if (!numlumps)
		I_Error("W_InitMultipleFiles: no files found");
}

bool TryLoadExtraLanguage(const char *name)
{
	int lumpnum = W_CheckNumForName(name);

	if (lumpnum < 0)
		return false;

	int length = W_LumpLength(lumpnum);
	char *data = new char[length + 1];

	W_ReadLump(lumpnum, data);
	data[length] = 0;

	DDF_ReadLangs(data, length);

	delete[] data;

	return true;
}

// MUNDO HACK, but if only fixable by a new wad structure...
void LoadTntPlutStrings(void)
{
	if (strcmp(iwad_base.GetString(), "TNT") == 0)
		TryLoadExtraLanguage("TNTLANG");

	if (strcmp(iwad_base.GetString(), "PLUTONIA") == 0)
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
			DDF_SetBoomConflict(true);

			// call read function
			ext_loaded = (* DDF_Readers[d].func)(NULL, 0);
		}

		for (int f = 0; f < data_files.GetSize(); f++)
		{
			data_file_c *df = data_files[f];

			// all script files get parsed here
			if (d == RTS_READER && df->kind == FLKIND_Script)
			{
				RAD_LoadFile(df->file_name);
				continue;
			}

			if (df->kind >= FLKIND_Demo)
				continue;

			if (ext_loaded && df->kind == FLKIND_EWad)
				continue;

			int lump = df->ddf_lumps[d];

			if (lump < 0)
				continue;

			char *data;
			int length = W_LumpLength(lump);

			data = new char[length + 1];
			W_ReadLump(lump, data);
			data[length] = 0;

			DDF_SetBoomConflict(df->kind == FLKIND_EWad);

			// call read function
			(* DDF_Readers[d].func)(data, length);

			delete [] data;

			// special handling for TNT and Plutonia
			if (d == LANG_READER && df->kind == FLKIND_EWad)
				LoadTntPlutStrings();
		}

		DDF_SetBoomConflict(false);

		epi::string_c msg_buf;

		msg_buf.Format("Loaded %s %s\n", (d == NUM_DDF_READERS-1) ? "RTS" : "DDF",
			DDF_Readers[d].print_name);

		E_ProgressMessage(msg_buf.GetString());

		E_LocalProgress(d, NUM_DDF_READERS);
	}
}

//
// W_GetFileName
//
// Returns the filename of the WAD file containing the given lump, or
// NULL if it wasn't a WAD file (e.g. a pure lump).
//
const char *W_GetFileName(int lump)
{
	lumpinfo_t *l;
	data_file_c *df;

	DEV_ASSERT2(0 <= lump && lump < numlumps);

	l = lumpinfo + lump;
	df = data_files[l->file];

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
	DEV_ASSERT2(0 <= lump && lump < numlumps);

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
		!strncmp(lumpinfo[lumpmap[i]].name, buf, 8))
	{
		return lumpmap[i];
	}

	return -1;
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
		lumpinfo_t *L = lumpinfo + lumpmap[i];

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
	for (int file = data_files.GetSize()-1; file >= 0; file--)
	{
		int i;
		data_file_c *df = data_files[file];

		// look for start name
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
// W_GetListLumps
//
// Returns NULL for an empty list.
//
epi::u32array_c& W_GetListLumps(int file, lumplist_e which)
{
	DEV_ASSERT2(0 <= file && file < data_files.GetSize());

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

//
// W_GetNumFiles
//
int W_GetNumFiles(void)
{
	return data_files.GetSize();
}

//
// W_GetFileCompatMode
//
int W_GetFileCompatMode(int file)
{
	return data_files[file]->compat_mode;
}

//
// W_GetFileForLump
//
int W_GetFileForLump(int lump)
{
	DEV_ASSERT2(lump >= 0 && lump < numlumps);

	return lumpinfo[lump].file;
}

//
// W_ReadLump
//
// Loads the lump into the given buffer,
// which must be >= W_LumpLength().
//
static void W_ReadLump(int lump, void *dest)
{
	if (lump >= numlumps)
		I_Error("W_ReadLump: %i >= numlumps", lump);

	lumpinfo_t *L = lumpinfo + lump;
	data_file_c *df = data_files[L->file];

	// -KM- 1998/07/31 This puts the loading icon in the corner of the screen :-)
	display_disk = true;

    df->file->Seek(L->position, epi::file_c::SEEKPOINT_START);

    int c = df->file->Read(dest, L->size);

	if (c < L->size)
		I_Error("W_ReadLump: only read %i of %i on lump %i", c, L->size, lump);
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
const void *W_CacheLumpNum2 (int lump)
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
		h = (lumpheader_t *) Z_Malloc(sizeof(lumpheader_t) + W_LumpLength(lump));
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
	p = (void *) Z_Malloc(length);
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
