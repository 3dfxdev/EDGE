//----------------------------------------------------------------------------
//  EDGE Data Definition File Code (Styles)
//----------------------------------------------------------------------------
// 
//  Copyright (c) 2004  The EDGE Team.
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
// Style Setup and Parser Code
//

#include "i_defs.h"

#include "ddf_locl.h"
#include "ddf_main.h"
#include "ddf_font.h"
#include "ddf_style.h"

#include "./epi/epiutil.h"

#undef  DF
#define DF  DDF_CMD

styledef_c *default_style;

static void DDF_StyleGetBkgSpecials(const char *info, void *storage);

styledef_container_c styledefs;

static styledef_c buffer_style;
static styledef_c* dynamic_style;

// dummy structures (used to calculate field offsets)
static backgroundstyle_c buffer_bgstyle;
static textstyle_c  buffer_textstyle;
static soundstyle_c buffer_soundstyle;

#undef  DDF_CMD_BASE
#define DDF_CMD_BASE  buffer_bgstyle

static const commandlist_t background_commands[] =
{
	DF("COLOUR", colour, DDF_MainGetRGB),
	DF("TRANSLUCENCY", translucency, DDF_MainGetPercent),
    DF("IMAGE", image_name, DDF_MainGetString),
    DF("SPECIAL", special, DDF_StyleGetBkgSpecials),
    DF("SCALE",  scale,  DDF_MainGetFloat),
    DF("ASPECT", aspect, DDF_MainGetFloat),

	DDF_CMD_END
};

#undef  DDF_CMD_BASE
#define DDF_CMD_BASE  buffer_textstyle

static const commandlist_t text_commands[] =
{
    DF("COLOURMAP", colmap, DDF_MainGetColourmap),
	DF("COLOUR",    colour, DDF_MainGetRGB),
	DF("TRANSLUCENCY", translucency, DDF_MainGetPercent),
    DF("FONT",   font,   DDF_MainLookupFont),
    DF("SCALE",  scale,  DDF_MainGetFloat),
    DF("ASPECT", aspect, DDF_MainGetFloat),

	DDF_CMD_END
};

#undef  DDF_CMD_BASE
#define DDF_CMD_BASE  buffer_soundstyle

static const commandlist_t sound_commands[] =
{
	DF("BEGIN",  begin,  DDF_MainLookupSound),
	DF("END",    end,    DDF_MainLookupSound),
	DF("SELECT", select, DDF_MainLookupSound),
	DF("BACK",   back,   DDF_MainLookupSound),
	DF("ERROR",  error,  DDF_MainLookupSound),
	DF("MOVE",   move,   DDF_MainLookupSound),
	DF("SLIDER", slider, DDF_MainLookupSound),

	DDF_CMD_END
};

#undef  DDF_CMD_BASE
#define DDF_CMD_BASE  buffer_style

static const commandlist_t style_commands[] =
{
	// sub-commands
	DDF_SUB_LIST("BACKGROUND", bg, background_commands, buffer_bgstyle),
	DDF_SUB_LIST("TITLE", title, text_commands, buffer_textstyle),
	DDF_SUB_LIST("TEXT",  text,  text_commands, buffer_textstyle),
	DDF_SUB_LIST("ALT",   alt,   text_commands, buffer_textstyle),
	DDF_SUB_LIST("HELP",  help,  text_commands, buffer_textstyle),
	DDF_SUB_LIST("SOUND", sounds, sound_commands, buffer_soundstyle),

	DDF_CMD_END
};


//
//  DDF PARSE ROUTINES
//

static bool StyleStartEntry(const char *name)
{
	styledef_c *existing = NULL;

	if (name && name[0])
		existing = styledefs.Lookup(name);

	// not found, create a new one
	if (existing)
	{
		dynamic_style = existing;
	}
	else
	{
		dynamic_style = new styledef_c;

		if (name && name[0])
			dynamic_style->ddf.name.Set(name);
		else
			dynamic_style->ddf.SetUniqueName("UNNAMED_STYLE", styledefs.GetSize());

		styledefs.Insert(dynamic_style);
	}

	dynamic_style->ddf.number = 0;

	// instantiate the static entries
	buffer_style.Default();

	buffer_bgstyle.Default();
	buffer_textstyle.Default();
	buffer_soundstyle.Default();

	return (existing != NULL);	
}

