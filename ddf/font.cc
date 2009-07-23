//----------------------------------------------------------------------------
//  EDGE Data Definition File Code (Fonts)
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
// Font Setup and Parser Code
//

#include "local.h"

#include <map>

#include "font.h"

#undef  DF
#define DF  DDF_CMD

static fontdef_c dummy_font;
static fontdef_c *dynamic_font;

static void DDF_FontGetType( const char *info, void *storage);
static void DDF_FontGetPatch(const char *info, void *storage);

#define DDF_CMD_BASE  dummy_font

static const commandlist_t font_commands[] =
{
	DF("TYPE",     type,       DDF_FontGetType),
	DF("PATCHES",  patches,    DDF_FontGetPatch),
	DF("IMAGE",    image_name, DDF_MainGetString),
	DF("MISSING_PATCH", missing_patch, DDF_MainGetString),

	DDF_CMD_END
};


typedef std::map<const char *, fontdef_c *, DDF_Name_Cmp> fontdef_container_c;

fontdef_container_c fontdefs;


//
//  DDF PARSE ROUTINES
//

static void FontStartEntry(const char *name)
{
	dynamic_font = NULL;

	// FIXME: catch this before we get here
	if (!name || !name[0])
	{
		DDF_Error("New font entry is missing a name!");
	}

	dynamic_font = DDF_LookupFont(name);

	if (! dynamic_font)
	{
		dynamic_font = new fontdef_c();
		dynamic_font->ddf.name = name;
	}

	// instantiate the static entry
	dummy_font.Default();
}

static void FontParseField(const char *field, const char *contents, int index, bool is_last)
{
#if (DEBUG_DDF)  
	I_Debugf("FONT_PARSE: %s = %s;\n", field, contents);
#endif

	if (! DDF_MainParseField(font_commands, field, contents))
		DDF_Error("Unknown fonts.ddf command: %s\n", field);
}

static void FontFinishEntry(void)
{
	if (dummy_font.type == FNTYP_UNSET)
		DDF_Error("No type specified for font.\n");

	if (dummy_font.type == FNTYP_Patch && ! dummy_font.patches)
	{
		DDF_Error("Missing font patch list.\n");
	}

	// FIXME: check FNTYP_Image

	// transfer static entry to dynamic entry
	dynamic_font->CopyDetail(dummy_font);

	// link it into map
	const char *name = dynamic_font->ddf.name.c_str();

	fontdefs[name] = dynamic_font;
}

static void FontClearAll(void)
{
	// safe to just delete all fonts
	fontdef_container_c::iterator it;

	for (it = fontdefs.begin(); it != fontdefs.end(); it++)
		delete it->second;

	fontdefs.clear();
}


readinfo_t font_readinfo =
{
	"DDF_InitFonts", // message
	"FONTS", // tag

	FontStartEntry,
	FontParseField,
	FontFinishEntry,
	FontClearAll 
};


void DDF_FontInit(void)
{
}

void DDF_FontCleanUp(void)
{
	// FIXME: this is an Engine problem, not a DDF one
	if (fontdefs.size() == 0)
		I_Error("There are no fonts defined in DDF !\n");
}


static void DDF_FontGetType(const char *info, void *storage)
{
	SYS_ASSERT(storage);

	fonttype_e *type = (fonttype_e *) storage;

	if (DDF_CompareName(info, "PATCH") == 0)
		(*type) = FNTYP_Patch;
	else if (DDF_CompareName(info, "IMAGE") == 0)
		(*type) = FNTYP_Image;
	else
		DDF_Error("Unknown font type: %s\n", info);
}

static int FontParseCharacter(const char *buf)
{
#if 0  // the main parser strips out all the " quotes
	while (isspace(*buf))
		buf++;

	if (buf[0] == '"')
	{
		// check for escaped quote
		if (buf[1] == '\\' && buf[2] == '"')
			return '"';

		return buf[1];
	}
#endif

	if (buf[0] > 0 && isdigit(buf[0]) && isdigit(buf[1]))
		return atoi(buf);

	return buf[0];

#if 0
	DDF_Error("Malformed character name: %s\n", buf);
	return 0;
#endif
}

//
// Formats: PATCH123("x"), PATCH065(65),
//          PATCH456("a" : "z"), PATCH033(33:111).
//
static void DDF_FontGetPatch(const char *info, void *storage)
{
	fontpatch_c ** patches = (fontpatch_c **) storage;

	char patch_buf[100];
	char range_buf[100];

	if (! DDF_MainDecodeBrackets(info, patch_buf, range_buf, 100))
		DDF_Error("Malformed font patch: %s\n", info);

	// find dividing colon
	char *colon = NULL;
	
	if (strlen(range_buf) > 1)
		colon = (char *) DDF_MainDecodeList(range_buf, ':', true);

	if (colon)
		*colon++ = 0;

	int char1, char2;

	// get the characters

	char1 = FontParseCharacter(range_buf);

	if (colon)
	{
		char2 = FontParseCharacter(colon);

		if (char1 > char2)
			DDF_Error("Bad character range: %s > %s\n", range_buf, colon);
	}
	else
		char2 = char1;

	fontpatch_c *pat = new fontpatch_c(char1, char2, patch_buf);

	// add to list
	pat->next = *patches;
	*patches = pat;
}


// ---> fontpatch_c class

fontpatch_c::fontpatch_c(int _ch1, int _ch2, const char *_pat1) :
	char1(_ch1), char2(_ch2), patch1(_pat1), next(NULL)
{ }

// ---> fontdef_c class

//
// fontdef_c constructor
//
fontdef_c::fontdef_c() : ddf()
{
	Default();
}

//
// fontdef_c Copy constructor
//
fontdef_c::fontdef_c(const fontdef_c &rhs)
{
	Copy(rhs);
}

//
// fontdef_c::Copy()
//
void fontdef_c::Copy(const fontdef_c &src)
{
	ddf = src.ddf;
	CopyDetail(src);
}

//
// fontdef_c::CopyDetail()
//
// Copies all the detail with the exception of ddf info
//
void fontdef_c::CopyDetail(const fontdef_c &src)
{
	type = src.type;
	patches = src.patches;  // FIXME: copy list
	image_name = src.image_name;
	missing_patch = src.missing_patch;
}

//
// fontdef_c::Default()
//
void fontdef_c::Default()
{
	ddf.Default();

	type = FNTYP_Patch;
	patches = NULL;
	image_name.clear();
	missing_patch.clear();
}

//
// fontdef_c assignment operator
//
fontdef_c& fontdef_c::operator= (const fontdef_c &rhs)
{
	if (&rhs != this)
		Copy(rhs);
	
	return *this;
}


fontdef_c * DDF_LookupFont(const char *refname)
{
	if (!refname || !refname[0])
		return NULL;

	fontdef_container_c::iterator it;
	
	it = fontdefs.find(refname);

	if (it == fontdefs.end())
		return NULL;

	return it->second;
}


void DDF_MainLookupFont(const char *info, void *storage)
{
	fontdef_c **dest = (fontdef_c **)storage;

	*dest = DDF_LookupFont(info);

	if (*dest == NULL)
		DDF_Error("Unknown font: %s\n", info);
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
