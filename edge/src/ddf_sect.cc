//----------------------------------------------------------------------------
//  EDGE Data Definition File Code (Sectors)
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2001  The EDGE Team.
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
// Sector Setup and Parser Code
//
// -KM- 1998/09/27 Written.
//

#include "i_defs.h"

#include "ddf_locl.h"
#include "ddf_main.h"
#include "dm_state.h"
#include "m_fixed.h"
#include "p_mobj.h"
#include "p_local.h"
#include "z_zone.h"

#undef  DF
#define DF  DDF_CMD

#define DDF_SectHashFunc(x)  (((x) + 211) % 211)

static specialsector_t buffer_sect;
static specialsector_t *dynamic_sect;

static const specialsector_t template_sect =
{
	DDF_BASE_NIL,  // ddf

	false,      // secret
	GRAVITY,    // gravity
	FRICTION,   // friction
	VISCOSITY,  // viscosity
	DRAG,       // drag 
	false,      // crush

	// Floor
	{
		mov_undefined,  // type
		false,  // is_ceiling
		false,  // crush
		0,      // speed_up
		0,      // speed_down
		REF_Absolute, // destref
		0,      // dest
		REF_Absolute, // otherref
		0,      // other
		"",     // tex
		0,      // wait
		0,      // prewait
		NULL,   // sfxstart
		NULL,   // sfxup
		NULL,   // sfxdown
		NULL,   // sfxstop
		0,      // scroll_angle
		0.0f     // scroll_speed
	},

	// Ceiling
	{
		mov_undefined,  // type
		true,   // is_ceiling
		false,  // crush
		0,      // speed_up
		0,      // speed_down
		REF_Absolute, // destref
		0,      // dest
		REF_Absolute, // otherref
		0,      // other
		"",     // tex
		0,      // wait
		0,      // prewait
		NULL,   // sfxstart
		NULL,   // sfxup
		NULL,   // sfxdown
		NULL,   // sfxstop
		0,      // scroll_angle
		0.0f     // scroll_speed
	},

	// Elevator
	{
		mov_undefined,  // type
		-1,             // speed up
		-1,             // speed down
		0,              // wait
		0,              // prewait
		sfx_None,       // SFX start
		sfx_None,       // SFX up
		sfx_None,       // SFX down
		sfx_None        // SFX stop
	},

	// Light
	{ LITE_None, 64, PERCENT_MAKE(50), 0,0,0, 8 },

	// Damage
	{
		0,      // nominal
		-1,     // linear_max
		-1,     // error
		31,     // delay (in tics)
		NULL_LABEL, NULL_LABEL, NULL_LABEL,  // override labels
		false   // no_armour
	},

	SECSP_None,   // special_flags
	EXIT_None,    // exit type
	NULL,         // colourmap
	NULL,         // ambient_sfx
	DEFAULT_APPEAR,  // appear
	4,            // crush_time
	10.0f,        // crush_damage

	0.0f,         // push_speed
	0.0f,         // push_zspeed

	0             // push_angle
};

specialsector_t ** ddf_sectors = NULL;
int num_ddf_sectors = 0;

static stack_array_t ddf_sectors_a;

static const specialsector_t *sector_lookup_cache[211];

// BOOM generalised sector support
static specialsector_t ** ddf_gen_sectors;
int num_ddf_gen_sectors;
static stack_array_t ddf_gen_sectors_a;

void DDF_SectGetSpecialFlags(const char *info, void *storage);

#undef  DDF_CMD_BASE
#define DDF_CMD_BASE  buffer_sect

