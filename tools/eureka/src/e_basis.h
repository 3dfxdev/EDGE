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

public:
};


class Vertex
{
public:
	int x;
	int y;

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

public:
};


class RadTrig
{
public:
	int x;
	int y;
	int w;
	int h;

	int z1;
	int z2;

	int name;
	int tag;
	int flags;
	int code;

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
