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
#include "../src/system/i_defs_gl.h" //unwanted engine link?

#include "image.h" //con_font
#include "font.h"

#include "../src/r_image.h" //unwanted engine link?
#include "../src/r_modes.h" //Unwanted engine link?

static fontdef_c *dynamic_font;

static void DDF_FontGetType(const char *info, void *storage);
static void DDF_FontGetPatch(const char *info, void *storage);

#define DDF_CMD_BASE  dummy_font
static fontdef_c dummy_font;

static const  imagedef_c *ddf_font;

static const commandlist_t font_commands[] =
{
	DDF_FIELD("TYPE",     type,       DDF_FontGetType),
	DDF_FIELD("PATCHES",  patches,    DDF_FontGetPatch),
	DDF_FIELD("IMAGE",    image_name, DDF_MainGetString),
	DDF_FIELD("MISSING_PATCH", missing_patch, DDF_MainGetString),

	DDF_CMD_END
};

// -ACB- 2004/06/03 Replaced array and size with purpose-built class
fontdef_container_c fontdefs;

//
//  DDF PARSE ROUTINES
//
static void FontStartEntry(const char *name, bool extend)
{
	if (!name || !name[0])
	{
		DDF_WarnError("New font entry is missing a name!");
		name = "FONT_WITH_NO_NAME";
	}

	dynamic_font = fontdefs.Lookup(name);

	if (extend)
	{
		if (!dynamic_font)
			DDF_Error("Unknown font to extend: %s\n", name);
		return;
	}

	// replaces the existing entry
	if (dynamic_font)
	{
		dynamic_font->Default();
		return;
	}

	// not found, create a new one
	dynamic_font = new fontdef_c;

	dynamic_font->name = name;

	fontdefs.Insert(dynamic_font);
}

static void FontParseField(const char *field, const char *contents, int index, bool is_last)
{
#if (DEBUG_DDF)
	I_Debugf("FONT_PARSE: %s = %s;\n", field, contents);
#endif

	if (DDF_MainParseField(font_commands, field, contents, (byte *)dynamic_font))
		return;  // OK

	DDF_Error("Unknown fonts.ddf command: %s\n", field);
}

static void FontFinishEntry(void)
{
	if (dynamic_font->type == FNTYP_UNSET)
		DDF_Error("No type specified for font.\n");

	if (dynamic_font->type == FNTYP_Patch && !dynamic_font->patches)
		DDF_Error("Missing font patch list.\n");

		if (dynamic_font->type == FNTYP_Image && !dynamic_font->image_name)
		DDF_Error("Missing font image.\n");
}

static void FontClearAll(void)
{
	I_Warning("Ignoring #CLEARALL in fonts.ddf\n");
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
		fonts.message = NULL;
		fonts.filename = NULL;
		fonts.lumpname = "DDFFONT";
	}
	else
	{
		fonts.message = "DDF_InitFonts";
		fonts.filename = "fonts.ddf";
		fonts.lumpname = NULL;
	}

	fonts.start_entry = FontStartEntry;
	fonts.parse_field = FontParseField;
	fonts.finish_entry = FontFinishEntry;
	fonts.clear_all = FontClearAll;

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
	SYS_ASSERT(storage);

	fonttype_e *type = (fonttype_e *)storage;

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

static int FNSZ;
static int XMUL;
static int YMUL;

static void Image_CalcSizes()
{
	// determine font sizing and spacing
	if (SCREENWIDTH < 400)
	{
		FNSZ = 10; XMUL = 7; YMUL = 12;
	}
	else if (SCREENWIDTH < 700)
	{
		FNSZ = 13; XMUL = 9; YMUL = 15;
	}
	else
	{
		FNSZ = 16; XMUL = 11; YMUL = 19;
	}
}

void DDF_SetupFont(void)
{
	if (!imagedefs.Lookup("DDF_FONT_2", INS_Graphic))
	{
		imagedef_c *def = new imagedef_c;

		def->name = "DDF_FONT_2";
		def->belong = INS_Graphic;

		def->info.Set("DDFFONT2");

		def->type = IMGDT_Lump;
		def->format = LIF_PNG;
		def->special = (image_special_e)(IMGSP_Clamp | IMGSP_Smooth | IMGSP_NoMip);

		imagedefs.Insert(def);
	}

	Image_CalcSizes();
}

static void DDF_DrawChar(int x, int y, char ch, rgbcol_t col)
{
	if (x + FNSZ < 0)
		return;

	float alpha = 1.0f;

	glColor4f(RGB_RED(col)/255.0f, RGB_GRN(col)/255.0f, 
	          RGB_BLU(col)/255.0f, alpha);

	int px =      int((byte)ch) % 16;
	int py = 15 - int((byte)ch) / 16;

	float tx1 = (px  ) / 16.0;
	float tx2 = (px+1) / 16.0;

	float ty1 = (py  ) / 16.0;
	float ty2 = (py+1) / 16.0;

	glBegin(GL_POLYGON);
  
	glTexCoord2f(tx1, ty1);
	glVertex2i(x, y);

	glTexCoord2f(tx1, ty2); 
	glVertex2i(x, y + FNSZ);
  
	glTexCoord2f(tx2, ty2);
	glVertex2i(x + FNSZ, y + FNSZ);
  
	glTexCoord2f(tx2, ty1);
	glVertex2i(x + FNSZ, y);
  
	glEnd();
}

// writes the text on coords (x,y) of the console
#if 0
static void DDF_DrawText(int x, int y, const char *s, rgbcol_t col)
{
	GLuint tex_id = W_ImageCache(ddf_font);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, tex_id);

	glEnable(GL_BLEND);
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0);

	for (; *s; s++)
	{
		DDF_DrawChar(x, y, *s, col);

		x += XMUL;

		if (x >= SCREENWIDTH)
			break;
	}

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_BLEND);
}
#endif // 0




//
// DDF_FontGetPatch
//
// Formats: PATCH123("x"), PATCH065(65),
//          PATCH456("a" : "z"), PATCH033(33:111).
//
static void DDF_FontGetPatch(const char *info, void *storage)
{
	fontpatch_c **patch_list = (fontpatch_c **)storage;

	char patch_buf[100];
	char range_buf[100];

	if (!DDF_MainDecodeBrackets(info, patch_buf, range_buf, 100))
		DDF_Error("Malformed font patch: %s\n", info);

	// find dividing colon
	char *colon = NULL;

	if (strlen(range_buf) > 1)
		colon = (char *)DDF_MainDecodeList(range_buf, ':', true);

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
	pat->next = *patch_list;

	*patch_list = pat;
}

// ---> fontpatch_c class

fontpatch_c::fontpatch_c(int _ch1, int _ch2, const char *_pat1) :
	next(NULL), char1(_ch1), char2(_ch2), patch1(_pat1)
{ }

// ---> fontdef_c class

//
// fontdef_c constructor
//
fontdef_c::fontdef_c() : name()
{
	Default();
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
	type = FNTYP_Patch;
	patches = NULL;
	image_name.clear();
	missing_patch.clear();
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

	for (epi::array_iterator_c it = GetIterator(0); it.IsValid(); it++) //TODO: V803 https://www.viva64.com/en/w/v803/ Decreased performance. In case 'it' is iterator it's more effective to use prefix form of increment. Replace iterator++ with ++iterator.
	{
		fontdef_c *f = ITERATOR_TO_TYPE(it, fontdef_c*);
		if (DDF_CompareName(f->name.c_str(), refname) == 0)
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

	if (*dest == NULL)
		DDF_Error("Unknown font: %s\n", info);
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab