//------------------------------------------------------------------------
//  LEVEL LOAD / SAVE
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2009 Andrew Apted
//  Copyright (C) 1997-2003 André Majorel et al
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

#include "main.h"

//#include "m_bitvec.h"
//#include "m_dialog.h"
//#include "m_game.h"
#include "e_basis.h"
#include "levels.h"  // update_level_bounds()
#include "w_rawdef.h"
#include "w_structs.h"

#include "w_wad.h"


MDirPtr Level;      /* master dictionary entry for the level */


static void LoadVertices(short lev_idx)
{
	Lump_c *lump = editing_wad->FindLumpInLevel("VERTEXES", lev_idx);
	if (! lump)
		FatalError("No vertex lump!\n");

	if (! lump->Seek())
		FatalError("Error seeking to vertex lump!\n");

	int count = lump->Length() / sizeof(raw_vertex_t);

# if DEBUG_LOAD
	PrintDebug("GetVertices: num = %d\n", count);
# endif

	if (count == 0)
		FatalError("Couldn't find any Vertices\n");

	Vertices.reserve(count);

	for (int i = 0; i < count; i++)
	{
		raw_vertex_t raw;

		if (! lump->Read(&raw, sizeof(raw)))
			FatalError("Error reading vertices.\n");

		Vertex *vert = new Vertex;

		vert->x = LE_S16(raw.x);
		vert->y = LE_S16(raw.y);

		Vertices.push_back(vert);
	}
}


static void LoadSectors(short lev_idx)
{
	Lump_c *lump = editing_wad->FindLumpInLevel("SECTORS", lev_idx);
	if (! lump)
		FatalError("No sector lump!\n");

	if (! lump->Seek())
		FatalError("Error seeking to sector lump!\n");

	int count = lump->Length() / sizeof(raw_sector_t);

# if DEBUG_LOAD
	PrintDebug("GetSectors: num = %d\n", count);
# endif

	if (count == 0)
		FatalError("Couldn't find any Sectors\n");

	Sectors.reserve(count);

	for (int i = 0; i < count; i++)
	{
		raw_sector_t raw;

		if (! lump->Read(&raw, sizeof(raw)))
			FatalError("Error reading sectors.\n");

		Sector *sec = new Sector;

		sec->floorh = LE_S16(raw.floorh);
		sec->ceilh  = LE_S16(raw.ceilh);

		sec->floor_tex = BA_InternaliseShortStr(raw.floor_tex, 8);
		sec->ceil_tex  = BA_InternaliseShortStr(raw.ceil_tex,  8);

		sec->light = LE_U16(raw.light);
		sec->type  = LE_U16(raw.type);
		sec->tag   = LE_S16(raw.tag);

		Sectors.push_back(sec);
	}
}


static void LoadThings(short lev_idx)
{
	Lump_c *lump = editing_wad->FindLumpInLevel("THINGS", lev_idx);
	if (! lump)
		FatalError("No things lump!\n");

	if (! lump->Seek())
		FatalError("Error seeking to things lump!\n");

	int count = lump->Length() / sizeof(raw_thing_t);

# if DEBUG_LOAD
	PrintDebug("GetThings: num = %d\n", count);
# endif

	if (count == 0)
	{
		// Note: no error if no things exist, even though technically a map
		// will be unplayable without the player starts.
//!!!!		PrintWarn("Couldn't find any Things!\n");
		return;
	}

	for (int i = 0; i < count; i++)
	{
		raw_thing_t raw;

		if (! lump->Read(&raw, sizeof(raw)))
			FatalError("Error reading things.\n");

		Thing *th = new Thing;

		th->x = LE_S16(raw.x);
		th->y = LE_S16(raw.y);

		th->angle   = LE_U16(raw.angle);
		th->type    = LE_U16(raw.type);
		th->options = LE_U16(raw.options);

		Things.push_back(th);
	}
}


