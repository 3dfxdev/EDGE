//----------------------------------------------------------------------------
//  EDGE Data Definition File Code (Linedefs)
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

#include "local.h"

#include <limits.h>

#include "line.h"

#undef  DF
#define DF  DDF_CMD

#define DDF_LineHashFunc(x)  (((x) + LOOKUP_CACHESIZE) % LOOKUP_CACHESIZE)

// -KM- 1999/01/29 Improved scrolling.
// Scrolling
typedef enum
{
	dir_none  = 0,
	dir_vert  = 1,
	dir_up    = 2,
	dir_horiz = 4,
	dir_left  = 8
}
scrolldirs_e;

linetype_c buffer_line;
linetype_c *dynamic_line;

// these bits logically belong with buffer_line:
static float s_speed;
static scrolldirs_e s_dir;

linetype_container_c linetypes;		// <-- User-defined

movplanedef_c buffer_floor;
ladderdef_c buffer_ladder;
sliding_door_c buffer_slider;

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
static void DDF_LineGetLineEffect(const char *info, void *storage);
static void DDF_LineGetSectorEffect(const char *info, void *storage);
static void DDF_LineGetPortalEffect(const char *info, void *storage);
static void DDF_LineGetSlopeType(const char *info, void *storage);

static void DDF_LineMakeCrush(const char *info, void *storage);

#undef  DDF_CMD_BASE
#define DDF_CMD_BASE  buffer_floor

const commandlist_t floor_commands[] =
{
	DF("TYPE", type, DDF_SectGetMType),
	DF("SPEED_UP",   speed_up,   DDF_MainGetFloat),
	DF("SPEED_DOWN", speed_down, DDF_MainGetFloat),
	DF("DEST_REF",   destref,    DDF_SectGetDestRef),
	DF("DEST_OFFSET", dest, DDF_MainGetFloat),
	DF("OTHER_REF",   otherref,  DDF_SectGetDestRef),
	DF("OTHER_OFFSET", other, DDF_MainGetFloat),
	DF("CRUSH_DAMAGE", crush_damage, DDF_MainGetNumeric),
	DF("TEXTURE", tex, DDF_MainGetLumpName),
	DF("PAUSE_TIME", wait,  DDF_MainGetTime),
	DF("WAIT_TIME", prewait,  DDF_MainGetTime),
	DF("SFX_START", sfxstart, DDF_MainLookupSound),
	DF("SFX_UP",    sfxup,    DDF_MainLookupSound),
	DF("SFX_DOWN",  sfxdown,  DDF_MainLookupSound),
	DF("SFX_STOP",  sfxstop,  DDF_MainLookupSound),
	DF("SCROLL_ANGLE", scroll_angle,DDF_MainGetAngle),
	DF("SCROLL_SPEED", scroll_speed,DDF_MainGetFloat),
	DF("IGNORE_TEXTURE", ignore_texture, DDF_MainGetBoolean),

	DDF_CMD_END
};

#undef  DDF_CMD_BASE
#define DDF_CMD_BASE  buffer_ladder

const commandlist_t ladder_commands[] =
{
	DF("HEIGHT", height, DDF_MainGetFloat),
	DDF_CMD_END
};

#undef  DDF_CMD_BASE
#define DDF_CMD_BASE  buffer_slider

const commandlist_t slider_commands[] =
{
	DF("TYPE",  type, DDF_LineGetSlideType),
	DF("SPEED", speed, DDF_MainGetFloat),
	DF("PAUSE_TIME", wait, DDF_MainGetTime),
	DF("SEE_THROUGH", see_through, DDF_MainGetBoolean),
	DF("DISTANCE",  distance,  DDF_MainGetPercent),
	DF("SFX_START", sfx_start, DDF_MainLookupSound),
	DF("SFX_OPEN",  sfx_open,  DDF_MainLookupSound),
	DF("SFX_CLOSE", sfx_close, DDF_MainLookupSound),
	DF("SFX_STOP",  sfx_stop,  DDF_MainLookupSound),

	DDF_CMD_END
};

#undef  DDF_CMD_BASE
#define DDF_CMD_BASE  buffer_line

