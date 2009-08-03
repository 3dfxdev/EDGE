//------------------------------------------------------------------------
//  LEVEL LOAD / SAVE
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2009 Andrew Apted
//  Copyright (C) 1997-2003 Andr� Majorel et al
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
//  in the public domain in 1994 by Rapha�l Quinet and Brendon Wyber.
//
//------------------------------------------------------------------------

#include "main.h"

#include "e_basis.h"
#include "e_loadsave.h"
#include "levels.h"  // update_level_bounds()
#include "w_rawdef.h"
#include "w_structs.h"

#include "w_wad.h"


Wad_file * edit_wad;
short      edit_level;


void FreshLevel()
{
	BA_ClearAll();

	Sector *sec = new Sector;
	Sectors.push_back(sec);

	sec->ceilh = 256;
	sec->light = 192;

	Thing *th = new Thing;
	Things.push_back(th);

	th->type = 1;

	for (int i = 0; i < 4; i++)
	{
		Vertex *v = new Vertex;
		Vertices.push_back(v);

		v->x = (i >= 2) ? 256 : -256;
		v->y = (i==1 || i==2) ? 256 : -256;

		SideDef *sd = new SideDef;
		SideDefs.push_back(sd);

		LineDef *ld = new LineDef;
		LineDefs.push_back(ld);

		ld->start = i;
		ld->end   = (i+1) % 4;
		ld->flags = MLF_Blocking;
		ld->right = i;
	}

	update_level_bounds();
}


//------------------------------------------------------------------------


static void LoadVertices()
{
	Lump_c *lump = edit_wad->FindLumpInLevel("VERTEXES", edit_level);
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


static void LoadSectors()
{
	Lump_c *lump = edit_wad->FindLumpInLevel("SECTORS", edit_level);
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


static void LoadThings()
{
	Lump_c *lump = edit_wad->FindLumpInLevel("THINGS", edit_level);
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


static void LoadSideDefs()
{
	Lump_c *lump = edit_wad->FindLumpInLevel("SIDEDEFS", edit_level);
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


static void LoadLineDefs()
{
	Lump_c *lump = edit_wad->FindLumpInLevel("LINEDEFS", edit_level);
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

void LoadLevel(const char *levelname)
{
edit_wad = master_dir[0];
SYS_ASSERT(edit_wad);

edit_level = edit_wad->FindLevel(levelname);
SYS_ASSERT(edit_level >= 0);

	LogPrintf("Loading level: %s\n", levelname);

	BA_ClearAll();

	LoadVertices();
	LoadThings  ();
	LoadSectors ();
	LoadSideDefs();
	LoadLineDefs();

	// Node builders create a lot of new vertices for segs.
	// However they just get in the way for editing, so remove them.
	RemoveUnusedVertices();

	update_level_bounds();
}


//------------------------------------------------------------------------


static void SaveVertices()
{
	int size = NumVertices * (int)sizeof(raw_vertex_t);

	Lump_c *lump = edit_wad->AddLump("VERTEXES", size);

	for (int i = 0; i < NumVertices; i++)
	{
		Vertex *vert = Vertices[i];

		raw_vertex_t raw;

		raw.x = LE_S16(vert->x);
		raw.y = LE_S16(vert->y);

		lump->Write(&raw, sizeof(raw));
	}

	lump->Finish();
}


static void SaveSectors()
{
	int size = NumSectors * (int)sizeof(raw_sector_t);

	Lump_c *lump = edit_wad->AddLump("SECTORS", size);

	for (int i = 0; i < NumSectors; i++)
	{
		Sector *sec = Sectors[i];

		raw_sector_t raw;
		
		raw.floorh = LE_S16(sec->floorh);
		raw.ceilh  = LE_S16(sec->ceilh);

		strncpy(raw.floor_tex, sec->FloorTex(), sizeof(raw.floor_tex));
		strncpy(raw.ceil_tex,  sec->CeilTex(),  sizeof(raw.ceil_tex));

		raw.light = LE_U16(sec->light);
		raw.type  = LE_U16(sec->type);
		raw.tag   = LE_U16(sec->tag);

		lump->Write(&raw, sizeof(raw));
	}

	lump->Finish();
}


static void SaveThings()
{
	int size = NumThings * (int)sizeof(raw_thing_t);

	Lump_c *lump = edit_wad->AddLump("THINGS", size);

	for (int i = 0; i < NumThings; i++)
	{
		Thing *th = Things[i];

		raw_thing_t raw;
		
		raw.x = LE_S16(th->x);
		raw.y = LE_S16(th->y);

		raw.angle   = LE_U16(th->angle);
		raw.type    = LE_U16(th->type);
		raw.options = LE_U16(th->options);

		lump->Write(&raw, sizeof(raw));
	}

	lump->Finish();
}


static void SaveSideDefs()
{
	int size = NumSideDefs * (int)sizeof(raw_sidedef_t);

	Lump_c *lump = edit_wad->AddLump("SIDEDEFS", size);

	for (int i = 0; i < NumSideDefs; i++)
	{
		SideDef *side = SideDefs[i];

		raw_sidedef_t raw;
		
		raw.x_offset = LE_S16(side->x_offset);
		raw.y_offset = LE_S16(side->y_offset);

		strncpy(raw.upper_tex, side->UpperTex(), sizeof(raw.upper_tex));
		strncpy(raw.lower_tex, side->LowerTex(), sizeof(raw.lower_tex));
		strncpy(raw.mid_tex,   side->MidTex(),   sizeof(raw.mid_tex));

		raw.sector = LE_U16(side->sector);

		lump->Write(&raw, sizeof(raw));
	}

	lump->Finish();
}


static void SaveLineDefs()
{
	int size = NumLineDefs * (int)sizeof(raw_linedef_t);

	Lump_c *lump = edit_wad->AddLump("LINEDEFS", size);

	for (int i = 0; i < NumThings; i++)
	{
		LineDef *ld = LineDefs[i];

		raw_linedef_t raw;
		
		raw.start = LE_U16(ld->start);
		raw.end   = LE_U16(ld->end);

		raw.flags = LE_U16(ld->flags);
		raw.type  = LE_U16(ld->type);
		raw.tag   = LE_S16(ld->tag);

		raw.right = (ld->right >= 0) ? LE_U16(ld->right) : 0xFFFF;
		raw.left  = (ld->left  >= 0) ? LE_U16(ld->left)  : 0xFFFF;

		lump->Write(&raw, sizeof(raw));
	}

	lump->Finish();
}


static void EmptyLump(const char *name)
{
	edit_wad->AddLump(name)->Finish();
}


void SaveLevel(const char *levelname)
{
	LogPrintf("Saving level: %s\n", levelname);

	edit_wad->BeginWrite();

	if (edit_level >= 0)
		edit_wad->RemoveLevel(edit_level);

	edit_wad->AddLevel(levelname, 0)->Finish();

	SaveThings  ();
	SaveLineDefs();
	SaveSideDefs();
	SaveVertices();

	EmptyLump("SEGS");
	EmptyLump("SSECTORS");
	EmptyLump("NODES");

	SaveSectors();

	EmptyLump("REJECT");
	EmptyLump("BLOCKMAP");

	// write out the new directory
	edit_wad->EndWrite();
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