static const commandlist_t sect_commands[] =
{
  DDF_SUB_LIST("FLOOR",    f,      floor_commands,    dummy_floor),
  DDF_SUB_LIST("CEILING",  c,      floor_commands,    dummy_floor),
  DDF_SUB_LIST("ELEVATOR", e,      elevator_commands, dummy_elevator),
  DDF_SUB_LIST("DAMAGE",   damage, damage_commands,   dummy_damage),

	DF("SECRET", secret, DDF_MainGetBoolean),
	DF("SPECIAL", special_flags, DDF_SectGetSpecialFlags),
	DF("CRUSH", crush, DDF_MainGetBoolean),

	DF("LIGHT TYPE", l.type, DDF_SectGetLighttype),
	DF("LIGHT LEVEL", l.level, DDF_MainGetNumeric),
	DF("LIGHT DARKTIME", l.darktime, DDF_MainGetTime),
	DF("LIGHT BRIGHTTIME", l.brighttime, DDF_MainGetTime),
	DF("LIGHT CHANCE", l.chance, DDF_MainGetPercent),
	DF("LIGHT SYNC", l.sync, DDF_MainGetTime),
	DF("LIGHT STEP", l.step, DDF_MainGetNumeric),
	DF("EXIT", e_exit, DDF_SectGetExit),
	DF("USE COLOURMAP", use_colourmap, DDF_MainGetColourmap),
	DF("GRAVITY", gravity, DDF_MainGetFloat),
	DF("FRICTION", friction, DDF_MainGetFloat),
	DF("VISCOSITY", viscosity, DDF_MainGetFloat),
	DF("DRAG", drag, DDF_MainGetFloat),
	DF("AMBIENT SOUND", ambient_sfx, DDF_MainLookupSound),
	DF("WHEN APPEAR", appear, DDF_MainGetWhenAppear),
	DF("PUSH ANGLE", push_angle, DDF_MainGetAngle),
	DF("PUSH SPEED", push_speed, DDF_MainGetFloat),
	DF("PUSH ZSPEED", push_zspeed, DDF_MainGetFloat),

	// -AJA- backwards compatibility cruft...
	DF("!DAMAGE", damage.nominal, DDF_MainGetFloat),
	DF("!DAMAGETIME", damage.delay, DDF_MainGetTime),
	DF("!SOUND", ddf, DDF_DummyFunction),
	DF("!LIGHT PROBABILITY", ddf, DDF_DummyFunction),

	DF("CRUSH TIME", crush_time, DDF_MainGetTime),
	DF("CRUSH DAMAGE", crush_damage, DDF_MainGetFloat),

	DDF_CMD_END
};


//
//  DDF PARSE ROUTINES
//

//
// SectorStartEntry
//
static bool SectorStartEntry(const char *name)
{
	int i;
	bool replaces = false;
	int number = MAX(0, atoi(name));

	if (number == 0)
		DDF_Error("Bad sector number in sectors.ddf: %s\n", name);

	for (i=0; i < num_ddf_sectors; i++)
	{
		if (ddf_sectors[i]->ddf.number == number)
		{
			dynamic_sect = ddf_sectors[i];
			replaces = true;
			break;
		}
	}

	// if found, adjust pointer array to keep newest entries at end
	if (replaces && i < (num_ddf_sectors-1))
	{
		Z_MoveData(ddf_sectors + i, ddf_sectors + i + 1, specialsector_t *,
			num_ddf_sectors - i);
		ddf_sectors[num_ddf_sectors - 1] = dynamic_sect;
	}

	// not found, create a new one
	if (!replaces)
	{
		Z_SetArraySize(&ddf_sectors_a, ++num_ddf_sectors);
		dynamic_sect = ddf_sectors[num_ddf_sectors - 1];
	}

	dynamic_sect->ddf.name   = NULL;
	dynamic_sect->ddf.number = number;

	// instantiate the static entry
	buffer_sect = template_sect;

	return replaces;
}

