//----------------------------------------------------------------------------
//  EDGE Data Definition File Code (Level Defines)
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
// Level Setup and Parser Code
//

#include "i_defs.h"

#include "ddf_locl.h"
#include "ddf_main.h"
#include "dm_state.h"
#include "m_fixed.h"
#include "m_math.h"
#include "p_mobj.h"
#include "w_wad.h"
#include "z_zone.h"

#undef  DF
#define DF  DDF_CMD

static mapstuff_t buffer_map;
static mapstuff_t *dynamic_map;

static const mapstuff_t template_map =
{
	DDF_BASE_NIL,  // ddf

	NULL,  // next
	NULL,  // description
	"",    // namegraphic
	"",    // lump
	"",    // sky
	"",    // surround
	0,     // music

	0,     // partime
	NULL,  // episode_name
	MPF_None,     // force_on
	MPF_None,     // force_off
	"",    // nextmapname
	"",    // secretmapname
	0,     // autotag
	LMODEL_Doom,  // lighting
	WISTYLE_Doom, // wistyle

	// f_pre
	{
		NULL,  // text
		"",    // text_back
		"",    // text_flat
		3,     // text_speed
		150,   // text_wait
		0,     // numpics
		0,     // picwait
		NULL,  // pics
		false, // docast
		false, // dobunny
		0      // music
	},

	// f_end
	{
		NULL,  // text
		"",    // text_back
		"",    // text_flat
		3,     // text_speed
		150,   // text_wait
		0,     // numpics
		0,     // picwait
		NULL,  // pics
		false, // docast
		false, // dobunny
		0      // music
	}
};

mapstuff_t ** level_maps = NULL;
int num_level_maps = 0;

static stack_array_t level_maps_a;


static void DDF_LevelGetSpecials(const char *info, void *storage);
static void DDF_LevelGetPic(const char *info, void *storage);
static void DDF_LevelGetLighting(const char *info, void *storage);
static void DDF_LevelGetWistyle(const char *info, void *storage);

static finale_t dummy_finale;

#undef  DDF_CMD_BASE
#define DDF_CMD_BASE  dummy_finale

static const commandlist_t finale_commands[] =
{
	DF("TEXT", text, DDF_MainGetString),
    DF("TEXT GRAPHIC", text_back, DDF_MainGetInlineStr10),
    DF("TEXT FLAT", text_flat, DDF_MainGetInlineStr10),
    DF("TEXT SPEED", text_speed, DDF_MainGetFloat),
    DF("TEXT WAIT", text_wait, DDF_MainGetNumeric),
    DF("GRAPHIC", text, DDF_LevelGetPic),
    DF("GRAPHIC WAIT", picwait, DDF_MainGetTime),
    DF("CAST", docast, DDF_MainGetBoolean),
    DF("BUNNY", dobunny, DDF_MainGetBoolean),
    DF("MUSIC", music, DDF_MainGetNumeric),

	DDF_CMD_END
};

// -KM- 1998/11/25 Finales are all go.

#undef  DDF_CMD_BASE
#define DDF_CMD_BASE  buffer_map

static const commandlist_t level_commands[] =
{
	// sub-commands
	DDF_SUB_LIST("PRE", f_pre, finale_commands, dummy_finale),
	DDF_SUB_LIST("END", f_end, finale_commands, dummy_finale),

	DF("LUMPNAME", lump, DDF_MainGetInlineStr10),
	DF("DESCRIPTION", description, DDF_MainGetString),
	DF("NAME GRAPHIC", namegraphic, DDF_MainGetInlineStr10),
	DF("SKY TEXTURE", sky, DDF_MainGetInlineStr10),
	DF("MUSIC ENTRY", music, DDF_MainGetNumeric),
	DF("SURROUND FLAT", surround, DDF_MainGetInlineStr10),
	DF("NEXT MAP", nextmapname, DDF_MainGetInlineStr10),
	DF("SECRET MAP", secretmapname, DDF_MainGetInlineStr10),
	DF("AUTOTAG", autotag, DDF_MainGetNumeric),
	DF("PARTIME", partime, DDF_MainGetTime),
	DF("EPISODE", episode_name, DDF_MainGetString),
	DF("LIGHTING", lighting, DDF_LevelGetLighting),
	DF("STATS", wistyle, DDF_LevelGetWistyle),
	DF("SPECIAL", ddf, DDF_LevelGetSpecials),

	DDF_CMD_END
};

