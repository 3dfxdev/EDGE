//----------------------------------------------------------------------------
//  EDGE WAD Support Code
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

#include "dm_type.h"
#include "e_main.h"
#include "e_search.h"
#include "m_argv.h"
#include "m_misc.h"
#include "m_swap.h"
#include "e_player.h"
#include "rad_trig.h"
#include "w_textur.h"
#include "w_image.h"
#include "z_zone.h"


typedef enum 
{
	DATAFILE_TYPE_WAD, 
	DATAFILE_TYPE_SINGLELUMP
}
data_file_type_e;

typedef struct data_file_s
{
	const char *file_name;

	// type of file
	data_file_type_e type;

	// file handle, as returned from open().
	int handle;

	// sprite list
	// Note: we allocate 16 integers at a time
	int *sprite_lumps;
	int sprite_num;

	// flat list (also 16 at a time)
	int *flat_lumps;
	int flat_num;

	// texture information
	wadtex_resource_t wadtex;
}
data_file_t;

// Raw filenames
typedef struct raw_filename_s
{
	const char *file_name;
	bool allow_ddf;
}
raw_filename_t;

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
#define LUMPID ((int)0xAC45197e)
	// Should be LUMPID
	int id;
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

static data_file_t *data_files = NULL;
static int datafile = -1;

// the first datafile which contains a PLAYPAL lump
static int palette_datafile = -1;

// Sprites & Flats
bool within_sprite_list;
bool within_flat_list;

int addwadnum = 0;
static int maxwadfiles = 0;
static raw_filename_t *wadfiles = NULL;

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
// Is the name a dummy sprite/flat flag ?
//
static bool IsDummySF(const char *name)
{
	return (strncmp(name, "S1_START", 8) == 0 ||
			strncmp(name, "S2_START", 8) == 0 ||
			strncmp(name, "S3_START", 8) == 0 ||
			strncmp(name, "F1_START", 8) == 0 ||
			strncmp(name, "F2_START", 8) == 0 ||
			strncmp(name, "F3_START", 8) == 0);
}

static int FileLength(int handle)
{
	struct stat fileinfo;

	if (fstat(handle, &fileinfo) == -1)
		I_Error("Error fstating");

	return fileinfo.st_size;
}

static void ExtractFileBase(const char *path, char *dest)
{
	const char *src;
	int length;

	src = path + strlen(path) - 1;

	// back up until a \ or the start
	while (src != path
		   && *(src - 1) != '\\'
		   && *(src - 1) != '/'
		   && *(src - 1) != ':')  // Kester added :
	{
		src--;
	}

	// copy up to eight characters
	Z_Clear(dest, char, 8);
	length = 0;

	while (*src && *src != '.')
	{
		if (++length == 9)
			I_Error("Filename base of %s >8 chars", path);

		*dest++ = toupper((int)*src++);
	}
}

