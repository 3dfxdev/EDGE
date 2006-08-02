//----------------------------------------------------------------------------
//  EDGE Data Definition File Code (Colourmaps)
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
// Colourmap handling.
//

#include "i_defs.h"

#include "ddf_locl.h"
#include "ddf_main.h"
#include "ddf_colm.h"

#include <string.h>

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#undef  DF
#define DF  DDF_CMD

static colourmap_c buffer_colmap;
static colourmap_c *dynamic_colmap;

colourmap_container_c colourmaps;

void DDF_ColmapGetSpecial(const char *info, void *storage);

#define DDF_CMD_BASE  buffer_colmap

static const commandlist_t colmap_commands[] =
{
	DF("LUMP",    lump_name, DDF_MainGetInlineStr10),
	DF("START",   start,     DDF_MainGetNumeric),
	DF("LENGTH",  length,    DDF_MainGetNumeric),
	DF("SPECIAL", special,   DDF_ColmapGetSpecial),

	DF("GL COLOUR",   gl_colour,   DDF_MainGetRGB),
	DF("ALT COLOUR",  alt_colour,  DDF_MainGetRGB),
	DF("WASH COLOUR", wash_colour, DDF_MainGetRGB),
	DF("WASH TRANSLUCENCY", wash_trans, DDF_MainGetPercent),

	// -AJA- backwards compatibility cruft...
	DF("!PRIORITY", ddf, DDF_DummyFunction),

	DDF_CMD_END
};


//
//  DDF PARSE ROUTINES
//

static bool ColmapStartEntry(const char *name)
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
			dynamic_colmap->ddf.name.Set(name);
		else
			dynamic_colmap->ddf.SetUniqueName("UNNAMED_COLMAP", colourmaps.GetSize());

		colourmaps.Insert(dynamic_colmap);
	}

	dynamic_colmap->ddf.number = 0;

	// instantiate the static entry
	buffer_colmap.Default();

	// make sure fonts get whitened properly (as the default)
	if (strncasecmp(dynamic_colmap->ddf.name.GetString(), "TEXT", 4) == 0)
	{
		buffer_colmap.special = COLSP_Whiten;
	}

	return (existing != NULL);
}

static void ColmapParseField(const char *field, const char *contents,
    int index, bool is_last)
{
#if (DEBUG_DDF)  
	L_WriteDebug("COLMAP_PARSE: %s = %s;\n", field, contents);
#endif

	if (DDF_MainParseField(colmap_commands, field, contents))
		return;

	if (ddf_version < 0x128)
	{
		// handle properties (old crud)
		if (index == 0 && DDF_CompareName(contents, "TRUE") == 0)
		{
			L_WriteDebug("COLMAP PROPERTY CRUD: %s = %s\n", field, contents);
			DDF_ColmapGetSpecial(field, NULL);
			return;
		}
	}

	DDF_WarnError2(0x128, "Unknown colmap.ddf command: %s\n", field);
}

static void ColmapFinishEntry(void)
{
	if (buffer_colmap.start < 0)
	{
		DDF_WarnError2(0x128, "Bad START value for colmap: %d\n", buffer_colmap.start);
		buffer_colmap.start = 0;
	}

	if (buffer_colmap.length <= 0)
	{
		DDF_WarnError2(0x128, "Bad LENGTH value for colmap: %d\n", buffer_colmap.length);
		buffer_colmap.length = 1;
	}

	if (buffer_colmap.lump_name.IsEmpty() && buffer_colmap.gl_colour == RGB_NO_VALUE)
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


bool DDF_ReadColourMaps(void *data, int size)
{
	readinfo_t colm_r;

	colm_r.memfile = (char*)data;
	colm_r.memsize = size;
	colm_r.tag = "COLOURMAPS";
	colm_r.entries_per_dot = 2;

	if (colm_r.memfile)
	{
		colm_r.message = NULL;
		colm_r.filename = NULL;
		colm_r.lumpname = "DDFCOLM";
	}
	else
	{
		colm_r.message = "DDF_InitColourMaps";
		colm_r.filename = "colmap.ddf";
		colm_r.lumpname = NULL;
	}

	colm_r.start_entry  = ColmapStartEntry;
	colm_r.parse_field  = ColmapParseField;
	colm_r.finish_entry = ColmapFinishEntry;
	colm_r.clear_all    = ColmapClearAll;

	return DDF_MainReadFile(&colm_r);
}

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
	{"WHITEN", COLSP_Whiten,  false},

	// -AJA- backwards compatibility cruft...
	{"!SKY",   0, 0},
	{NULL, 0, 0}
};

//
// DDF_ColmapGetSpecial
//
// Gets the colourmap specials.
//
void DDF_ColmapGetSpecial(const char *info, void *storage)
{
	int flag_value;

	switch (DDF_MainCheckSpecialFlag(info, colmap_specials, &flag_value,
				true, false))
	{
		case CHKF_Positive:
			buffer_colmap.special = (colourspecial_e)(buffer_colmap.special | flag_value);
			break;

		case CHKF_Negative:
			buffer_colmap.special = (colourspecial_e)(buffer_colmap.special & ~flag_value);
			break;

		case CHKF_User:
		case CHKF_Unknown:
			DDF_WarnError2(0x128, "DDF_ColmapGetSpecial: Unknown Special: %s", info);
			break;
	}
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

//
// colourmap_c::Copy()
//
void colourmap_c::Copy(colourmap_c &src)
{
	ddf = src.ddf;
	CopyDetail(src);
}

//
// colourmap_c::CopyDetail()
//
void colourmap_c::CopyDetail(colourmap_c &src)
{
	lump_name = src.lump_name;

	start   = src.start;
	length  = src.length;
	special = src.special;

	gl_colour   = src.gl_colour;
	alt_colour  = src.alt_colour;
	font_colour = src.font_colour;
	wash_colour = src.wash_colour;
	wash_trans  = src.wash_trans;
	
	// FIXME!!! Cache struct to class
	cache.data = src.cache.data;
}

//
// colourmap_c::Default()
//
void colourmap_c::Default()
{
	ddf.Default();
	
	lump_name.Clear();

	start   = 0;
	length  = 0;
	special = COLSP_None;
	
	gl_colour   = RGB_NO_VALUE;
	alt_colour  = RGB_NO_VALUE;
	font_colour = RGB_NO_VALUE;
	wash_colour = RGB_NO_VALUE;
	wash_trans  = PERCENT_MAKE(20);

	// FIXME!!! Cache struct to class
	cache.data = NULL;
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

//
// colourmap_container_c::colourmap_container_c()
//
colourmap_container_c::colourmap_container_c() : epi::array_c(sizeof(atkdef_c*))
{
	num_disabled = 0;	
}

//
// ~colourmap_container_c::colourmap_container_c()
//
colourmap_container_c::~colourmap_container_c()
{
	Clear();					// <-- Destroy self before exiting
}

//
// colourmap_container_c::CleanupObject
//
void colourmap_container_c::CleanupObject(void *obj)
{
	colourmap_c *c = *(colourmap_c**)obj;

	if (c)
		delete c;

	return;
}

//
// colourmap_c* colourmap_container_c::Lookup()
//
colourmap_c* colourmap_container_c::Lookup(const char *refname)
{
	epi::array_iterator_c it;
	colourmap_c *c;

	if (!refname || !refname[0])
		return NULL;

	for (it = GetIterator(num_disabled); it.IsValid(); it++)
	{
		c = ITERATOR_TO_TYPE(it, colourmap_c*);
		if (DDF_CompareName(c->ddf.name.GetString(), refname) == 0)
			return c;
	}

	return NULL;
}
