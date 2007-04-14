//----------------------------------------------------------------------------
//  EDGE Video Code
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
//  Based on the DOOM source code, released by Id Software under the
//  following copyright:
//
//    Copyright (C) 1993-1996 by id Software, Inc.
//
//----------------------------------------------------------------------------
//
// Original Author: Chi Hoang
//

#ifndef __VIDEORES_H__
#define __VIDEORES_H__

#include "i_defs.h"

#include "epi/arrays.h"

// Screen mode information.
typedef struct scrmode_s
{
	int width;
	int height;
	int depth;
	bool windowed;
	int sysdepth;
}
scrmode_t;

// Screen mode list object
class scrmodelist_c : public epi::array_c
{
public:
	scrmodelist_c() : epi::array_c(sizeof(scrmode_t*)) {}
	~scrmodelist_c() { Clear(); } 

	enum incrementtype_e { RES, DEPTH, WINDOWMODE };

private:
	void CleanupObject(void *obj)
	{
		scrmode_t* sm = *(scrmode_t**)obj; 
		
		if (sm) 
			delete sm;
	};

public:
	int Add(scrmode_t *sm);

    int Compare(scrmode_t *sm1, scrmode_t *sm2);

	void Dump(void);

	int Find(int w, int h, int bpp, bool windowed);
	int FindNearest(int w, int h, int bpp, bool windowed);
	int FindWithDepthBias(int w, int h, int bpp, bool windowed);
	int FindWithWindowModeBias(int w, int h, int bpp, bool windowed);

	scrmode_t* GetAt(int idx) { return *(scrmode_t**)FetchObject(idx); } 

	int GetSize() {	return array_entries; } 
	
	int Prev(int idx, incrementtype_e type);
	int Next(int idx, incrementtype_e type);
	
	scrmode_t* operator[](int idx) { return *(scrmode_t**)FetchObject(idx); } 
};

// Exported Func
void V_InitResolution(void);
void V_AddAvailableResolution(i_scrmode_t *mode);
void V_GetSysRes(scrmode_t *src, i_scrmode_t *dest);

// call this to change the resolution before the next frame.
// -ACB- 2005/03/06: Now uses an index in the scrmodelist
void R_ChangeResolution(int res_idx); 

// only call these when it really is time to do the actual resolution
// or view size change, i.e. at the start of a frame.
void R_ExecuteChangeResolution(void);
void R_ExecuteSetViewSize(void);


// Exported Vars
extern int SCREENWIDTH;
extern int SCREENHEIGHT;
extern int SCREENBITS;
extern bool SCREENWINDOW;

extern scrmodelist_c scrmodelist;

// Macros
#define FROM_320(x)  ((x) * SCREENWIDTH  / 320)
#define FROM_200(y)  ((y) * SCREENHEIGHT / 200)

#endif // __VIDEORES_H__

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
