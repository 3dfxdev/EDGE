//----------------------------------------------------------------------------
//  EDGE Data Definition File Code (Linedefs)
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
// Line Definitions Setup and Parser Code
//
// -KM- 1998/09/01 Written.
// -ACB- 1998/09/06 Beautification: cleaned up so I can read it :).
// -KM- 1998/10/29 New types of linedefs added: colourmap, sound, friction, gravity
//                  auto, singlesided, music, lumpcheck
//                  Removed sector movement to ddf_main.c, so can be accesed by
//                  ddf_sect.c
//
// -ACB- 2001/02/04 DDF_GetSecHeightReference moved to p_plane.c
//

#include "i_defs.h"

#include "ddf_locl.h"
#include "ddf_main.h"
#include "dm_state.h"
#include "m_fixed.h"
#include "p_mobj.h"
#include "p_local.h"
#include "z_zone.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#undef  DF
#define DF  DDF_CMD

#define DDF_LineHashFunc(x)  (((x) + 211) % 211)

// -KM- 1999/01/29 Improved scrolling.
// Scrolling
typedef enum
{
	dir_none = 0,
	dir_vert = 1,
	dir_up = 2,
	dir_horiz = 4,
	dir_left = 8
}
scrolldirs_e;

static linedeftype_t buffer_line;
static linedeftype_t *dynamic_line;

// these bits logically belong with buffer_line:
static float s_speed;
static scrolldirs_e s_dir;

