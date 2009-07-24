//----------------------------------------------------------------------------
//  EDGE Data Definition File Codes (Switch textures)
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
// Switch Texture Setup and Parser Code
//

#include "local.h"

#include "switch.h"
#include "sfx.h"


#undef  DDF_CMD_BASE
#define DDF_CMD_BASE  dummy_switchdef
static switchdef_c dummy_switchdef;

static const commandlist_t switch_commands[] =
{
	DDF_CMD("ON_TEXTURE",  name1, DDF_MainGetLumpName),
	DDF_CMD("OFF_TEXTURE", name2, DDF_MainGetLumpName),
	DDF_CMD("ON_SOUND",  on_sfx,  DDF_MainLookupSound),
	DDF_CMD("OFF_SOUND", off_sfx, DDF_MainLookupSound),
	DDF_CMD("TIME", time, DDF_MainGetTime),

	// -AJA- backwards compatibility cruft...
	DDF_CMD("!SOUND", on_sfx, DDF_MainLookupSound),

	DDF_CMD_END
};


switchdef_container_c switchdefs;

static switchdef_c *dynamic_switchdef;


//
//  DDF PARSE ROUTINES
//

static void SwitchStartEntry(const char *name)
{
	if (!name || !name[0])
	{
		DDF_WarnError("New switch entry is missing a name!");
		name = "SWITCH_WITH_NO_NAME";
	}

	dynamic_switchdef = switchdefs.Find(name);

	if (! dynamic_switchdef)
	{
		dynamic_switchdef = new switchdef_c;
		dynamic_switchdef->name = name;

		switchdefs.Insert(dynamic_switchdef);
	}
}

static void SwitchParseField(const char *field, const char *contents,
		int index, bool is_last)
{
#if (DEBUG_DDF)  
	I_Debugf("SWITCH_PARSE: %s = %s;\n", field, contents);
#endif

	if (! DDF_MainParseField((char *)dynamic_switchdef, switch_commands, field, contents))
	{
		DDF_WarnError("Unknown switch.ddf command: %s\n", field);
	}
}

static void SwitchFinishEntry(void)
{
	if (! dynamic_switchdef->name1[0])
		DDF_Error("Missing first name for switch.\n");

	if (! dynamic_switchdef->name2[0])
		DDF_Error("Missing last name for switch.\n");

	if (dynamic_switchdef->time <= 0)
		DDF_Error("Bad time value for switch: %d\n", dynamic_switchdef->time);
}

static void SwitchClearAll(void)
{
	// safe here to delete all switches
	switchdefs.Clear();
}


readinfo_t switch_readinfo =
{
	"DDF_InitSwitches",  // messages
	"SWITCHES",  // tag

	SwitchStartEntry,
	SwitchParseField,
	SwitchFinishEntry,
	SwitchClearAll 
};


void DDF_SwitchInit(void)
{
	switchdefs.Clear();
}

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

		I_Debugf("- SWITCHES LUMP: off '%s' : on '%s'\n", off_name, on_name);
				
		// ignore zero-length names
		if (!off_name[0] || !on_name[0])
			continue;

		switchdef_c *def = new switchdef_c;

		def->name = "BOOM_SWITCH";

		def->Default();
		
		def->name1.Set( on_name);
		def->name2.Set(off_name);

		switchdefs.Insert(def);
	}
}


// ---> switchdef_c class

switchdef_c::switchdef_c() : name()
{
	Default();
}

switchdef_c::~switchdef_c()
{
}


void switchdef_c::Default()
{
	name1.clear();
	name2.clear();

	on_sfx = sfx_None;
	off_sfx = sfx_None;

	time = BUTTONTIME;
}

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
		if (DDF_CompareName(sw->name.c_str(), name) == 0)
			return sw;
	}

	return NULL;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
