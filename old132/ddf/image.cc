//----------------------------------------------------------------------------
//  EDGE Data Definition File Code (Images)
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
// Image Setup and Parser Code
//

#include "local.h"

#include "epi/path.h"

#include "image.h"

#undef  DF
#define DF  DDF_CMD

static void DDF_ImageGetType(const char *info, void *storage);
static void DDF_ImageGetSpecial(const char *info, void *storage);
static void DDF_ImageGetFixTrans(const char *info, void *storage);

// -ACB- 1998/08/10 Use DDF_MainGetLumpName for getting the..lump name.
// -KM- 1998/09/27 Use DDF_MainGetTime for getting tics

#define DDF_CMD_BASE  dummy_image
static imagedef_c dummy_image;

static const commandlist_t image_commands[] =
{
	DF("IMAGE_DATA", name,     DDF_ImageGetType),
	DF("SPECIAL",    special,  DDF_ImageGetSpecial),
	DF("X_OFFSET",   x_offset, DDF_MainGetNumeric),
	DF("Y_OFFSET",   y_offset, DDF_MainGetNumeric),
	DF("SCALE",      scale,    DDF_MainGetFloat),
	DF("ASPECT",     aspect,   DDF_MainGetFloat),
	DF("FIX_TRANS",  fix_trans, DDF_ImageGetFixTrans),

	DDF_CMD_END
};


static imagedef_c *dynamic_image;

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

static void ImageStartEntry(const char *name, bool extend)
{
	if (!name || !name[0])
		DDF_Error("New image entry is missing a name!");

	I_Debugf("ImageStartEntry [%s]\n", name);

	dynamic_image = NULL;

	image_namespace_e belong = INS_Graphic;

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

		dynamic_image = imagedefs.Lookup(name, belong);
	}

	if (extend)
	{
		if (! dynamic_image)
			DDF_Error("Unknown image to extend: %s\n", name);
		return;
	}

	// replaces the existing entry
	if (dynamic_image)
	{
		dynamic_image->Default();
		dynamic_image->belong = belong;
		return;
	}

	// not found, create a new one
	dynamic_image = new imagedef_c;

	dynamic_image->name = name;
	dynamic_image->belong = belong;

	imagedefs.Insert(dynamic_image);
}


static void ImageDoTemplate(const char *contents, int index)
{
	if (index > 0)
		DDF_Error("Template must be a single name (not a list).\n");

	// TODO: support images in other namespaces
	if (strchr(contents, ':'))
		DDF_Error("Cannot use image prefixes for templates.\n");

	imagedef_c *other = imagedefs.Lookup(contents, dynamic_image->belong);
	if (!other || other == dynamic_image)
		DDF_Error("Unknown image template: '%s'\n", contents);

	dynamic_image->CopyDetail(*other);
}


static void ImageParseField(const char *field, const char *contents, int index, bool is_last)
{
#if (DEBUG_DDF)  
	I_Debugf("IMAGE_PARSE: %s = %s;\n", field, contents);
#endif

	if (DDF_CompareName(field, "TEMPLATE") == 0)
	{
		ImageDoTemplate(contents, index);
		return;
	}

	if (! DDF_MainParseField((char *)dynamic_image, image_commands, field, contents))
	{
		DDF_Error("Unknown images.ddf command: %s\n", field);
	}
}


static void ImageFinishEntry(void)
{
	if (dynamic_image->type == IMGDT_File)
	{
        const char *filename = dynamic_image->info.c_str();

		// determine format
        std::string ext(epi::PATH_GetExtension(filename));

		if (DDF_CompareName(ext.c_str(), "png") == 0)
			dynamic_image->format = LIF_PNG;
		else if (DDF_CompareName(ext.c_str(), "jpg")  == 0 ||
				 DDF_CompareName(ext.c_str(), "jpeg") == 0)
			dynamic_image->format = LIF_JPEG;
		else if (DDF_CompareName(ext.c_str(), "tga") == 0)
			dynamic_image->format = LIF_TGA;
		else
			DDF_Error("Unknown image extension for '%s'\n", filename);
	}

	// TODO check more stuff...
}


static void ImageClearAll(void)
{
	// safe to just delete all images
	imagedefs.Clear();
}


