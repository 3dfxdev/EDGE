//----------------------------------------------------------------------------
//  EDGE Data Definition File Code (Images)
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
// Image Setup and Parser Code
//

#include "i_defs.h"

#include "ddf_locl.h"
#include "ddf_main.h"
#include "ddf_image.h"
#include "dm_state.h"
#include "m_misc.h"  // M_CheckExtension
#include "p_spec.h"
#include "r_local.h"

#undef  DF
#define DF  DDF_CMD

static imagedef_c buffer_image;
static imagedef_c *dynamic_image;

static void DDF_ImageGetType(const char *info, void *storage);
static void DDF_ImageGetSpecial(const char *info, void *storage);

// -ACB- 1998/08/10 Use DDF_MainGetLumpName for getting the..lump name.
// -KM- 1998/09/27 Use DDF_MainGetTime for getting tics

#define DDF_CMD_BASE  buffer_image

static const commandlist_t image_commands[] =
{
	DF("IMAGE DATA", type,     DDF_ImageGetType),
	DF("SPECIAL",    special,  DDF_ImageGetSpecial),
	DF("X OFFSET",   x_offset, DDF_MainGetNumeric),
	DF("Y OFFSET",   y_offset, DDF_MainGetNumeric),
	DF("SCALE",      scale,    DDF_MainGetFloat),
	DF("ASPECT",     aspect,   DDF_MainGetFloat),

	DDF_CMD_END
};

imagedef_container_c imagedefs;

static image_namespace_e GetImageNamespace(const char *prefix)
{
	if (DDF_CompareName(prefix, "gfx") == 0)
		return INS_Graphic;

	if (DDF_CompareName(prefix, "tex") == 0)
		return INS_Texture;

	if (DDF_CompareName(prefix, "flat") == 0)
		return INS_Flat;

	if (DDF_CompareName(prefix, "spr") == 0)
		return INS_Sprite;
	
	DDF_Error("Invalid image prefix '%s' (use: gfx,tex,flat,spr)\n", prefix);
	return INS_Flat; /* NOT REACHED */ 
}

//
//  DDF PARSE ROUTINES
//
static bool ImageStartEntry(const char *name)
{
	L_WriteDebug("ImageStartEntry [%s]\n", name);

	bool replaces = false;

	image_namespace_e belong = INS_Graphic;

	if (name && name[0])
	{
		char *pos = strchr(name, ':');

		if (! pos)
			DDF_Error("Missing image prefix.\n");

		if (pos)
		{
			epi::string_c s;

			s.Empty();
			s.AddChars(name, 0, pos - name);
				
			if (s.IsEmpty())
				DDF_Error("Missing image prefix.\n");

			belong = GetImageNamespace(s.GetString());

			name = pos + 1;

			if (! name[0])
				DDF_Error("Missing image name.\n");
		}

		// W_Image code has limited space for the name
		if (strlen(name) > 15)
			DDF_Error("Image name [%s] too long.\n", name);

		epi::array_iterator_c it;
		imagedef_c *a;

		for (it = imagedefs.GetBaseIterator(); it.IsValid(); it++)
		{
			a = ITERATOR_TO_TYPE(it, imagedef_c*);
			if (DDF_CompareName(a->ddf.name.GetString(), name) == 0)
			{
				dynamic_image = a;
				replaces = true;
				break;
			}
		}
	}

	// not found, create a new one
	if (! replaces)
	{
		dynamic_image = new imagedef_c;

		if (name && name[0])
			dynamic_image->ddf.name.Set(name);
		else
			dynamic_image->ddf.SetUniqueName("UNNAMED", imagedefs.GetSize());

		imagedefs.Insert(dynamic_image);
	}

	dynamic_image->ddf.number = 0;

	// instantiate the static entry
	buffer_image.Default();
	buffer_image.belong = belong;

	return replaces;
}