//
// SectorParseField
//
static void SectorParseField(const char *field, const char *contents,
							 int index, bool is_last)
{
#if (DEBUG_DDF)  
	L_WriteDebug("SECTOR_PARSE: %s = %s;\n", field, contents);
#endif

	if (DDF_MainParseField(sect_commands, field, contents))
		return;

	// handle properties
	if (index == 0 && DDF_CompareName(contents, "TRUE") == 0)
	{
		DDF_SectGetSpecialFlags(field, NULL);  // FIXME FOR OFFSETS
		return;
	}

	DDF_WarnError("Unknown sectors.ddf command: %s\n", field);
}

//
// SectorFinishEntry
//
static void SectorFinishEntry(void)
{
	ddf_base_t base;

	// FIXME: check stuff...

	// transfer static entry to dynamic entry

	base = dynamic_sect->ddf;
	dynamic_sect[0] = buffer_sect;
	dynamic_sect->ddf = base;

	// compute CRC...
	CRC32_Init(&dynamic_sect->ddf.crc);

	// FIXME: add stuff...

	CRC32_Done(&dynamic_sect->ddf.crc);
}

//
// SectorClearAll
//
static void SectorClearAll(void)
{
	// it is safe to just delete all sector types

	num_ddf_sectors = 0;
	Z_SetArraySize(&ddf_sectors_a, num_ddf_sectors);
}


//
// DDF_ReadSectors
//
void DDF_ReadSectors(void *data, int size)
{
	readinfo_t sects;

	sects.memfile = (char*)data;
	sects.memsize = size;
	sects.tag = "SECTORS";
	sects.entries_per_dot = 1;

	if (sects.memfile)
	{
		sects.message = NULL;
		sects.filename = NULL;
		sects.lumpname = "DDFSECT";
	}
	else
	{
		sects.message = "DDF_InitSectors";
		sects.filename = "sectors.ddf";
		sects.lumpname = NULL;
	}

	sects.start_entry  = SectorStartEntry;
	sects.parse_field  = SectorParseField;
	sects.finish_entry = SectorFinishEntry;
	sects.clear_all    = SectorClearAll;

	DDF_MainReadFile(&sects);
}

//
// DDF_SectorInit
//
void DDF_SectorInit(void)
{
	Z_InitStackArray(&ddf_sectors_a, (void ***)&ddf_sectors,
		sizeof(specialsector_t), 0);

	Z_InitStackArray(&ddf_gen_sectors_a, (void ***)&ddf_gen_sectors, 
		sizeof(specialsector_t), 0);

	// clear lookup cache
	memset(sector_lookup_cache, 0, sizeof(sector_lookup_cache));
}

//
// DDF_SectorCleanUp
//
void DDF_SectorCleanUp(void)
{
	/* nothing to do */
}

//----------------------------------------------------------------------------

//
// DDF_SectorLookupGeneralised
//
// Support for BOOM generalised sector types.
// 
static const specialsector_t *DDF_SectorLookupGeneralised(int number)
{
	int i;
	specialsector_t *sector;

	for (i=0; i < num_ddf_gen_sectors; i++)
	{
		if (ddf_gen_sectors[i]->ddf.number == number)
			return ddf_gen_sectors[i];
	}

	// this sector type does not exist yet in the array of dynamic
	// sector types.  Thus we need to create it.

	Z_SetArraySize(&ddf_gen_sectors_a, ++num_ddf_gen_sectors);

	sector = ddf_gen_sectors[num_ddf_gen_sectors - 1];

	// instantiate it with defaults
	(*sector) = template_sect;

	DDF_BoomMakeGenSector(sector, number);

	return (const specialsector_t *) sector;
}

void DDF_SectorClearGeneralised(void)
{
	num_ddf_gen_sectors = 0;
	Z_SetArraySize(&ddf_gen_sectors_a, num_ddf_gen_sectors);

	// clear the cache
	memset(sector_lookup_cache, 0, sizeof(sector_lookup_cache));
}