readinfo_t image_readinfo =
{
	"DDF_InitImages",  // message
	"IMAGES",  // tag

	ImageStartEntry,
	ImageParseField,
	ImageFinishEntry,
	ImageClearAll 
};


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

	if (! imagedefs.Lookup("DLIGHT_EXP", INS_Graphic))
	{
		imagedef_c *def = new imagedef_c;

		def->Default();

		def->name = "DLIGHT_EXP";
		def->info = "DLITEXPN";

		def->belong  = INS_Graphic;
		def->type    = IMGDT_Lump;
		def->format  = LIF_PNG;
		def->special = (image_special_e) (IMGSP_Clamp | IMGSP_Smooth | IMGSP_NoMip);

		imagedefs.Insert(def);
	}

	if (! imagedefs.Lookup("FUZZ_MAP", INS_Texture))
	{
		imagedef_c *def = new imagedef_c;

		def->Default();

		def->name = "FUZZ_MAP";
		def->info = "FUZZMAP8";

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


static void ImageParseColour(imagedef_c *def, const char *value)
{
	def->type = IMGDT_Colour;

	DDF_MainGetRGB(value, &def->colour);
}

static void ImageParseBuiltin(imagedef_c *def, const char *value)
{
	def->type = IMGDT_Builtin;

	if (DDF_CompareName(value, "LINEAR") == 0)
		def->builtin = BLTIM_Linear;
	else if (DDF_CompareName(value, "QUADRATIC") == 0)
		def->builtin = BLTIM_Quadratic;
	else if (DDF_CompareName(value, "SHADOW") == 0)
		def->builtin = BLTIM_Shadow;
	else
		DDF_Error("Unknown image BUILTIN kind: %s\n", value);
}

static void ImageParseFile(imagedef_c *def, const char *value)
{
	// ouch, hard work here...
	def->type = IMGDT_File;
	def->info = value;
}

static void ImageParseLump(imagedef_c *def, const char *spec)
{
	def->type = IMGDT_Lump;

	const char *colon = DDF_MainDecodeList(spec, ':', true);

	if (! colon || colon == spec || (colon - spec) >= 16 || colon[1] == 0)
		DDF_Error("Malformed image lump spec: 'LUMP:%s'\n", spec);

	char keyword[20];

	strncpy(keyword, spec, colon - spec);
	keyword[colon - spec] = 0;

	// store the lump name
	def->info.Set(colon + 1);

	if (DDF_CompareName(keyword, "PNG") == 0)
	{
		def->format = LIF_PNG;
	}
	else if (DDF_CompareName(keyword, "JPG") == 0 ||
	         DDF_CompareName(keyword, "JPEG") == 0)
	{
		def->format = LIF_JPEG;
	}
	else if (DDF_CompareName(keyword, "TGA") == 0)
	{
		def->format = LIF_TGA;
	}
	else
		DDF_Error("Unknown image format: %s (use PNG or JPEG)\n", keyword);
}


static void DDF_ImageGetType(const char *info, void *storage)
{
	imagedef_c *def = (imagedef_c *) storage;

	const char *colon = DDF_MainDecodeList(info, ':', true);

	if (! colon || colon == info || (colon - info) >= 16 || colon[1] == 0)
		DDF_Error("Malformed image type spec: %s\n", info);

	char keyword[20];

	strncpy(keyword, info, colon - info);
	keyword[colon - info] = 0;

	if (DDF_CompareName(keyword, "COLOUR") == 0)
	{
		ImageParseColour(def, colon + 1);
	}
	else if (DDF_CompareName(keyword, "BUILTIN") == 0)
	{
		ImageParseBuiltin(def, colon + 1);
	}
	else if (DDF_CompareName(keyword, "FILE") == 0)
	{
		ImageParseFile(def, colon + 1);
	}
	else if (DDF_CompareName(keyword, "LUMP") == 0)
	{
		ImageParseLump(def, colon + 1);
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

	int value;

	switch (DDF_MainCheckSpecialFlag(info, image_specials, &value))
	{
		case CHKF_Positive:
			*dest = (image_special_e)(*dest | value);
			break;

		case CHKF_Negative:
			*dest = (image_special_e)(*dest & ~value);
			break;

		default:
			DDF_WarnError("Unknown image special: %s\n", info);
			break;
	}
}

static void DDF_ImageGetFixTrans(const char *info, void *storage)
{
	int *fix_trans = (int *)storage;

	if (DDF_CompareName(info, "NONE") == 0)
	{
		*fix_trans = FIXTRN_None;
	}
	else if (DDF_CompareName(info, "BLACKEN") == 0)
	{
		*fix_trans = FIXTRN_Blacken;
	}
	else
		DDF_Error("Unknown FIX_TRANS type: %s\n", info);
}


// ---> imagedef_c class

imagedef_c::imagedef_c() : name(), info()
{
	Default();
}

imagedef_c::~imagedef_c()
{
}


void imagedef_c::Default()
{
	belong  = INS_Graphic;
	type    = IMGDT_Colour;
	colour  = 0x000000;  // black
	builtin = BLTIM_Quadratic;
	format  = LIF_PNG;

	info.clear();

	special  = IMGSP_None;
	x_offset = y_offset = 0;

	scale  = 1.0f;
	aspect = 1.0f;
	fix_trans = FIXTRN_None;
}


//
// Copies all the detail with the exception of ddf info
//
void imagedef_c::CopyDetail(const imagedef_c &src)
{
	// NOTE: belong is not copied

	type    = src.type;
	colour  = src.colour;
	builtin = src.builtin;
	info   = src.info;
	format  = src.format;

	special  = src.special;
	x_offset = src.x_offset;
	y_offset = src.y_offset;
	scale    = src.scale;
	aspect   = src.aspect;
	fix_trans = src.fix_trans;
}


// ---> imagedef_container_c class

void imagedef_container_c::CleanupObject(void *obj)
{
	imagedef_c *a = *(imagedef_c**)obj;

	if (a) delete a;
}

imagedef_c * imagedef_container_c::Lookup(const char *refname, image_namespace_e belong)
{
	epi::array_iterator_c it;

	if (!refname || !refname[0])
		return NULL;

	for (it = GetBaseIterator(); it.IsValid(); it++)
	{
		imagedef_c *g = ITERATOR_TO_TYPE(it, imagedef_c*);
		if (DDF_CompareName(g->name.c_str(), refname) == 0 && g->belong == belong)
			return g;
	}

	return NULL;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