static void ImageParseField(const char *field, const char *contents, int index, bool is_last)
{
#if (DEBUG_DDF)  
	L_WriteDebug("IMAGE_PARSE: %s = %s;\n", field, contents);
#endif

	if (! DDF_MainParseField(image_commands, field, contents))
		DDF_Error("Unknown images.ddf command: %s\n", field);
}

static void ImageFinishEntry(void)
{
	if (buffer_image.type == IMGDT_File)  //  || buffer_image.type == IMGDT_Package)
	{
		// determine format
		const char *basename = buffer_image.name.GetString();

		if (M_CheckExtension("png", basename) == EXT_MATCHING)
			buffer_image.format = LIF_PNG;
		else if (M_CheckExtension("jpg", basename) == EXT_MATCHING ||
		         M_CheckExtension("jpeg", basename) == EXT_MATCHING)
		{
			buffer_image.format = LIF_JPEG;
		}
		else
			DDF_Error("Unknown image extension for '%s'\n", basename);
	}

	// check stuff...

	// transfer static entry to dynamic entry
	dynamic_image->CopyDetail(buffer_image);

	// Compute CRC.  In this case, there is no need, since images
	// have no real impact on the game simulation.
	dynamic_image->ddf.crc.Reset();
}

static void ImageClearAll(void)
{
	// safe to just delete all images
	imagedefs.Clear();
}


bool DDF_ReadImages(void *data, int size)
{
	readinfo_t images;

	images.memfile = (char*)data;
	images.memsize = size;
	images.tag = "IMAGES";
	images.entries_per_dot = 2;

	if (images.memfile)
	{
		images.message = NULL;
		images.filename = NULL;
		images.lumpname = "DDFIMAGE";
	}
	else
	{
		images.message = "DDF_InitImages";
		images.filename = "images.ddf";
		images.lumpname = NULL;
	}

	images.start_entry  = ImageStartEntry;
	images.parse_field  = ImageParseField;
	images.finish_entry = ImageFinishEntry;
	images.clear_all    = ImageClearAll;

	return DDF_MainReadFile(&images);
}

//
// DDF_ImageInit
//
void DDF_ImageInit(void)
{
	imagedefs.Clear();
}

//
// DDF_ImageCleanUp
//
void DDF_ImageCleanUp(void)
{
	imagedefs.Trim();		// <-- Reduce to allocated size
}

static void ImageParseColour(const char *value)
{
	DDF_MainGetRGB(value, &buffer_image.colour);
}

static void ImageParseBuiltin(const char *value)
{
	if (DDF_CompareName(value, "LINEAR") == 0)
		buffer_image.builtin = BLTIM_Linear;
	else if (DDF_CompareName(value, "QUADRATIC") == 0)
		buffer_image.builtin = BLTIM_Quadratic;
	else if (DDF_CompareName(value, "SHADOW") == 0)
		buffer_image.builtin = BLTIM_Shadow;
	else
		DDF_Error("Unknown image BUILTIN kind: %s\n", value);
}

static void ImageParseName(const char *value)
{
	// ouch, hard work here...
	buffer_image.name.Set(value);
}

static void ImageParseLump(const char *spec)
{
	const char *colon = DDF_MainDecodeList(spec, ':', true);

	if (! colon || colon == spec || (colon - spec) >= 16 || colon[1] == 0)
		DDF_Error("Malformed image lump spec: 'LUMP:%s'\n", spec);

	char keyword[20];

	strncpy(keyword, spec, colon - spec);
	keyword[colon - spec] = 0;

	// store the lump name
	buffer_image.name.Set(colon + 1);

	if (DDF_CompareName(keyword, "PNG") == 0)
	{
		buffer_image.format = LIF_PNG;
	}
	else if (DDF_CompareName(keyword, "JPG") == 0 ||
	         DDF_CompareName(keyword, "JPEG") == 0)
	{
		buffer_image.format = LIF_JPEG;
	}
	else
		DDF_Error("Unknown image format: %s (use PNG or JPEG)\n", keyword);
}