//
// DDF_SectorLookupNum
//
// Returns the special sector properties from given specialtype
//
// -KM-  1998/09/19 Wrote Procedure
// -AJA- 2000/02/09: Reworked.  Hash table is now a cache.
//
const specialsector_t *DDF_SectorLookupNum(int number)
{
	int i;
	int slot = DDF_SectHashFunc(number);

	// check the lookup cache...
	if (sector_lookup_cache[slot] &&
		sector_lookup_cache[slot]->ddf.number == number)
	{
		return sector_lookup_cache[slot];
	}

	// check for BOOM generalised sector types
	if ((level_flags.compat_mode == CM_BOOM) && number >= 0x20)
	{
		sector_lookup_cache[slot] = DDF_SectorLookupGeneralised(number);
		return sector_lookup_cache[slot];
	}

	// find sector by number
	// NOTE: we go backwards, so that later entries are found first
	for (i=num_ddf_sectors-1; i >= 0; i--)
	{
		if (ddf_sectors[i]->ddf.number == number)
			break;
	}

	if (i < 0)
	{
		// -AJA- 1999/06/19: Don't crash out if the sector type is unknown,
		// just print a message and ignore it (like for unknown thing types).

		I_Warning("Unknown sector type: %d\n", number);

		return &template_sect;
	}

	// update the cache
	sector_lookup_cache[slot] = ddf_sectors[i];

	return sector_lookup_cache[slot];
}

static specflags_t sector_specials[] =
{
	{"WHOLE REGION", SECSP_WholeRegion, 0},
	{"PROPORTIONAL", SECSP_Proportional, 0},
	{"PUSH ALL", SECSP_PushAll, 0},
	{"PUSH CONSTANT", SECSP_PushConstant, 0},
	{"AIRLESS", SECSP_AirLess, 0},
	{"SWIM", SECSP_Swimming, 0},
	{NULL, 0, 0}
};

//
// DDF_SectGetSpecialFlags
//
// Gets the sector specials.
//
void DDF_SectGetSpecialFlags(const char *info, void *storage)
{
	int flag_value;

	switch (DDF_MainCheckSpecialFlag(info, sector_specials, &flag_value, true, false))
	{
		case CHKF_Positive:
			buffer_sect.special_flags = (sector_flag_e)(buffer_sect.special_flags | flag_value);
			break;

		case CHKF_Negative:
			buffer_sect.special_flags = (sector_flag_e)(buffer_sect.special_flags & ~flag_value);
			break;

		case CHKF_User:
		case CHKF_Unknown:
			DDF_WarnError("Unknown sector special: %s", info);
			break;
	}
}

static specflags_t exit_types[] =
{
	{"NONE", EXIT_None, 0},
	{"NORMAL", EXIT_Normal, 0},
	{"SECRET", EXIT_Secret, 0},

	// -AJA- backwards compatibility cruft...
	{"!EXIT", EXIT_Normal, 0},
	{NULL, 0, 0}
};

//
// DDF_SectGetExit
//
// Get the exit type
//
void DDF_SectGetExit(const char *info, void *storage)
{
	int *dest = (int *)storage;
	int flag_value;

	switch (DDF_MainCheckSpecialFlag(info, exit_types, &flag_value,
		false, false))
	{
		case CHKF_Positive:
		case CHKF_Negative:
			(*dest) = flag_value;
			break;

		case CHKF_User:
		case CHKF_Unknown:
			DDF_WarnError("Unknown Exit type: %s\n", info);
			break;
	}
}

static specflags_t light_types[] =
{
	{"NONE", LITE_None, 0},
	{"SET",  LITE_Set,  0},
	{"FADE", LITE_Fade, 0},
	{"STROBE", LITE_Strobe, 0},
	{"FLASH",  LITE_Flash,  0},
	{"GLOW",   LITE_Glow,   0},
	{"FLICKER", LITE_FireFlicker, 0},
	{NULL, 0, 0}
};

