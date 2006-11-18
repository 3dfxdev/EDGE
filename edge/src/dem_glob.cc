//----------------------------------------------------------------------------
//  EDGE New Demo Handling (Global info)
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
// See the file "docs/demo_fmt.txt" for a description of the new
// demo system.
//
// FIXME: merge common code!! (demo and savegame)
//

#include "i_defs.h"

#include "dem_glob.h"
#include "dem_chunk.h"
#include "z_zone.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// forward decls:
static void DG_GetInt(const char *info, void *storage);
static void DG_GetString(const char *info, void *storage);
static void DG_GetCheckCRC(const char *info, void *storage);
static void DG_GetLevelFlags(const char *info, void *storage);

static const char *DG_PutInt(void *storage);
static const char *DG_PutString(void *storage);
static const char *DG_PutCheckCRC(void *storage);
static const char *DG_PutLevelFlags(void *storage);


static saveglobals_t dummy_glob;
static saveglobals_t *cur_globs = NULL;

typedef struct
{
	// global name
	const char *name;

	// parse function.  `storage' is where the data should go (for
	// routines that don't modify the cur_globs structure directly).
	void (* parse_func)(const char *info, void *storage);

	// stringify function.  Return string must be freed.
	const char * (* stringify_func)(void *storage);

	// field offset (given as a pointer within dummy struct)
	const char *offset_p;
}
demo_global_t;


#define GLOB_OFF(field)  ((const char *) &dummy_glob.field)

static const demo_global_t demo_globals[] =
{
	{ "GAME",  DG_GetString, DG_PutString, GLOB_OFF(game) },
	{ "LEVEL", DG_GetString, DG_PutString, GLOB_OFF(level) },
	{ "FLAGS", DG_GetLevelFlags, DG_PutLevelFlags, GLOB_OFF(flags) },
	{ "GRAVITY", DG_GetInt, DG_PutInt, GLOB_OFF(gravity) },

	{ "SKILL", DG_GetInt, DG_PutInt, GLOB_OFF(skill) },
	{ "NETGAME", DG_GetInt, DG_PutInt, GLOB_OFF(netgame) },
	{ "P_RANDOM", DG_GetInt, DG_PutInt, GLOB_OFF(p_random) },
	{ "CONSOLE_PLAYER", DG_GetInt, DG_PutInt, GLOB_OFF(console_player) },

///---	{ "TOTAL_KILLS",   DG_GetInt, DG_GetInt, GLOB_OFF(total_kills) },
///---	{ "TOTAL_ITEMS",   DG_GetInt, DG_GetInt, GLOB_OFF(total_items) },
///---	{ "TOTAL_SECRETS", DG_GetInt, DG_GetInt, GLOB_OFF(total_secrets) },

///---	{ "DESCRIPTION", DG_GetString, DG_PutString, GLOB_OFF(description) },

	{ "DESC_DATE", DG_GetString, DG_PutString, GLOB_OFF(desc_date) },

	{ "MAPSECTOR", DG_GetCheckCRC, DG_PutCheckCRC, GLOB_OFF(mapsector) },
	{ "MAPLINE",   DG_GetCheckCRC, DG_PutCheckCRC, GLOB_OFF(mapline) },
	{ "MAPTHING",  DG_GetCheckCRC, DG_PutCheckCRC, GLOB_OFF(mapthing) },

	{ "RSCRIPT", DG_GetCheckCRC, DG_PutCheckCRC, GLOB_OFF(rscript) },
	{ "DDFATK",  DG_GetCheckCRC, DG_PutCheckCRC, GLOB_OFF(ddfatk) },
	{ "DDFGAME", DG_GetCheckCRC, DG_PutCheckCRC, GLOB_OFF(ddfgame) },
	{ "DDFLEVL", DG_GetCheckCRC, DG_PutCheckCRC, GLOB_OFF(ddflevl) },
	{ "DDFLINE", DG_GetCheckCRC, DG_PutCheckCRC, GLOB_OFF(ddfline) },
	{ "DDFSECT", DG_GetCheckCRC, DG_PutCheckCRC, GLOB_OFF(ddfsect) },
	{ "DDFMOBJ", DG_GetCheckCRC, DG_PutCheckCRC, GLOB_OFF(ddfmobj) },
	{ "DDFWEAP", DG_GetCheckCRC, DG_PutCheckCRC, GLOB_OFF(ddfweap) },

	{ NULL, NULL, 0 }
};


//----------------------------------------------------------------------------
//
//  PARSERS
//

static void DG_GetInt(const char *info, void *storage)
{
	int *dest = (int *)storage;

	DEV_ASSERT2(info && storage);

	*dest = strtol(info, NULL, 0);
}

static void DG_GetString(const char *info, void *storage)
{
	char **dest = (char **)storage;

	DEV_ASSERT2(info && storage);

	// free any previous string
	if (*dest)
		Z_Free(*dest);

	if (info[0] == 0)
		*dest = NULL;
	else
		*dest = Z_StrDup(info);
}