//
// DDF_ImageGetType
//
static void DDF_ImageGetType(const char *info, void *storage)
{
	const char *colon = DDF_MainDecodeList(info, ':', true);

	if (! colon || colon == info || (colon - info) >= 16 || colon[1] == 0)
		DDF_Error("Malformed image type spec: %s\n", info);

	char keyword[20];

	strncpy(keyword, info, colon - info);
	keyword[colon - info] = 0;

	if (DDF_CompareName(keyword, "COLOUR") == 0)
	{
		buffer_image.type = IMGDT_Colour;
		ImageParseColour(colon + 1);
	}
	else if (DDF_CompareName(keyword, "BUILTIN") == 0)
	{
		buffer_image.type = IMGDT_Builtin;
		ImageParseBuiltin(colon + 1);
	}
	else if (DDF_CompareName(keyword, "FILE") == 0)
	{
		buffer_image.type = IMGDT_File;
		ImageParseName(colon + 1);
	}
	else if (DDF_CompareName(keyword, "LUMP") == 0)
	{
		buffer_image.type = IMGDT_Lump;
		ImageParseLump(colon + 1);
	}
	else
		DDF_Error("Unknown image type: %s\n", keyword);
}

static specflags_t image_specials[] =
{
    {"CROSSHAIR", IMGSP_Crosshair, 0},
    {"ALPHA",     IMGSP_NoAlpha,   1},
    {"MIP",       IMGSP_NoMip,     1},
    {"CLAMP",     IMGSP_Clamp,     0},
    {"SMOOTH",    IMGSP_Smooth,    0},
    {NULL, 0, 0}
};

//
// DDF_ImageGetSpecial
//
static void DDF_ImageGetSpecial(const char *info, void *storage)
{
	image_special_e *dest = (image_special_e *)storage;

	int flag_value;

	switch (DDF_MainCheckSpecialFlag(info, image_specials,
			&flag_value, true, false))
	{
		case CHKF_Positive:
			*dest = (image_special_e)(*dest | flag_value);
			break;

		case CHKF_Negative:
			*dest = (image_special_e)(*dest & ~flag_value);
			break;

		case CHKF_User:
		case CHKF_Unknown:
			DDF_WarnError("Unknown image special: %s\n", info);
			break;
	}
}


// ---> imagedef_c class

//
// imagedef_c constructor
//
imagedef_c::imagedef_c() : name()
{
	Default();
}

//
// imagedef_c Copy constructor
//
imagedef_c::imagedef_c(const imagedef_c &rhs)
{
	Copy(rhs);
}

//
// imagedef_c::Copy()
//
void imagedef_c::Copy(const imagedef_c &src)
{
	ddf = src.ddf;
	CopyDetail(src);
}

//
// imagedef_c::CopyDetail()
//
// Copies all the detail with the exception of ddf info
//
void imagedef_c::CopyDetail(const imagedef_c &src)
{
	belong  = src.belong;
	type    = src.type;
	colour  = src.colour;
	builtin = src.builtin;
	name    = src.name;
	format  = src.format;

	special  = src.special;
	x_offset = src.x_offset;
	y_offset = src.y_offset;
	scale    = src.scale;
	aspect   = src.aspect;
}

//
// imagedef_c::Default()
//
void imagedef_c::Default()
{
	ddf.Default();

	belong  = INS_Graphic;
	type    = IMGDT_Colour;
	colour  = 0x000000;  // black
	builtin = BLTIM_Quadratic;
	format  = LIF_PNG;

	name.Clear();

	special  = IMGSP_None;
	x_offset = y_offset = 0;

	scale  = 1.0f;
	aspect = 1.0f;
}

//
// imagedef_c assignment operator
//
imagedef_c& imagedef_c::operator= (const imagedef_c &rhs)
{
	if (&rhs != this)
		Copy(rhs);
	
	return *this;
}

// ---> imagedef_container_c class

//
// imagedef_container_c::CleanupObject()
//
void imagedef_container_c::CleanupObject(void *obj)
{
	imagedef_c *a = *(imagedef_c**)obj;

	if (a) delete a;
}