static const commandlist_t linedef_commands[] =
{
	// sub-commands
	DDF_SUB_LIST("FLOOR",    f, floor_commands,    buffer_floor),
	DDF_SUB_LIST("CEILING",  c, floor_commands,    buffer_floor),
	DDF_SUB_LIST("SLIDER",   s, slider_commands,   buffer_slider),
	DDF_SUB_LIST("LADDER",   ladder, ladder_commands, buffer_ladder),

	DF("NEWTRIGGER", newtrignum, DDF_MainGetNumeric),
	DF("ACTIVATORS", obj, DDF_LineGetActivators),
	DF("TYPE", type, DDF_LineGetTrigType),
	DF("KEYS", keys, DDF_LineGetSecurity),
	DF("FAILED_MESSAGE", failedmessage, DDF_MainGetString),
	DF("COUNT", count, DDF_MainGetNumeric),

	DF("DONUT", d.dodonut, DDF_MainGetBoolean),
	DF("DONUT_IN_SFX", d.d_sfxin, DDF_MainLookupSound),
	DF("DONUT_IN_SFXSTOP", d.d_sfxinstop, DDF_MainLookupSound),
	DF("DONUT_OUT_SFX", d.d_sfxout, DDF_MainLookupSound),
	DF("DONUT_OUT_SFXSTOP", d.d_sfxoutstop, DDF_MainLookupSound),

	DF("TELEPORT", t.teleport, DDF_MainGetBoolean),
	DF("TELEPORT_DELAY", t.delay, DDF_MainGetTime),
	DF("TELEIN_EFFECTOBJ", t.inspawnobj_ref, DDF_MainGetString),
	DF("TELEOUT_EFFECTOBJ", t.outspawnobj_ref, DDF_MainGetString),
	DF("TELEPORT_SPECIAL", t.special, DDF_LineGetTeleportSpecial),

	DF("LIGHT_TYPE", l.type, DDF_SectGetLighttype),
	DF("LIGHT_LEVEL", l.level, DDF_MainGetNumeric),
	DF("LIGHT_DARK_TIME", l.darktime, DDF_MainGetTime),
	DF("LIGHT_BRIGHT_TIME", l.brighttime, DDF_MainGetTime),
	DF("LIGHT_CHANCE", l.chance, DDF_MainGetPercent),
	DF("LIGHT_SYNC", l.sync, DDF_MainGetTime),
	DF("LIGHT_STEP", l.step, DDF_MainGetNumeric),
	DF("EXIT", e_exit, DDF_SectGetExit),
	DF("HUB_EXIT", hub_exit, DDF_MainGetNumeric),

	// FIXME: WTF
	{"SCROLL", DDF_LineGetScroller, &s_dir, NULL},
	{"SCROLLING_SPEED", DDF_MainGetFloat, &s_speed, NULL},

	DF("SCROLL_XSPEED", s_xspeed, DDF_MainGetFloat),
	DF("SCROLL_YSPEED", s_yspeed, DDF_MainGetFloat),
	DF("SCROLL_PARTS", scroll_parts, DDF_LineGetScrollPart),
	DF("USE_COLOURMAP", use_colourmap, DDF_MainGetColourmap),
	DF("GRAVITY", gravity, DDF_MainGetFloat),
	DF("FRICTION", friction, DDF_MainGetFloat),
	DF("VISCOSITY", viscosity, DDF_MainGetFloat),
	DF("DRAG", drag, DDF_MainGetFloat),
	DF("AMBIENT_SOUND", ambient_sfx, DDF_MainLookupSound),
	DF("ACTIVATE_SOUND", activate_sfx, DDF_MainLookupSound),
	DF("MUSIC", music, DDF_MainGetNumeric),
	DF("AUTO", autoline, DDF_MainGetBoolean),
	DF("SINGLESIDED", singlesided, DDF_MainGetBoolean),
	DF("EXTRAFLOOR_TYPE", ef.type, DDF_LineGetExtraFloor),
	DF("EXTRAFLOOR_CONTROL", ef.control, DDF_LineGetEFControl),
	DF("TRANSLUCENCY", translucency, DDF_MainGetPercent),
	DF("WHEN_APPEAR", appear, DDF_MainGetWhenAppear),
	DF("SPECIAL", special_flags, DDF_LineGetSpecialFlags),
	DF("RADIUS_TRIGGER", trigger_effect, DDF_LineGetRadTrig),
	DF("LINE_EFFECT", line_effect, DDF_LineGetLineEffect),
	DF("LINE_PARTS",  line_parts,  DDF_LineGetScrollPart),
	DF("SECTOR_EFFECT", sector_effect, DDF_LineGetSectorEffect),
	DF("PORTAL_TYPE",   portal_effect, DDF_LineGetPortalEffect),
	DF("SLOPE_TYPE", slope_type, DDF_LineGetSlopeType),
	DF("COLOUR", fx_color, DDF_MainGetRGB),

	// -AJA- backwards compatibility cruft...
	DF("CRUSH", ddf, DDF_LineMakeCrush),
	DF("SECSPECIAL", ddf, DDF_DummyFunction),

	DF("!EXTRAFLOOR_TRANSLUCENCY", translucency, DDF_MainGetPercent),
	DF("!SOUND", ddf, DDF_DummyFunction),
	DF("!LIGHT_PROBABILITY", ddf, DDF_DummyFunction),

	DDF_CMD_END
};


typedef struct
{
	const char *s;
	scrolldirs_e dir;
}
scroll_kludge_t;

static scroll_kludge_t s_scroll[] =
{
	{ "NONE",  dir_none  },
	{ "UP",    (scrolldirs_e)(dir_vert | dir_up) },
	{ "DOWN",  dir_vert  },
	{ "LEFT",  (scrolldirs_e)(dir_horiz | dir_left) },
	{ "RIGHT", dir_horiz },
	{ NULL,    dir_none  }
};


static struct  // FIXME: APPLIES TO NEXT 3 TABLES !
{
	const char *s;
	int n;
}

