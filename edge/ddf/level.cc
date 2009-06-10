//----------------------------------------------------------------------------
//  EDGE Data Definition File Code (Level Defines)
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2009  The EDGE Team.
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

#include "local.h"

#include "epi/utility.h"

#include "level.h"

#undef  DF
#define DF  DDF_CMD

       void DDF_LevelGetSpecials(const char *info, void *storage);
static void DDF_LevelGetPic(const char *info, void *storage);
static void DDF_LevelGetWistyle(const char *info, void *storage);

mapdef_container_c mapdefs;

static mapdef_c buffer_map;
static mapdef_c* dynamic_map;

static map_finaledef_c buffer_finale;

#undef  DDF_CMD_BASE
#define DDF_CMD_BASE  buffer_finale

static const commandlist_t finale_commands[] =
{
	DF("TEXT", text, DDF_MainGetString),
    DF("TEXT_GRAPHIC", text_back, DDF_MainGetLumpName),
    DF("TEXT_FLAT", text_flat, DDF_MainGetLumpName),
    DF("TEXT_SPEED", text_speed, DDF_MainGetFloat),
    DF("TEXT_WAIT", text_wait, DDF_MainGetNumeric),
	DF("COLOUR", text_rgb, DDF_MainGetRGB),
    DF("COLOURMAP", text_colmap, DDF_MainGetColourmap),
    DF("GRAPHIC", text, DDF_LevelGetPic),
    DF("GRAPHIC_WAIT", picwait, DDF_MainGetTime),
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
	DDF_SUB_LIST("PRE", f_pre, finale_commands, buffer_finale),
	DDF_SUB_LIST("END", f_end, finale_commands, buffer_finale),

	DF("LUMPNAME", lump, DDF_MainGetLumpName),
	DF("DESCRIPTION", description, DDF_MainGetString),
	DF("NAME_GRAPHIC", namegraphic, DDF_MainGetLumpName),
	DF("SKY_TEXTURE", sky, DDF_MainGetLumpName),
	DF("MUSIC_ENTRY", music, DDF_MainGetNumeric),
	DF("SURROUND_FLAT", surround, DDF_MainGetLumpName),
	DF("NEXT_MAP", nextmapname, DDF_MainGetLumpName),
	DF("SECRET_MAP", secretmapname, DDF_MainGetLumpName),
	DF("AUTOTAG", autotag, DDF_MainGetNumeric),
	DF("PARTIME", partime, DDF_MainGetTime),
	DF("EPISODE", episode_name, DDF_MainGetString),
	DF("STATS", wistyle, DDF_LevelGetWistyle),
	DF("SPECIAL", features, DDF_LevelGetSpecials),

	// -AJA- backwards compatibility cruft...
	DF("LIGHTING", ddf, DDF_DummyFunction),

	DDF_CMD_END
};

static specflags_t map_specials[] =
{
    {"CHEATS",   MPF_NoCheats, 1},
    {"JUMPING",  MPF_NoJumping, 1},
    {"MLOOK",    MPF_NoMLook, 1},
    {"FREELOOK", MPF_NoMLook, 1},  // -AJA- backwards compat.

    {"CROUCHING",    MPF_NoCrouching, 1},
    {"TRUE3D",       MPF_NoTrue3D, 1},
    {"MOREBLOOD",    MPF_NoMoreBlood, 1},
    {"NORMAL_BLOOD", MPF_NoMoreBlood, 0},

    {"ITEM_RESPAWN",  MPF_ItemRespawn, 0},
    {"FAST_MONSTERS", MPF_FastMon, 0},
    {"ENEMY_STOMP",   MPF_Stomp, 0},
    {"RESET_PLAYER",  MPF_ResetPlayer, 0},
    {"LIMIT_ZOOM",    MPF_LimitZoom, 0},

    {NULL, 0, 0}
};


//
//  DDF PARSE ROUTINES
//

static bool LevelStartEntry(const char *name)
{
	mapdef_c *existing = NULL;

	if (name && name[0])
		existing = mapdefs.Lookup(name);

	// not found, create a new one
	if (existing)
	{
		dynamic_map = existing;
	}
	else
	{
		dynamic_map = new mapdef_c;

		if (name && name[0])
			dynamic_map->ddf.name = name;
		else
			dynamic_map->ddf.SetUniqueName("UNNAMED_LEVEL_MAP", mapdefs.GetSize());

		mapdefs.Insert(dynamic_map);
	}

	dynamic_map->ddf.number = 0;

	// instantiate the static entries
	buffer_map.Default();
	buffer_finale.Default();

	return (existing != NULL);	
}

static void LevelParseField(const char *field, const char *contents,
							int index, bool is_last)
{
#if (DEBUG_DDF)  
	I_Debugf("LEVEL_PARSE: %s = %s;\n", field, contents);
#endif

	if (! DDF_MainParseField(level_commands, field, contents))
		DDF_WarnError2(128, "Unknown levels.ddf command: %s\n", field);
}

static void LevelFinishEntry(void)
{
	// check stuff
	if (buffer_map.episode_name == NULL)
		DDF_Error("Level entry must have an EPISODE name !\n");

	// FIXME: check more stuff here...

	// transfer static entry to dynamic entry
	dynamic_map->CopyDetail(buffer_map);

	// compute CRC...
	// FIXME! Do something...
}

static void LevelClearAll(void)
{
	// safe to delete the level entries -- no refs
	mapdefs.Clear();
}


bool DDF_ReadLevels(void *data, int size)
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

	return DDF_MainReadFile(&levels);
}

