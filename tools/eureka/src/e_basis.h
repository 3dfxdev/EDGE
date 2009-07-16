//------------------------------------------------------------------------
//  BASIC OBJECT HANDLING
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor (C) 2001-2009 Andrew Apted
//                     (C) 1997-2003 André Majorel et al
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
//------------------------------------------------------------------------
//
//  Based on Yadex which incorporated code from DEU 5.21 that was put
//  in the public domain in 1994 by Raphaël Quinet and Brendon Wyber.
//
//------------------------------------------------------------------------

#ifndef __E_BASIS_H__
#define __E_BASIS_H__

class Thing
{
public:
	int x;
	int y;
	int angle;
	int type;
	int options;

	enum { F_X, F_Y, F_ANGLE, F_TYPE, F_OPTIONS };

public:
};


class Vertex
{
public:
	int x;
	int y;

	enum { F_X, F_Y };

public:
};


class Sector
{
public:
	int floorh;
	int ceilh;
	int floor_tex;
	int ceil_tex;
	int light;
	int type;
	int tag;

	enum { F_FLOORH, F_CEILH, F_FLOOR_TEX, F_CEIL_TEX, F_LIGHT, F_TYPE, F_TAG };

public:
};


class SideDef
{
public:
	int x_offset;
	int y_offset;
	int upper_tex;
	int mid_tex;
	int lower_tex;
	int sector;

	enum { F_X_OFFSET, F_Y_OFFSET, F_UPPER_TEX, F_MID_TEX, F_LOWER_TEX, F_SECTOR };

public:
};


class LineDef
{
public:
	int start;
	int end;
	int flags;
	int type;
	int tag;
	int side_R;
	int side_L;

	enum { F_START, F_END, F_FLAGS, F_TYPE, F_TAG, F_SIDE_R, F_SIDE_L };

public:
};


class RadTrig
{
public:
	int mx;  // mid-point
	int my;
	int rw;  // radius (width/2 and height/2)
	int rh;

	int z1;
	int z2;

	int name;
	int tag;
	int flags;
	int code;

	enum { F_MX, F_MY, F_RW, F_RH, F_Z1, F_Z2, F_NAME, F_TAG, F_FLAGS, F_CODE };

public:
};


extern std::vector<Thing *>   Things;
extern std::vector<Vertex *>  Vertices;
extern std::vector<Sector *>  Sectors;
extern std::vector<SideDef *> SideDefs;
extern std::vector<LineDef *> LineDefs;
extern std::vector<RadTrig *> RadTrigs;

#define NumThings     ((int)Things.size())
#define NumVertices   ((int)Vertices.size())
#define NumSectors    ((int)Sectors.size())
#define NumSideDefs   ((int)SideDefs.size())
#define NumLineDefs   ((int)LineDefs.size())
#define NumRadTrigs   ((int)RadTrigs.size())

#endif  /* __E_BASIS_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