// FIXME: use keytype_names (in ddf_mobj.c)
s_keys[] =
{
	{ "NONE",           KF_NONE },

	{ "BLUE_CARD",      KF_BlueCard },
	{ "YELLOW_CARD",    KF_YellowCard },
	{ "RED_CARD",       KF_RedCard },
	{ "BLUE_SKULL",     KF_BlueSkull },
	{ "YELLOW_SKULL",   KF_YellowSkull },
	{ "RED_SKULL",      KF_RedSkull },
	{ "GREEN_CARD",     KF_GreenCard },
	{ "GREEN_SKULL",    KF_GreenSkull },

	{ "GOLD_KEY",       KF_GoldKey },
	{ "SILVER_KEY",     KF_SilverKey },
	{ "BRASS_KEY",      KF_BrassKey },
	{ "COPPER_KEY",     KF_CopperKey },
	{ "STEEL_KEY",      KF_SteelKey },
	{ "WOODEN_KEY",     KF_WoodenKey },
	{ "FIRE_KEY",       KF_FireKey },
	{ "WATER_KEY",      KF_WaterKey },

	// backwards compatibility
	{ "REQUIRES_ALL", KF_STRICTLY_ALL |
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

static bool LinedefStartEntry(const char *name)
{
	int number = MAX(0, atoi(name));

	if (number == 0)
		DDF_Error("Bad linedef number in lines.ddf: %s\n", name);

	epi::array_iterator_c it;
	linetype_c *existing = NULL;

	existing = linetypes.Lookup(number);
	if (existing)
	{
		dynamic_line = existing;
	}
	else
	{
		dynamic_line = new linetype_c;
		dynamic_line->ddf.number = number;
		linetypes.Insert(dynamic_line);
	}

	// instantiate the static entry
	buffer_line.Default();

	s_speed = 1.0f;
	s_dir = dir_none;

	return (existing != NULL);
}

static void LinedefParseField(const char *field, const char *contents,
							  int index, bool is_last)
{
#if (DEBUG_DDF)  
	I_Debugf("LINEDEF_PARSE: %s = %s;\n", field, contents);
#endif

	if (DDF_MainParseField(linedef_commands, field, contents))
		return;

	DDF_WarnError2(128, "Unknown lines.ddf command: %s\n", field);
}

static void LinedefFinishEntry(void)
{
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

	if (buffer_line.hub_exit > 0)
		buffer_line.e_exit = EXIT_Hub;

	// check stuff...

	if (buffer_line.ef.type != EXFL_None)
	{
		// AUTO is no longer needed for extrafloors
		buffer_line.autoline = false;

		if ((buffer_line.ef.type & EXFL_Flooder) && (buffer_line.ef.type & EXFL_NoShade))
		{
			DDF_WarnError2(129, "FLOODER and NOSHADE tags cannot be used together.\n");
			buffer_line.ef.type = (extrafloor_type_e)(buffer_line.ef.type & ~EXFL_Flooder);
		}

		if (! (buffer_line.ef.type & EXFL_Present))
		{
			DDF_WarnError2(129, "Extrafloor type missing THIN, THICK or LIQUID.\n");
			buffer_line.ef.type = EXFL_None;
		}
	}

	if (buffer_line.friction != FLO_UNUSED && buffer_line.friction < 0.05f)
	{
		DDF_WarnError2(129, "Friction value too low (%1.2f), it would prevent "
			"all movement.\n", buffer_line.friction);
		buffer_line.friction = 0.05f;
	}

	if (buffer_line.viscosity != FLO_UNUSED && buffer_line.viscosity > 0.95f)
	{
		DDF_WarnError2(129, "Viscosity value too high (%1.2f), it would prevent "
			"all movement.\n", buffer_line.viscosity);
		buffer_line.viscosity = 0.95f;
	}

	// FIXME: check more stuff...

	// transfer static entry to dynamic entry
	dynamic_line->CopyDetail(buffer_line);

	// compute CRC...
	// FIXME: Do something!
}

static void LinedefClearAll(void)
{
	// it is safe to just delete all the lines
	linetypes.Reset();
}

bool DDF_ReadLines(void *data, int size)
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

	return DDF_MainReadFile(&lines);
}

void DDF_LinedefInit(void)
{
	linetypes.Reset();
	
	// Insert the template line as the first entry, this is used
	// should the lookup fail	
	linetype_c *l;
	
	l = new linetype_c;
	l->Default();
	l->ddf.number = -1;
	linetypes.Insert(l);
}

void DDF_LinedefCleanUp(void)
{
	epi::array_iterator_c it;
	linetype_c *l;
	
	for (it=linetypes.GetBaseIterator(); it.IsValid(); it++)
	{
		l = ITERATOR_TO_TYPE(it, linetype_c*);

		cur_ddf_entryname = epi::STR_Format("[%d]  (lines.ddf)", l->ddf.number);

		l->t.inspawnobj = l->t.inspawnobj_ref ?
			mobjtypes.Lookup(l->t.inspawnobj_ref) : NULL;

		l->t.outspawnobj = l->t.outspawnobj_ref ?
			mobjtypes.Lookup(l->t.outspawnobj_ref) : NULL;

		cur_ddf_entryname.clear();
	}

	linetypes.Trim();
}

//
// Check for scroll types
//
void DDF_LineGetScroller(const char *info, void *storage)
{
	for (int i = 0; s_scroll[i].s; i++)
	{
		if (DDF_CompareName(info, s_scroll[i].s) == 0)
		{
			s_dir = (scrolldirs_e)(s_dir | s_scroll[i].dir);
			return;
		}
	}
	DDF_WarnError2(129, "Unknown scroll direction %s\n", info);
}

//
// Get Red/Blue/Yellow keys
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
		if (DDF_CompareName(info, s_keys[i].s) == 0)
		{
			buffer_line.keys = (keys_e)(buffer_line.keys | s_keys[i].n);

			if (required)
				buffer_line.keys = (keys_e)(buffer_line.keys | KF_STRICTLY_ALL);

			return;
		}
	}

	DDF_WarnError2(129, "Unknown key type %s\n", info);
}

//
// Check for walk/push/shoot
//
void DDF_LineGetTrigType(const char *info, void *storage)
{
	int i;

	for (i = sizeof(s_trigger) / sizeof(s_trigger[0]); i--;)
	{
		if (DDF_CompareName(info, s_trigger[i].s) == 0)
		{
#if 0  // DISABLED FOR NOW
			if (global_flags.edge_compat && (trigger_e)s_trigger[i].n == line_manual)
			{
				buffer_line.type = line_pushable;
				return;
			}
#endif
					
			buffer_line.type = (trigger_e)s_trigger[i].n;
			return;
		}
	}

	DDF_WarnError2(129, "Unknown Trigger type %s\n", info);
}

