//----------------------------------------------------------------------------
//  EDGE Data Definition File Code (Images)
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
// Image Setup and Parser Code
//

#include "local.h"

#include "epi/path.h"

#include "image.h"

#undef  DF
#define DF  DDF_CMD

static imagedef_c buffer_image;
static imagedef_c *dynamic_image;

static void DDF_ImageGetType(const char *info, void *storage);
static void DDF_ImageGetSpecial(const char *info, void *storage);
static void DDF_ImageGetFixTrans(const char *info, void *storage);

// -ACB- 1998/08/10 Use DDF_MainGetLumpName for getting the..lump name.
// -KM- 1998/09/27 Use DDF_MainGetTime for getting tics

#define DDF_CMD_BASE  buffer_image

static const commandlist_t image_commands[] =
{
	DF("IMAGE_DATA", type,     DDF_ImageGetType),
	DF("SPECIAL",    special,  DDF_ImageGetSpecial),
	DF("X_OFFSET",   x_offset, DDF_MainGetNumeric),
	DF("Y_OFFSET",   y_offset, DDF_MainGetNumeric),
	DF("SCALE",      scale,    DDF_MainGetFloat),
	DF("ASPECT",     aspect,   DDF_MainGetFloat),
	DF("FIX_TRANS",  fix_trans, DDF_ImageGetFixTrans),

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
	I_Debugf("ImageStartEntry [%s]\n", name);

	bool replaces = false;

	image_namespace_e belong = INS_Graphic;

	if (name && name[0])
	{
		char *pos = strchr(name, ':');

		if (! pos)
			DDF_Error("Missing image prefix.\n");

		if (pos)
		{
			std::string nspace(name, pos - name);

			if (nspace.empty())
				DDF_Error("Missing image prefix.\n");

			belong = GetImageNamespace(nspace.c_str());

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
			if (DDF_CompareName(a->ddf.name.c_str(), name) == 0)
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
	I_Debugf("IMAGE_PARSE: %s = %s;\n", field, contents);
#endif

	if (! DDF_MainParseField(image_commands, field, contents))
		DDF_Error("Unknown images.ddf command: %s\n", field);
}

static void ImageFinishEntry(void)
{
	if (buffer_image.type == IMGDT_File)
	{
        const char *filename = buffer_image.name.c_str();

		// determine format
        std::string ext(epi::PATH_GetExtension(filename));

		if (DDF_CompareName(ext.c_str(), "png") == 0)
			buffer_image.format = LIF_PNG;
		else if (DDF_CompareName(ext.c_str(), "jpg")  == 0 ||
				 DDF_CompareName(ext.c_str(), "jpeg") == 0)
			buffer_image.format = LIF_JPEG;
		else if (DDF_CompareName(ext.c_str(), "tga") == 0)
			buffer_image.format = LIF_TGA;
		else
			DDF_Error("Unknown image extension for '%s'\n", filename);
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


void DDF_ImageInit(void)
{
	imagedefs.Clear();
}


static void AddEssentialImages(void)
{
	// -AJA- this is a hack, these really should just be added to
	//       our standard IMAGES.DDF file.  However some Mods use
	//       standalone DDF and in that case these essential images
	//       would never get loaded.

	if (! imagedefs.Lookup("DLIGHT_EXP"))
	{
		imagedef_c *def = new imagedef_c;

		def->ddf.name.Set("DLIGHT_EXP");
		def->ddf.number = 0;
		def->ddf.crc.Reset();

		def->Default();

		def->name.Set("DLITEXPN");

		def->belong  = INS_Graphic;
		def->type    = IMGDT_Lump;
		def->format  = LIF_PNG;
		def->special = (image_special_e) (IMGSP_Clamp | IMGSP_Smooth | IMGSP_NoMip);

		imagedefs.Insert(def);
	}

	if (! imagedefs.Lookup("FUZZ_MAP"))
	{
		imagedef_c *def = new imagedef_c;

		def->ddf.name.Set("FUZZ_MAP");
		def->ddf.number = 0;
		def->ddf.crc.Reset();

		def->Default();

		def->name.Set("FUZZMAP8");

		def->belong  = INS_Texture;
		def->type    = IMGDT_Lump;
		def->format  = LIF_PNG;
		def->special = (image_special_e) (IMGSP_NoSmooth | IMGSP_NoMip);

		imagedefs.Insert(def);
	}
}

void DDF_ImageCleanUp(void)
{
 	AddEssentialImages();

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
	else if (DDF_CompareName(keyword, "TGA") == 0)
	{
		buffer_image.format = LIF_TGA;
	}
	else
		DDF_Error("Unknown image format: %s (use PNG or JPEG)\n", keyword);
}


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
    {"NOALPHA",       IMGSP_NoAlpha,   0},
    {"FORCE_MIP",     IMGSP_Mip,       0},
    {"FORCE_NOMIP",   IMGSP_NoMip,     0},
    {"FORCE_CLAMP",   IMGSP_Clamp,     0},
    {"FORCE_SMOOTH",  IMGSP_Smooth,    0},
    {"FORCE_NOSMOOTH",IMGSP_NoSmooth,  0},
    {"CROSSHAIR",     IMGSP_Crosshair, 0},
    {NULL, 0, 0}
};


static void DDF_ImageGetSpecial(const char *info, void *storage)
{
	image_special_e *dest = (image_special_e *)storage;

	int flag_value;

	switch (DDF_MainCheckSpecialFlag(info, image_specials,
			&flag_value, false /* allow_prefixes */, false))
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

static void DDF_ImageGetFixTrans(const char *info, void *storage)
{
	if (DDF_CompareName(info, "NONE") == 0)
	{
		buffer_image.fix_trans = FIXTRN_None;
	}
	else if (DDF_CompareName(info, "BLACKEN") == 0)
	{
		buffer_image.fix_trans = FIXTRN_Blacken;
	}
	else
		DDF_Error("Unknown FIX_TRANS type: %s\n", info);
}


// ---> imagedef_c class

imagedef_c::imagedef_c() : name()
{
	Default();
}

imagedef_c::imagedef_c(const imagedef_c &rhs)
{
	Copy(rhs);
}

void imagedef_c::Copy(const imagedef_c &src)
{
	ddf = src.ddf;
	CopyDetail(src);
}

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
	fix_trans = src.fix_trans;
}

void imagedef_c::Default()
{
	ddf.Default();

	belong  = INS_Graphic;
	type    = IMGDT_Colour;
	colour  = 0x000000;  // black
	builtin = BLTIM_Quadratic;
	format  = LIF_PNG;

	name.clear();

	special  = IMGSP_None;
	x_offset = y_offset = 0;

	scale  = 1.0f;
	aspect = 1.0f;
	fix_trans = FIXTRN_None;
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

imagedef_c* imagedef_container_c::Lookup(const char *refname)
{
	epi::array_iterator_c it;

	if (!refname || !refname[0])
		return NULL;

	for (it = GetBaseIterator(); it.IsValid(); it++)
	{
		imagedef_c *g = ITERATOR_TO_TYPE(it, imagedef_c*);
		if (DDF_CompareName(g->ddf.name.c_str(), refname) == 0)
			return g;
	}

	return NULL;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