static void DG_GetCheckCRC(const char *info, void *storage)
{
	crc_check_t *dest = (crc_check_t *)storage;

	DEV_ASSERT2(info && storage);

	sscanf(info, "%d %u", &dest->count, &dest->crc);
}

static void DG_GetLevelFlags(const char *info, void *storage)
{
	gameflags_t *dest = (gameflags_t *)storage;
	int flags;

	DEV_ASSERT2(info && storage);

	flags = strtol(info, NULL, 0);

	Z_Clear(dest, gameflags_t, 1);

#define HANDLE_FLAG(var, specflag)  \
	(var) = (flags & (specflag)) ? true : false;

	HANDLE_FLAG(dest->jump, MPF_Jumping);
	HANDLE_FLAG(dest->crouch, MPF_Crouching);
	HANDLE_FLAG(dest->mlook, MPF_Mlook);
	HANDLE_FLAG(dest->itemrespawn, MPF_ItemRespawn);
	HANDLE_FLAG(dest->fastparm, MPF_FastParm);
	HANDLE_FLAG(dest->true3dgameplay, MPF_True3D);
	HANDLE_FLAG(dest->more_blood, MPF_MoreBlood);
	HANDLE_FLAG(dest->cheats, MPF_Cheats);
	HANDLE_FLAG(dest->respawn, MPF_Respawn);
	HANDLE_FLAG(dest->res_respawn, MPF_ResRespawn);
	HANDLE_FLAG(dest->have_extra, MPF_Extras);
	HANDLE_FLAG(dest->limit_zoom, MPF_LimitZoom);
	HANDLE_FLAG(dest->shadows, MPF_Shadows);
	HANDLE_FLAG(dest->halos, MPF_Halos);
	HANDLE_FLAG(dest->kicking, MPF_Kicking);
	HANDLE_FLAG(dest->weapon_switch, MPF_WeaponSwitch);
	HANDLE_FLAG(dest->nomonsters, MPF_NoMonsters);

#undef HANDLE_FLAG

	dest->edge_compat = (flags & MPF_BoomCompat) ? false : true;

	dest->autoaim = (flags & MPF_AutoAim) ? 
		((flags & MPF_AutoAimMlook) ? AA_MLOOK : AA_ON) : AA_OFF;
}


//----------------------------------------------------------------------------
//
//  STRINGIFIERS
//

static const char *DG_PutInt(void *storage)
{
	int *src = (int *)storage;
	char buffer[40];

	DEV_ASSERT2(storage);

	sprintf(buffer, "%d", *src);

	return Z_StrDup(buffer);
}

static const char *DG_PutString(void *storage)
{
	char **src = (char **)storage;

	DEV_ASSERT2(storage);

	if (*src == NULL)
		return (const char *) Z_ClearNew(char, 1);

	return Z_StrDup(*src);
}

static const char *DG_PutCheckCRC(void *storage)
{
	crc_check_t *src = (crc_check_t *)storage;
	char buffer[80];

	DEV_ASSERT2(storage);

	sprintf(buffer, "%d %u", src->count, src->crc);

	return Z_StrDup(buffer);
}

static const char *DG_PutLevelFlags(void *storage)
{
	gameflags_t *src = (gameflags_t *)storage;
	int flags;

	DEV_ASSERT2(storage);

	flags = 0;

#define HANDLE_FLAG(var, specflag)  \
	if (var) flags |= (specflag);

	HANDLE_FLAG(src->jump, MPF_Jumping);
	HANDLE_FLAG(src->crouch, MPF_Crouching);
	HANDLE_FLAG(src->mlook, MPF_Mlook);
	HANDLE_FLAG(src->itemrespawn, MPF_ItemRespawn);
	HANDLE_FLAG(src->fastparm, MPF_FastParm);
	HANDLE_FLAG(src->true3dgameplay, MPF_True3D);
	HANDLE_FLAG(src->more_blood, MPF_MoreBlood);
	HANDLE_FLAG(src->cheats, MPF_Cheats);
	HANDLE_FLAG(src->respawn, MPF_Respawn);
	HANDLE_FLAG(src->res_respawn, MPF_ResRespawn);
	HANDLE_FLAG(src->have_extra, MPF_Extras);
	HANDLE_FLAG(src->limit_zoom, MPF_LimitZoom);
	HANDLE_FLAG(src->shadows, MPF_Shadows);
	HANDLE_FLAG(src->halos, MPF_Halos);
	HANDLE_FLAG(src->kicking, MPF_Kicking);
	HANDLE_FLAG(src->weapon_switch, MPF_WeaponSwitch);
	HANDLE_FLAG(src->nomonsters, MPF_NoMonsters);

#undef HANDLE_FLAG

	if (!src->edge_compat)
		flags |= MPF_BoomCompat;

	if (src->autoaim != AA_OFF)
		flags |= MPF_AutoAim;
	if (src->autoaim == AA_MLOOK)
		flags |= MPF_AutoAimMlook;

	return DG_PutInt(&flags);
}


//----------------------------------------------------------------------------
//
//  MISCELLANY
//

saveglobals_t *DEM_NewGLOB(void)
{
	saveglobals_t *globs;

	globs = Z_ClearNew(saveglobals_t, 1);

	return globs;
}

