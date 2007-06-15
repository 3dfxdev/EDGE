//----------------------------------------------------------------------------
//  EDGE Data Definition File Codes (Switch textures)
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
// Switch Texture Setup and Parser Code
//

#include "i_defs.h"

#include <string.h>

#include "ddf_locl.h"
#include "ddf_main.h"
#include "ddf_swth.h"

#include "dm_state.h"
#include "p_local.h"
#include "p_spec.h"

#undef  DF
#define DF  DDF_CMD

static switchdef_c buffer_switchdef;
static switchdef_c *dynamic_switchdef;

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
	switchdef_c *existing = NULL;

	if (name && name[0])
		existing = switchdefs.Find(name);
	
	if (existing)
	{
		dynamic_switchdef = existing;	
	}	
	else
	{
		dynamic_switchdef = new switchdef_c;
		
		if (name && name[0])
			dynamic_switchdef->ddf.name.Set(name);
		else
			dynamic_switchdef->ddf.SetUniqueName("UNNAMED_SWITCH", switchdefs.GetSize());
		
		switchdefs.Insert(dynamic_switchdef);
	}
	
	dynamic_switchdef->ddf.number = 0;

	// instantiate the static entry
	buffer_switchdef.Default();

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
	if (!buffer_switchdef.name1[0])
		DDF_Error("Missing first name for switch.\n");

	if (!buffer_switchdef.name2[0])
		DDF_Error("Missing last name for switch.\n");

	if (buffer_switchdef.time <= 0)
		DDF_Error("Bad time value for switch: %d\n", buffer_switchdef.time);

	// transfer static entry to dynamic entry
	dynamic_switchdef->CopyDetail(buffer_switchdef);

	// Compute CRC.  In this case, there is no need, since switch
	// textures have zero impact on the game simulation.
	dynamic_switchdef->ddf.crc.Reset();
}

static void SwitchClearAll(void)
{
	// safe here to delete all switches
	switchdefs.Clear();
}


bool DDF_ReadSwitch(void *data, int size)
{
#if (DEBUG_DDF)
	epi::array_iterator_c it;
	switchdef_c *sw;
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

	if (! DDF_MainReadFile(&switches))
		return false;

#if (DEBUG_DDF)
	L_WriteDebug("DDF_ReadSW: Switch List:\n");

	for (it = switchdefs.GetBaseIterator(); it.IsValid(); it++)
	{
		sw = ITERATOR_TO_TYPE(it, switchdef_c*);
		
		L_WriteDebug("  Num: %d  ON: '%s'  OFF: '%s'\n", 
						i, sw->name1, sw->name2);
	}
#endif

	return true;
}

//
// DDF_SwitchInit
//
void DDF_SwitchInit(void)
{
	switchdefs.Clear();
}

//
// DDF_SwitchCleanUp
//
void DDF_SwitchCleanUp(void)
{
	switchdefs.Trim();
}

void DDF_ParseSWITCHES(const byte *data, int size)
{
	for (; size >= 20; data += 20, size -= 20)
	{
		if (data[18] == 0)  // end marker
			break;

		char  on_name[10];
		char off_name[10];

		// make sure names are NUL-terminated
		memcpy( on_name, data+9, 9);  on_name[8] = 0;
		memcpy(off_name, data+0, 9); off_name[8] = 0;

		L_WriteDebug("- SWITCHES LUMP: off '%s' : on '%s'\n", off_name, on_name);
				
		// ignore zero-length names
		if (!off_name[0] || !on_name[0])
			continue;

		switchdef_c *def = new switchdef_c;

		def->ddf.SetUniqueName("BOOM_SWITCH", switchdefs.GetSize());
		def->ddf.number = 0;
		def->ddf.crc.Reset();

		def->Default();
		
		def->name1.Set( on_name);
		def->name2.Set(off_name);

		switchdefs.Insert(def);
	}
}


// ---> switchdef_c class

//
// switchdef_c Constructor
//
switchdef_c::switchdef_c()
{
	Default();
}

//
// switchdef_c Copy constructor
//
switchdef_c::switchdef_c(switchdef_c &rhs)
{
	Copy(rhs);
}

//
// switchdef_c::Copy()
//
void switchdef_c::Copy(switchdef_c &src)
{
	ddf = src.ddf;
	CopyDetail(src);
}

//
// switchdef_c::CopyDetail()
//
// Copies all the detail with the exception of ddf info
//
void switchdef_c::CopyDetail(switchdef_c &src)
{
	name1 = src.name1;
	name2 = src.name2;

	on_sfx = src.on_sfx;
	off_sfx = src.off_sfx;

	time = src.time;
}

//
// switchdef_c::Default()
//
void switchdef_c::Default()
{
	ddf.Default();

	name1.Clear();
	name2.Clear();

	on_sfx = sfx_None;
	off_sfx = sfx_None;

	time = BUTTONTIME;
}

//
// switchdef_c assignment operator
//
switchdef_c& switchdef_c::operator=(switchdef_c &rhs)
{
	if (&rhs != this)
		Copy(rhs);
	
	return *this;
}

// --> switchdef_container_c Class

//
// switchdef_container_c::CleanupObject()
//
void switchdef_container_c::CleanupObject(void *obj)
{
	switchdef_c *sw = *(switchdef_c**)obj;

	if (sw)
		delete sw;

	return;
}

//
// switchdef_c* switchdef_container_c::Find()
//
switchdef_c* switchdef_container_c::Find(const char *name)
{
	epi::array_iterator_c it;
	switchdef_c *sw;

	for (it = GetBaseIterator(); it.IsValid(); it++)
	{
		sw = ITERATOR_TO_TYPE(it, switchdef_c*);
		if (DDF_CompareName(sw->ddf.name.GetString(), name) == 0)
			return sw;
	}

	return NULL;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
