//----------------------------------------------------------------------------
//  EDGE Data Definition File Code (Fonts)
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
// Font Setup and Parser Code
//

#include "i_defs.h"

#include "ddf_locl.h"
#include "ddf_main.h"
#include "ddf_font.h"

#undef  DF
#define DF  DDF_CMD

static fontdef_c buffer_font;
static fontdef_c *dynamic_font;

static void DDF_FontGetType( const char *info, void *storage);
static void DDF_FontGetPatch(const char *info, void *storage);

#define DDF_CMD_BASE  buffer_font

static const commandlist_t font_commands[] =
{
	DF("TYPE",     type,     DDF_FontGetType),
	DF("PATCHES",  patches,  DDF_FontGetPatch),
	DF("MISSING_PATCH", missing_patch, DDF_MainGetString),

	DDF_CMD_END
};

// -ACB- 2004/06/03 Replaced array and size with purpose-built class
fontdef_container_c fontdefs;

//
//  DDF PARSE ROUTINES
//
static bool FontStartEntry(const char *name)
{
	bool replaces = false;

	if (name && name[0])
	{
		epi::array_iterator_c it;
		fontdef_c *a;

		for (it = fontdefs.GetBaseIterator(); it.IsValid(); it++)
		{
			a = ITERATOR_TO_TYPE(it, fontdef_c*);
			if (DDF_CompareName(a->ddf.name.GetString(), name) == 0)
			{
				dynamic_font = a;
				replaces = true;
				break;
			}
		}
	}

	// not found, create a new one
	if (! replaces)
	{
		dynamic_font = new fontdef_c;

		if (name && name[0])
			dynamic_font->ddf.name.Set(name);
		else
			dynamic_font->ddf.SetUniqueName("UNNAMED_FONT", fontdefs.GetSize());

		fontdefs.Insert(dynamic_font);
	}

	dynamic_font->ddf.number = 0;

	// instantiate the static entry
	buffer_font.Default();
	return replaces;
}

static void FontParseField(const char *field, const char *contents, int index, bool is_last)
{
#if (DEBUG_DDF)  
	L_WriteDebug("FONT_PARSE: %s = %s;\n", field, contents);
#endif

	if (! DDF_MainParseField(font_commands, field, contents))
		DDF_Error("Unknown fonts.ddf command: %s\n", field);
}

static void FontFinishEntry(void)
{
	if (! buffer_font.patches)
	{
		DDF_Error("Missing font patch list.\n");
	}

	// transfer static entry to dynamic entry
	dynamic_font->CopyDetail(buffer_font);

	// Compute CRC.  In this case, there is no need, since fonts
	// have zero impact on the game simulation.
	dynamic_font->ddf.crc.Reset();
}

static void FontClearAll(void)
{
	// safe to just delete all fonts
	fontdefs.Clear();
}


bool DDF_ReadFonts(void *data, int size)
{
	readinfo_t fonts;

	fonts.memfile = (char*)data;
	fonts.memsize = size;
	fonts.tag = "FONTS";
	fonts.entries_per_dot = 2;

	if (fonts.memfile)
	{
		fonts.message  = NULL;
		fonts.filename = NULL;
		fonts.lumpname = "DDFFONT";
	}
	else
	{
		fonts.message  = "DDF_InitFonts";
		fonts.filename = "fonts.ddf";
		fonts.lumpname = NULL;
	}

	fonts.start_entry  = FontStartEntry;
	fonts.parse_field  = FontParseField;
	fonts.finish_entry = FontFinishEntry;
	fonts.clear_all    = FontClearAll;

	return DDF_MainReadFile(&fonts);
}

//
// DDF_FontInit
//
void DDF_FontInit(void)
{
	fontdefs.Clear();		// <-- Consistent with existing behaviour (-ACB- 2004/05/04)
}

//
// DDF_FontCleanUp
//
void DDF_FontCleanUp(void)
{
	if (fontdefs.GetSize() == 0)
		I_Error("There are no fonts defined in DDF !\n");

	fontdefs.Trim();		// <-- Reduce to allocated size
}

//
// DDF_FontGetType
//
static void DDF_FontGetType(const char *info, void *storage)
{
	DEV_ASSERT2(storage);

	fonttype_e *type = (fonttype_e *) storage;

	if (DDF_CompareName(info, "PATCH") == 0)
		(*type) = FNTYP_Patch;
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

	if (buf[0]>0 && isdigit(buf[0]) && isdigit(buf[1]))
		return atoi(buf);
	
	return buf[0];

#if 0
	DDF_Error("Malformed character name: %s\n", buf);
	return 0;
#endif
}

//
// DDF_FontGetPatch
//
// Formats: PATCH123("x"), PATCH065(65),
//          PATCH456("a" : "z"), PATCH033(33:111).
//
static void DDF_FontGetPatch(const char *info, void *storage)
{
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
	pat->next = buffer_font.patches;
	buffer_font.patches = pat;
}


// ---> fontpatch_c class

fontpatch_c::fontpatch_c(int _ch1, int _ch2, const char *_pat1) :
	next(NULL), char1(_ch1), char2(_ch2), patch1(_pat1)
{
}

// ---> fontdef_c class

//
// fontdef_c constructor
//
fontdef_c::fontdef_c()
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
	missing_patch.Clear();
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

// ---> fontdef_container_c class

//
// fontdef_container_c::CleanupObject()
//
void fontdef_container_c::CleanupObject(void *obj)
{
	fontdef_c *a = *(fontdef_c**)obj;

	if (a) delete a;
}

//
// fontdef_container_c::Lookup()
//
fontdef_c* fontdef_container_c::Lookup(const char *refname)
{
	if (!refname || !refname[0])
		return NULL;
	
	for (epi::array_iterator_c it = GetIterator(0); it.IsValid(); it++)
	{
		fontdef_c *f = ITERATOR_TO_TYPE(it, fontdef_c*);
		if (DDF_CompareName(f->ddf.name.GetString(), refname) == 0)
			return f;
	}

	return NULL;
}

//
// DDF_MainLookupFont
//
void DDF_MainLookupFont(const char *info, void *storage)
{
	fontdef_c **dest = (fontdef_c **)storage;

	*dest = fontdefs.Lookup(info);

	// FIXME: throw epi::error_c
	if (*dest == NULL)
		DDF_Error("Unknown font: %s\n", info);
}

