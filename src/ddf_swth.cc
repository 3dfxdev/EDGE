//----------------------------------------------------------------------------
//  EDGE Data Definition File Codes (Switch textures)
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
// Switch Texture Setup and Parser Code
//

#include "i_defs.h"

#include "ddf_locl.h"
#include "ddf_main.h"
#include "dm_state.h"
#include "p_local.h"
#include "p_spec.h"
#include "z_zone.h"

#undef  DF
#define DF  DDF_CMD

static switchdef_t buffer_switchdef;
static switchdef_t *dynamic_switchdef;

static const switchdef_t template_switchdef =
{
	DDF_BASE_NIL,  // ddf
	"",    // name1;
	"",    // name2;
	sfx_None,  // on_sfx;
	sfx_None,  // off_sfx;
	BUTTONTIME, // time
	{{0,0}}  // cache
};

switchdef_container_c switchdefs;

#undef  DDF_CMD_BASE
#define DDF_CMD_BASE  buffer_switchdef

static const commandlist_t switch_commands[] =
{
	DF("ON TEXTURE",  name1, DDF_MainGetInlineStr10),
	DF("OFF TEXTURE", name2, DDF_MainGetInlineStr10),
	DF("ON SOUND",  on_sfx,  DDF_MainLookupSound),
	DF("OFF SOUND", off_sfx, DDF_MainLookupSound),
	DF("TIME", time, DDF_MainGetTime),

	// -AJA- backwards compatibility cruft...
	DF("!SOUND", on_sfx, DDF_MainLookupSound),

	DDF_CMD_END
};


//
//  DDF PARSE ROUTINES
//

static bool SwitchStartEntry(const char *name)
{
	switchdef_t *existing = NULL;

	if (name && name[0])
		existing = switchdefs.Find(name);
	
	if (existing)
	{
		dynamic_switchdef = existing;	
	}	
	else
	{
		dynamic_switchdef = new switchdef_t;
		memset(dynamic_switchdef, 0, sizeof(switchdef_t));
		switchdefs.Insert(dynamic_switchdef);
	}
	
	dynamic_switchdef->ddf.number = 0;

	// instantiate the static entry
	buffer_switchdef = template_switchdef;

	return (existing != NULL);
}

static void SwitchParseField(const char *field, const char *contents,
		int index, bool is_last)
{
#if (DEBUG_DDF)  
	L_WriteDebug("SWITCH_PARSE: %s = %s;\n", field, contents);
#endif

	if (! DDF_MainParseField(switch_commands, field, contents))
		DDF_WarnError2(0x128, "Unknown switch.ddf command: %s\n", field);
}

static void SwitchFinishEntry(void)
{
	ddf_base_t base;

	if (!buffer_switchdef.name1[0])
		DDF_Error("Missing first name for switch.\n");

	if (!buffer_switchdef.name2[0])
		DDF_Error("Missing last name for switch.\n");

	if (buffer_switchdef.time <= 0)
		DDF_Error("Bad time value for switch: %d\n", buffer_switchdef.time);

	// transfer static entry to dynamic entry

	base = dynamic_switchdef->ddf;
	dynamic_switchdef[0] = buffer_switchdef;
	dynamic_switchdef->ddf = base;

	// Compute CRC.  In this case, there is no need, since switch
	// textures have zero impact on the game simulation.
	dynamic_switchdef->ddf.crc = 0;
}

static void SwitchClearAll(void)
{
	// safe here to delete all switches
	switchdefs.Clear();
}


void DDF_ReadSW(void *data, int size)
{
#if (DEBUG_DDF)
	epi::array_iterator_c it;
	switchdef_t *sw;
#endif

	readinfo_t switches;

	switches.memfile = (char*)data;
	switches.memsize = size;
	switches.tag = "SWITCHES";
	switches.entries_per_dot = 2;

	if (switches.memfile)
	{
		switches.message = NULL;
		switches.filename = NULL;
		switches.lumpname = "DDFSWTH";
	}
	else
	{
		switches.message = "DDF_InitSwitches";
		switches.filename = "switch.ddf";
		switches.lumpname = NULL;
	}

	switches.start_entry  = SwitchStartEntry;
	switches.parse_field  = SwitchParseField;
	switches.finish_entry = SwitchFinishEntry;
	switches.clear_all    = SwitchClearAll;

	DDF_MainReadFile(&switches);

#if (DEBUG_DDF)
	L_WriteDebug("DDF_ReadSW: Switch List:\n");

	for (it = switchdefs.GetBaseIterator(); it.IsValid(); it++)
	{
		sw = ITERATOR_TO_TYPE(it, switchdef_t*);
		
		L_WriteDebug("  Num: %d  ON: '%s'  OFF: '%s'\n", 
						i, sw->name1, sw->name2);
	}
#endif
}

//
// DDF_SWInit
//
void DDF_SWInit(void)
{
	switchdefs.Clear();
}

//
// DDF_SWCleanUp
//
void DDF_SWCleanUp(void)
{
	switchdefs.Trim();
}

//
// switchdef_container_c::CleanupObject()
//
void switchdef_container_c::CleanupObject(void *obj)
{
	switchdef_t *sw = *(switchdef_t**)obj;

	if (sw)
	{
		if (sw->ddf.name) { Z_Free(sw->ddf.name); }
		delete sw;
	}

	return;
}

//
// switchdef_t* switchdef_container_c::Find()
//
switchdef_t* switchdef_container_c::Find(const char *name)
{
	epi::array_iterator_c it;
	switchdef_t *sw;

	for (it = GetBaseIterator(); it.IsValid(); it++)
	{
		sw = ITERATOR_TO_TYPE(it, switchdef_t*);
		if (DDF_CompareName(sw->ddf.name, name) == 0)
			return sw;
	}

	return NULL;
}