//
// Get player/monsters/missiles
//
void DDF_LineGetActivators(const char *info, void *storage)
{
	int i;

	for (i = sizeof(s_activators) / sizeof(s_activators[0]); i--;)
	{
		if (DDF_CompareName(info, s_activators[i].s) == 0)
		{
			buffer_line.obj = (trigacttype_e)(buffer_line.obj | s_activators[i].n);
			return;
		}
	}

	DDF_WarnError2(129,"Unknown Activator type %s\n", info);
}

static specflags_t extrafloor_types[] =
{
	// definers:
	{"THIN",          EF_DEF_THIN,       0},
	{"THICK",         EF_DEF_THICK,      0},
	{"LIQUID",        EF_DEF_LIQUID,     0},

	// modifiers:
	{"SEE_THROUGH",   EXFL_SeeThrough,   0},
	{"WATER",         EXFL_Water,        0},
	{"SHADE",         EXFL_NoShade,      1},
	{"FLOODER",       EXFL_Flooder,      0},
	{"SIDE_UPPER",    EXFL_SideUpper,    0},
	{"SIDE_LOWER",    EXFL_SideLower,    0},
	{"SIDE_MIDY",     EXFL_SideMidY,     0},
	{"BOOMTEX",       EXFL_BoomTex,      0},

	// backwards compatibility...
	{"FALL_THROUGH",  EXFL_Liquid, 0},
	{"SHOOT_THROUGH", 0, 0},
	{NULL, 0, 0}
};


//
// Gets the extra floor type(s).
//
// -AJA- 1999/06/21: written.
// -AJA- 2000/03/27: updated for simpler system.
//
void DDF_LineGetExtraFloor(const char *info, void *storage)
{
	extrafloor_type_e *type = (extrafloor_type_e *)storage;

	if (DDF_CompareName(info, "NONE") == 0)
	{
		*type = EXFL_None;
		return;
	}

	int value;

	switch (DDF_MainCheckSpecialFlag(info, extrafloor_types, &value, true))
	{
		case CHKF_Positive:
			*type = (extrafloor_type_e)(*type | value);
			break;

		case CHKF_Negative:
			*type = (extrafloor_type_e)(*type & ~value);
			break;

		default:
			DDF_WarnError2(129, "Unknown Extrafloor Type: %s\n", info);
			break;
	}
}

static specflags_t ef_control_types[] =
{
	{"NONE",   EFCTL_None,   0},
	{"REMOVE", EFCTL_Remove, 0},
	{NULL, 0, 0}
};


void DDF_LineGetEFControl(const char *info, void *storage)
{
	extrafloor_control_e *control = (extrafloor_control_e *)storage;

	int value;

	switch (DDF_MainCheckSpecialFlag(info, ef_control_types, &value))
	{
		case CHKF_Positive:
		case CHKF_Negative:
			*control = (extrafloor_control_e)(value);
			break;

		default:
			DDF_WarnError("Unknown CONTROL_EXTRAFLOOR tag: %s", info);
			break;
	}
}

#define TELSP_AllSame  \
    ((teleportspecial_e)(TELSP_Relative | TELSP_SameHeight | \
	  TELSP_SameSpeed | TELSP_SameOffset))

#define TELSP_Preserve  \
    ((teleportspecial_e)(TELSP_SameAbsDir | TELSP_SameHeight | TELSP_SameSpeed))

static specflags_t teleport_specials[] =
{
	{"RELATIVE",   TELSP_Relative, 0},
	{"SAME_HEIGHT",TELSP_SameHeight, 0},
	{"SAME_SPEED", TELSP_SameSpeed, 0},
	{"SAME_OFFSET",TELSP_SameOffset, 0},
	{"ALL_SAME",   TELSP_AllSame, 0},

	{"LINE",       TELSP_Line, 0},
	{"FLIPPED",    TELSP_Flipped, 0},
	{"SILENT",     TELSP_Silent, 0},

	// these modes are deprecated (kept for B.C.)
	// FIXME: show a warning if used (cannot use "!" prefix)
	{"SAME_DIR",   TELSP_SameAbsDir, 0},
	{"ROTATE",     TELSP_Rotate, 0},
	{"PRESERVE",   TELSP_Preserve, 0},

	{NULL, 0, 0}
};

//
// Gets the teleporter special flags.
//
// -AJA- 1999/07/12: written.
//
void DDF_LineGetTeleportSpecial(const char *info, void *storage)
{
	teleportspecial_e *dest = (teleportspecial_e *)storage;

	int value;

	switch (DDF_MainCheckSpecialFlag(info, teleport_specials, &value, true))
	{
		case CHKF_Positive:
			*dest = (teleportspecial_e)(*dest | value);
			break;

		case CHKF_Negative:
			*dest = (teleportspecial_e)(*dest & ~value);
			break;

		default:
			DDF_WarnError("DDF_LineGetTeleportSpecial: Unknown Special: %s\n", info);
			break;
	}
}

static specflags_t scrollpart_specials[] =
{
	{"RIGHT_UPPER", SCPT_RightUpper, 0},
	{"RIGHT_MIDDLE", SCPT_RightMiddle, 0},
	{"RIGHT_LOWER", SCPT_RightLower, 0},
	{"RIGHT", SCPT_RIGHT, 0},
	{"LEFT_UPPER", SCPT_LeftUpper, 0},
	{"LEFT_MIDDLE", SCPT_LeftMiddle, 0},
	{"LEFT_LOWER", SCPT_LeftLower, 0},
	{"LEFT", SCPT_LEFT, 0},
	{"LEFT_REVERSE_X", SCPT_LeftRevX, 0},
	{"LEFT_REVERSE_Y", SCPT_LeftRevY, 0},
	{NULL, 0, 0}
};

