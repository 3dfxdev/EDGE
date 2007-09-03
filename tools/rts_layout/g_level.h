//------------------------------------------------------------------------
//  LEVEL : Level structures & read/write functions.
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

#ifndef __G_LEVEL_H__
#define __G_LEVEL_H__

#include "raw_wad.h"
#include "g_wad.h"


class vertex_c
{
public:
   vertex_c(int _idx, const raw_vertex_t *raw);
   vertex_c(int _idx, const raw_v2_vertex_t *raw);
  ~vertex_c();

public:
  // coordinates
  double x, y;
};


class sector_c
{
public:
   sector_c(int _idx, const raw_sector_t *raw);
  ~sector_c();

public:
  // sector index.  Always valid after loading & pruning.
  int index;

  // heights
  int floor_h, ceil_h;

  // textures
  char floor_tex[8];
  char ceil_tex[8];

  // attributes
  int light;
  int special;
  int tag;
};


class sidedef_c
{
public:
   sidedef_c(int _idx, const raw_sidedef_t *raw);
  ~sidedef_c();

public:
  // adjacent sector.  Can be NULL (invalid sidedef)
  sector_c *sector;

  // offset values
  int x_offset, y_offset;

  // texture names
  char upper_tex[8];
  char lower_tex[8];
  char mid_tex[8];

  // sidedef index.  Always valid after loading & pruning.
  int index;
};


class linedef_c
{
public:
   linedef_c(int _idx, const raw_linedef_t *raw);
   linedef_c(int _idx, const raw_hexen_linedef_t *raw);
  ~linedef_c();

public:
  vertex_c *start;    // from this vertex...
  vertex_c *end;      // ... to this vertex

  sidedef_c *right;   // right sidedef
  sidedef_c *left;    // left sidede, or NULL if none

  // line is marked two-sided
  char two_sided;

  // zero length (line should be totally ignored)
  char zero_len;

  int flags;
  int type;
  int tag;

  // Hexen support
  int specials[5];

  // linedef index.  Always valid after loading & pruning of zero
  // length lines has occurred.
  int index;
};


class thing_c
{
public:
   thing_c(int _idx, const raw_thing_t *raw);
   thing_c(int _idx, const raw_hexen_thing_t *raw);
  ~thing_c();

public:
  int x, y;
  int type;
  int options;
  int angle;

  // Always valid (thing indices never change).
  int index;
};


/* ----- Level data ----------------------- */

class level_c
{
private:
  level_c(wad_c *wad);

public:
  ~level_c();
 
public:
  wad_c *base;

  bool is_hexen;

  std::vector<vertex_c *>  verts;
  std::vector<sector_c *>  sectors;
  std::vector<sidedef_c *> sides;
  std::vector<linedef_c *> lines;
  std::vector<thing_c *>   things;

public:
  static level_c *LoadLevel(wad_c *wad);
  // load the level from the WAD, using the current level
  // (so you need to successfully call wad->FindLevel() before).

  void GetBounds(double *lx, double *ly, double *hx, double *hy);

private:
  void GetVertexes();
  void GetSectors();
  void GetThings();
  void GetSidedefs();
  void GetLinedefs();

  void GetThingsHexen();
  void GetLinedefsHexen();
};


#endif // __G_LEVEL_H__

//--- editor settings ---
// vi:ts=2:sw=2:expandtab
