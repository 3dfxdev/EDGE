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

#ifndef __DDF_COLM__
#define __DDF_COLM__

#include "dm_defs.h"

#include "./epi/utility.h"

// -AJA- 1999/07/09: colmap.ddf structures.

typedef enum
{
	// Default value
	COLSP_None     = 0x0000,
	
	// don't apply gun-flash type effects (looks silly for fog)
	COLSP_NoFlash  = 0x0001,

	// for fonts, apply the FONTWHITEN mapping first
	COLSP_Whiten   = 0x0002
}
colourspecial_e;

typedef struct colmapcache_s
{
	byte *data;
}
colmapcache_t;

class colourmap_c
{
public:
	colourmap_c();
	colourmap_c(colourmap_c &rhs); 
	~colourmap_c();

private:
	void Copy(colourmap_c &src);

public:
	void CopyDetail(colourmap_c &src);
	void Default();
	
	colourmap_c& operator=(colourmap_c &rhs);

	ddf_base_c ddf;

	lumpname_c lump_name;

	int start;
	int length;

	colourspecial_e special;

	// colours for GL renderer
	rgbcol_t gl_colour;
	rgbcol_t alt_colour;
	rgbcol_t font_colour;  // (computed only, not in DDF)

	rgbcol_t wash_colour;  // solid_box over the top (UNUSED RIGHT NOW)
	percent_t wash_trans;  //

	colmapcache_t cache;
};

// Colourmap container
class colourmap_container_c : public epi::array_c
{
public:
	colourmap_container_c();
	~colourmap_container_c();

private:
	void CleanupObject(void *obj);

	int num_disabled;					// Number of disabled 

public:
	// List Management
	int GetSize() {	return array_entries; } 
	int Insert(colourmap_c *c) { return InsertObject((void*)&c); }
	colourmap_c* operator[](int idx) { return *(colourmap_c**)FetchObject(idx); } 

	// Inliners
	int GetDisabledCount() { return num_disabled; }
	void SetDisabledCount(int _num_disabled) { num_disabled = _num_disabled; }

	// Search Functions
	colourmap_c* Lookup(const char* refname);
};

extern colourmap_container_c colourmaps;	// -ACB- 2004/06/10 Implemented

bool DDF_ReadColourMaps(void *data, int size);

#endif  /* __DDF_COLM__ */