const linedeftype_t template_line =
{
	DDF_BASE_NIL,  // ddf

	0,          // newtrignum
	line_none,  // how triggered (walk/switch/shoot)
	trig_none,  // things that can trigger it
	KF_NONE,    // keys needed
	-1,         // can be activated repeatedly
	-1,         // special type: don't change.
	false,      // crushing

	// Floor
	{
		mov_undefined,  // type
		false,          // is_ceiling
		false,          // crush
		-1, -1,         // speed up/down
		REF_Absolute,   // dest ref
		0,              // dest
		(heightref_e)(REF_Surrounding | REF_HIGHEST | REF_INCLUDE),  // other ref
		0,              // other
		"",             // texture
		0, 0,           // wait, prewait
		sfx_None, sfx_None, sfx_None, sfx_None,  // SFX start/up/down/stop
		0, 0.0          // scroll_angle, scroll_speed
	},

	// Ceiling
	{
		mov_undefined,  // type
		true,           // is_ceiling
		false,          // crush
		-1, -1,         // speed up/down
		REF_Absolute,   // dest ref
		0,              // dest
		(heightref_e)(REF_Current | REF_CEILING),  // other ref
		0,              // other
		"",             // texture
		0, 0,           // wait, prewait
		sfx_None, sfx_None, sfx_None, sfx_None,  // SFX start/up/down/stop
		0, 0.0          // scroll_angle, scroll_speed
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

	// Donut
	{
		false,  //  dodonut
		sfx_None, sfx_None,  // SFX down/stop inner circle
		sfx_None, sfx_None   // SFX up/stop outer loop
	},

	// Sliding Door
	{
		SLIDE_None,   // sliding type
		4.0,          // speed (distance per tic)
		150,          // wait time
		false,        // see through ?
		PERCENT_MAKE(90), // distance
		sfx_None,     // sfx_start
		sfx_None,     // sfx_open
		sfx_None,     // sfx_close
		sfx_None      // sfx_stop
	},

	// Tile Skies
	{
		TILESKY_None,  // type
		1,             // layer
		10,            // number
		1.2f,           // squish
		-0.9f,          // offset
	},

	// Ladders
	{
		0    // height
	},

	// Teleport
	{
		false,     // is a teleporter
		NULL,      // effect in object
		NULL,      // in object's name
		NULL,      // effect out object
		NULL,      // out object's name
		0,         // delay
		TELSP_None // special
	},

	// Lights
	{
		LITE_None, //  lights action
		64,        //  set light to this level
		PERCENT_MAKE(50), //  chance value
		0,0,0,     //  dark/bright time, sync
		8          //  step value
	},

	EXIT_None,   // exit type
	0, 0,        // scrolling X/Y
	SCPT_None,   // scroll parts
	NULL,        // security message
	NULL,        // colourmap
  FLO_UNUSED,  // gravity
  FLO_UNUSED,  // friction
  FLO_UNUSED,  // viscosity
  FLO_UNUSED,  // drag
	sfx_None,    // ambient_sfx
	sfx_None,    // activate_sfx
	0,           // music
	false,       // automatic line
	false,       // single sided

	// Extra floor:
	{
		EXFL_None,   // type
		EFCTL_None   // control
	},

	PERCENT_MAKE(100), // translucency
	DEFAULT_APPEAR,    // appear
	LINSP_None,        // special_flags
	0,                 // trigger_effect
	LINEFX_None,       // line_effect
	SCPT_None,         // line_parts
	SECTFX_None        // sector_effect
};

const moving_plane_t donut_floor =
{
	mov_Once,        //  Type
	false,           //  is_ceiling
	false,           //  crush
	FLOORSPEED / 2,  //  speed up
	FLOORSPEED / 2,  //  speed down
	REF_Absolute,    //  dest ref
	(float)INT_MAX,//  dest
	REF_Absolute,    //  other ref
	(float)INT_MAX,//  other
	"",              //  texture
	0, 0,            //  wait, prewait
	0, 0, 0, 0,      //  SFX start/up/down/stop
	0, 0             //  Scroll angle/speed
};

linedeftype_t ** ddf_linetypes;
int num_ddf_linetypes;

static stack_array_t ddf_linetypes_a;
static const linedeftype_t *line_lookup_cache[211];

// BOOM generalised linetype support
static linedeftype_t ** ddf_gen_lines;
int num_ddf_gen_lines;
static stack_array_t ddf_gen_lines_a;

static void DDF_LineGetTrigType(const char *info, void *storage);
static void DDF_LineGetActivators(const char *info, void *storage);
static void DDF_LineGetSecurity(const char *info, void *storage);
static void DDF_LineGetScroller(const char *info, void *storage);
static void DDF_LineGetScrollPart(const char *info, void *storage);
static void DDF_LineGetExtraFloor(const char *info, void *storage);
static void DDF_LineGetEFControl(const char *info, void *storage);
static void DDF_LineGetTeleportSpecial(const char *info, void *storage);
static void DDF_LineGetRadTrig(const char *info, void *storage);
static void DDF_LineGetSpecialFlags(const char *info, void *storage);
static void DDF_LineGetSlideType(const char *info, void *storage);
static void DDF_LineGetSkyType(const char *info, void *storage);
static void DDF_LineGetLineEffect(const char *info, void *storage);
static void DDF_LineGetSectorEffect(const char *info, void *storage);

moving_plane_t dummy_floor;
elevator_sector_t dummy_elevator;

static sliding_door_t dummy_slider;
static tilesky_info_t dummy_tilesky;
static ladder_info_t dummy_ladder;

#undef  DDF_CMD_BASE
#define DDF_CMD_BASE  dummy_floor

const commandlist_t floor_commands[] =
{
  DF("TYPE", type, DDF_SectGetMType),
  DF("SPEED UP",   speed_up,   DDF_MainGetFloat),
  DF("SPEED DOWN", speed_down, DDF_MainGetFloat),
  DF("DEST REF",   destref,    DDF_SectGetDestRef),
  DF("DEST OFFSET", dest, DDF_MainGetFloat),
  DF("OTHER REF",   otherref,  DDF_SectGetDestRef),
  DF("OTHER OFFSET", other, DDF_MainGetFloat),
  DF("TEXTURE", tex, DDF_MainGetInlineStr10),
  DF("PAUSE TIME", wait,  DDF_MainGetTime),
  DF("WAIT TIME", prewait,  DDF_MainGetTime),
  DF("SFX START", sfxstart, DDF_MainLookupSound),
  DF("SFX UP",    sfxup,    DDF_MainLookupSound),
  DF("SFX DOWN",  sfxdown,  DDF_MainLookupSound),
  DF("SFX STOP",  sfxstop,  DDF_MainLookupSound),
  DF("SCROLL ANGLE", scroll_angle,DDF_MainGetAngle),
  DF("SCROLL SPEED", scroll_speed,DDF_MainGetFloat),

	DDF_CMD_END
};

#undef  DDF_CMD_BASE
#define DDF_CMD_BASE  dummy_elevator

const commandlist_t elevator_commands[] =
{
  DF("TYPE", type, DDF_SectGetMType),
  DF("SPEED UP",   speed_up,   DDF_MainGetFloat),
  DF("SPEED DOWN", speed_down, DDF_MainGetFloat),
  DF("PAUSE TIME", wait, DDF_MainGetTime),
  DF("WAIT TIME", prewait,   DDF_MainGetTime),
  DF("SFX START", sfxstart,  DDF_MainLookupSound),
  DF("SFX UP",    sfxup,     DDF_MainLookupSound),
  DF("SFX DOWN",  sfxdown,   DDF_MainLookupSound),
  DF("SFX STOP",  sfxstop,   DDF_MainLookupSound),

	DDF_CMD_END
};

#undef  DDF_CMD_BASE
#define DDF_CMD_BASE  dummy_tilesky

const commandlist_t tilesky_commands[] =
{
  DF("TYPE",   type,   DDF_LineGetSkyType),
  DF("LAYER",  layer,  DDF_MainGetNumeric),
  DF("NUMBER", number, DDF_MainGetNumeric),
  DF("SQUISH", squish, DDF_MainGetFloat),
  DF("OFFSET", offset, DDF_MainGetFloat),

	DDF_CMD_END
};

#undef  DDF_CMD_BASE
#define DDF_CMD_BASE  dummy_ladder

const commandlist_t ladder_commands[] =
{
  DF("HEIGHT", height, DDF_MainGetFloat),
	DDF_CMD_END
};

#undef  DDF_CMD_BASE
#define DDF_CMD_BASE  dummy_slider

const commandlist_t slider_commands[] =
{
  DF("TYPE",  type, DDF_LineGetSlideType),
  DF("SPEED", speed, DDF_MainGetFloat),
  DF("PAUSE TIME", wait, DDF_MainGetTime),
  DF("SEE THROUGH", see_through, DDF_MainGetBoolean),
  DF("DISTANCE",  distance,  DDF_MainGetPercent),
  DF("SFX START", sfx_start, DDF_MainLookupSound),
  DF("SFX OPEN",  sfx_open,  DDF_MainLookupSound),
  DF("SFX CLOSE", sfx_close, DDF_MainLookupSound),
  DF("SFX STOP",  sfx_stop,  DDF_MainLookupSound),

	DDF_CMD_END
};

#undef  DDF_CMD_BASE
#define DDF_CMD_BASE  buffer_line

static const commandlist_t linedef_commands[] =
{
	// sub-commands
  DDF_SUB_LIST("FLOOR",    f, floor_commands,    dummy_floor),
  DDF_SUB_LIST("CEILING",  c, floor_commands,    dummy_floor),
  DDF_SUB_LIST("ELEVATOR", e, elevator_commands, dummy_elevator),
  DDF_SUB_LIST("SLIDER",   s, slider_commands,   dummy_slider),
  DDF_SUB_LIST("TILESKY",  sky, tilesky_commands, dummy_tilesky),
  DDF_SUB_LIST("LADDER",   ladder, ladder_commands, dummy_ladder),

	DF("NEWTRIGGER", newtrignum, DDF_MainGetNumeric),
	DF("ACTIVATORS", obj, DDF_LineGetActivators),
	DF("TYPE", type, DDF_LineGetTrigType),
	DF("KEYS", keys, DDF_LineGetSecurity),
	DF("FAILED MESSAGE", failedmessage, DDF_MainGetString),
	DF("COUNT", count, DDF_MainGetNumeric),
	DF("SECSPECIAL", specialtype, DDF_MainGetNumeric),
	DF("CRUSH", crush, DDF_MainGetBoolean),

	DF("DONUT", d.dodonut, DDF_MainGetBoolean),
	DF("DONUT IN SFX", d.d_sfxin, DDF_MainLookupSound),
	DF("DONUT IN SFXSTOP", d.d_sfxinstop, DDF_MainLookupSound),
	DF("DONUT OUT SFX", d.d_sfxout, DDF_MainLookupSound),
	DF("DONUT OUT SFXSTOP", d.d_sfxoutstop, DDF_MainLookupSound),

	DF("TELEPORT", t.teleport, DDF_MainGetBoolean),
	DF("TELEPORT DELAY", t.delay, DDF_MainGetTime),
	DF("TELEIN EFFECTOBJ", t.inspawnobj_ref, DDF_MainGetString),
	DF("TELEOUT EFFECTOBJ", t.outspawnobj_ref, DDF_MainGetString),
	DF("TELEPORT SPECIAL", t.special, DDF_LineGetTeleportSpecial),

	DF("LIGHT TYPE", l.type, DDF_SectGetLighttype),
	DF("LIGHT LEVEL", l.level, DDF_MainGetNumeric),
	DF("LIGHT DARK TIME", l.darktime, DDF_MainGetTime),
	DF("LIGHT BRIGHT TIME", l.brighttime, DDF_MainGetTime),
	DF("LIGHT CHANCE", l.chance, DDF_MainGetPercent),
	DF("LIGHT SYNC", l.sync, DDF_MainGetTime),
	DF("LIGHT STEP", l.step, DDF_MainGetNumeric),
	DF("EXIT", e_exit, DDF_SectGetExit),

	{"SCROLL", DDF_LineGetScroller, &s_dir, NULL},
	{"SCROLLING SPEED", DDF_MainGetFloat, &s_speed, NULL},

	DF("SCROLL XSPEED", s_xspeed, DDF_MainGetFloat),
	DF("SCROLL YSPEED", s_yspeed, DDF_MainGetFloat),
	DF("SCROLL PARTS", scroll_parts, DDF_LineGetScrollPart),
	DF("USE COLOURMAP", use_colourmap, DDF_MainGetColourmap),
	DF("GRAVITY", gravity, DDF_MainGetFloat),
	DF("FRICTION", friction, DDF_MainGetFloat),
	DF("VISCOSITY", viscosity, DDF_MainGetFloat),
	DF("DRAG", drag, DDF_MainGetFloat),
	DF("AMBIENT SOUND", ambient_sfx, DDF_MainLookupSound),
	DF("ACTIVATE SOUND", activate_sfx, DDF_MainLookupSound),
	DF("MUSIC", music, DDF_MainGetNumeric),
	DF("AUTO", autoline, DDF_MainGetBoolean),
	DF("SINGLESIDED", singlesided, DDF_MainGetBoolean),
	DF("EXTRAFLOOR TYPE", ddf, DDF_LineGetExtraFloor),
	DF("EXTRAFLOOR CONTROL", ddf, DDF_LineGetEFControl),
	DF("TRANSLUCENCY", translucency, DDF_MainGetPercent),
	DF("WHEN APPEAR", appear, DDF_MainGetWhenAppear),
	DF("SPECIAL", special_flags, DDF_LineGetSpecialFlags),
	DF("RADIUS TRIGGER", ddf, DDF_LineGetRadTrig),
	DF("LINE EFFECT", line_effect, DDF_LineGetLineEffect),
	DF("LINE PARTS",  line_parts,  DDF_LineGetScrollPart),
	DF("SECTOR EFFECT", sector_effect, DDF_LineGetSectorEffect),

	// -AJA- backwards compatibility cruft...
	DF("!EXTRAFLOOR TRANSLUCENCY", translucency, DDF_MainGetPercent),
	DF("!SOUND", ddf, DDF_DummyFunction),
	DF("!LIGHT PROBABILITY", ddf, DDF_DummyFunction),

	DDF_CMD_END
};


// -AJA- FIXME

static struct
{
	char *s;
	int n;
}

s_scroll[] =
{
	{ "NONE", dir_none | dir_none } ,
	{ "UP", (dir_vert | dir_up) | ((~dir_none) << 16) } ,
	{ "DOWN", (dir_vert) | ((~dir_up) << 16) } ,
	{ "LEFT", (dir_horiz | dir_left) | ((~dir_none) << 16) } ,
	{ "RIGHT", (dir_horiz) | ((~dir_left) << 16) }
}
,

// FIXME: use keytype_names (in ddf_mobj.c)
s_keys[] =
{
	{ "NONE",           KF_NONE },

	{ "BLUE CARD",      KF_BlueCard },
	{ "YELLOW CARD",    KF_YellowCard },
	{ "RED CARD",       KF_RedCard },
	{ "BLUE SKULL",     KF_BlueSkull },
	{ "YELLOW SKULL",   KF_YellowSkull },
	{ "RED SKULL",      KF_RedSkull },
	{ "GREEN CARD",     KF_GreenCard },
	{ "GREEN SKULL",    KF_GreenSkull },

	{ "GOLD KEY",       KF_GoldKey },
	{ "SILVER KEY",     KF_SilverKey },
	{ "BRASS KEY",      KF_BrassKey },
	{ "COPPER KEY",     KF_CopperKey },
	{ "STEEL KEY",      KF_SteelKey },
	{ "WOODEN KEY",     KF_WoodenKey },
	{ "FIRE KEY",       KF_FireKey },
	{ "WATER KEY",      KF_WaterKey },

	// backwards compatibility
	{ "REQUIRES ALL", KF_STRICTLY_ALL |
	KF_BlueCard | KF_YellowCard | KF_RedCard |
	KF_BlueSkull | KF_YellowSkull | KF_RedSkull }
}
,

s_trigger[] =
{
	{ "WALK",  line_walkable },
	{ "PUSH",  line_pushable },
	{ "SHOOT", line_shootable },
	{ "MANUAL", line_manual }
}
,

s_activators[] =
{
	{ "PLAYER",  trig_player } ,
	{ "MONSTER", trig_monster },
	{ "OTHER",   trig_other },

	// obsolete stuff
	{ "MISSILE", 0 }
};

//
//  DDF PARSE ROUTINES
//

//
// LinedefStartEntry
//
static bool LinedefStartEntry(const char *name)
{
	int i;
	bool replaces = false;
	int number = MAX(0, atoi(name));

	if (number == 0)
		DDF_Error("Bad linedef number in lines.ddf: %s\n", name);

	for (i=0; i < num_ddf_linetypes; i++)
	{
		if (ddf_linetypes[i]->ddf.number == number)
		{
			dynamic_line = ddf_linetypes[i];
			replaces = true;
			break;
		}
	}

	// if found, adjust pointer array to keep newest entries at end
	if (replaces && i < (num_ddf_linetypes-1))
	{
		Z_MoveData(ddf_linetypes + i, ddf_linetypes + i + 1, linedeftype_t *,
			num_ddf_linetypes - i);
		ddf_linetypes[num_ddf_linetypes - 1] = dynamic_line;
	}

	// not found, create a new one
	if (! replaces)
	{
		Z_SetArraySize(&ddf_linetypes_a, ++num_ddf_linetypes);

		dynamic_line = ddf_linetypes[num_ddf_linetypes - 1];
	}

	dynamic_line->ddf.name   = NULL;
	dynamic_line->ddf.number = number;

	s_speed = 1.0f;
	s_dir = dir_none;

	// instantiate the static entry
	buffer_line = template_line;

	return replaces;
}

//
// LinedefParseField
//
static void LinedefParseField(const char *field, const char *contents,
							  int index, bool is_last)
{
#if (DEBUG_DDF)  
	L_WriteDebug("LINEDEF_PARSE: %s = %s;\n", field, contents);
#endif

	if (DDF_MainParseField(linedef_commands, field, contents))
		return;

	// handle properties
	if (index == 0 && DDF_CompareName(contents, "TRUE") == 0)
	{
		DDF_LineGetSpecialFlags(field, NULL);  // FIXME FOR OFFSETS
		return;
	}

	DDF_WarnError("Unknown lines.ddf command: %s\n", field);
}

//
// LinedefFinishEntry
//
static void LinedefFinishEntry(void)
{
	ddf_base_t base;

	buffer_line.c.crush = buffer_line.f.crush = buffer_line.crush;

	// -KM- 1999/01/29 Convert old style scroller to new.
	if (s_dir & dir_vert)
	{
		if (s_dir & dir_up)
			buffer_line.s_yspeed = s_speed;
		else
			buffer_line.s_yspeed = -s_speed;
	}

	if (s_dir & dir_horiz)
	{
		if (s_dir & dir_left)
			buffer_line.s_xspeed = s_speed;
		else
			buffer_line.s_xspeed = -s_speed;
	}

	// backwards compat: COUNT=0 means no limit on triggering
	if (buffer_line.count == 0)
		buffer_line.count = -1;

	// check stuff...

	if (buffer_line.ef.type)
	{
		if ((buffer_line.ef.type & EXFL_Flooder) && (buffer_line.ef.type & EXFL_NoShade))
		{
			DDF_WarnError("FLOODER and NOSHADE tags cannot be used together.\n");
			buffer_line.ef.type = (extrafloor_type_e)(buffer_line.ef.type & ~EXFL_Flooder);
		}

		if (! (buffer_line.ef.type & EXFL_Present))
		{
			DDF_WarnError("Extrafloor type missing THIN, THICK or LIQUID.\n");
			buffer_line.ef.type = EXFL_None;
		}
	}

	if (buffer_line.friction != FLO_UNUSED && buffer_line.friction < 0.01)
	{
		DDF_WarnError("Friction value too low (%1.2f), it would prevent "
			"all movement.\n", buffer_line.friction);
		buffer_line.friction = 0.1f;
	}

	if (buffer_line.viscosity != FLO_UNUSED && buffer_line.viscosity > 0.99)
	{
		DDF_WarnError("Viscosity value too high (%1.2f), it would prevent "
			"all movement.\n", buffer_line.viscosity);
		buffer_line.viscosity = 0.9f;
	}

	// FIXME: check more stuff...

	// transfer static entry to dynamic entry

	base = dynamic_line->ddf;
	dynamic_line[0] = buffer_line;
	dynamic_line->ddf = base;

	// compute CRC...
	CRC32_Init(&dynamic_line->ddf.crc);

	// FIXME: add stuff...

	CRC32_Done(&dynamic_line->ddf.crc);
}

//
// LinedefClearAll
//
static void LinedefClearAll(void)
{
	// it is safe to just delete all the lines

	num_ddf_linetypes = 0;
	Z_SetArraySize(&ddf_linetypes_a, num_ddf_linetypes);
}

//
// DDF_ReadLines
//
void DDF_ReadLines(void *data, int size)
{
	readinfo_t lines;

	lines.memfile = (char*)data;
	lines.memsize = size;
	lines.tag = "LINES";
	lines.entries_per_dot = 6;

	if (lines.memfile)
	{
		lines.message = NULL;
		lines.filename = NULL;
		lines.lumpname = "DDFLINE";
	}
	else
	{
		lines.message = "DDF_InitLinedefs";
		lines.filename = "lines.ddf";
		lines.lumpname = NULL;
	}

	lines.start_entry  = LinedefStartEntry;
	lines.parse_field  = LinedefParseField;
	lines.finish_entry = LinedefFinishEntry;
	lines.clear_all    = LinedefClearAll;

	DDF_MainReadFile(&lines);
}

//
// DDF_LinedefInit
//
void DDF_LinedefInit(void)
{
	Z_InitStackArray(&ddf_linetypes_a, (void ***)&ddf_linetypes, sizeof(linedeftype_t), 0);
	Z_InitStackArray(&ddf_gen_lines_a, (void ***)&ddf_gen_lines, sizeof(linedeftype_t), 0);

	// clear lookup cache
	memset(line_lookup_cache, 0, sizeof(line_lookup_cache));
}

//
// DDF_LinedefCleanUp
//
void DDF_LinedefCleanUp(void)
{
	int i;

	for (i=0; i < num_ddf_linetypes; i++)
	{
		linedeftype_t *line = ddf_linetypes[i];

		DDF_ErrorSetEntryName("[%d]  (lines.ddf)", line->ddf.number);

		line->t.inspawnobj = line->t.inspawnobj_ref ?
			DDF_MobjLookup(line->t.inspawnobj_ref) : NULL;

		line->t.outspawnobj = line->t.outspawnobj_ref ?
			DDF_MobjLookup(line->t.outspawnobj_ref) : NULL;

		DDF_ErrorClearEntryName();
	}
}

//
// DDF_LineGetScroller
//
// Check for scroll types
//
void DDF_LineGetScroller(const char *info, void *storage)
{
	int i;

	for (i = sizeof(s_scroll) / sizeof(s_scroll[0]); i--;)
	{
		if (!stricmp(info, s_scroll[i].s))
		{
			s_dir = (scrolldirs_e)((s_dir & s_scroll[i].n) >> 16);
			s_dir = (scrolldirs_e)((s_dir | s_scroll[i].n) & 0xffff);
			return;
		}
	}
	DDF_WarnError("Unknown scroll direction %s\n", info);
}

//
// DDF_LineGetSecurity
//
// Get Red/Blue/Yellow
//
void DDF_LineGetSecurity(const char *info, void *storage)
{
	int i;
	bool required = false;

	if (info[0] == '+')
	{
		required = true;
		info++;
	}
	else if (buffer_line.keys & KF_STRICTLY_ALL)
	{
		// -AJA- when there is at least one required key, then the
		// non-required keys don't have any effect.
		return;
	}

	for (i = sizeof(s_keys) / sizeof(s_keys[0]); i--;)
	{
		if (!stricmp(info, s_keys[i].s))
		{
			buffer_line.keys = (keys_e)(buffer_line.keys | s_keys[i].n);

			if (required)
				buffer_line.keys = (keys_e)(buffer_line.keys | KF_STRICTLY_ALL);

			return;
		}
	}

	DDF_WarnError("Unknown key type %s\n", info);
}

//
// DDF_LineGetTrigType
//
// Check for walk/push/shoot
//
void DDF_LineGetTrigType(const char *info, void *storage)
{
	int i;

	for (i = sizeof(s_trigger) / sizeof(s_trigger[0]); i--;)
	{
		if (!stricmp(info, s_trigger[i].s))
		{
			buffer_line.type = (trigger_e)s_trigger[i].n;
			return;
		}
	}

	DDF_WarnError("Unknown Trigger type %s\n", info);
}

//
// DDF_LineGetActivators
//
// Get player/monsters/missiles
//
void DDF_LineGetActivators(const char *info, void *storage)
{
	int i;

	for (i = sizeof(s_activators) / sizeof(s_activators[0]); i--;)
	{
		if (!stricmp(info, s_activators[i].s))
		{
			buffer_line.obj = (trigacttype_e)(buffer_line.obj | s_activators[i].n);
			return;
		}
	}

	DDF_WarnError("Unknown Activator type %s\n", info);
}

static specflags_t extrafloor_types[] =
{
	// definers:
	{"THIN",          EF_DEF_THIN,       0},
	{"THICK",         EF_DEF_THICK,      0},
	{"LIQUID",        EF_DEF_LIQUID,     0},

	// modifiers:
	{"SEE THROUGH",   EXFL_SeeThrough,   0},
	{"WATER",         EXFL_Water,        0},
	{"SHADE",         EXFL_NoShade,      1},
	{"FLOODER",       EXFL_Flooder,      0},
	{"SIDE UPPER",    EXFL_SideUpper,    0},
	{"SIDE LOWER",    EXFL_SideLower,    0},
	{"BOOMTEX",       EXFL_BoomTex,      0},

	// backwards compat...
	{"FALL THROUGH",  EXFL_Liquid, 0},
	{"SHOOT THROUGH", 0, 0},
	{NULL, 0, 0}
};

//
// DDF_LineGetExtraFloor
//
// Gets the extra floor type(s).
//
// -AJA- 1999/06/21: written.
// -AJA- 2000/03/27: updated for simpler system.
//
void DDF_LineGetExtraFloor(const char *info, void *storage)
{
	int flag_value;

	if (DDF_CompareName(info, "NONE") == 0)
	{
		buffer_line.ef.type = EXFL_None;
		return;
	}

	switch (DDF_MainCheckSpecialFlag(info, extrafloor_types,
		&flag_value, true, false))
	{
		case CHKF_Positive:
			buffer_line.ef.type = (extrafloor_type_e)(buffer_line.ef.type | flag_value);
			break;

		case CHKF_Negative:
			buffer_line.ef.type = (extrafloor_type_e)(buffer_line.ef.type & ~flag_value);
			break;

		case CHKF_User:
		case CHKF_Unknown:
			DDF_WarnError("Unknown Extrafloor Type: %s", info);
			break;
	}
}

static specflags_t ef_control_types[] =
{
	{"NONE",   EFCTL_None,   0},
	{"REMOVE", EFCTL_Remove, 0},
	{NULL, 0, 0}
};

//
// DDF_LineGetEFControl
//
void DDF_LineGetEFControl(const char *info, void *storage)
{
	int flag_value;

	switch (DDF_MainCheckSpecialFlag(info, ef_control_types, &flag_value, false, false))
	{
		case CHKF_Positive:
		case CHKF_Negative:
			buffer_line.ef.control = (extrafloor_control_e)(flag_value);
			break;

		case CHKF_User:
		case CHKF_Unknown:
			DDF_WarnError("Unknown CONTROL_EXTRAFLOOR tag: %s", info);
			break;
	}
}

static specflags_t teleport_specials[] =
{
	{"SAME DIR", TELSP_SameDir, 0},
	{"SAME HEIGHT", TELSP_SameHeight, 0},
	{"SAME SPEED", TELSP_SameSpeed, 0},
	{"SAME OFFSET", TELSP_SameOffset, 0},
	{"PRESERVE", TELSP_Preserve, 0},
	{"ROTATE", TELSP_Rotate, 0},
	{NULL, 0, 0}
};

//
// DDF_LineGetTeleportSpecial
//
// Gets the teleporter special flags.
//
// -AJA- 1999/07/12: written.
//
void DDF_LineGetTeleportSpecial(const char *info, void *storage)
{
	int flag_value;

	switch (DDF_MainCheckSpecialFlag(info, teleport_specials,
		&flag_value, true, false))
	{
		case CHKF_Positive:
			buffer_line.t.special = (teleportspecial_e)(buffer_line.t.special | flag_value);
			break;

		case CHKF_Negative:
			buffer_line.t.special = (teleportspecial_e)(buffer_line.t.special & ~flag_value);
			break;

		case CHKF_User:
		case CHKF_Unknown:
			DDF_WarnError("DDF_LineGetTeleportSpecial: Unknown Special: %s", info);
			break;
	}
}

static specflags_t scrollpart_specials[] =
{
	{"RIGHT UPPER", SCPT_RightUpper, 0},
	{"RIGHT MIDDLE", SCPT_RightMiddle, 0},
	{"RIGHT LOWER", SCPT_RightLower, 0},
	{"RIGHT", SCPT_RIGHT, 0},
	{"LEFT UPPER", SCPT_LeftUpper, 0},
	{"LEFT MIDDLE", SCPT_LeftMiddle, 0},
	{"LEFT LOWER", SCPT_LeftLower, 0},
	{"LEFT", SCPT_LEFT, 0},
	{"LEFT REVERSE X", SCPT_LeftRevX, 0},
	{"LEFT REVERSE Y", SCPT_LeftRevY, 0},
	{NULL, 0, 0}
};

//
// DDF_LineGetScrollPart
//
// Gets the scroll part flags.
//
// -AJA- 1999/07/12: written.
//
void DDF_LineGetScrollPart(const char *info, void *storage)
{
	int flag_value;
	scroll_part_e *dest = (scroll_part_e *)storage;

	if (DDF_CompareName(info, "NONE") == 0)
	{
		(*dest) = SCPT_None;
		return;
	}

	switch (DDF_MainCheckSpecialFlag(info, scrollpart_specials,
		&flag_value, true, false))
	{
		case CHKF_Positive:
			(*dest) = (scroll_part_e)((*dest) | flag_value);
			break;

		case CHKF_Negative:
			(*dest) = (scroll_part_e)((*dest) & ~flag_value);
			break;

		case CHKF_User:
		case CHKF_Unknown:
			DDF_WarnError("DDF_LineGetScrollPart: Unknown Part: %s", info);
			break;
	}
}

//----------------------------------------------------------------------------

//
// DDF_LineLookupGeneralised
//
// Support for BOOM generalised linetypes.
// 
static const linedeftype_t *DDF_LineLookupGeneralised(int number)
{
	int i;
	linedeftype_t *line;

	for (i=0; i < num_ddf_gen_lines; i++)
	{
		if (ddf_gen_lines[i]->ddf.number == number)
			return ddf_gen_lines[i];
	}

	// this linetype does not exist yet in the array of dynamic
	// linetypes.  Thus we need to create it.

	Z_SetArraySize(&ddf_gen_lines_a, ++num_ddf_gen_lines);

	line = ddf_gen_lines[num_ddf_gen_lines - 1];

	// instantiate it with defaults
	(*line) = template_line;

	DDF_BoomMakeGenLine(line, number);

	return (const linedeftype_t *) line;
}

void DDF_LineClearGeneralised(void)
{
	num_ddf_gen_lines = 0;
	Z_SetArraySize(&ddf_gen_lines_a, num_ddf_gen_lines);

	// clear the cache
	memset(line_lookup_cache, 0, sizeof(line_lookup_cache));
}

//
// DDF_LineLookupNum
//
// Returns the special linedef properties from given specialtype
//
// -KM-  1998/09/01 Wrote Procedure
// -ACB- 1998/09/06 Remarked and Reformatted....
//
const linedeftype_t *DDF_LineLookupNum(int number)
{
	int slot = DDF_LineHashFunc(number);
	int i;

	// check the cache
	if (line_lookup_cache[slot] &&
		line_lookup_cache[slot]->ddf.number == number)
	{
		return line_lookup_cache[slot];
	}

	// check for BOOM generalised linetype
	if ((level_flags.compat_mode == CM_BOOM) && number >= 0x2F80)
	{
		line_lookup_cache[slot] = DDF_LineLookupGeneralised(number);
		return line_lookup_cache[slot];
	}

	// find line by number
	// NOTE: go backwards, so newer ones are found first
	for (i=num_ddf_linetypes-1; i >= 0; i--)
	{
		if (ddf_linetypes[i]->ddf.number == number)
			break;
	}

	if (i < 0)
	{
		// -AJA- 1999/06/19: Don't crash out if the line type is unknown, just
		// print a message and ignore it (like for unknown thing types).

		I_Warning("Unknown line type %i\n", number);

		return &template_line;
	}

	// update the cache
	line_lookup_cache[slot] = ddf_linetypes[i];

	return line_lookup_cache[slot];
}

static specflags_t line_specials[] =
{
	{"MUST REACH", LINSP_MustReach, 0},
	{"SWITCH SEPARATE", LINSP_SwitchSeparate, 0},
	{NULL, 0, 0}
};

//
// DDF_LineGetSpecialFlags
//
// Gets the line special flags.
//
void DDF_LineGetSpecialFlags(const char *info, void *storage)
{
	int flag_value;

	switch (DDF_MainCheckSpecialFlag(info, line_specials, &flag_value,
		true, false))
	{
	case CHKF_Positive:
		buffer_line.special_flags = (line_special_e)(buffer_line.special_flags | flag_value);
		break;

	case CHKF_Negative:
		buffer_line.special_flags = (line_special_e)(buffer_line.special_flags & ~flag_value);
		break;

	case CHKF_User:
	case CHKF_Unknown:
		DDF_WarnError("Unknown line special: %s", info);
		break;
	}
}

//
// DDF_LineGetRadTrig
//
// Gets the line's radius trigger effect.
//
static void DDF_LineGetRadTrig(const char *info, void *storage)
{
	if (DDF_CompareName(info, "ENABLE_TAGGED") == 0)
	{
		buffer_line.trigger_effect = +1;
		return;
	}
	if (DDF_CompareName(info, "DISABLE_TAGGED") == 0)
	{
		buffer_line.trigger_effect = -1;
		return;
	}

	DDF_WarnError("DDF_LineGetRadTrig: Unknown effect: %s\n", info);
}

static const specflags_t slidingdoor_names[] =
{
	{"NONE",   SLIDE_None,   0},
	{"LEFT",   SLIDE_Left,   0},
	{"RIGHT",  SLIDE_Right,  0},
	{"CENTER", SLIDE_Center, 0},
	{"CENTRE", SLIDE_Center, 0},   // synonym
	{NULL, 0, 0}
};

//
// DDF_LineGetSlideType
//
static void DDF_LineGetSlideType(const char *info, void *storage)
{
	if (CHKF_Positive != DDF_MainCheckSpecialFlag(info,
		slidingdoor_names, (int *) storage, false, false))
	{
		DDF_WarnError("DDF_LineGetSlideType: Unknown slider: %s\n", info);
	}
}

static const specflags_t tilesky_names[] =
{
	{"NONE",    TILESKY_None,    0},
	{"FLAT",    TILESKY_Flat,    0},
	{"TEXTURE", TILESKY_Texture, 0},
	{NULL, 0, 0}
};

//
// DDF_LineGetSkyType
//
static void DDF_LineGetSkyType(const char *info, void *storage)
{
	if (CHKF_Positive != DDF_MainCheckSpecialFlag(info,
		tilesky_names, (int *) storage, false, false))
	{
		DDF_WarnError("DDF_LineGetSkyType: Unknown type: %s\n", info);
	}
}

static specflags_t line_effect_names[] =
{
	{"TRANSLUCENT",    LINEFX_Translucency, 0},
	{"VECTOR SCROLL",  LINEFX_VectorScroll, 0},
	{"OFFSET SCROLL",  LINEFX_OffsetScroll, 0},

	{"SCALE TEX",      LINEFX_Scale,         0},
	{"SKEW TEX",       LINEFX_Skew,          0},
	{"LIGHT WALL",     LINEFX_LightWall,     0},

	{"UNBLOCK THINGS", LINEFX_UnblockThings, 0},
	{"BLOCK SHOTS",    LINEFX_BlockShots,    0},
	{"BLOCK SIGHT",    LINEFX_BlockSight,    0},
	{NULL, 0, 0}
};

//
// DDF_LineGetLineEffect
//
// Gets the line effect flags.
//
static void DDF_LineGetLineEffect(const char *info, void *storage)
{
	int flag_value;

	if (DDF_CompareName(info, "NONE") == 0)
	{
		buffer_line.line_effect = LINEFX_None;
		return;
	}

	switch (DDF_MainCheckSpecialFlag(info, line_effect_names,
		&flag_value, true, false))
	{
		case CHKF_Positive:
			buffer_line.line_effect = (line_effect_type_e)(buffer_line.line_effect | flag_value);
			break;

		case CHKF_Negative:
			buffer_line.line_effect = (line_effect_type_e)(buffer_line.line_effect & ~flag_value);
			break;

		case CHKF_User:
		case CHKF_Unknown:
			DDF_WarnError("Unknown line effect type: %s", info);
			break;
	}
}

static specflags_t sector_effect_names[] =
{
	{"LIGHT FLOOR",     SECTFX_LightFloor,     0},
	{"LIGHT CEILING",   SECTFX_LightCeiling,   0},
	{"SCROLL FLOOR",    SECTFX_ScrollFloor,    0},
	{"SCROLL CEILING",  SECTFX_ScrollCeiling,  0},
	{"PUSH THINGS",     SECTFX_PushThings,     0},
	{"RESET FLOOR",     SECTFX_ResetFloor,     0},
	{"RESET CEILING",   SECTFX_ResetCeiling,   0},

	{"ALIGN FLOOR",     SECTFX_AlignFloor,     0},
	{"ALIGN CEILING",   SECTFX_AlignCeiling,   0},
	{"SCALE FLOOR",     SECTFX_ScaleFloor,     0},
	{"SCALE CEILING",   SECTFX_ScaleCeiling,   0},
	{NULL, 0, 0}
};

//
// DDF_LineGetSectorEffect
//
// Gets the sector effect flags.
//
static void DDF_LineGetSectorEffect(const char *info, void *storage)
{
	int flag_value;

	if (DDF_CompareName(info, "NONE") == 0)
	{
		buffer_line.sector_effect = SECTFX_None;
		return;
	}

	switch (DDF_MainCheckSpecialFlag(info, sector_effect_names,	&flag_value, true, false))
	{
		case CHKF_Positive:
			buffer_line.sector_effect = (sector_effect_type_e)(buffer_line.sector_effect | flag_value);
			break;

		case CHKF_Negative:
			buffer_line.sector_effect = (sector_effect_type_e)(buffer_line.sector_effect & ~flag_value);
			break;

		case CHKF_User:
		case CHKF_Unknown:
			DDF_WarnError("Unknown sector effect type: %s", info);
			break;
	}
}
