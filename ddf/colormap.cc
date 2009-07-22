//----------------------------------------------------------------------------
//  EDGE Data Definition File Code (Colourmaps)
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
// Colourmap handling.
//

#include "local.h"

#include "colormap.h"

#undef  DF
#define DF  DDF_CMD

static colourmap_c buffer_colmap;
static colourmap_c *dynamic_colmap;

colourmap_container_c colourmaps;

void DDF_ColmapGetSpecial(const char *info, void *storage);

#define DDF_CMD_BASE  buffer_colmap

static const commandlist_t colmap_commands[] =
{
	DF("LUMP",    lump_name, DDF_MainGetLumpName),
	DF("START",   start,     DDF_MainGetNumeric),
	DF("LENGTH",  length,    DDF_MainGetNumeric),
	DF("SPECIAL", special,   DDF_ColmapGetSpecial),
	DF("GL_COLOUR", gl_colour, DDF_MainGetRGB),

	// -AJA- backwards compatibility cruft...
	DF("!PRIORITY", ddf, DDF_DummyFunction),
	DF("!ALT_COLOUR",  ddf, DDF_DummyFunction),
	DF("!WASH_COLOUR", ddf, DDF_DummyFunction),
	DF("!WASH_TRANSLUCENCY", ddf, DDF_DummyFunction),

	DDF_CMD_END
};


//
//  DDF PARSE ROUTINES
//

static void ColmapStartEntry(const char *name)
{
	colourmap_c *existing = NULL;

	if (name && name[0])
		existing = colourmaps.Lookup(name);

	// not found, create a new one
	if (existing)
	{
		dynamic_colmap = existing;
	}
	else
	{
		dynamic_colmap = new colourmap_c;

		if (name && name[0])
			dynamic_colmap->ddf.name = name;
		else
			dynamic_colmap->ddf.SetUniqueName("UNNAMED_COLMAP", colourmaps.GetSize());

		colourmaps.Insert(dynamic_colmap);
	}

	dynamic_colmap->ddf.number = 0;

	// instantiate the static entry
	buffer_colmap.Default();
}

static void ColmapParseField(const char *field, const char *contents,
    int index, bool is_last)
{
#if (DEBUG_DDF)  
	I_Debugf("COLMAP_PARSE: %s = %s;\n", field, contents);
#endif

	if (DDF_MainParseField(colmap_commands, field, contents))
		return;

	DDF_WarnError2(128, "Unknown colmap.ddf command: %s\n", field);
}

static void ColmapFinishEntry(void)
{
	if (buffer_colmap.start < 0)
	{
		DDF_WarnError2(128, "Bad START value for colmap: %d\n", buffer_colmap.start);
		buffer_colmap.start = 0;
	}

	if (! buffer_colmap.lump_name.empty() && buffer_colmap.length <= 0)
	{
		DDF_WarnError2(128, "Bad LENGTH value for colmap: %d\n", buffer_colmap.length);
		buffer_colmap.length = 1;
	}

	if (buffer_colmap.lump_name.empty() && buffer_colmap.gl_colour == RGB_NO_VALUE)
		DDF_Error("Colourmap entry missing LUMP or GL_COLOUR.\n");

	// transfer static entry to dynamic entry
	dynamic_colmap->CopyDetail(buffer_colmap);

	// Compute CRC.  In this case, there is no need, since colourmaps
	// only affect rendering, they have zero effect on the game
	// simulation itself.
	dynamic_colmap->ddf.crc.Reset();
}

static void ColmapClearAll(void)
{
	// not safe to delete colourmaps -- disable them
	colourmaps.SetDisabledCount(colourmaps.GetSize());
}


readinfo_t colormap_readinfo =
{
	"DDF_InitColourMaps",  // message
	"COLOURMAPS", // tag

	ColmapStartEntry,
	ColmapParseField,
	ColmapFinishEntry,
	ColmapClearAll 
};


void DDF_ColmapInit(void)
{
	colourmaps.Clear();
}

void DDF_ColmapCleanUp(void)
{
	colourmaps.Trim();
}

