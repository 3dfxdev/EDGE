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

#ifndef __DDF_IMAGE__
#define __DDF_IMAGE__

#include "dm_defs.h"

#include "./epi/epiutil.h"

//
// -AJA- 2004/11/16 Images.ddf
//
typedef enum
{
	IMGDT_Colour = 0,   // solid colour
	IMGDT_Builtin,      // built-in pre-fab DYI kit
	IMGDT_File,         // load from an image file

	// future:
	// IMGDT_WadFlat
	// IMGDT_WadTex
	// IMGDT_WadGfx
	// IMGDT_WadSprite
	// IMGDT_WadPlaySkin
	// IMGDT_WadTexPatch
	// IMGDT_Package
	// IMGDT_Composed
}
imagedata_type_e;

typedef enum
{
	BLTIM_Quadratic = 0,
	BLTIM_Linear,
	BLTIM_Shadow
}
builtin_image_e;

typedef enum
{
	IMGSP_None = 0,

	IMGSP_NoAlpha = 0x0001,   // image does not require an alpha channel
	IMGSP_NoMip   = 0x0002,   // disable mip-mapping on image
	IMGSP_Clamp   = 0x0004,   // clamp image
	IMGSP_Smooth  = 0x0008,   // force smoothing
}
image_special_e;

class imagedef_c
{
public:
	imagedef_c();
	imagedef_c(const imagedef_c &rhs);
	~imagedef_c() {};

private:
	void Copy(const imagedef_c &src);

public:
	void CopyDetail(const imagedef_c &src);
	void Default(void);
	imagedef_c& operator= (const imagedef_c &rhs);

	// Member vars....
	ddf_base_c ddf;

	imagedata_type_e type;

	// future:
	//   image_specialisation_c *specialisations;  // Egads, nomenclature explosion!!

	rgbcol_t colour;          // IMGDT_Colour
	builtin_image_e builtin;  // IMGDT_Builtin

	image_special_e special;

	epi::strent_c name;   // IMGDT_WadXXX, IMGDT_Package, IMGDT_File

	int builtin_size;
	int x_offset, y_offset;

	// COMPOSE specifics:
	//   rgbcol_t base_col;
	//   percent_t base_trans;
	//   compose_overlay_t *overlays;

	// WAD specifics:
	//   lumpname_c palette;
	//   colourmap_c *colmap;

	// CONVERSION
	//   float gamma;
	//   h_formula_t   conv_h;
	//   rgb_formula_t conv_s, conv_v;
	//   rgb_formula_t conv_r, conv_g, conv_b, conv_a;

	// RENDERING specifics:
	float scale, aspect;
	//   percent_t translucency;
	//   angle_t rotation;
};

// Our imagedefs container
class imagedef_container_c : public epi::array_c 
{
public:
	imagedef_container_c() : epi::array_c(sizeof(imagedef_c*)) {}
	~imagedef_container_c() { Clear(); } 

private:
	void CleanupObject(void *obj);

public:
	int GetSize() {	return array_entries; } 
	int Insert(imagedef_c *a) { return InsertObject((void*)&a); }
	imagedef_c* operator[](int idx) { return *(imagedef_c**)FetchObject(idx); } 

	// Search Functions
	imagedef_c* Lookup(const char* refname);
};

extern imagedef_container_c imagedefs;

void DDF_MainLookupImage(const char *info, void *storage);
bool DDF_ReadImages(void *data, int size);

#endif  /* __DDF_IMAGE__ */