static specflags_t map_specials[] =
{
    {"JUMPING", MPF_Jumping, 0},
    {"MLOOK", MPF_Mlook, 0},
    {"FREELOOK", MPF_Mlook, 0},  // -AJA- backwards compat.
    {"TRANSLUCENCY", MPF_Translucency, 0},
    {"CHEATS", MPF_Cheats, 0},
    {"ITEM RESPAWN", MPF_ItemRespawn, 0},
    {"FAST MONSTERS", MPF_FastParm, 0},
    {"RESURRECT RESPAWN", MPF_ResRespawn, 0},
    {"TELEPORT RESPAWN", MPF_ResRespawn, 1},
    {"STRETCH SKY", MPF_StretchSky, 0},
    {"NORMAL SKY", MPF_StretchSky, 1},
    {"TRUE3D", MPF_True3D, 0},
    {"ENEMY STOMP", MPF_Stomp, 0},
    {"MORE BLOOD", MPF_MoreBlood, 0},
    {"NORMAL BLOOD", MPF_MoreBlood, 1},
    {"RESPAWN", MPF_Respawn, 0},
    {"AUTOAIM", MPF_AutoAim, 0},
    {"AA MLOOK", MPF_AutoAimMlook, 0},
    {"EXTRAS", MPF_Extras, 0},
    {"RESET PLAYER", MPF_ResetPlayer, 0},
    {"LIMIT ZOOM", MPF_LimitZoom, 0},
    {"SHADOWS", MPF_Shadows, 0},
    {"HALOS", MPF_Halos, 0},
    {"CROUCHING", MPF_Crouching, 0},
    {"WEAPON KICK", MPF_Kicking, 0},
    {"BOOM COMPAT", MPF_BoomCompat, 0},
    {NULL, 0, 0}
};


//
//  DDF PARSE ROUTINES
//

static bool LevelStartEntry(const char *name)
{
	int i;
	bool replaces = false;

	if (name && name[0])
	{
		for (i=0; i < num_level_maps; i++)
		{
			if (DDF_CompareName(level_maps[i]->ddf.name, name) == 0)
			{
				dynamic_map = level_maps[i];
				replaces = true;
				break;
			}
		}

		// if found, adjust pointer array to keep newest entries at end
		if (replaces && i < (num_level_maps-1))
		{
			Z_MoveData(level_maps + i, level_maps + i + 1, mapstuff_t *,
				num_level_maps - i);
			level_maps[num_level_maps - 1] = dynamic_map;
		}
	}

	// not found, create a new one
	if (! replaces)
	{
		Z_SetArraySize(&level_maps_a, ++num_level_maps);

		dynamic_map = level_maps[num_level_maps - 1];
		dynamic_map->ddf.name = (name && name[0]) ? Z_StrDup(name) :
		DDF_MainCreateUniqueName("UNNAMED_LEVEL_MAP", num_level_maps);
	}

	dynamic_map->ddf.number = 0;

	// instantiate the static entry
	buffer_map = template_map;

	return replaces;
}

static void LevelParseField(const char *field, const char *contents,
							int index, bool is_last)
{
#if (DEBUG_DDF)  
	L_WriteDebug("LEVEL_PARSE: %s = %s;\n", field, contents);
#endif

	if (! DDF_MainParseField(level_commands, field, contents))
		DDF_WarnError("Unknown levels.ddf command: %s\n", field);
}

static void LevelFinishEntry(void)
{
	ddf_base_t base;

	// check stuff

	if (buffer_map.episode_name == NULL)
		DDF_Error("Level entry must have an EPISODE name !\n");

	// FIXME: check more stuff here...

	// transfer static entry to dynamic entry

	base = dynamic_map->ddf;
	dynamic_map[0] = buffer_map;
	dynamic_map->ddf = base;

	// compute CRC...
	CRC32_Init(&dynamic_map->ddf.crc);

	// FIXME: add stuff...

	CRC32_Done(&dynamic_map->ddf.crc);

	// Initial Hack
	currentmap = dynamic_map;
}

static void LevelClearAll(void)
{
	// safe to delete the level entries -- no refs

	num_level_maps = 0;
	Z_SetArraySize(&level_maps_a, num_level_maps);
}