void DEM_FreeGLOB(saveglobals_t *globs)
{
	if (globs->game)
		Z_Free((char *)globs->game);

	if (globs->level)
		Z_Free((char *)globs->level);

	if (globs->wad_names)
		Z_Free(globs->wad_names);

	// FIXME: globs->players

	Z_Free(globs);
}


//----------------------------------------------------------------------------
//
//  LOADING GLOBALS
//

static bool GlobReadVARI(saveglobals_t *globs)
{
	const char *var_name;
	const char *var_data;

	int i;
	void *storage;

	if (! DEM_PushReadChunk("Vari"))
		return false;

	var_name = DEM_GetString();
	var_data = DEM_GetString();

	if (! DEM_PopReadChunk() || !var_name || !var_data)
	{
		if (var_name) Z_Free((char *)var_name);
		if (var_data) Z_Free((char *)var_data);

		return false;
	}

	// find variable in list 
	for (i=0; demo_globals[i].name; i++)
	{
		if (strcmp(demo_globals[i].name, var_name) == 0)
			break;
	}

	if (demo_globals[i].name)
	{
    int offset = demo_globals[i].offset_p - (char *) &dummy_glob;

		// found it, so parse it
    storage = ((char *) globs) + offset;

		(* demo_globals[i].parse_func)(var_data, storage);
	}
	else
	{
		I_Warning("GlobReadVARI: unknown global: %s\n", var_name);
	}

	Z_Free((char *)var_name);
	Z_Free((char *)var_data);

	return true;
}

static bool GlobReadPLYR(saveglobals_t *glob)
{
	if (! DEM_PushReadChunk("Plyr"))
		return false;

	//!!! IMPLEMENT THIS

	DEM_PopReadChunk();
	return true;
}

static bool GlobReadWADS(saveglobals_t *glob)
{
	if (! DEM_PushReadChunk("Wads"))
		return false;

	//!!! IMPLEMENT THIS

	DEM_PopReadChunk();
	return true;
}


//
// DEM_LoadGLOB
//
saveglobals_t *DEM_LoadGLOB(void)
{
	char marker[6];

	saveglobals_t *globs;

	DEM_GetMarker(marker);

	if (strcmp(marker, "Glob") != 0 || ! DEM_PushReadChunk("Glob"))
		return NULL;

	cur_globs = globs = DEM_NewGLOB();

	// read through all the chunks, picking the bits we need

	for (;;)
	{
		if (DEM_GetError() != 0)
			break;  /// set error !!

		if (DEM_RemainingChunkSize() < 4)
			break;

		DEM_GetMarker(marker);

		if (strcmp(marker, "Vari") == 0)
		{
			GlobReadVARI(globs);
			continue;
		}
		if (strcmp(marker, "Plyr") == 0)
		{
			GlobReadPLYR(globs);
			continue;
		}
		if (strcmp(marker, "Wads") == 0)
		{
			GlobReadWADS(globs);
			continue;
		}

		// skip chunk
		I_Warning("LOAD_DEMO: Unknown GLOB chunk [%s]\n", marker);

		if (! DEM_SkipReadChunk(marker))
			break;
	}

	DEM_PopReadChunk();  /// check err

	return globs;
}


//----------------------------------------------------------------------------
//
//  SAVING GLOBALS
//

static void GlobWriteVARI(saveglobals_t *globs)
{
	int i;

	for (i=0; demo_globals[i].name; i++)
	{
		int offset = demo_globals[i].offset_p - (char *) &dummy_glob;

		const char *data;
		void *storage = ((char *) globs) + offset;

		data = (* demo_globals[i].stringify_func)(storage);
		DEV_ASSERT2(data);

		DEM_PushWriteChunk("Vari");
		DEM_PutString(demo_globals[i].name);
		DEM_PutString(data);
		DEM_PopWriteChunk();

		Z_Free((char *)data);
	}
}

static void GlobWritePLYR(saveglobals_t *globs)
{
	// !!!! FIXME: loop over all players

	DEM_PushWriteChunk("Plyr");

	DEM_PutInt(0);  // player ID
	DEM_PutInt(0);  // player flags

	DEM_PutString("Johnny");    // player name
	DEM_PutString("OUR HERO");  // DDF mobj type

	DEM_PopWriteChunk();
}

static void GlobWriteWADS(saveglobals_t *globs)
{
	int i;

	if (! globs->wad_names)
		return;

	DEV_ASSERT2(globs->wad_num > 0);

	DEM_PushWriteChunk("Wads");
	DEM_PutInt(globs->wad_num);

	for (i=0; i < globs->wad_num; i++)
		DEM_PutString(globs->wad_names[i]);

	DEM_PopWriteChunk();
}


//
// DEM_SaveGLOB
//
void DEM_SaveGLOB(saveglobals_t *globs)
{
	cur_globs = globs;

	DEM_PushWriteChunk("Glob");

	GlobWriteVARI(globs);
	GlobWritePLYR(globs);
	GlobWriteWADS(globs);

	// all done
	DEM_PopWriteChunk();
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
