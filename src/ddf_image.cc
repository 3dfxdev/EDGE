//----------------------------------------------------------------------------
//  EDGE Data Definition File Code (Images)
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2004  The EDGE Team.
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
	DF("IMAGE DATA", type, DDF_ImageGetType),
	DF("SPECIAL",    special, DDF_ImageGetSpecial),

	DDF_CMD_END
};

imagedef_container_c imagedefs;

//
//  DDF PARSE ROUTINES
//
static bool ImageStartEntry(const char *name)
{
I_Printf("ImageStartEntry [%s]\n", name);
	bool replaces = false;

	if (name && name[0])
	{
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
	// check stuff...

	// transfer static entry to dynamic entry
	dynamic_image->CopyDetail(buffer_image);

	// Compute CRC.  In this case, there is no need, since images
	// have no real impact on the game simulation.
	dynamic_image->ddf.crc.Reset();
I_Printf("ImageFinishEntry [%s] size %d\n", dynamic_image->ddf.name.GetString(),
	imagedefs.GetSize());
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

//
// DDF_ImageGetType
//
static void DDF_ImageGetType(const char *info, void *storage)
{

	bool *is_tex = (bool *) storage;

return;	// !!!! FIXME: DDF_ImageGetType

	if (DDF_CompareName(info, "FLAT") == 0)
		(*is_tex) = false;
	else if (DDF_CompareName(info, "TEXTURE") == 0)
		(*is_tex) = true;
	else
	{
		DDF_WarnError2(0x128, "Unknown images type: %s\n", info);
		(*is_tex) = false;
	}
}

static specflags_t image_specials[] =
{
    {"ALPHA", IMGSP_NoAlpha, 1},
    {"CLAMP X", IMGSP_ClampX, 0},
    {"CLAMP Y", IMGSP_ClampY, 0},
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
imagedef_c::imagedef_c()
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
	type    = src.type;
	colour  = src.colour;
	builtin = src.builtin;

	special = src.special;
}

//
// imagedef_c::Default()
//
void imagedef_c::Default()
{
	ddf.Default();

	type = IMGDT_Colour;
	colour = 0xff7000;  // black  !!!!!! FIXME
	builtin = BLTIM_Quadratic;

	special = IMGSP_None;
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

