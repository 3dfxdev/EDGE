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
#include "w_rawdef.h"
#include "w_structs.h"
#include "w_file.h"
#include "w_io.h"
#include "w_wads.h"


MDirPtr Level;      /* master dictionary entry for the level */


static void LoadVertices()
{
	MDirPtr dir = FindMasterDir(Level, "VERTEXES");
	if (dir == 0)
		FatalError("No vertex lump!\n");

	const Wad_file *wf = dir->wadfile;
	wf->seek(dir->dir.start);
	if (wf->error ())
		FatalError("Error seeking to vertex lump!\n");

	int count = (int)dir->dir.size / sizeof(raw_vertex_t);

# if DEBUG_LOAD
	PrintDebug("GetVertices: num = %d\n", count);
# endif

	if (count == 0)
		FatalError("Couldn't find any Vertices\n");

	Vertices.reserve(count);

	for (int i = 0; i < count; i++)
	{
		raw_vertex_t raw;

		wf->read_bytes(&raw, sizeof(raw));
		if (wf->error())
			FatalError("Error reading vertices.\n");

		Vertex *vert = new Vertex;

		vert->x = LE_S16(raw.x);
		vert->y = LE_S16(raw.y);

		Vertices.push_back(vert);
	}
}


static void LoadSectors()
{
	MDirPtr dir = FindMasterDir(Level, "SECTORS");
	if (dir == 0)
		FatalError("No sector lump!\n");

	const Wad_file *wf = dir->wadfile;
	wf->seek(dir->dir.start);
	if (wf->error ())
		FatalError("Error seeking to sector lump!\n");

	int count = (int)dir->dir.size / sizeof(raw_sector_t);

# if DEBUG_LOAD
	PrintDebug("GetSectors: num = %d\n", count);
# endif

	if (count == 0)
		FatalError("Couldn't find any Sectors\n");

	Sectors.reserve(count);

	for (int i = 0; i < count; i++)
	{
		raw_sector_t raw;

		wf->read_bytes(&raw, sizeof(raw));
		if (wf->error())
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


void LoadThings(void)
{
  int i, count=-1;
  raw_thing_t *raw;
  lump_t *lump = FindLevelLump("THINGS");

  if (lump)
    count = lump->length / sizeof(raw_thing_t);

  if (!lump || count == 0)
  {
    // Note: no error if no things exist, even though technically a map
    // will be unplayable without the player starts.
    PrintWarn("Couldn't find any Things!\n");
    return;
  }

  DisplayTicker();

# if DEBUG_LOAD
  PrintDebug("GetThings: num = %d\n", count);
# endif

  raw = (raw_thing_t *) lump->data;

  for (i=0; i < count; i++, raw++)
  {
    thing_t *thing = NewThing();

    thing->x = LE_S16(raw->x);
    thing->y = LE_S16(raw->y);

    thing->type = LE_U16(raw->type);
    thing->options = LE_U16(raw->options);

    thing->index = i;
  }
}


void LoadSideDefs()
{
  int i, count=-1;
  raw_sidedef_t *raw;
  lump_t *lump = FindLevelLump("SIDEDEFS");

  if (lump)
    count = lump->length / sizeof(raw_sidedef_t);

  if (!lump || count == 0)
    FatalError("Couldn't find any Sidedefs");

  DisplayTicker();

# if DEBUG_LOAD
  PrintDebug("GetSidedefs: num = %d\n", count);
# endif

  raw = (raw_sidedef_t *) lump->data;

  for (i=0; i < count; i++, raw++)
  {
    sidedef_t *side = NewSidedef();

    side->sector = (LE_S16(raw->sector) == -1) ? NULL :
        LookupSector(LE_U16(raw->sector));

    if (side->sector)
      side->sector->ref_count++;

    side->x_offset = LE_S16(raw->x_offset);
    side->y_offset = LE_S16(raw->y_offset);

    memcpy(side->upper_tex, raw->upper_tex, sizeof(side->upper_tex));
    memcpy(side->lower_tex, raw->lower_tex, sizeof(side->lower_tex));
    memcpy(side->mid_tex,   raw->mid_tex,   sizeof(side->mid_tex));

    /* sidedef indices never change */
    side->index = i;
  }
}


static INLINE_G sidedef_t *SafeLookupSidedef(uint16_g num)
{
  if (num == 0xFFFF)
    return NULL;

  if ((int)num >= num_sidedefs && (sint16_g)(num) < 0)
    return NULL;

  return LookupSidedef(num);
}


void LoadLineDefs()
{
  int i, count=-1;
  raw_linedef_t *raw;
  lump_t *lump = FindLevelLump("LINEDEFS");

  if (lump)
    count = lump->length / sizeof(raw_linedef_t);

  if (!lump || count == 0)
    FatalError("Couldn't find any Linedefs");

  DisplayTicker();

# if DEBUG_LOAD
  PrintDebug("GetLinedefs: num = %d\n", count);
# endif

  raw = (raw_linedef_t *) lump->data;

  for (i=0; i < count; i++, raw++)
  {
    linedef_t *line;

    vertex_t *start = LookupVertex(LE_U16(raw->start));
    vertex_t *end   = LookupVertex(LE_U16(raw->end));

    start->ref_count++;
    end->ref_count++;

    line = NewLinedef();

    line->start = start;
    line->end   = end;

    /* check for zero-length line */
    line->zero_len = (fabs(start->x - end->x) < DIST_EPSILON) && 
        (fabs(start->y - end->y) < DIST_EPSILON);

    line->flags = LE_U16(raw->flags);
    line->type = LE_U16(raw->type);
    line->tag  = LE_S16(raw->tag);

    line->two_sided = (line->flags & LINEFLAG_TWO_SIDED) ? TRUE : FALSE;
    line->is_precious = (line->tag >= 900 && line->tag < 1000) ? 
        TRUE : FALSE;

    line->right = SafeLookupSidedef(LE_U16(raw->sidedef1));
    line->left  = SafeLookupSidedef(LE_U16(raw->sidedef2));

    if (line->right)
    {
      line->right->ref_count++;
      line->right->on_special |= (line->type > 0) ? 1 : 0;
    }

    if (line->left)
    {
      line->left->ref_count++;
      line->left->on_special |= (line->type > 0) ? 1 : 0;
    }

    line->self_ref = (line->left && line->right &&
        (line->left->sector == line->right->sector));

    line->index = i;
  }
}



/*
   read in the level data
*/

int LoadLevel (const char *levelname)
{
	/* Find the various level information from the master directory */
	//DisplayMessage (-1, -1, "Reading data for level %s...", levelname);
	Level = FindMasterDir (MasterDir, levelname);
	if (!Level)
		FatalError("level data not found");

	BA_ClearAll();


	LoadVertices();
	LoadThings();
	LoadSectors();
	LoadSideDefs();
	LoadLineDefs();


	int rc = 0;
	int OldNumVertices;

	/* Get the number of vertices */
	s32_t v_offset = 42;
	s32_t v_length = 42;
	{
		const char *lump_name = "BUG";
		if (yg_level_format == YGLF_ALPHA)  // Doom alpha
			lump_name = "POINTS";
		else
			lump_name = "VERTEXES";
		dir = FindMasterDir (Level, lump_name);
		if (dir == 0)
			OldNumVertices = 0;
		else
		{
			v_offset = dir->dir.start;
			v_length = dir->dir.size;
			if (yg_level_format == YGLF_ALPHA)  // Doom alpha: skip leading count
			{
				v_offset += 4;
				v_length -= 4;
			}
			OldNumVertices = (int) (v_length / WAD_VERTEX_BYTES);
			if ((s32_t) (OldNumVertices * WAD_VERTEX_BYTES) != v_length)
				warn ("the %s lump has a weird size."
						" The wad might be corrupt.\n", lump_name);
		}
	}

	// Read THINGS
	{
		const char *lump_name = "THINGS";
		verbmsg ("Reading %s things", levelname);
		s32_t offset = 42;
		s32_t length;
			NumThings = 0;
		else
		{
			offset = dir->dir.start;
			length = dir->dir.size;
			if (MainWad == Iwad4)  // Hexen mode
			{
				NumThings = (int) (length / WAD_HEXEN_THING_BYTES);
				if ((s32_t) (NumThings * WAD_HEXEN_THING_BYTES) != length)
					warn ("the %s lump has a weird size."
							" The wad might be corrupt.\n", lump_name);
			}
			else                    // Doom/Heretic/Strife mode
			{
				if (yg_level_format == YGLF_ALPHA)  // Doom alpha: skip leading count
				{
					offset += 4;
					length -= 4;
				}
				size_t thing_size = yg_level_format == YGLF_ALPHA ? 12 : WAD_THING_BYTES;
				NumThings = (int) (length / thing_size);
				if ((s32_t) (NumThings * thing_size) != length)
					warn ("the %s lump has a weird size."
							" The wad might be corrupt.\n", lump_name);
			}
		}

		if (NumThings > 0)
		{
			Things = (TPtr) GetMemory ((unsigned long) NumThings
					* sizeof (struct Thing));
			{
				err ("%s: seek error", lump_name);
				rc = 1;
				goto byebye;
			}
			if (MainWad == Iwad4)    // Hexen mode
				for (long n = 0; n < NumThings; n++)
				{
					byte dummy2[6];
					wf->read_i16   ();         // Tid
					wf->read_i16   (&Things[n].x );
					wf->read_i16   (&Things[n].y );
					wf->read_i16   ();         // Height
					wf->read_i16   (&Things[n].angle);
					wf->read_i16   (&Things[n].type );
					wf->read_i16   (&Things[n].options );
					wf->read_bytes (dummy2, sizeof dummy2);
					if (wf->error ())
					{
						err ("%s: error reading thing #%ld", lump_name, n);
						rc = 1;
						goto byebye;
					}
				}
			else         // Doom/Heretic/Strife mode
				for (long n = 0; n < NumThings; n++)
				{
					wf->read_i16 (&Things[n].x );
					wf->read_i16 (&Things[n].y );
					wf->read_i16 (&Things[n].angle);
					wf->read_i16 (&Things[n].type );
					if (yg_level_format == YGLF_ALPHA)
						wf->read_i16 ();    // Alpha. Don't know what it's for.
					wf->read_i16 (&Things[n].options );
					if (wf->error ())
					{
						err ("%s: error reading thing #%ld", lump_name, n);
						rc = 1;
						goto byebye;
					}
				}
		}
	}

	// Read LINEDEFS
	if (yg_level_format != YGLF_ALPHA)
	{
		const char *lump_name = "LINEDEFS";
		verbmsg (" linedefs");
		dir = FindMasterDir (Level, lump_name);
		if (dir == 0)
			NumLineDefs = 0;
		else
		{
			if (MainWad == Iwad4)  // Hexen mode
			{
				NumLineDefs = (int) (dir->dir.size / WAD_HEXEN_LINEDEF_BYTES);
				if ((s32_t) (NumLineDefs * WAD_HEXEN_LINEDEF_BYTES) != dir->dir.size)
					warn ("the %s lump has a weird size."
							" The wad might be corrupt.\n", lump_name);
			}
			else                   // Doom/Heretic/Strife mode
			{
				NumLineDefs = (int) (dir->dir.size / WAD_LINEDEF_BYTES);
				if ((s32_t) (NumLineDefs * WAD_LINEDEF_BYTES) != dir->dir.size)
					warn ("the %s lump has a weird size."
							" The wad might be corrupt.\n", lump_name);
			}
		}
		if (NumLineDefs > 0)
		{
			LineDefs = (LDPtr) GetMemory ((unsigned long) NumLineDefs
					* sizeof (struct LineDef));
			const Wad_file *wf = dir->wadfile;
			wf->seek (dir->dir.start);
			if (wf->error ())
			{
				err ("%s: seek error", lump_name);
				rc = 1;
				goto byebye;
			}
			if (MainWad == Iwad4)  // Hexen mode
				for (long n = 0; n < NumLineDefs; n++)
				{
					byte dummy[6];
					wf->read_i16   (&LineDefs[n].start);
					wf->read_i16   (&LineDefs[n].end);
					wf->read_i16   (&LineDefs[n].flags);
					wf->read_bytes (dummy, sizeof dummy);
					wf->read_i16   (&LineDefs[n].side_R);
					wf->read_i16   (&LineDefs[n].side_L);
					LineDefs[n].type = dummy[0];
					LineDefs[n].tag  = dummy[1];  // arg1 often contains a tag
					if (wf->error ())
					{
						err ("%s: error reading linedef #%ld", lump_name, n);
						rc = 1;
						goto byebye;
					}
				}
			else                   // Doom/Heretic/Strife mode
				for (long n = 0; n < NumLineDefs; n++)
				{
					wf->read_i16 (&LineDefs[n].start);
					wf->read_i16 (&LineDefs[n].end);
					wf->read_i16 (&LineDefs[n].flags);
					wf->read_i16 (&LineDefs[n].type);
					wf->read_i16 (&LineDefs[n].tag);
					wf->read_i16 (&LineDefs[n].side_R);
					wf->read_i16 (&LineDefs[n].side_L);
					if (wf->error ())
					{
						err ("%s: error reading linedef #%ld", lump_name, n);
						rc = 1;
						goto byebye;
					}
				}
		}
	}

	// Read SIDEDEFS
	{
		const char *lump_name = "SIDEDEFS";
		verbmsg (" sidedefs");
		dir = FindMasterDir (Level, lump_name);
		if (dir)
		{
			NumSideDefs = (int) (dir->dir.size / WAD_SIDEDEF_BYTES);
			if ((s32_t) (NumSideDefs * WAD_SIDEDEF_BYTES) != dir->dir.size)
				warn ("the SIDEDEFS lump has a weird size."
						" The wad might be corrupt.\n");
		}
		else
			NumSideDefs = 0;
		if (NumSideDefs > 0)
		{
			SideDefs = (SDPtr) GetMemory ((unsigned long) NumSideDefs
					* sizeof (struct SideDef));
			const Wad_file *wf = dir->wadfile;
			wf->seek (dir->dir.start);
			if (wf->error ())
			{
				err ("%s: seek error", lump_name);
				rc = 1;
				goto byebye;
			}
			for (long n = 0; n < NumSideDefs; n++)
			{
				wf->read_i16   (&SideDefs[n].x_offset);
				wf->read_i16   (&SideDefs[n].y_offset);
				wf->read_bytes (&SideDefs[n].upper_tex, WAD_TEX_NAME);
				wf->read_bytes (&SideDefs[n].lower_tex, WAD_TEX_NAME);
				wf->read_bytes (&SideDefs[n].mid_tex, WAD_TEX_NAME);
				wf->read_i16   (&SideDefs[n].sector);
				if (wf->error ())
				{
					err ("%s: error reading sidedef #%ld", lump_name, n);
					rc = 1;
					goto byebye;
				}
			}
		}
	}

	/* Sanity checkings on linedefs: the 1st and 2nd vertices
	   must exist. The 1st and 2nd sidedefs must exist or be
	   set to -1. */
	for (long n = 0; n < NumLineDefs; n++)
	{
		if (LineDefs[n].side_R != -1
				&& outside (LineDefs[n].side_R, 0, NumSideDefs - 1))
		{
			err ("linedef %ld has bad 1st sidedef number %d, giving up",
					n, LineDefs[n].side_R);
			rc = 1;
			goto byebye;
		}
		if (LineDefs[n].side_L != -1
				&& outside (LineDefs[n].side_L, 0, NumSideDefs - 1))
		{
			err ("linedef %ld has bad 2nd sidedef number %d, giving up",
					n, LineDefs[n].side_L);
			rc = 1;
			goto byebye;
		}
		if (outside (LineDefs[n].start, 0, OldNumVertices -1))
		{
			err ("linedef %ld has bad 1st vertex number %d, giving up",
					n, LineDefs[n].start);
			rc = 1;
			goto byebye;
		}
		if (outside (LineDefs[n].end, 0, OldNumVertices - 1))
		{
			err ("linedef %ld has bad 2nd vertex number %d, giving up",
					n, LineDefs[n].end);
			rc = 1;
			goto byebye;
		}
	}


	/* Read the vertices. If the wad has been run through a nodes
	   builder, there is a bunch of vertices at the end that are not
	   used by any linedefs. Those vertices have been created by the
	   nodes builder for the segs. We ignore them, because they're
	   useless to the level designer. However, we do NOT ignore
	   unused vertices in the middle because that's where the
	   "string art" bug came from.

	   Note that there is absolutely no guarantee that the nodes
	   builders add their own vertices at the end, instead of at the
	   beginning or right in the middle, AFAIK. It's just that they
	   all seem to do that (1). What if some don't ? Well, we would
	   end up with many unwanted vertices in the level data. Nothing
	   that a good CheckCrossReferences() couldn't take care of. */
	{
		verbmsg (" vertices");
		int last_used_vertex = -1;
		for (long n = 0; n < NumLineDefs; n++)
		{
			last_used_vertex = MAX(last_used_vertex, LineDefs[n].start);
			last_used_vertex = MAX(last_used_vertex, LineDefs[n].end);
		}
		NumVertices = last_used_vertex + 1;
		// This block is only here to warn me if (1) is false.
		{
			bitvec_c vertex_used (OldNumVertices);
			for (long n = 0; n < NumLineDefs; n++)
			{
				vertex_used.set (LineDefs[n].start);
				vertex_used.set (LineDefs[n].end);
			}
			int unused = 0;
			for (long n = 0; n <= last_used_vertex; n++)
			{
				if (! vertex_used.get (n))
					unused++;
			}
			if (unused > 0)
			{
				warn ("this level has unused vertices in the middle.\n");
				warn ("total %d, tail %d (%d%%), unused %d (",
						OldNumVertices,
						OldNumVertices - NumVertices,
						NumVertices - unused
						? 100 * (OldNumVertices - NumVertices) / (NumVertices - unused)
						: 0,
						unused);
				int first = 1;
				for (int n = 0; n <= last_used_vertex; n++)
				{
					if (! vertex_used.get (n))
					{
						if (n == 0 || vertex_used.get (n - 1))
						{
							if (first)
								first = 0;
							else
								warn (", ");
							warn ("%d", n);
						}
						else if (n == last_used_vertex || vertex_used.get (n + 1))
							warn ("-%d", n);
					}
				}
				warn (")\n");
			}
		}
		// Now load all the vertices except the unused ones at the end.
		if (NumVertices > 0)
		{
			const char *lump_name = "BUG";
			Vertices = (VPtr) GetMemory ((unsigned long) NumVertices
					* sizeof (struct Vertex));
			if (yg_level_format == YGLF_ALPHA)  // Doom alpha
				lump_name = "POINTS";
			else
				lump_name = "VERTEXES";
			dir = FindMasterDir (Level, lump_name);
			if (dir == 0)
				goto vertexes_done;   // FIXME isn't that fatal ?
			{
				const Wad_file *wf = dir->wadfile;
				wf->seek (v_offset);
				if (wf->error ())
				{
					err ("%s: seek error", lump_name);
					rc = 1;
					goto byebye;
				}
				MapMaxX = -32767;
				MapMaxY = -32767;
				MapMinX = 32767;
				MapMinY = 32767;
				for (long n = 0; n < NumVertices; n++)
				{
					s16_t val;
					wf->read_i16 (&val);
					if (val < MapMinX)
						MapMinX = val;
					if (val > MapMaxX)
						MapMaxX = val;
					Vertices[n].x = val;
					wf->read_i16 (&val);
					if (val < MapMinY)
						MapMinY = val;
					if (val > MapMaxY)
						MapMaxY = val;
					Vertices[n].y = val;
					if (wf->error ())
					{
						err ("%s: error reading vertex #%ld", lump_name, n);
						rc = 1;
						goto byebye;
					}
				}
			}
vertexes_done:
			;
		}
	}

	// Ignore SEGS, SSECTORS and NODES

	// Read SECTORS
	{
		const char *lump_name = "SECTORS";
		verbmsg (" sectors\n");
		dir = FindMasterDir (Level, lump_name);
		if (yg_level_format != YGLF_ALPHA)
		{
			if (dir)
			{
				NumSectors = (int) (dir->dir.size / WAD_SECTOR_BYTES);
				if ((s32_t) (NumSectors * WAD_SECTOR_BYTES) != dir->dir.size)
					warn ("the %s lump has a weird size."
							" The wad might be corrupt.\n", lump_name);
			}
			else
				NumSectors = 0;
			if (NumSectors > 0)
			{
				Sectors = (SPtr) GetMemory ((unsigned long) NumSectors
						* sizeof (struct Sector));
				const Wad_file *wf = dir->wadfile;
				wf->seek (dir->dir.start);
				if (wf->error ())
				{
					err ("%s: seek error", lump_name);
					rc = 1;
					goto byebye;
				}
				for (long n = 0; n < NumSectors; n++)
				{
					wf->read_i16   (&Sectors[n].floorh);
					wf->read_i16   (&Sectors[n].ceilh);
					wf->read_bytes (&Sectors[n].floor_tex, WAD_FLAT_NAME);
					wf->read_bytes (&Sectors[n].ceil_tex,  WAD_FLAT_NAME);
					wf->read_i16   (&Sectors[n].light);
					wf->read_i16   (&Sectors[n].type);
					wf->read_i16   (&Sectors[n].tag);
					if (wf->error ())
					{
						err ("%s: error reading sector #%ld", lump_name, n);
						rc = 1;
						goto byebye;
					}
				}
			}
		}
		else  // Doom alpha--a wholly different SECTORS format
		{
			s32_t  *offset_table = 0;
			s32_t   nsectors     = 0;
			s32_t   nflatnames   = 0;
			char *flatnames    = 0;
			if (dir == 0)
			{
				warn ("%s: lump not found in directory\n", lump_name);  // FIXME fatal ?
				goto sectors_alpha_done;
			}
			{
				const Wad_file *wf = dir->wadfile;
				wf->seek (dir->dir.start);
				if (wf->error ())
				{
					err ("%s: seek error", lump_name);
					rc = 1;
					goto byebye;
				}
				wf->read_i32 (&nsectors);
				if (wf->error ())
				{
					err ("%s: error reading sector count", lump_name);
					rc = 1;
					goto byebye;
				}
				if (nsectors < 0)
				{
					warn ("Negative sector count. Clamping to 0.\n");
					nsectors = 0;
				}
				NumSectors = nsectors;
				Sectors = (SPtr) GetMemory ((unsigned long) NumSectors
						* sizeof (struct Sector));
				offset_table = new s32_t[nsectors];
				for (size_t n = 0; n < (size_t) nsectors; n++)
					wf->read_i32 (offset_table + n);
				if (wf->error ())
				{
					err ("%s: error reading offsets table", lump_name);
					rc = 1;
					goto sectors_alpha_done;
				}
				// Load FLATNAME
				{
					const char *lump_name = "FLATNAME";
					bool success = false;
					MDirPtr dir2 = FindMasterDir (Level, lump_name);
					if (dir2 == 0)
					{
						warn ("%s: lump not found in directory\n", lump_name);
						goto flatname_done;    // FIXME warn ?
					}
					{
						const Wad_file *wf = dir2->wadfile;
						wf->seek (dir2->dir.start);
						if (wf->error ())
						{
							warn ("%s: seek error\n", lump_name);
							goto flatname_done;
						}
						wf->read_i32 (&nflatnames);
						if (wf->error ())
						{
							warn ("%s: error reading flat name count\n", lump_name);
							nflatnames = 0;
							goto flatname_done;
						}
						if (nflatnames < 0 || nflatnames > 32767)
						{
							warn ("%s: bad flat name count, giving up\n", lump_name);
							nflatnames = 0;
							goto flatname_done;
						}
						else
						{
							flatnames = new char[WAD_FLAT_NAME * nflatnames];
							wf->read_bytes (flatnames, WAD_FLAT_NAME * nflatnames);
							if (wf->error ())
							{
								warn ("%s: error reading flat names\n", lump_name);
								nflatnames = 0;
								goto flatname_done;
							}
							success = true;
						}
					}
flatname_done:
					if (! success)
						warn ("%s: errors found, you'll have to do without flat names\n",
								lump_name);
				}
				for (size_t n = 0; n < (size_t) nsectors; n++)
				{
					wf->seek (dir->dir.start + offset_table[n]);
					if (wf->error ())
					{
						err ("%s: seek error", lump_name);
						rc = 1;
						goto sectors_alpha_done;
					}
					s16_t index;
					wf->read_i16 (&Sectors[n].floorh);
					wf->read_i16 (&Sectors[n].ceilh );
					wf->read_i16 (&index);
					if (nflatnames && flatnames && index >= 0 && index < nflatnames)
						memcpy (Sectors[n].floor_tex, flatnames + WAD_FLAT_NAME * index,
								WAD_FLAT_NAME);
					else
						strcpy (Sectors[n].floor_tex, "unknown");
					wf->read_i16 (&index);
					if (nflatnames && flatnames && index >= 0 && index < nflatnames)
						memcpy (Sectors[n].ceil_tex, flatnames + WAD_FLAT_NAME * index,
								WAD_FLAT_NAME);
					else
						strcpy (Sectors[n].ceil_tex, "unknown");
					wf->read_i16 (&Sectors[n].light);
					wf->read_i16 (&Sectors[n].type);
					wf->read_i16 (&Sectors[n].tag);
					// Don't know what the tail is for. Ignore it.
					if (wf->error ())
					{
						err ("%s: error reading sector #%ld", lump_name, long (n));
						rc = 1;
						goto sectors_alpha_done;
					}
				}
			}

sectors_alpha_done:
			if (offset_table != 0)
				delete[] offset_table;
			if (flatnames != 0)
				delete[] flatnames;
			if (rc != 0)
				goto byebye;
		}
	}

	/* Sanity checking on sidedefs: the sector must exist. I don't
	   make this a fatal error, though, because it's not exceptional
	   to find wads with unused sidedefs with a sector# of -1. Well
	   known ones include dyst3 (MAP06, MAP07, MAP08), mm (MAP16),
	   mm2 (MAP13, MAP28) and requiem (MAP03, MAP08, ...). */
	for (long n = 0; n < NumSideDefs; n++)
	{
		if (outside (SideDefs[n].sector, 0, NumSectors - 1))
			warn ("sidedef %ld has bad sector number %d\n",
					n, SideDefs[n].sector);
	}

	// Ignore REJECT and BLOCKMAP

	// Silly statistics
	verbmsg ("  %d things, %d vertices, %d linedefs, %d sidedefs, %d sectors\n",
			(int) NumThings, (int) NumVertices, (int) NumLineDefs,
			(int) NumSideDefs, (int) NumSectors);
	verbmsg ("  Map: (%d,%d)-(%d,%d)\n", MapMinX, MapMinY, MapMaxX, MapMaxY);

byebye:
	if (rc != 0)
		err ("%s: errors found, giving up", levelname);
	return rc;
}




//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