void DDF_ReadLevels(void *data, int size)
{
	readinfo_t levels;

	levels.memfile = (char*)data;
	levels.memsize = size;
	levels.tag = "LEVELS";
	levels.entries_per_dot = 2;

	if (levels.memfile)
	{
		levels.message = NULL;
		levels.filename = NULL;
		levels.lumpname = "DDFLEVL";
	}
	else
	{
		levels.message = "DDF_InitLevels";
		levels.filename = "levels.ddf";
		levels.lumpname = NULL;
	}

	levels.start_entry  = LevelStartEntry;
	levels.parse_field  = LevelParseField;
	levels.finish_entry = LevelFinishEntry;
	levels.clear_all    = LevelClearAll;

	DDF_MainReadFile(&levels);
}

void DDF_LevelInit(void)
{
	Z_InitStackArray(&level_maps_a, (void ***)&level_maps, sizeof(mapstuff_t), 0);
}

void DDF_LevelCleanUp(void)
{
	if (num_level_maps == 0)
		I_Error("There are no levels defined in DDF !\n");
}


//
// DDF_LevelMapLookup
//
// Changes the current map: this is globally visible to all those who ref
// ddf_main.h. It will check there is an map with that entry name exists
// and the map lump name also exists. Returns map if it exists.

const mapstuff_t *DDF_LevelMapLookup(const char *refname)
{
	int i;

	if (!refname)
		return NULL;

	for (i=num_level_maps-1; i >= 0; i--)
	{
		if (DDF_CompareName(level_maps[i]->ddf.name, refname) != 0)
			continue;

		if (W_CheckNumForName(level_maps[i]->lump) == -1)
			continue;

		return level_maps[i];
	}

	return NULL;
}

//
// DDF_LevelGetPic
//
// Adds finale pictures to the level's list.
//
void DDF_LevelGetPic(const char *info, void *storage)
{
	finale_t *f = (finale_t *)storage;

	Z_Resize(f->pics, char, 9 * (f->numpics + 1));
	Z_StrNCpy(f->pics + f->numpics * 9, info, 8);
	f->numpics++;
}

void DDF_LevelGetSpecials(const char *info, void *storage)
{
	// -AJA- 2000/02/02: reworked this for new system.

	int flag_value;

	switch (DDF_MainCheckSpecialFlag(info, map_specials, &flag_value, true, true))
	{
		case CHKF_Positive:
			buffer_map.force_on  |=  flag_value;
			buffer_map.force_off &= ~flag_value;
			break;

		case CHKF_Negative:
			buffer_map.force_on  &= ~flag_value;
			buffer_map.force_off |= flag_value;
			break;

		case CHKF_User:
			buffer_map.force_on  &= ~flag_value;
			buffer_map.force_off &= ~flag_value;
			break;

		case CHKF_Unknown:
			DDF_WarnError("DDF_LevelGetSpecials: Unknown level special: %s", info);
			break;
	}
}

static specflags_t lighting_names[] =
{
	{"DOOM",    LMODEL_Doom, 0},
	{"DOOMISH", LMODEL_Doomish, 0},
	{"FLAT",    LMODEL_Flat, 0},
	{"VERTEX",  LMODEL_Vertex, 0},
	{NULL, 0, 0}
};

static specflags_t wistyle_names[] =
{
	{"DOOM", WISTYLE_Doom, 0},
	{"NONE", WISTYLE_None, 0},
	{NULL, 0, 0}
};

void DDF_LevelGetLighting(const char *info, void *storage)
{
	int flag_value;

	if (CHKF_Positive != DDF_MainCheckSpecialFlag(info, 
		lighting_names, &flag_value, false, false))
	{
		DDF_WarnError("DDF_LevelGetLighting: Unknown model: %s", info);
		return;
	}

	((lighting_model_e *)storage)[0] = (lighting_model_e)flag_value;
}

void DDF_LevelGetWistyle(const char *info, void *storage)
{
	int flag_value;

	if (CHKF_Positive != DDF_MainCheckSpecialFlag(info, 
		wistyle_names, &flag_value, false, false))
	{
		DDF_WarnError("DDF_LevelGetWistyle: Unknown stats: %s", info);
		return;
	}

	((intermission_style_e *)storage)[0] = (intermission_style_e)flag_value;
}