//
// Gets the scroll part flags.
//
// -AJA- 1999/07/12: written.
//
void DDF_LineGetScrollPart(const char *info, void *storage)
{
	scroll_part_e *dest = (scroll_part_e *)storage;

	if (DDF_CompareName(info, "NONE") == 0)
	{
		(*dest) = SCPT_None;
		return;
	}

	int value;

	switch (DDF_MainCheckSpecialFlag(info, scrollpart_specials, &value, true))
	{
		case CHKF_Positive:
			(*dest) = (scroll_part_e)((*dest) | value);
			break;

		case CHKF_Negative:
			(*dest) = (scroll_part_e)((*dest) & ~value);
			break;

		default:
			DDF_WarnError("DDF_LineGetScrollPart: Unknown Part: %s", info);
			break;
	}
}

//----------------------------------------------------------------------------

static specflags_t line_specials[] =
{
	{"MUST_REACH",      LINSP_MustReach, 0},
	{"SWITCH_SEPARATE", LINSP_SwitchSeparate, 0},
	{"BACK_SECTOR",     LINSP_BackSector, 0},
	{NULL, 0, 0}
};

//
// Gets the line special flags.
//
void DDF_LineGetSpecialFlags(const char *info, void *storage)
{
	line_special_e *flags = (line_special_e *)storage;

	int value;

	switch (DDF_MainCheckSpecialFlag(info, line_specials, &value, true))
	{
	case CHKF_Positive:
		*flags = (line_special_e)(*flags | value);
		break;

	case CHKF_Negative:
		*flags = (line_special_e)(*flags & ~value);
		break;

	default:
		DDF_WarnError2(129, "Unknown line special: %s", info);
		break;
	}
}