//
// DDF_SectGetLighttype
//
// Get the light type
//
void DDF_SectGetLighttype(const char *info, void *storage)
{
	int *dest = (int *)storage;
	int flag_value;

	switch (DDF_MainCheckSpecialFlag(info, light_types, &flag_value,
		false, false))
	{
		case CHKF_Positive:
		case CHKF_Negative:
			(*dest) = flag_value;
			break;

		case CHKF_User:
		case CHKF_Unknown:
			DDF_WarnError("Unknown light type: %s\n", info);
			break;
	}
}

static specflags_t movement_types[] =
{
	{"MOVE", mov_Once, 0},
	{"MOVEWAITRETURN", mov_MoveWaitReturn, 0},
	{"CONTINUOUS", mov_Continuous, 0},
	{"PLAT", mov_Plat, 0},
	{"BUILDSTAIRS", mov_Stairs, 0},
	{"STOP", mov_Stop, 0},
	{NULL, 0, 0}
};

//
// DDF_SectGetMType
//
// Get movement types: MoveWaitReturn etc
//
void DDF_SectGetMType(const char *info, void *storage)
{
	int *dest = (int *)storage;
	int flag_value;

	switch (DDF_MainCheckSpecialFlag(info, movement_types, &flag_value,
		false, false))
	{
		case CHKF_Positive:
		case CHKF_Negative:
			(*dest) = flag_value;
			break;

		case CHKF_User:
		case CHKF_Unknown:
			DDF_WarnError("Unknown Movement type: %s\n", info);
			break;
	}
}

static specflags_t reference_types[] =
{
	{"ABSOLUTE", REF_Absolute, false},

	{"FLOOR", REF_Current, false},
	{"CEILING", REF_Current + REF_CEILING, false},

	// Note that LOSURROUNDINGFLOOR has the REF_INCLUDE flag, but the
	// others do not.  It's there to maintain backwards compatibility.
	// 
	{"LOSURROUNDINGCEILING", REF_Surrounding + REF_CEILING, false},
	{"HISURROUNDINGCEILING", REF_Surrounding + REF_CEILING + REF_HIGHEST, false},
	{"LOSURROUNDINGFLOOR", REF_Surrounding + REF_INCLUDE, false},
	{"HISURROUNDINGFLOOR", REF_Surrounding + REF_HIGHEST, false},

	// Note that REF_HIGHEST is used for the NextLowest types, and
	// vice versa, which may seem strange.  It's because the next
	// lowest sector is actually the highest of all adjacent sectors
	// that are lower than the current sector.
	// 
	{"NEXTLOWESTFLOOR", REF_Surrounding + REF_NEXT + REF_HIGHEST, false},
	{"NEXTHIGHESTFLOOR", REF_Surrounding + REF_NEXT, false},
	{"NEXTLOWESTCEILING", REF_Surrounding + REF_NEXT + REF_CEILING + REF_HIGHEST, false},
	{"NEXTHIGHESTCEILING", REF_Surrounding + REF_NEXT + REF_CEILING, false},

	{"LOWESTBOTTOMTEXTURE", REF_LowestLoTexture, false}
};

//
// DDF_SectGetDestRef
//
// Get surroundingsectorceiling/floorheight etc
//
void DDF_SectGetDestRef(const char *info, void *storage)
{
	int *dest = (int *)storage;
	int flag_value;

	// check for modifier flags
	if (DDF_CompareName(info, "INCLUDE") == 0)
	{
		*dest |= REF_INCLUDE;
		return;
	}
	else if (DDF_CompareName(info, "EXCLUDE") == 0)
	{
		*dest &= ~REF_INCLUDE;
		return;
	}

	switch (DDF_MainCheckSpecialFlag(info, reference_types, &flag_value,
		false, false))
	{
		case CHKF_Positive:
		case CHKF_Negative:
			(*dest) = flag_value;
			break;

		case CHKF_User:
		case CHKF_Unknown:
			DDF_WarnError("Unknown Reference Point: %s\n", info);
			break;
	}
}