//
// W_GetTextureLumps
//
void W_GetTextureLumps(int file, wadtex_resource_t *res)
{
	DEV_ASSERT2(0 <= file && file <= datafile);
	DEV_ASSERT2(res);

	res->palette  = data_files[file].wadtex.palette;
	res->pnames   = data_files[file].wadtex.pnames;
	res->texture1 = data_files[file].wadtex.texture1;
	res->texture2 = data_files[file].wadtex.texture2;

	// find an earlier PNAMES lump when missing.
	// Ditto for palette.

	if (res->texture1 >= 0 || res->texture2 >= 0)
	{
		int cur;

		for (cur=file; res->pnames == -1 && cur > 0; cur--)
			res->pnames = data_files[cur].wadtex.pnames;

		for (cur=file; res->palette == -1 && cur > 0; cur--)
			res->palette = data_files[cur].wadtex.palette;
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
    
	// sort it, primarily by increasing name, secondarily by decreasing
	// file number.

#define CMP(a, b)  \
    (strncmp(lumpinfo[a].name, lumpinfo[b].name, 8) < 0 ||    \
     (strncmp(lumpinfo[a].name, lumpinfo[b].name, 8) == 0 &&  \
      lumpinfo[a].sort_index > lumpinfo[b].sort_index))
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
static void SortSpriteLumps(data_file_t *f)
{
	if (f->sprite_num < 2)
		return;

#define CMP(a, b) (strncmp(lumpinfo[a].name, lumpinfo[b].name, 8) < 0)
	QSORT(int, f->sprite_lumps, f->sprite_num, CUTOFF);
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

// -KM- 1999/01/31 Order is important, Languages are loaded before sfx, etc...
typedef struct ddf_reader_s
{
	char *name;
	void (* func)(void *data, int size);
	int lump;
}
ddf_reader_t;

static ddf_reader_t DDF_Readers[] =
{
	{ "DDFLANG", DDF_ReadLangs, 0 } ,
	{ "DDFSFX",  DDF_ReadSFX, 0 } ,
	{ "DDFCOLM", DDF_ReadColourMaps, 0 } ,  // -AJA- 1999/07/09.
	{ "DDFATK",  DDF_ReadAtks, 0 } ,
	{ "DDFWEAP", DDF_ReadWeapons, 0 } ,
	{ "DDFTHING",DDF_ReadThings, 0 } ,
	{ "DDFPLAY", DDF_ReadMusicPlaylist, 0 } ,
	{ "DDFLINE", DDF_ReadLines, 0 } ,
	{ "DDFSECT", DDF_ReadSectors, 0 } ,
	{ "DDFSWTH", DDF_ReadSW, 0 } ,
	{ "DDFANIM", DDF_ReadAnims, 0 } ,
	{ "DDFGAME", DDF_ReadGames, 0 } ,
	{ "DDFLEVL", DDF_ReadLevels, 0 },
	{ "RSCRIPT", RAD_LoadLump, 0 }       // -AJA- 2000/04/21.
};

#define NUM_DDF_READERS  (int)(sizeof(DDF_Readers) / sizeof(ddf_reader_t))

//
// FreeLump
//
static void FreeLump(lumpheader_t *h)
{
	int lumpnum = h->lumpindex;

	cache_size -= W_LumpLength(lumpnum);
#ifdef DEVELOPERS
	if (h->id != LUMPID)
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
	if (item->id != LUMPID)
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

static INLINE void AddSpriteOrFlat(int **list, int *count, int item)
{
	// no more room ?
	if (((*count) % 16) == 0)
	{
		Z_Resize((*list), int, (*count) + 16);
	}
  
	(*list)[*count] = item;

	(*count) += 1;
}

//
// AddLump
//
static void AddLump(int lump, int pos, int size, int file, 
					int sort_index, const char *name, bool allow_ddf)
{
	int j;
	lumpinfo_t *lump_p = lumpinfo + lump;
  
	lump_p->position = pos;
	lump_p->size = size;
	lump_p->file = file;
	lump_p->sort_index = sort_index;

	Z_StrNCpy(lump_p->name, name, 8);
	strupr(lump_p->name);

	// -- handle special names --

	if (!strncmp(name, "PLAYPAL", 8))
	{
		data_files[file].wadtex.palette = lump;
		if (palette_datafile < 0)
			palette_datafile = file;
		return;
	}
	else if (!strncmp(name, "PNAMES", 8))
	{
		data_files[file].wadtex.pnames = lump;
		return;
	}
	else if (!strncmp(name, "TEXTURE1", 8))
	{
		data_files[file].wadtex.texture1 = lump;
		return;
	}
	else if (!strncmp(name, "TEXTURE2", 8))
	{
		data_files[file].wadtex.texture2 = lump;
		return;
	}

	// -KM- 1998/12/16 Load DDF/RSCRIPT file from wad.
	if (allow_ddf)
	{
		for (j=0; j < NUM_DDF_READERS; j++)
		{
			if (!strncmp(name, DDF_Readers[j].name, 8))
			{
				DDF_Readers[j].lump = lump;
				return;
			}
		}
	}

	// -- handle sprite & flat lists --
  
	if (IsS_START(lump_p->name))
	{
		within_sprite_list = true;
		return;
	}
	else if (IsS_END(lump_p->name))
	{
		if (!within_sprite_list)
			I_Warning("Unexpected S_END marker in wad.\n");

		within_sprite_list = false;
		return;
	}
	else if (IsF_START(lump_p->name))
	{
		within_flat_list = true;
		return;
	}
	else if (IsF_END(lump_p->name))
	{
		if (!within_flat_list)
			I_Warning("Unexpected F_END marker in wad.\n");

		within_flat_list = false;
		return;
	}

	// ignore zero size lumps or dummy markers
	if (lump_p->size > 0 && !IsDummySF(lump_p->name))
	{
		if (within_sprite_list)
			AddSpriteOrFlat(&data_files[file].sprite_lumps,
							&data_files[file].sprite_num, lump);
    
		if (within_flat_list)
			AddSpriteOrFlat(&data_files[file].flat_lumps,
							&data_files[file].flat_num, lump);
	}
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
static void AddFile(const char *filename, bool allow_ddf,
					int dyn_index)
{
	int j;
	int handle;
	int length;
	int startlump;

	wad_header_t header;
	wad_entry_t *fileinfo, *curinfo;

	// reset the sprite/flat list stuff
	within_sprite_list = within_flat_list = false;

	// open the file and add to directory
	for (j=0; j < NUM_DDF_READERS; j++)
		DDF_Readers[j].lump = -1;

	if ((handle = open(filename, O_RDONLY | O_BINARY)) == -1)
	{
		I_Printf(" couldn't open %s\n", filename);
		return;
	}

	I_Printf("  adding %s", filename);

	startlump = numlumps;
	datafile++;
	Z_Resize(data_files, data_file_t, datafile + 1);

	data_files[datafile].file_name = Z_StrDup(filename);
	data_files[datafile].handle = handle;
	data_files[datafile].sprite_lumps = NULL;
	data_files[datafile].sprite_num = 0;
	data_files[datafile].flat_lumps = NULL;
	data_files[datafile].flat_num = 0;
	data_files[datafile].wadtex.palette = -1;
	data_files[datafile].wadtex.pnames = -1;
	data_files[datafile].wadtex.texture1 = -1;
	data_files[datafile].wadtex.texture2 = -1;

	if (M_CheckExtension("wad", filename) == EXT_MATCHING ||
		M_CheckExtension("gwa", filename) == EXT_MATCHING ||
		M_CheckExtension("hwa", filename) == EXT_MATCHING)
	{
		// WAD file
		data_files[datafile].type = DATAFILE_TYPE_WAD;
		read(handle, &header, sizeof(wad_header_t));

		if (strncmp(header.identification, "IWAD", 4))
		{
			// Homebrew levels?
			if (strncmp(header.identification, "PWAD", 4))
			{
				I_Error("Wad file %s doesn't have IWAD or PWAD id\n", filename);
			}
		}

		header.numlumps = LONG(header.numlumps);
		header.infotableofs = LONG(header.infotableofs);

		length = header.numlumps * sizeof(wad_entry_t);
		fileinfo = Z_New(wad_entry_t, header.numlumps);

		lseek(handle, header.infotableofs, SEEK_SET);
		read(handle, fileinfo, length);

		// Fill in lumpinfo
		numlumps += header.numlumps;
		Z_Resize(lumpinfo, lumpinfo_t, numlumps);

		for (j=startlump,curinfo=fileinfo; j < numlumps; j++,curinfo++)
		{
			AddLump(j, LONG(curinfo->pos), LONG(curinfo->size),
					datafile, (dyn_index >= 0) ? dyn_index : datafile, 
					curinfo->name, allow_ddf);
		}

		Z_Free(fileinfo);
	}
	else
	{
		char lump_name[32];

		DEV_ASSERT2(dyn_index < 0);

		// single lump file
		data_files[datafile].type = DATAFILE_TYPE_SINGLELUMP;

		ExtractFileBase(filename, lump_name);

		// Fill in lumpinfo
		numlumps++;
		Z_Resize(lumpinfo, lumpinfo_t, numlumps);

		AddLump(startlump, 0, FileLength(handle), datafile, datafile, 
				lump_name, allow_ddf);
	}

	SortLumps();
	SortSpriteLumps(data_files + datafile);

	// set up caching
	Z_Resize(lumplookup, lumpheader_t *, numlumps);
	for (j=startlump; j < numlumps; j++)
		lumplookup[j] = NULL;

	// -KM- 1999/01/31 Load lumps in correct order.
	for (j=0; j < NUM_DDF_READERS; j++)
	{
		if (DDF_Readers[j].lump >= 0)
		{
			char *data;

			DEV_ASSERT2(dyn_index < 0);

			data = Z_New(char, W_LumpLength(DDF_Readers[j].lump) + 1);
			W_ReadLump(DDF_Readers[j].lump, data);
			data[W_LumpLength(DDF_Readers[j].lump)] = 0;

			// call read function
			(* DDF_Readers[j].func)(data, W_LumpLength(DDF_Readers[j].lump));

			Z_Free(data);
		}
	}

	I_Printf("\n");

	// check for unclosed sprite/flat lists
	if (within_sprite_list)
		I_Warning("Missing S_END marker in %s.\n", filename);

	if (within_flat_list)
		I_Warning("Missing F_END marker in %s.\n", filename);
   
	// -AJA- 1999/12/25: What did Santa bring EDGE ?  Just some support
	//       for "GWA" files (part of the "GL-Friendly Nodes" specs).
  
	if (M_CheckExtension("wad", filename) == EXT_MATCHING)
	{
		char namebuf[1024];

		int len = strlen(filename);

		DEV_ASSERT2(dyn_index < 0);

		// replace WAD extension with GWA
		strcpy(namebuf, filename);

		Z_StrNCpy(namebuf + len - 3, EDGEGWAEXT, 3);

		if (I_Access(namebuf))
		{
			i_time_t gwa_time;
			i_time_t wad_time;

			// OK, the GWA file exists. so check the time and date 
			// against the WAD file to see if we should include this
			// file or force the recreation of the GWA.

			if (!I_GetModifiedTime(filename, &wad_time))
				I_Error("AddFile: I_GetModifiedTime failed on %s\n",filename);

			if (!I_GetModifiedTime(namebuf, &gwa_time))
				I_Error("AddFile: I_GetModifiedTime failed on %s\n",namebuf);

			// Load it.  This recursion bit is
			// rather sneaky, hopefully it doesn't break anything...
			if (L_CompareTimeStamps(&gwa_time, &wad_time) >= 0)
				AddFile(namebuf, false, datafile);
		}
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
void W_AddRawFilename(const char *file, bool allow_ddf)
{
	raw_filename_t *r;

	L_WriteDebug("Added: %s\n", file);

	if (addwadnum == maxwadfiles)
		Z_Resize(wadfiles, raw_filename_t, ++maxwadfiles + 1);
  
	r = &wadfiles[addwadnum++];

	r->file_name = Z_StrDup(file);
	r->allow_ddf = allow_ddf;
}

//
// W_InitMultipleFiles
//
// Pass a null terminated list of files to use.
// All files are optional, but at least one file must be found.
// Files with a .wad extension are idlink files with multiple lumps.
// Other files are single lumps with the base filename for the lump name.
// Lump names can appear multiple times.
// The name searcher looks backwards, so a later file
//   does override all earlier ones.
//
bool W_InitMultipleFiles(void)
{
	int r;

	InitCaches();

	// open all the files, load headers, and count lumps
	numlumps = 0;

	// will be realloced as lumps are added
	lumpinfo = NULL;

	for (r=0; r < addwadnum; r++)
		AddFile(wadfiles[r].file_name, wadfiles[r].allow_ddf, -1);

	if (!numlumps)
	{
		I_Error("W_InitMultipleFiles: no files found");
		return false;
	}

	return true;
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
	data_file_t *f;

	DEV_ASSERT2(0 <= lump && lump < numlumps);

	l = lumpinfo + lump;
	f = data_files + l->file;

	if (f->type != DATAFILE_TYPE_WAD)
		return NULL;
  
	return f->file_name;
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
	lumpinfo_t *l;
	data_file_t *f;

	DEV_ASSERT2(0 <= lump && lump < numlumps);

	l = lumpinfo + lump;
	f = data_files + l->file;

	for (; f > (data_files + palette_datafile); f--)
	{
		if (f->type != DATAFILE_TYPE_WAD)
			continue;

		if (f->wadtex.palette >= 0)
			return f->wadtex.palette;
	}

	// none found
	return -1;
}

//
// W_AddDynamicGWA
//
// This is only used to dynamically add a GWA file after being built
// by the GLBSP plugin.
// 
void W_AddDynamicGWA(const char *filename, int map_lump)
{
	DEV_ASSERT2(0 <= map_lump && map_lump < numlumps);

	AddFile(filename, false, lumpinfo[map_lump].file);
}

//
// W_CheckNumForName
//
// Returns -1 if name not found.
//
// -ACB- 1998/08/09 Removed ifdef 0 stuff.
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
	int i, file;
	data_file_t *f;
  
	for (file=datafile; file >= 0; file--)
	{
		f = data_files + file;

		// look for start name
		for (i=0; i < f->flat_num; i++)
		{
			if (strncmp(start, W_GetLumpName(f->flat_lumps[i]), 8) == 0)
				break;
		}

		if (i >= f->flat_num)
			continue;

		(*s_offset) = i;

		// look for end name
		for (i++; i < f->flat_num; i++)
		{
			if (strncmp(end, W_GetLumpName(f->flat_lumps[i]), 8) == 0)
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
const int *W_GetListLumps(int file, lumplist_e which, int *count)
{
	DEV_ASSERT2(0 <= file && file <= datafile);

	switch (which)
	{
		case LMPLST_Sprites:
			(*count) = data_files[file].sprite_num;
			return data_files[file].sprite_lumps;
    
		case LMPLST_Flats:
			(*count) = data_files[file].flat_num;
			return data_files[file].flat_lumps;
	}

#ifdef DEVELOPERS
	I_Error("W_GetListLumps: bad `which' (%d)\n", which);
#endif
	return NULL;
}

//
// W_GetNumFiles
//
int W_GetNumFiles(void)
{
	return (datafile + 1);
}

//
// W_LumpRawInfo
//
// Retrieves the raw information about the lump: file handle, position
// within file, and size in bytes.  Returns true if successful, or
// false if (for some reason) the lump cannot be accessed externally.
// 
// Note: this call designed to allow MP3 lumps to be accessed by the
//       L_MP3 code -- it shouldn't otherwise be used.
//
bool W_LumpRawInfo(int lump, int *handle, int *pos, int *size)
{
	lumpinfo_t *l;
	data_file_t *f;

	if (lump >= numlumps)
		I_Error("W_LumpRawInfo: %i >= numlumps", lump);

	l = lumpinfo + lump;
	f = data_files + l->file;

	(*handle) = f->handle;
	(*pos)    = l->position;
	(*size)   = l->size;
  
	return true;
}

//
// W_ReadLump
//
// Loads the lump into the given buffer,
// which must be >= W_LumpLength().
//
void W_ReadLump(int lump, void *dest)
{
	int c;
	lumpinfo_t *l;
	data_file_t *f;

	if (lump >= numlumps)
		I_Error("W_ReadLump: %i >= numlumps", lump);

	l = lumpinfo + lump;
	f = data_files + l->file;

	// -KM- 1998/07/31 This puts the loading icon in the corner of the screen :-)
	display_disk = true;

	lseek(f->handle, l->position, SEEK_SET);

	c = read(f->handle, dest, l->size);

	if (c < l->size)
		I_Error("W_ReadLump: only read %i of %i on lump %i", c, l->size, lump);
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
	if (h[-1].id != LUMPID)
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
	if (h->id != LUMPID)
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
		h->id = LUMPID;
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



















