//----------------------------------------------------------------------------
//  EDGE Data Definition File Code (Level Defines)
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
// Level Setup and Parser Code
//

#include "i_defs.h"

#include "ddf_locl.h"
#include "ddf_main.h"

#include "epi/utility.h"

#undef  DF
#define DF  DDF_CMD

static void DDF_LevelGetSpecials(const char *info, void *storage);
static void DDF_LevelGetPic(const char *info, void *storage);
static void DDF_LevelGetLighting(const char *info, void *storage);
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
    DF("TEXT GRAPHIC", text_back, DDF_MainGetInlineStr10),
    DF("TEXT FLAT", text_flat, DDF_MainGetInlineStr10),
    DF("TEXT SPEED", text_speed, DDF_MainGetFloat),
    DF("TEXT WAIT", text_wait, DDF_MainGetNumeric),
    DF("COLOURMAP", text_colmap, DDF_MainGetColourmap),
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
	DDF_SUB_LIST("PRE", f_pre, finale_commands, buffer_finale),
	DDF_SUB_LIST("END", f_end, finale_commands, buffer_finale),

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

static specflags_t deprec_map_specials[] =
{
    {"TRANSLUCENCY", 0, 0},
    {NULL, 0, 0}
};

static specflags_t map_specials[] =
{
    {"JUMPING", MPF_Jumping, 0},
    {"MLOOK", MPF_Mlook, 0},
    {"FREELOOK", MPF_Mlook, 0},  // -AJA- backwards compat.
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
			dynamic_map->ddf.name.Set(name);
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
	L_WriteDebug("LEVEL_PARSE: %s = %s;\n", field, contents);
#endif

	if (! DDF_MainParseField(level_commands, field, contents))
		DDF_WarnError2(0x128, "Unknown levels.ddf command: %s\n", field);
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
}

//
// DDF_LevelGetPic
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

    // Handle depreciated flags
	switch (DDF_MainCheckSpecialFlag(info, deprec_map_specials, 
                                     &flag_value, true, true))
	{
		case CHKF_Positive:
		case CHKF_Negative:
		case CHKF_User:
            DDF_Warning("Level special '%s' deprecated.\n", info);
            return;
            break;

		case CHKF_Unknown:
			break;
    }

    // Handle working flags
	switch (DDF_MainCheckSpecialFlag(info, map_specials, 
                                     &flag_value, true, true))
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

// --> map finale definition class

//
// map_finaledef_c Constructor
//
map_finaledef_c::map_finaledef_c()
{
	Default();
}

//
// map_finaledef_c Copy constructor
//
map_finaledef_c::map_finaledef_c(map_finaledef_c &rhs)
{
	Copy(rhs);
}

//
// map_finaledef_c Destructor
//
map_finaledef_c::~map_finaledef_c()
{
}

//
// map_finaledef_c::Copy()
//
void map_finaledef_c::Copy(map_finaledef_c &src)
{
	text = src.text;

	text_back = src.text_back;
	text_flat = src.text_flat;
	text_speed = src.text_speed;
	text_wait = src.text_wait;
	text_colmap = src.text_colmap;
	
	pics = src.pics;		
	picwait = src.picwait;

	docast = src.docast;
	dobunny = src.dobunny;
	music = src.music;
}

//
// map_finaledef_c::Default()
//
void map_finaledef_c::Default()
{
	text.Clear();	
	text_back.Clear();
	text_flat.Clear();
	text_speed = 3.0f;
	text_wait = 150;
	text_colmap = NULL;

	pics.Clear();
	picwait = 0;

	docast = false;
	dobunny = false;
	music = 0;
}

//
// map_finaledef_c assignment operator
//
map_finaledef_c& map_finaledef_c::operator=(map_finaledef_c &rhs)
{
	if (&rhs != this)
		Copy(rhs);
		
	return *this;
}

// --> map definition class

//
// mapdef_c Constructor
//
mapdef_c::mapdef_c()
{
	Default();
}

//
// mapdef_c Copy constructor
//
mapdef_c::mapdef_c(mapdef_c &rhs)
{
	Copy(rhs);
}

//
// mapdef_c Destructor
//
mapdef_c::~mapdef_c()
{
}

//
// mapdef_c::Copy()
//
void mapdef_c::Copy(mapdef_c &src)
{
	ddf = src.ddf;
	CopyDetail(src);	
}

//
// mapdef_c::CopyDetail()
//
void mapdef_c::CopyDetail(mapdef_c &src)
{
	next = src.next;				// FIXME!! Gamestate data
	
	description = src.description;	
  	namegraphic = src.namegraphic;
  	lump = src.lump;
   	sky = src.sky;
   	surround = src.surround;
   	
   	music = src.music;
	partime = src.partime;
	
	episode_name = src.episode_name;
	
	force_on = src.force_on;
	force_off = src.force_off;

	nextmapname = src.nextmapname;
	secretmapname = src.secretmapname;

	autotag = src.autotag;

	lighting = src.lighting;
	wistyle = src.wistyle;

	f_pre = src.f_pre;
	f_end = src.f_end;
}

//
// mapdef_c::Default()
//
void mapdef_c::Default()
{
	ddf.Default();

	next = NULL;
	
	description.Clear();	
  	namegraphic.Clear();
  	lump.Clear();
   	sky.Clear();
   	surround.Clear();
   	
   	music = 0;
	partime = 0;

	episode_name.Clear();	
	force_on = MPF_None;
	force_off = MPF_None;

	nextmapname.Clear();
	secretmapname.Clear();

	autotag = 0;

	lighting = LMODEL_Doom;
	wistyle = WISTYLE_Doom;

	f_pre.Default();
	f_end.Default();
}

//
// mapdef_c assignment operator
//
mapdef_c& mapdef_c::operator=(mapdef_c &rhs)
{
	if (&rhs != this)
		Copy(rhs);
		
	return *this;
}

// --> map definition container class

//
// mapdef_container_c::CleanupObject()
//
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
	epi::array_iterator_c it;
	mapdef_c *m;

	if (!refname || !refname[0])
		return NULL;

	for (it = GetTailIterator(); it.IsValid(); it--)
	{
		m = ITERATOR_TO_TYPE(it, mapdef_c*);
		if (DDF_CompareName(m->ddf.name.GetString(), refname) == 0) // Create ddf compare function
			return m;
	}

	// FIXME!!! Throw an epi:error_c here
	return NULL;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