specflags_t colmap_specials[] =
{
	{"FLASH",  COLSP_NoFlash, true},

	// -AJA- backwards compatibility cruft...
	{"WHITEN", 0, 0},
	{"SKY",    0, 0},

	{NULL, 0, 0}
};

//
// Gets the colourmap specials.
//
void DDF_ColmapGetSpecial(const char *info, void *storage)
{
	colourspecial_e *spec = (colourspecial_e *)storage;

	int value;

	switch (DDF_MainCheckSpecialFlag(info, colmap_specials, &value, true))
	{
		case CHKF_Positive:
			*spec = (colourspecial_e)(*spec | value);
			break;

		case CHKF_Negative:
			*spec = (colourspecial_e)(*spec & ~value);
			break;

		default:
			DDF_WarnError2(128, "DDF_ColmapGetSpecial: Unknown Special: %s", info);
			break;
	}
}


// 
// This is used to make entries for lumps between C_START and C_END
// markers in a (BOOM) WAD file.
//
void DDF_ColourmapAddRaw(const char *lump_name, int size)
{
	if (size < 256)
	{
		I_Warning("WAD Colourmap '%s' too small (%d < %d)\n", lump_name, size, 256);
		return;
	}

	colourmap_c *def = colourmaps.Lookup(lump_name);

	// not found, create a new one
	if (! def)
	{
		def = new colourmap_c;

		colourmaps.Insert(def);
	}

	def->Default();

	def->ddf.name = lump_name;
	def->ddf.number = 0;
	def->ddf.crc.Reset();

	def->lump_name.Set(lump_name);

	def->start  = 0;
	def->length = MIN(32, size / 256);

	I_Debugf("- Added RAW colourmap '%s' start=%d length=%d\n",
		lump_name, def->start, def->length);
}


// --> Colourmap Class

//
// colourmap_c Constructor
//
colourmap_c::colourmap_c()
{
}

//
// colourmap_c Copy Constructor
//
colourmap_c::colourmap_c(colourmap_c &rhs)
{
	Copy(rhs);
}
 
//
// colourmap_c Deconstructor
//
colourmap_c::~colourmap_c()
{
}

void colourmap_c::Copy(colourmap_c &src)
{
	ddf = src.ddf;

	CopyDetail(src);
}

void colourmap_c::CopyDetail(colourmap_c &src)
{
	lump_name = src.lump_name;

	start   = src.start;
	length  = src.length;
	special = src.special;

	gl_colour   = src.gl_colour;
	font_colour = src.font_colour;

	// FIXME!!! Cache struct to class
	cache.data = src.cache.data;
	analysis = NULL;
}

void colourmap_c::Default()
{
	ddf.Default();
	
	lump_name.clear();

	start   = 0;
	length  = 0;
	special = COLSP_None;
	
	gl_colour   = RGB_NO_VALUE;
	font_colour = RGB_NO_VALUE;

	// FIXME!!! Cache struct to class
	cache.data = NULL;
	analysis = NULL;
}

//
// colourmap_c assignment operator
//
colourmap_c& colourmap_c::operator=(colourmap_c &rhs)
{
	if (&rhs != this)
		Copy(rhs);
		
	return *this;
}

// --> colourmap_container_c class

colourmap_container_c::colourmap_container_c() : epi::array_c(sizeof(atkdef_c*))
{
	num_disabled = 0;	
}

colourmap_container_c::~colourmap_container_c()
{
	Clear();					// <-- Destroy self before exiting
}

void colourmap_container_c::CleanupObject(void *obj)
{
	colourmap_c *c = *(colourmap_c**)obj;

	if (c)
		delete c;

	return;
}

colourmap_c* colourmap_container_c::Lookup(const char *refname)
{
	epi::array_iterator_c it;
	colourmap_c *c;

	if (!refname || !refname[0])
		return NULL;

	for (it = GetIterator(num_disabled); it.IsValid(); it++)
	{
		c = ITERATOR_TO_TYPE(it, colourmap_c*);
		if (DDF_CompareName(c->ddf.name.c_str(), refname) == 0)
			return c;
	}

	return NULL;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
