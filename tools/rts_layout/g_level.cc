//------------------------------------------------------------------------
//  LEVEL : Level structure read/write functions.
//------------------------------------------------------------------------
//
//  RTS Layout Tool (C) 2007 Andrew Apted
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

#include "headers.h"

#include "raw_wad.h"
#include "lib_util.h"
#include "main.h"

#include "g_wad.h"
#include "g_level.h"


// #define DEBUG_LOAD  1


//------------------------------------------------------------
//  VERTEXES
//------------------------------------------------------------

vertex_c::vertex_c(int _idx, const raw_vertex_t *raw)
{
  x = (double) LE_S16(raw->x);
  y = (double) LE_S16(raw->y);
}

vertex_c::vertex_c(int _idx, const raw_v2_vertex_t *raw)
{
  x = (double) LE_S32(raw->x) / 65536.0;
  y = (double) LE_S32(raw->y) / 65536.0;
}

vertex_c::~vertex_c()
{
  /* nothing to free */
}


void level_c::GetVertexes()
{
  wad_lump_c *lump = base->FindLumpInLevel("VERTEXES");
  int count = -1;

  if (lump)
  {
    base->CacheLump(lump);
    count = lump->length / sizeof(raw_vertex_t);
  }

#ifdef DEBUG_LOAD
  PrintDebug("GetVertexes: num = %d\n", count);
#endif

  if (!lump || count == 0)
    Main_FatalError("Couldn't find any Vertexes!\n");

  verts.resize(count);

  raw_vertex_t *raw = (raw_vertex_t *) lump->data;

  for (int i = 0; i < count; i++, raw++)
  {
    verts[i] = new vertex_c(i, raw);
  }
}

vertex_c * level_c::LookupVertex(u16_t idx)
{
  if (idx < verts.size())
    return verts[idx];
 
  Main_FatalError("Bad vertex index '%d' in map!\n", (int)idx);
  return NULL; /* NOT REACHED */
}


//------------------------------------------------------------
//  SECTORS
//------------------------------------------------------------

sector_c::sector_c(int _idx, const raw_sector_t *raw)
{
  index = _idx;

  floor_h = LE_S16(raw->floor_h);
  ceil_h  = LE_S16(raw->ceil_h);

  memcpy(floor_tex, raw->floor_tex, 8);
  memcpy(ceil_tex,  raw->ceil_tex,  8);

  // make sure these strings are NUL terminated
  floor_tex[8] = 0;
  ceil_tex[8]  = 0;

  light   = LE_U16(raw->light);
  special = LE_U16(raw->special);
  tag     = LE_S16(raw->tag);
}

sector_c::~sector_c()
{
  /* nothing to free */
}


void level_c::GetSectors()
{
  int count = -1;
  wad_lump_c *lump = base->FindLumpInLevel("SECTORS");

  if (lump)
  {
    base->CacheLump(lump);
    count = lump->length / sizeof(raw_sector_t);
  }

  if (!lump || count == 0)
    Main_FatalError("Couldn't find any Sectors!\n");

#ifdef DEBUG_LOAD
  PrintDebug("GetSectors: num = %d\n", count);
#endif

  sectors.resize(count);

  raw_sector_t *raw = (raw_sector_t *) lump->data;

  for (int i = 0; i < count; i++, raw++)
  {
    sectors[i] = new sector_c(i, raw);
  }
}

sector_c * level_c::LookupSector(u16_t idx)
{
  // -AJA- I'm not convinced this is correct, but glBSP does it too
  if (idx == 0xFFFF)
    return NULL;

  if (idx < sectors.size())
    return sectors[idx];
 
  Main_FatalError("Bad sector index '%d' in map!\n", (int)idx);
  return NULL; /* NOT REACHED */
}



//------------------------------------------------------------
//  THINGS
//------------------------------------------------------------

thing_c::thing_c(int _idx, const raw_thing_t *raw)
{
  index = _idx;

  x = LE_S16(raw->x);
  y = LE_S16(raw->y);

  type    = LE_U16(raw->type);
  options = LE_U16(raw->options);
  angle   = LE_S16(raw->angle);
}

thing_c::thing_c(int _idx, const raw_hexen_thing_t *raw)
{
  index = _idx;

  x = LE_S16(raw->x);
  y = LE_S16(raw->y);

  type    = LE_U16(raw->type);
  options = LE_U16(raw->options);
  angle   = LE_S16(raw->angle);
}

thing_c::~thing_c()
{
  /* nothing to free */
}