static void LoadSideDefs(short lev_idx)
{
	Lump_c *lump = editing_wad->FindLumpInLevel("SIDEDEFS", lev_idx);
	if (! lump)
		FatalError("No sidedefs lump!\n");

	if (! lump->Seek())
		FatalError("Error seeking to sidedefs lump!\n");

	int count = lump->Length() / sizeof(raw_sidedef_t);

# if DEBUG_LOAD
	PrintDebug("GetSidedefs: num = %d\n", count);
# endif

	if (count == 0)
		FatalError("Couldn't find any Sidedefs\n");

	for (int i = 0; i < count; i++)
	{
		raw_sidedef_t raw;

		if (! lump->Read(&raw, sizeof(raw)))
			FatalError("Error reading sidedefs.\n");

		SideDef *sd = new SideDef;

		sd->x_offset = LE_S16(raw.x_offset);
		sd->y_offset = LE_S16(raw.y_offset);

		sd->upper_tex = BA_InternaliseShortStr(raw.upper_tex, 8);
		sd->lower_tex = BA_InternaliseShortStr(raw.lower_tex, 8);
		sd->  mid_tex = BA_InternaliseShortStr(raw.  mid_tex, 8);

		sd->sector = LE_U16(raw.sector);

		if (sd->sector >= NumSectors)  // FIXME warning
			sd->sector = 0;

		SideDefs.push_back(sd);
	}
}


static void LoadLineDefs(short lev_idx)
{
	Lump_c *lump = editing_wad->FindLumpInLevel("LINEDEFS", lev_idx);
	if (! lump)
		FatalError("No linedefs lump!\n");

	if (! lump->Seek())
		FatalError("Error seeking to linedefs lump!\n");

	int count = lump->Length() / sizeof(raw_linedef_t);

# if DEBUG_LOAD
	PrintDebug("GetLinedefs: num = %d\n", count);
# endif

	if (count == 0)
		FatalError("Couldn't find any Linedefs");

	for (int i = 0; i < count; i++)
	{
		raw_linedef_t raw;

		if (! lump->Read(&raw, sizeof(raw)))
			FatalError("Error reading linedefs.\n");

		LineDef *ld = new LineDef;

		ld->start = LE_U16(raw.start);
		ld->end   = LE_U16(raw.end);

		ld->flags = LE_U16(raw.flags);
		ld->type  = LE_U16(raw.type);
		ld->tag   = LE_S16(raw.tag);

		ld->right = LE_U16(raw.right);
		ld->left  = LE_U16(raw.left);

		// FIXME vertex check

		if (ld->right == 0xFFFF)
			ld->right = -1;
		else if (ld->right >= NumSideDefs)
			ld->right = 0;  // FIXME warning

		if (ld->left == 0xFFFF)
			ld->left = -1;
		else if (ld->left >= NumSideDefs)
			ld->left = -1;  // FIXME warning

		LineDefs.push_back(ld);
	}
}


static void RemoveUnusedVertices()
{
	if (NumVertices == 0)
		return;
	
	bitvec_c *used_verts = new bitvec_c(NumVertices);

	for (int i = 0; i < NumLineDefs; i++)
	{
		used_verts->set(LineDefs[i]->start);
		used_verts->set(LineDefs[i]->end);
	}

	int new_count = NumVertices;

	while (new_count > 2 && !used_verts->get(new_count-1))
		new_count--;

	// we directly modify the vertex array here (which is not
	// normally kosher, but level loading is a special case).
	if (new_count < NumVertices)
	{
		LogPrintf("Removing %d unused vertices at end\n", new_count);

		for (int i = new_count; i < NumVertices; i++)
			delete Vertices[i];

		Vertices.resize(new_count);
	}

	delete used_verts;
}


/*
   read in the level data
*/

int LoadLevel(const char *levelname)
{
Wad_file *F = Wad_file::Open("doom2.wad");
SYS_ASSERT(F);

master_dir.push_back(F);

short lev_idx = WAD_FindEditLevel(levelname);
SYS_ASSERT(lev_idx >= 0);

	LogPrintf("Loading level: %s\n", levelname);

	/* Find the various level information from the master directory */
	//DisplayMessage (-1, -1, "Reading data for level %s...", levelname);
	Level = FindMasterDir (MasterDir, levelname);
	if (!Level)
		FatalError("level data not found");

	BA_ClearAll();

	LoadVertices(lev_idx);
	LoadThings(lev_idx);
	LoadSectors(lev_idx);
	LoadSideDefs(lev_idx);
	LoadLineDefs(lev_idx);

	// Node builders will create a lot of new vertices for segs.
	// However these would just get in the way for editing, so
	// we remove them.
	RemoveUnusedVertices();

	update_level_bounds();

	return 0;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