//
// Gets the line's radius trigger effect.
//
static void DDF_LineGetRadTrig(const char *info, void *storage)
{
	int *trig = (int *)storage;

	if (DDF_CompareName(info, "ENABLE_TAGGED") == 0)
	{
		*trig = +1;
		return;
	}
	else if (DDF_CompareName(info, "DISABLE_TAGGED") == 0)
	{
		*trig = -1;
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

static specflags_t line_effect_names[] =
{
	{"TRANSLUCENT",    LINEFX_Translucency, 0},
	{"VECTOR_SCROLL",  LINEFX_VectorScroll, 0},
	{"OFFSET_SCROLL",  LINEFX_OffsetScroll, 0},

	{"SCALE_TEX",      LINEFX_Scale,         0},
	{"SKEW_TEX",       LINEFX_Skew,          0},
	{"LIGHT_WALL",     LINEFX_LightWall,     0},

	{"UNBLOCK_THINGS", LINEFX_UnblockThings, 0},
	{"BLOCK_SHOTS",    LINEFX_BlockShots,    0},
	{"BLOCK_SIGHT",    LINEFX_BlockSight,    0},
	{NULL, 0, 0}
};

//
// Gets the line effect flags.
//
static void DDF_LineGetLineEffect(const char *info, void *storage)
{
	line_effect_type_e *dest = (line_effect_type_e *)storage;

	if (DDF_CompareName(info, "NONE") == 0)
	{
		*dest = LINEFX_NONE;
		return;
	}

	int value;

	switch (DDF_MainCheckSpecialFlag(info, line_effect_names,
		&value, true, false))
	{
		case CHKF_Positive:
			*dest = (line_effect_type_e)(*dest | value);
			break;

		case CHKF_Negative:
			*dest = (line_effect_type_e)(*dest & ~value);
			break;

		default:
			DDF_WarnError("Unknown line effect type: %s", info);
			break;
	}
}

static specflags_t sector_effect_names[] =
{
	{"LIGHT_FLOOR",     SECTFX_LightFloor,     0},
	{"LIGHT_CEILING",   SECTFX_LightCeiling,   0},
	{"SCROLL_FLOOR",    SECTFX_ScrollFloor,    0},
	{"SCROLL_CEILING",  SECTFX_ScrollCeiling,  0},
	{"PUSH_THINGS",     SECTFX_PushThings,     0},

	{"SET_FRICTION",    SECTFX_SetFriction,    0},
	{"WIND_FORCE",      SECTFX_WindForce,      0},
	{"CURRENT_FORCE",   SECTFX_CurrentForce,   0},
	{"POINT_FORCE",     SECTFX_PointForce,     0},

	{"RESET_FLOOR",     SECTFX_ResetFloor,     0},
	{"RESET_CEILING",   SECTFX_ResetCeiling,   0},
	{"ALIGN_FLOOR",     SECTFX_AlignFloor,     0},
	{"ALIGN_CEILING",   SECTFX_AlignCeiling,   0},
	{"SCALE_FLOOR",     SECTFX_ScaleFloor,     0},
	{"SCALE_CEILING",   SECTFX_ScaleCeiling,   0},
	{NULL, 0, 0}
};

//
// Gets the sector effect flags.
//
static void DDF_LineGetSectorEffect(const char *info, void *storage)
{
	sector_effect_type_e *sect_fx = (sector_effect_type_e *)storage;

	if (DDF_CompareName(info, "NONE") == 0)
	{
		*sect_fx = SECTFX_None;
		return;
	}

	int value;

	switch (DDF_MainCheckSpecialFlag(info, sector_effect_names, &value, true, false))
	{
		case CHKF_Positive:
			*sect_fx = (sector_effect_type_e)(*sect_fx | value);
			break;

		case CHKF_Negative:
			*sect_fx = (sector_effect_type_e)(*sect_fx & ~value);
			break;

		default:
			DDF_WarnError("Unknown sector effect type: %s", info);
			break;
	}
}

static specflags_t portal_effect_names[] =
{
	{"STANDARD",   PORTFX_Standard,  0},
	{"MIRROR",     PORTFX_Mirror,    0},
	{"CAMERA",     PORTFX_Camera,    0},

	{NULL, 0, 0}
};

//
// Gets the portal effect flags.
//
static void DDF_LineGetPortalEffect(const char *info, void *storage)
{
	portal_effect_type_e *port_fx = (portal_effect_type_e *)storage;

	if (DDF_CompareName(info, "NONE") == 0)
	{
		*port_fx = PORTFX_None;
		return;
	}

	int value;

	switch (DDF_MainCheckSpecialFlag(info, portal_effect_names, &value, true, false))
	{
		case CHKF_Positive:
			*port_fx = (portal_effect_type_e)(*port_fx | value);
			break;

		case CHKF_Negative:
			*port_fx = (portal_effect_type_e)(*port_fx & ~value);
			break;

		default:
			DDF_WarnError("Unknown portal type: %s", info);
			break;
	}
}

static specflags_t slope_type_names[] =
{
	{"FAKE_FLOOR",   SLP_DetailFloor,   0},
	{"FAKE_CEILING", SLP_DetailCeiling, 0},

	{NULL, 0, 0}
};

static void DDF_LineGetSlopeType(const char *info, void *storage)
{
	slope_type_e *sltype = (slope_type_e *)storage;

	if (DDF_CompareName(info, "NONE") == 0)
	{
		*sltype = SLP_NONE;
		return;
	}

	int value;

	switch (DDF_MainCheckSpecialFlag(info, slope_type_names, &value, true))
	{
		case CHKF_Positive:
			*sltype = (slope_type_e)(*sltype | value);
			break;

		case CHKF_Negative:
			*sltype = (slope_type_e)(*sltype & ~value);
			break;

		default:
			DDF_WarnError("Unknown slope type: %s", info);
			break;
	}
}

static void DDF_LineMakeCrush(const char *info, void *storage)
{
	// FIXME
	buffer_line.f.crush_damage = 10;
	buffer_line.c.crush_damage = 10;
}


//----------------------------------------------------------------------------


// --> Donut definition class

//
// donutdef_c Constructor
//
donutdef_c::donutdef_c()
{
}

//
// donutdef_c Copy constructor
//
donutdef_c::donutdef_c(donutdef_c &rhs)
{
	Copy(rhs);
}

//
// donutdef_c Destructor
//
donutdef_c::~donutdef_c()
{
}

//
// donutdef_c::Copy()
//
void donutdef_c::Copy(donutdef_c &src)
{
	dodonut = src.dodonut;

	// FIXME! Strip out the d_ since we're not trying to
	// to differentiate them now?
	d_sfxin = src.d_sfxin; 
	d_sfxinstop = src.d_sfxinstop;
	d_sfxout = src.d_sfxout; 
	d_sfxoutstop = src.d_sfxoutstop;
}

//
// donutdef_c::Default()
//
void donutdef_c::Default()
{
	dodonut = false;
	d_sfxin = NULL;
	d_sfxinstop = NULL;
	d_sfxout = NULL; 
	d_sfxoutstop = NULL;
}

//
// donutdef_c assignment operator
//
donutdef_c& donutdef_c::operator=(donutdef_c &rhs)
{
	if(&rhs != this)
		Copy(rhs);

	return *this;
}


// --> Extrafloor definition class

//
// extrafloordef_c Constructor
//
extrafloordef_c::extrafloordef_c()
{
}

//
// extrafloordef_c Copy constructor
//
extrafloordef_c::extrafloordef_c(extrafloordef_c &rhs)
{
	Copy(rhs);
}

//
// extrafloordef_c Destructor
//
extrafloordef_c::~extrafloordef_c()
{
}

//
// extrafloordef_c::Copy()
//
void extrafloordef_c::Copy(extrafloordef_c &src)
{
	control = src.control;
	type = src.type;
}

//
// extrafloordef_c::Default()
//
void extrafloordef_c::Default()
{
	control = EFCTL_None;
	type = EXFL_None;
}

//
// extrafloordef_c assignment operator
//
extrafloordef_c& extrafloordef_c::operator=(extrafloordef_c &rhs)
{
	if(&rhs != this)
		Copy(rhs);

	return *this;
}


// --> Ladder definition class

//
// ladderdef_c Constructor
//
ladderdef_c::ladderdef_c()
{
}

//
// ladderdef_c Copy constructor
//
ladderdef_c::ladderdef_c(ladderdef_c &rhs)
{
	Copy(rhs);
}

//
// ladderdef_c Destructor
//
ladderdef_c::~ladderdef_c()
{
}

//
// ladderdef_c::Copy()
//
void ladderdef_c::Copy(ladderdef_c &src)
{
	height = src.height;
}

//
// ladderdef_c::Default()
//
void ladderdef_c::Default()
{
	height = 0.0f;
}

//
// ladderdef_c assignment operator
//
ladderdef_c& ladderdef_c::operator=(ladderdef_c &rhs)
{
	if(&rhs != this)
		Copy(rhs);

	return *this;
}

// --> Light effect definition class

//
// lightdef_c Constructor
//
lightdef_c::lightdef_c()
{
}

//
// lightdef_c Copy constructor
//
lightdef_c::lightdef_c(lightdef_c &rhs)
{
	Copy(rhs);
}

//
// lightdef_c Destructor
//
lightdef_c::~lightdef_c()
{
}

//
// lightdef_c::Copy()
//
void lightdef_c::Copy(lightdef_c &src)
{
	type = src.type;
	level = src.level;
	chance = src.chance;
	darktime = src.darktime;
	brighttime = src.brighttime;
	sync = src.sync;
	step = src.step;
}

//
// lightdef_c::Default()
//
void lightdef_c::Default()
{
	type = LITE_None;
	level = 64;
	chance = PERCENT_MAKE(50);
	darktime = 0;
	brighttime = 0;
	sync = 0;
	step = 8;
}

//
// lightdef_c assignment operator
//
lightdef_c& lightdef_c::operator=(lightdef_c &rhs)
{
	if(&rhs != this)
		Copy(rhs);

	return *this;
}

// --> Moving plane definition class

//
// movplanedef_c Constructor
//
movplanedef_c::movplanedef_c()
{
}

//
// movplanedef_c Copy constructor
//
movplanedef_c::movplanedef_c(movplanedef_c &rhs)
{
	Copy(rhs);
}

//
// movplanedef_c Destructor
//
movplanedef_c::~movplanedef_c()
{
}

//
// movplanedef_c::Copy()
//
void movplanedef_c::Copy(movplanedef_c &src)
{
	type = src.type;
	is_ceiling = src.is_ceiling;
	speed_up = src.speed_up;
	speed_down = src.speed_down;
	destref = src.destref;
	dest = src.dest;
	otherref = src.otherref;
	other = src.other;
	crush_damage = src.crush_damage;
	tex = src.tex;
	wait = src.wait;
    prewait = src.prewait;
	sfxstart = src.sfxstart;
	sfxup = src.sfxup;
	sfxdown = src.sfxdown;
	sfxstop = src.sfxstop;
	scroll_angle = src.scroll_angle;
	scroll_speed = src.scroll_speed;
	ignore_texture = src.ignore_texture;
}

//
// movplanedef_c::Default()
//
void movplanedef_c::Default(movplanedef_c::default_e def)
{
	type = mov_undefined;

	if (def == DEFAULT_CeilingLine || def == DEFAULT_CeilingSect)
		is_ceiling = true;
	else
		is_ceiling = false;

	switch (def)
	{
		case DEFAULT_CeilingLine:
		case DEFAULT_FloorLine:
		{
			speed_up = -1;
			speed_down = -1;
			break;
		}
		
		case DEFAULT_DonutFloor:
		{
			speed_up = FLOORSPEED/2;
			speed_down = FLOORSPEED/2;
			break;
		}
		
		default:
		{
			speed_up = 0;
			speed_down = 0;
			break;
		}
	}
	
	destref = REF_Absolute;

	// FIXME!!! Why are we using INT_MAX with a fp number?
	dest = (def != DEFAULT_DonutFloor) ? 0.0f : (float)INT_MAX;
	
	switch (def)
	{
		case DEFAULT_CeilingLine:
		{
			otherref = (heightref_e)(REF_Current|REF_CEILING);
			break;
		}
		
		case DEFAULT_FloorLine:
		{
			otherref = (heightref_e)(REF_Surrounding|REF_HIGHEST|REF_INCLUDE);
			break;
		}
		
		default:
		{
			otherref = REF_Absolute;
			break;
		}
	}
	
	// FIXME!!! Why are we using INT_MAX with a fp number?
	other = (def != DEFAULT_DonutFloor) ? 0.0f : (float)INT_MAX;

	crush_damage = 0;

	tex.clear();
	
	wait = 0;
	prewait = 0;
	
	sfxstart = NULL;
	sfxup = NULL;
	sfxdown = NULL;
	sfxstop = NULL;
	
	scroll_angle = 0;
	scroll_speed = 0.0f;

	ignore_texture = false;
}

//
// movplanedef_c assignment operator
//
movplanedef_c& movplanedef_c::operator=(movplanedef_c &rhs)
{
	if(&rhs != this)
		Copy(rhs);

	return *this;
}

// --> Sliding door definition class

//
// sliding_door_c Constructor
//
sliding_door_c::sliding_door_c()
{
}

//
// sliding_door_c Copy constructor
//
sliding_door_c::sliding_door_c(sliding_door_c &rhs)
{
	Copy(rhs);
}

//
// sliding_door_c Destructor
//
sliding_door_c::~sliding_door_c()
{
}

//
// sliding_door_c::Copy()
//
void sliding_door_c::Copy(sliding_door_c &src)
{
	type = src.type;
	speed = src.speed;
	wait = src.wait;
	see_through = src.see_through;
	distance = src.distance;
	sfx_start = src.sfx_start;
	sfx_open = src.sfx_open;	
	sfx_close = src.sfx_close;
	sfx_stop = src.sfx_stop;
}

//
// sliding_door_c::Default()
//
void sliding_door_c::Default()
{
	type = SLIDE_None;   
	speed =4.0f;          
	wait = 150;          
	see_through = false;        
	distance = PERCENT_MAKE(90); 
	sfx_start =	sfx_None;
	sfx_open = sfx_None;
	sfx_close = sfx_None;
	sfx_stop = sfx_None;
}

//
// sliding_door_c assignment operator
//
sliding_door_c& sliding_door_c::operator=(sliding_door_c &rhs)
{
	if(&rhs != this)
		Copy(rhs);

	return *this;
}

// --> Teleport point definition class

//
// teleportdef_c Constructor
//
teleportdef_c::teleportdef_c()
{
}

//
// teleportdef_c Copy constructor
//
teleportdef_c::teleportdef_c(teleportdef_c &rhs)
{
	Copy(rhs);
}

//
// teleportdef_c Destructor
//
teleportdef_c::~teleportdef_c()
{
}

//
// teleportdef_c::Copy()
//
void teleportdef_c::Copy(teleportdef_c &src)
{
	teleport = src.teleport;
	
	inspawnobj = src.inspawnobj;	
	inspawnobj_ref = src.inspawnobj_ref;
	
	outspawnobj = src.outspawnobj;	
	outspawnobj_ref = src.outspawnobj_ref;

	special = src.special;
	delay = src.delay;
}

//
// teleportdef_c::Default()
//
void teleportdef_c::Default()
{
	teleport = false;
	
	inspawnobj = NULL;	
	inspawnobj_ref.clear();
	
	outspawnobj = NULL;      	
	outspawnobj_ref.clear(); 

	delay = 0;
	special = TELSP_None;
}

//
// teleportdef_c assignment operator
//
teleportdef_c& teleportdef_c::operator=(teleportdef_c &rhs)
{
	if(&rhs != this)
		Copy(rhs);

	return *this;
}

// --> Line definition type class

//
// linetype_c Constructor
//
linetype_c::linetype_c()
{
	Default();
}

linetype_c::linetype_c(linetype_c &rhs)
{
	Copy(rhs);
}

//
// linetype_c Destructor
//
linetype_c::~linetype_c()
{
}
	

void linetype_c::Copy(linetype_c &src)
{
	ddf = src.ddf;
	CopyDetail(src);
}


void linetype_c::CopyDetail(linetype_c &src)
{
	newtrignum = src.newtrignum;
	type = src.type;
	obj = src.obj;
	keys = src.keys;
	count = src.count;

	f = src.f;
	c = src.c;
	d = src.d;
	s = src.s;
	t = src.t;
	l = src.l;

	ladder = src.ladder;
	e_exit = src.e_exit;
	hub_exit = src.hub_exit;
	s_xspeed = src.s_xspeed;
	s_yspeed = src.s_yspeed;
	scroll_parts = src.scroll_parts;

	failedmessage = src.failedmessage;

	use_colourmap = src.use_colourmap;
	gravity = src.gravity;
	friction = src.friction;
	viscosity = src.viscosity;
	drag = src.drag;
	ambient_sfx = src.ambient_sfx;
	activate_sfx = src.activate_sfx;
	music = src.music;
	autoline = src.autoline;
	singlesided = src.singlesided;
	ef = src.ef;
	translucency = src.translucency;
	appear = src.appear;

	special_flags = src.special_flags;
	trigger_effect = src.trigger_effect;
	line_effect = src.line_effect;
	line_parts = src.line_parts;
	sector_effect = src.sector_effect;
	portal_effect = src.portal_effect;
	slope_type = src.slope_type;
	fx_color = src.fx_color;
}


void linetype_c::Default(void)
{
	ddf.Default();

	newtrignum = 0;
	type = line_none;
	obj = trig_none;
	keys = KF_NONE;
	count = -1;

	f.Default(movplanedef_c::DEFAULT_FloorLine);		
	c.Default(movplanedef_c::DEFAULT_CeilingLine);		

	d.Default();		// Donut
	s.Default();		// Sliding Door
	
	t.Default();		// Teleport
	l.Default();		// Light definition
	
	ladder.Default();	// Ladder
	
	e_exit = EXIT_None;
	hub_exit = 0;
	s_xspeed = 0.0f;
	s_yspeed = 0.0f;
	scroll_parts = SCPT_None;
	
	failedmessage.clear();

	use_colourmap = NULL;
	gravity = FLO_UNUSED;
	friction = FLO_UNUSED;
	viscosity = FLO_UNUSED;
	drag = FLO_UNUSED;
	ambient_sfx = sfx_None;
	activate_sfx = sfx_None;
	music = 0;
	autoline = false;
	singlesided = false;

	ef.Default();
	
	translucency = PERCENT_MAKE(100);
	appear = DEFAULT_APPEAR;    
	special_flags = LINSP_None;
	trigger_effect = 0;
	line_effect = LINEFX_NONE;
	line_parts = SCPT_None;
	sector_effect = SECTFX_None;
	portal_effect = PORTFX_None;
	slope_type = SLP_NONE;
	fx_color = RGB_MAKE(0,0,0);
}

//
// linetype_c assignment operator
//
linetype_c& linetype_c::operator=(linetype_c &rhs)
{
	if (&rhs != this)
		Copy(rhs);
		
	return *this;
}

// --> Line definition type container class

//
// linetype_container_c Constructor
//
linetype_container_c::linetype_container_c() : 
	epi::array_c(sizeof(linetype_c*))
{
	Reset();
}

//
// linetype_container_c Destructor
//
linetype_container_c::~linetype_container_c()
{
	Clear();
}

void linetype_container_c::CleanupObject(void *obj)
{
	linetype_c *l = *(linetype_c**)obj;

	if (l)
		delete l;

	return;
}

//
// Looks an linetype by id, returns NULL if line can't be found.
//
linetype_c* linetype_container_c::Lookup(const int id)
{
	int slot = DDF_LineHashFunc(id);

	// check the cache
	if (lookup_cache[slot] &&
		lookup_cache[slot]->ddf.number == id)
	{
		return lookup_cache[slot];
	}

	epi::array_iterator_c it;
	linetype_c *l = 0;

	for (it = GetTailIterator(); it.IsValid(); it--)
	{
		l = ITERATOR_TO_TYPE(it, linetype_c*);
		if (l->ddf.number == id)
		{
			break;
		}
	}

	// FIXME!! Throw an epi::error here
	if (!it.IsValid())
		return NULL;

	// update the cache
	lookup_cache[slot] = l;
	return l;
}

//
// Clears down both the data and the cache
//
void linetype_container_c::Reset()
{
	Clear();
	memset(lookup_cache, 0, sizeof(linetype_c*) * LOOKUP_CACHESIZE);
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
