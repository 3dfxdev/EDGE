//----------------------------------------------------------------------------
//  EDGE Data Definition File Code (HUD)
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
// HUD Setup and Parser Code
//

#include "i_defs.h"

#include "ddf_locl.h"
#include "ddf_main.h"
#include "ddf_hud.h"

#undef  DF
#define DF  DDF_CMD

static huddef_c buffer_hud;
static huddef_c *dynamic_hud;

static void DDF_HudGetUsage( const char *info, void *storage);

#define DDF_CMD_BASE  buffer_hud

static const commandlist_t hud_commands[] =
{
	DF("USAGE",    usage,     DDF_HudGetUsage),

	DDF_CMD_END
};

// -ACB- 2004/06/03 Replaced array and size with purpose-built class
huddef_container_c huddefs;

//
//  DDF PARSE ROUTINES
//
static bool HudStartEntry(const char *name)
{
L_WriteDebug("Start hud [%s]\n", name);
	bool replaces = false;

	if (name && name[0])
	{
		epi::array_iterator_c it;
		huddef_c *a;

		for (it = huddefs.GetBaseIterator(); it.IsValid(); it++)
		{
			a = ITERATOR_TO_TYPE(it, huddef_c*);
			if (DDF_CompareName(a->ddf.name.GetString(), name) == 0)
			{
				dynamic_hud = a;
				replaces = true;
				break;
			}
		}
	}

	// not found, create a new one
	if (! replaces)
	{
		dynamic_hud = new huddef_c;

		if (name && name[0])
			dynamic_hud->ddf.name.Set(name);
		else
			dynamic_hud->ddf.SetUniqueName("UNNAMED_HUD", huddefs.GetSize());

		huddefs.Insert(dynamic_hud);
	}

	dynamic_hud->ddf.number = 0;

	// instantiate the static entry
	buffer_hud.Default();
	return replaces;
}

static void HudParseField(const char *field, const char *contents, int index, bool is_last)
{
#if (DEBUG_DDF)  
	L_WriteDebug("HUD_PARSE: %s = %s;\n", field, contents);
#endif

	if (! DDF_MainParseField(hud_commands, field, contents))
		DDF_Error("Unknown hud.ddf command: %s\n", field);
}

static void HudFinishEntry(void)
{
	// transfer static entry to dynamic entry
	dynamic_hud->CopyDetail(buffer_hud);

	// Compute CRC.  In this case, there is no need, since huds
	// have zero impact on the game simulation.
	dynamic_hud->ddf.crc.Reset();
}

static void HudClearAll(void)
{
	// safe to just delete all huds
	huddefs.Clear();
}


bool DDF_ReadHuds(void *data, int size)
{
	readinfo_t huds;

	huds.memfile = (char*)data;
	huds.memsize = size;
	huds.tag = "HUDS";
	huds.entries_per_dot = 2;

	if (huds.memfile)
	{
		huds.message  = NULL;
		huds.filename = NULL;
		huds.lumpname = "DDFHUD";
	}
	else
	{
		huds.message  = "DDF_InitHuds";
		huds.filename = "hud.ddf";
		huds.lumpname = NULL;
	}

	huds.start_entry  = HudStartEntry;
	huds.parse_field  = HudParseField;
	huds.finish_entry = HudFinishEntry;
	huds.clear_all    = HudClearAll;

	return DDF_MainReadFile(&huds);
}

//
// DDF_HudInit
//
void DDF_HudInit(void)
{
	huddefs.Clear();		// <-- Consistent with existing behaviour (-ACB- 2004/05/04)
}

//
// DDF_HudCleanUp
//
void DDF_HudCleanUp(void)
{
	if (huddefs.GetSize() == 0)
		I_Error("There are no huds defined in DDF !\n");

	huddefs.Trim();		// <-- Reduce to allocated size
}

//
// DDF_HudGetType
//
static void DDF_HudGetUsage(const char *info, void *storage)
{
	SYS_ASSERT(storage);

	int *usage = (int *) storage;

	if (DDF_CompareName(info, "PLAYER") == 0)
		(*usage) = huddef_c::U_Player;
	else if (DDF_CompareName(info, "AUTOMAP") == 0)
		(*usage) = huddef_c::U_Automap;
	else if (DDF_CompareName(info, "RTS") == 0)
		(*usage) = huddef_c::U_RTS;
	else
		DDF_Error("Unknown hud usage: %s\n", info);
}


// ---> huddef_c class

//
// huddef_c constructor
//
huddef_c::huddef_c()
{
	Default();
}

//
// huddef_c Copy constructor
//
huddef_c::huddef_c(const huddef_c &rhs)
{
	Copy(rhs);
}

//
// huddef_c::Copy()
//
void huddef_c::Copy(const huddef_c &src)
{
	ddf = src.ddf;
	CopyDetail(src);
}

//
// huddef_c::CopyDetail()
//
// Copies all the detail with the exception of ddf info
//
void huddef_c::CopyDetail(const huddef_c &src)
{
	usage = src.usage;
	elements = src.elements;  // FIXME: copy list
}

//
// huddef_c::Default()
//
void huddef_c::Default()
{
	ddf.Default();

	usage = 0;
	elements = NULL;
}

//
// huddef_c assignment operator
//
huddef_c& huddef_c::operator= (const huddef_c &rhs)
{
	if (&rhs != this)
		Copy(rhs);
	
	return *this;
}

// ---> huddef_container_c class

//
// huddef_container_c::CleanupObject()
//
void huddef_container_c::CleanupObject(void *obj)
{
	huddef_c *a = *(huddef_c**)obj;

	if (a) delete a;
}

//
// huddef_container_c::Lookup()
//
huddef_c* huddef_container_c::Lookup(const char *refname)
{
	if (!refname || !refname[0])
		return NULL;
	
	for (epi::array_iterator_c it = GetIterator(0); it.IsValid(); it++)
	{
		huddef_c *f = ITERATOR_TO_TYPE(it, huddef_c*);
		if (DDF_CompareName(f->ddf.name.GetString(), refname) == 0)
			return f;
	}

	return NULL;
}

//
// DDF_MainLookupHUD
//
void DDF_MainLookupHUD(const char *info, void *storage)
{
	huddef_c **dest = (huddef_c **)storage;

	*dest = huddefs.Lookup(info);

	// FIXME: throw epi::error_c
	if (*dest == NULL)
		DDF_Error("Unknown hud: %s\n", info);
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
