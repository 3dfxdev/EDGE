//----------------------------------------------------------------------------
//  EDGE2 Poly-Object Code
//----------------------------------------------------------------------------
//
//  Copyright (c) 1999-2017  The EDGE2 Team.
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

#ifndef __P_POBJ__
#define __P_POBJ__


typedef struct polyobj_s
{
	int index;
	int mirror;
	int sound;

	line_t *start;
	line_t **lines;
	seg_t *segs;
	int count;

	mobj_t *anchor;
	mobj_t *mobj;

	int state;
	int delay;
	int last_angle;
	float last_x;
	float last_y;

	int action;
	int args[5];

}
polyobj_t;


void P_ClearPolyobjects(void);
void P_AddPolyobject(int ix, line_t *start);
void P_PostProcessPolyObjs(void);
void P_UpdatePolyObj(mobj_t *mobj);
polyobj_t *P_GetPolyobject(int ix);

void PO_RotateLeft(int pobj, int speed, int angle);


#endif // __P_POBJ__

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