void level_c::GetThings()
{
  int count = -1;
  wad_lump_c *lump = base->FindLumpInLevel("THINGS");

  if (lump)
  {
    base->CacheLump(lump);
    count = lump->length / sizeof(raw_thing_t);
  }

  if (!lump || count == 0)
  {
    // Note: no error if no things exist, even though technically a map
    // will be unplayable without the player starts.
    return;
  }

#ifdef DEBUG_LOAD
  PrintDebug("GetThings: num = %d\n", count);
#endif

  things.resize(count);

  raw_thing_t *raw = (raw_thing_t *) lump->data;

  for (int i = 0; i < count; i++, raw++)
  {
    things[i] = new thing_c(i, raw);
  }
}


void level_c::GetThingsHexen()
{
  int count = -1;
  wad_lump_c *lump = base->FindLumpInLevel("THINGS");

  if (lump)
  {
    base->CacheLump(lump);
    count = lump->length / sizeof(raw_hexen_thing_t);
  }

  if (!lump || count == 0)
  {
    // Note: no error if no things exist, even though technically a map
    // will be unplayable without the player starts.
    return;
  }

#ifdef DEBUG_LOAD
  PrintDebug("GetThingsHexen: num = %d\n", count);
#endif

  things.resize(count);

  raw_hexen_thing_t *raw = (raw_hexen_thing_t *) lump->data;

  for (int i = 0; i < count; i++, raw++)
  {
    things[i] = new thing_c(i, raw);
  }
}


//------------------------------------------------------------
//  SIDEDEFS
//------------------------------------------------------------

sidedef_c::sidedef_c(level_c *lev, int _idx, const raw_sidedef_t *raw)
{
  index = _idx;

  sector = lev->LookupSector(LE_U16(raw->sector));

  x_offset = LE_S16(raw->x_offset);
  y_offset = LE_S16(raw->y_offset);

  memcpy(upper_tex, raw->upper_tex, 8);
  memcpy(lower_tex, raw->lower_tex, 8);
  memcpy(mid_tex,   raw->mid_tex,   8);

  // make sure these strings are NUL terminated
  upper_tex[8] = 0;
  lower_tex[8] = 0;
  mid_tex[8]   = 0;
}

sidedef_c::~sidedef_c()
{
  /* nothing to free */
}


void level_c::GetSidedefs()
{
  int count = -1;
  wad_lump_c *lump = base->FindLumpInLevel("SIDEDEFS");

  if (lump)
  {
    base->CacheLump(lump);
    count = lump->length / sizeof(raw_sidedef_t);
  }

  if (!lump || count == 0)
    Main_FatalError("Couldn't find any Sidedefs!\n");

#ifdef DEBUG_LOAD
  PrintDebug("GetSidedefs: num = %d\n", count);
#endif

  sides.resize(count);

  raw_sidedef_t *raw = (raw_sidedef_t *) lump->data;

  for (int i = 0; i < count; i++, raw++)
  {
    sides[i] = new sidedef_c(this, i, raw);
  }
}

sidedef_c * level_c::LookupSidedef(u16_t idx)
{
  // an index of -1 ALWAYS means NULL
  if (idx == 0xFFFF)
    return NULL;

  if (idx < sides.size())
    return sides[idx];
 
  // allow other negative numbers to also mean NULL
  if ((s16_t)idx < 0)
    return NULL;

  Main_FatalError("Bad sidedef index '%d' in map!\n", (int)idx);
  return NULL; /* NOT REACHED */
}


//------------------------------------------------------------
//  LINEDEFS
//------------------------------------------------------------

linedef_c::linedef_c(level_c *lev, int _idx, const raw_linedef_t *raw)
{
  index = _idx;

  start = lev->LookupVertex(LE_U16(raw->start));
  end   = lev->LookupVertex(LE_U16(raw->end));

  /* check for zero-length line */
  zero_len = (fabs(start->x - end->x) < 0.1) && 
             (fabs(start->y - end->y) < 0.1);

  flags = LE_U16(raw->flags);
  type  = LE_U16(raw->type);
  tag   = LE_S16(raw->tag);

  two_sided = (flags & LINEFLAG_TWO_SIDED) ? true : false;

  right = lev->LookupSidedef(LE_U16(raw->sidedef1));
  left  = lev->LookupSidedef(LE_U16(raw->sidedef2));
}