void DDF_LevelInit(void)
{
	mapdefs.Clear();
}

void DDF_LevelCleanUp(void)
{
	if (mapdefs.GetSize() == 0)
		I_Error("There are no levels defined in DDF !\n");

	mapdefs.Trim();

	// lookup episodes

	epi::array_iterator_c it;

	for (it = mapdefs.GetTailIterator(); it.IsValid(); it--)
	{
		mapdef_c *m = ITERATOR_TO_TYPE(it, mapdef_c*);
		
		m->episode = gamedefs.Lookup(m->episode_name.c_str());

		if (! m->episode_name)
			I_Printf("WARNING: Cannot find episode '%s' for map entry [%s]\n",
					 m->episode_name.c_str(), m->ddf.name.c_str());
	}
}

//
// Adds finale pictures to the level's list.
//
void DDF_LevelGetPic(const char *info, void *storage)
{
	map_finaledef_c *f = (map_finaledef_c *)storage;

	f->pics.Insert(info);
}

void DDF_LevelGetSpecials(const char *info, void *storage)
{
	// -AJA- 2000/02/02: reworked this for new system.
	int flag_value;

	int *dest = (int *)storage;

    // Handle working flags
	switch (DDF_MainCheckSpecialFlag(info, map_specials, &flag_value, true))
	{
		case CHKF_Positive:
			*dest |= flag_value;
			break;

		case CHKF_Negative:
			*dest &= ~flag_value;
			break;

		default:
			DDF_Warning("Unknown level special: %s", info);
			break;
	}
}

static specflags_t wistyle_names[] =
{
	{"DOOM", WISTYLE_Doom, 0},
	{"NONE", WISTYLE_None, 0},
	{NULL, 0, 0}
};

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

//------------------------------------------------------------------

// --> map finale definition class

map_finaledef_c::map_finaledef_c()
{
	Default();
}

map_finaledef_c::map_finaledef_c(map_finaledef_c &rhs)
{
	Copy(rhs);
}

map_finaledef_c::~map_finaledef_c()
{
}

void map_finaledef_c::Copy(map_finaledef_c &src)
{
	text = src.text;

	text_back = src.text_back;
	text_flat = src.text_flat;
	text_speed = src.text_speed;
	text_wait = src.text_wait;
	text_rgb = src.text_rgb;
	text_colmap = src.text_colmap;
	
	pics = src.pics;		
	picwait = src.picwait;

	docast = src.docast;
	dobunny = src.dobunny;
	music = src.music;
}

void map_finaledef_c::Default()
{
	text.clear();	
	text_back.clear();
	text_flat.clear();
	text_speed = 3.0f;
	text_wait = 150;
	text_rgb = RGB_NO_VALUE;
	text_colmap = NULL;

	pics.Clear();
	picwait = 0;

	docast = false;
	dobunny = false;
	music = 0;
}

map_finaledef_c& map_finaledef_c::operator=(map_finaledef_c &rhs)
{
	if (&rhs != this)
		Copy(rhs);
		
	return *this;
}

// --> map definition class

mapdef_c::mapdef_c()
{
	Default();
}

mapdef_c::mapdef_c(mapdef_c &rhs)
{
	Copy(rhs);
}

mapdef_c::~mapdef_c()
{
}

void mapdef_c::Copy(mapdef_c &src)
{
	ddf = src.ddf;
	CopyDetail(src);	
}

void mapdef_c::CopyDetail(mapdef_c &src)
{
///---	next = src.next;				// FIXME!! Gamestate data

	description = src.description;	
  	namegraphic = src.namegraphic;
  	lump = src.lump;
   	sky = src.sky;
   	surround = src.surround;
   	
   	music = src.music;
	partime = src.partime;
	
	episode_name = src.episode_name;
	
	features = src.features;

	nextmapname = src.nextmapname;
	secretmapname = src.secretmapname;

	autotag = src.autotag;

	wistyle = src.wistyle;

	f_pre = src.f_pre;
	f_end = src.f_end;
}

void mapdef_c::Default()
{
	ddf.Default();

	description.clear();	
  	namegraphic.clear();
  	lump.clear();
   	sky.clear();
   	surround.clear();
   	
   	music = 0;
	partime = 0;

	episode = NULL;
	episode_name.clear();	

	features = MPF_NONE;

	nextmapname.clear();
	secretmapname.clear();

	autotag = 0;

	wistyle = WISTYLE_Doom;

	f_pre.Default();
	f_end.Default();
}

mapdef_c& mapdef_c::operator=(mapdef_c &rhs)
{
	if (&rhs != this)
		Copy(rhs);
		
	return *this;
}

// --> map definition container class

void mapdef_container_c::CleanupObject(void *obj)
{
	mapdef_c *m = *(mapdef_c**)obj;

	if (m)
		delete m;

	return;
}

//
// mapdef_c* mapdef_container_c::Lookup()
//
// Looks an gamedef by name, returns a fatal error if it does not exist.
//
mapdef_c* mapdef_container_c::Lookup(const char *refname)
{
	if (!refname || !refname[0])
		return NULL;

	epi::array_iterator_c it;

	for (it = GetTailIterator(); it.IsValid(); it--)
	{
		mapdef_c *m = ITERATOR_TO_TYPE(it, mapdef_c*);

		// ignore maps with unknown episode_name
		if (! m->episode)
			continue;

		if (DDF_CompareName(m->ddf.name.c_str(), refname) == 0)
			return m;
	}

	return NULL;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