static void StyleParseField(const char *field, const char *contents,
							int index, bool is_last)
{
#if (DEBUG_DDF)  
	L_WriteDebug("STYLE_PARSE: %s = %s;\n", field, contents);
#endif

	if (! DDF_MainParseField(style_commands, field, contents))
		DDF_WarnError2(0x128, "Unknown styles.ddf command: %s\n", field);
}

static void StyleFinishEntry(void)
{
	// FIXME: check stuff

	// transfer static entry to dynamic entry
	dynamic_style->CopyDetail(buffer_style);

	// FIXME: compute CRC...
}

static void StyleClearAll(void)
{
	// safe to delete the style entries -- no refs
	styledefs.Clear();
}


bool DDF_ReadStyles(void *data, int size)
{
	readinfo_t styles;

	styles.memfile = (char*)data;
	styles.memsize = size;
	styles.tag = "STYLES";
	styles.entries_per_dot = 2;

	if (styles.memfile)
	{
		styles.message = NULL;
		styles.filename = NULL;
		styles.lumpname = "DDFSTYLE";
	}
	else
	{
		styles.message = "DDF_InitStyles";
		styles.filename = "styles.ddf";
		styles.lumpname = NULL;
	}

	styles.start_entry  = StyleStartEntry;
	styles.parse_field  = StyleParseField;
	styles.finish_entry = StyleFinishEntry;
	styles.clear_all    = StyleClearAll;

	return DDF_MainReadFile(&styles);
}

void DDF_StyleInit(void)
{
	styledefs.Clear();
}

void DDF_StyleCleanUp(void)
{
	if (styledefs.GetSize() == 0)
		I_Error("There are no styles defined in DDF !\n");
	
	default_style = styledefs.Lookup("DEFAULT");

	if (! default_style)
		I_Error("Styles.ddf is missing the [DEFAULT] style.\n");

	styledefs.Trim();
}

static specflags_t background_specials[] =
{
    {"TILED", BKGSP_Tiled, 0},
    {NULL, 0, 0}
};

void DDF_StyleGetBkgSpecials(const char *info, void *storage)
{
	background_special_e *dest = (background_special_e *)storage;

	int flag_value;

	switch (DDF_MainCheckSpecialFlag(info, background_specials,
			&flag_value, true, false))
	{
		case CHKF_Positive:
			*dest = (background_special_e)(*dest | flag_value);
			break;

		case CHKF_Negative:
			*dest = (background_special_e)(*dest & ~flag_value);
			break;

		case CHKF_User:
		case CHKF_Unknown:
			DDF_WarnError("Unknown background style special: %s", info);
			break;
	}
}

// --> backgroundstyle_c definition class

//
// backgroundstyle_c Constructor
//
backgroundstyle_c::backgroundstyle_c()
{
	Default();
}

//
// backgroundstyle_c Copy constructor
//
backgroundstyle_c::backgroundstyle_c(const backgroundstyle_c &rhs)
{
	*this = rhs;
}

//
// backgroundstyle_c Destructor
//
backgroundstyle_c::~backgroundstyle_c()
{
}

//
// backgroundstyle_c::Default()
//
void backgroundstyle_c::Default()
{
	colour = 0x000000;
	translucency = PERCENT_MAKE(100);

	image_name.Clear();	
	cache_image = NULL;

	special = (background_special_e) 0;
	scale  = 1.0f;
	aspect = 1.0f;
}

//
// backgroundstyle_c assignment operator
//
backgroundstyle_c& backgroundstyle_c::operator= (const backgroundstyle_c &rhs)
{
	if (&rhs != this)
	{
		colour = rhs.colour;
		translucency = rhs.translucency;

		image_name = rhs.image_name;

		special = rhs.special;
		scale   = rhs.scale;
		aspect  = rhs.aspect;

		cache_image = NULL;  // don't need to copy cache, just reset it
	}
		
	return *this;
}