linedef_c::linedef_c(level_c *lev, int _idx, const raw_hexen_linedef_t *raw)
{
  index = _idx;

  start = lev->LookupVertex(LE_U16(raw->start));
  end   = lev->LookupVertex(LE_U16(raw->end));

  // check for zero-length line
  zero_len = (fabs(start->x - end->x) < 0.1) && 
             (fabs(start->y - end->y) < 0.1);

  flags = LE_U16(raw->flags);
  type  = raw->type;
  tag   = 0;

  /* read specials */
  for (int j = 0; j < 5; j++)
    specials[j] = raw->specials[j];

  // -JL- Added missing twosided flag handling that caused a broken reject
  two_sided = (flags & LINEFLAG_TWO_SIDED) ? true : false;

  right = lev->LookupSidedef(LE_U16(raw->sidedef1));
  left  = lev->LookupSidedef(LE_U16(raw->sidedef2));
}

linedef_c::~linedef_c()
{
  /* nothing to free */
}


void level_c::GetLinedefs()
{
  int count = -1;
  wad_lump_c *lump = base->FindLumpInLevel("LINEDEFS");

  if (lump)
  {
    base->CacheLump(lump);
    count = lump->length / sizeof(raw_linedef_t);
  }

  if (!lump || count == 0)
    Main_FatalError("Couldn't find any Linedefs!\n");

#ifdef DEBUG_LOAD
  PrintDebug("GetLinedefs: num = %d\n", count);
#endif

  lines.resize(count);

  raw_linedef_t *raw = (raw_linedef_t *) lump->data;

  for (int i = 0; i < count; i++, raw++)
  {
    lines[i] = new linedef_c(this, i, raw);
  }
}


void level_c::GetLinedefsHexen()
{
  int count = -1;
  wad_lump_c *lump = base->FindLumpInLevel("LINEDEFS");

  if (lump)
  {
    base->CacheLump(lump);
    count = lump->length / sizeof(raw_hexen_linedef_t);
  }

  if (!lump || count == 0)
    Main_FatalError("Couldn't find any Linedefs!\n");

#ifdef DEBUG_LOAD
  PrintDebug("GetLinedefsHexen: num = %d\n", count);
#endif

  lines.resize(count);

  raw_hexen_linedef_t *raw = (raw_hexen_linedef_t *) lump->data;

  for (int i = 0; i < count; i++, raw++)
  {
    lines[i] = new linedef_c(this, i, raw);
  }
}


//------------------------------------------------------------
//  WHOLE LEVEL STUFF
//------------------------------------------------------------


level_c::level_c(wad_c *wad) : base(wad),
      verts(), sectors(), sides(), lines(), things()
{ }

level_c::~level_c()
{
  base = NULL;

  int i;

  for (i=0; i < (int)verts.size(); i++)   delete verts[i];
  for (i=0; i < (int)sectors.size(); i++) delete sectors[i];
  for (i=0; i < (int)sides.size(); i++)   delete sides[i];
  for (i=0; i < (int)lines.size(); i++)   delete lines[i];
  for (i=0; i < (int)things.size(); i++)  delete things[i];
}


level_c * level_c::LoadLevel(wad_c *wad)
{
  // ---- Normal stuff ----

///!!!  if (! wad->FindLevel(level_name))
///!!!    FatalError("Unable to find level: %s\n", level_name);

  level_c *lev = new level_c(wad);
    
  lev->is_hexen = (wad->FindLumpInLevel("BEHAVIOR") != NULL);

  lev->GetVertexes();
  lev->GetSectors();
  lev->GetSidedefs();

  if (lev->is_hexen)
  {
    lev->GetLinedefsHexen();
    lev->GetThingsHexen();
  }
  else // doom
  {
    lev->GetLinedefs();
    lev->GetThings();
  }

  DebugPrintf("Loaded %d vertexes, %d sectors, %d things, %d sides, %d lines\n", 
    (int)lev->verts.size(), (int)lev->sectors.size(),
    (int)lev->things.size(),
    (int)lev->sides.size(), (int)lev->lines.size());

  return lev;
}


void level_c::ComputeBounds(double *lx, double *ly, double *hx, double *hy)
{
  *lx = +99999.0; *ly = +99999.0;
  *hx = -99999.0; *hy = -99999.0;
  
  for (int i = 0; i < (int)lines.size(); i++)
  {
    for (int vert = 0; vert < 2; vert++)
    {
      vertex_c *v = vert ? lines[i]->end : lines[i]->start;

      if (v->x < *lx) *lx = v->x;
      if (v->y < *ly) *ly = v->y;
      if (v->x > *hx) *hx = v->x;
      if (v->y > *hy) *hy = v->y;
    }
  }
}

//--- editor settings ---
// vi:ts=2:sw=2:expandtab