// --> textstyle_c definition class

//
// textstyle_c Constructor
//
textstyle_c::textstyle_c()
{
	Default();
}

//
// textstyle_c Copy constructor
//
textstyle_c::textstyle_c(const textstyle_c &rhs)
{
	*this = rhs;
}

//
// textstyle_c Destructor
//
textstyle_c::~textstyle_c()
{
}

//
// textstyle_c::Default()
//
void textstyle_c::Default()
{
	colmap = NULL;
	colour = 0xFFFFFF;  // white
	translucency = PERCENT_MAKE(100);

	font   = NULL;
	scale  = 1.0f;
	aspect = 1.0f;
}

//
// textstyle_c assignment operator
//
textstyle_c& textstyle_c::operator= (const textstyle_c &rhs)
{
	if (&rhs != this)
	{
		colmap = rhs.colmap;
		colour = rhs.colour;
		translucency = rhs.translucency;

		font   = rhs.font;
		scale  = rhs.scale;
		aspect = rhs.aspect;
	}
		
	return *this;
}

// --> soundstyle_c definition class

//
// soundstyle_c Constructor
//
soundstyle_c::soundstyle_c()
{
	Default();
}

//
// soundstyle_c Copy constructor
//
soundstyle_c::soundstyle_c(const soundstyle_c &rhs)
{
	*this = rhs;
}

//
// soundstyle_c Destructor
//
soundstyle_c::~soundstyle_c()
{
}

//
// soundstyle_c::Default()
//
void soundstyle_c::Default()
{
	begin  = NULL;
	end    = NULL;
	select = NULL;
	back   = NULL;
	error  = NULL;
	move   = NULL;
	slider = NULL;
}

//
// soundstyle_c assignment operator
//
soundstyle_c& soundstyle_c::operator= (const soundstyle_c &rhs)
{
	if (&rhs != this)
	{
		begin  = rhs.begin;
		end    = rhs.end;
		select = rhs.select;
		back   = rhs.back;
		error  = rhs.error;
		move   = rhs.move;
		slider = rhs.slider;
	}

	return *this;
}

// --> style definition class

//
// styledef_c Constructor
//
styledef_c::styledef_c()
{
	Default();
}

//
// styledef_c Copy constructor
//
styledef_c::styledef_c(const styledef_c &rhs)
{
	Copy(rhs);
}

//
// styledef_c Destructor
//
styledef_c::~styledef_c()
{
}

//
// styledef_c::Copy()
//
void styledef_c::Copy(const styledef_c &src)
{
	ddf = src.ddf;
	CopyDetail(src);	
}

//
// styledef_c::CopyDetail()
//
void styledef_c::CopyDetail(const styledef_c &src)
{
	bg = src.bg;

	title = src.title;
	text  = src.text;
	alt   = src.alt;
	help  = src.help;

	sounds = src.sounds;
}

//
// styledef_c::Default()
//
void styledef_c::Default()
{
	ddf.Default();

	bg.Default();

	title.Default();
	text.Default();
	alt.Default();
	help.Default();

	sounds.Default();
}

//
// styledef_c assignment operator
//
styledef_c& styledef_c::operator= (const styledef_c &rhs)
{
	if (&rhs != this)
		Copy(rhs);

	return *this;
}

// --> map definition container class

//
// styledef_container_c::CleanupObject()
//
void styledef_container_c::CleanupObject(void *obj)
{
	styledef_c *m = *(styledef_c**)obj;

	if (m) delete m;
}

//
// styledef_container_c::Lookup()
//
// Finds a styledef by name, returns NULL if it doesn't exist.
//
styledef_c* styledef_container_c::Lookup(const char *refname)
{
	epi::array_iterator_c it;
	styledef_c *m;

	if (!refname || !refname[0])
		return NULL;

	for (it = GetTailIterator(); it.IsValid(); it--)
	{
		m = ITERATOR_TO_TYPE(it, styledef_c*);
		if (DDF_CompareName(m->ddf.name.GetString(), refname) == 0)
			return m;
	}

	return NULL;
}
