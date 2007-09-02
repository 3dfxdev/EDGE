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


class node_c;
class sector_c;


class vertex_c
{
public:
  // coordinates
  double x, y;

  // vertex index.  Always valid after loading and pruning of unused
  // vertices has occurred.  For GL vertices, bit 30 will be set.
  int index;

  vertex_c(int _idx, const raw_vertex_t *raw);
  vertex_c(int _idx, const raw_v2_vertex_t *raw);
  ~vertex_c();
};

#define IS_GL_VERTEX  (1 << 30)


class sector_c
{
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

  sector_c(int _idx, const raw_sector_t *raw);
  ~sector_c();
};


class sidedef_c
{
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

  sidedef_c(int _idx, const raw_sidedef_t *raw);
  ~sidedef_c();
};


class linedef_c
{
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

  linedef_c(int _idx, const raw_linedef_t *raw);
  linedef_c(int _idx, const raw_hexen_linedef_t *raw);
  ~linedef_c();
};


class thing_c
{
public:
  int x, y;
  int type;
  int options;

  // other info (angle, and hexen stuff) omitted.  We don't need to
  // write the THING lump, only read it.

  // Always valid (thing indices never change).
  int index;

  thing_c(int _idx, const raw_thing_t *raw);
  thing_c(int _idx, const raw_hexen_thing_t *raw);
  ~thing_c();
};


class seg_c
{
public:
  // link for list
  struct seg_c *next;

  vertex_c *start;   // from this vertex...
  vertex_c *end;     // ... to this vertex

  // linedef that this seg goes along, or NULL if miniseg
  linedef_c *linedef;

  // adjacent sector, or NULL if invalid sidedef or miniseg
  sector_c *sector;

  // 0 for right, 1 for left
  int side;

  // seg index.  Only valid once the seg has been added to a
  // subsector.  A negative value means it is invalid -- there
  // shouldn't be any of these once the BSP tree has been built.
  int index;

  // precomputed data for faster calculations
  double psx, psy;
  double pex, pey;
  double pdx, pdy;

  double p_length, p_angle;
  double p_para,   p_perp;

  seg_c(int _idx, const raw_gl_seg_t *raw);
  seg_c(int _idx, const raw_v3_seg_t *raw);
  ~seg_c();

  void precompute_data();
};


class subsec_c
{
public:
  // list of segs
  seg_c *seg_list;

  // count of segs
  int seg_count;

  // subsector index in lump.
  int index;

  // approximate middle point
  double mid_x;
  double mid_y;

  subsec_c(int _idx, const raw_subsec_t *raw);
  subsec_c(int _idx, const raw_v3_subsec_t *raw);
  ~subsec_c();

  void append_seg(seg_c *cur);
  void build_seg_list(int first, int count);
  // builds the list of segs, and also determines mid point.
};


typedef struct bbox_s
{
  int minx, miny;
  int maxx, maxy;

  void from_raw(const raw_bbox_t *raw);
}
bbox_t;


typedef struct child_s
{
  // child node or subsector (one must be NULL)
  node_c *node;
  subsec_c *subsec;

  // child bounding box
  bbox_t bounds;
}
child_t;


class node_c
{
public:
  int x, y;     // starting point
  int dx, dy;   // offset to ending point

  // right & left children
  child_t r;
  child_t l;

  // node index in lump
  int index;

  node_c (int _idx, const raw_node_t *raw);
  ~node_c();
};


/* ----- Level data arrays ----------------------- */

template <typename TYPE> class container_tp
{
public:
  int num;

private:
  TYPE ** arr;

  const char *const name;

public:
  container_tp(const char *type_name) : num(0), arr(NULL), name(type_name)
  {
  }
  
  ~container_tp()
  {
    if (arr)
      FreeAll();
  }

  void Allocate(int _num)
  {
    if (arr)
      FreeAll();

    num = _num;
    arr = new TYPE* [num];

    for (int i = 0; i < num; i++)
    {
      arr[i] = NULL;  
    }
  }

  void FreeAll()
  {
    for (int i = 0; i < num; i++)
    {
      if (arr[i] != NULL)
        delete arr[i];  
    }

    delete[] arr;

    num = 0;
    arr = NULL;
  }

  void Set(int index, TYPE *cur)
  {
    if (arr[index] != NULL)
      delete arr[index];

    arr[index] = cur;
  }

  TYPE *Get(int index)
  {
    if (index < 0 || index >= num)
    {
      FatalError("No such %s number #%d", name, index);
    }

    return arr[index];
  }
};

#define EXTERN_LEVELARRAY(TYPE, BASEVAR)  \
    extern container_tp<TYPE> BASEVAR;

EXTERN_LEVELARRAY(vertex_c,  lev_vertices)
EXTERN_LEVELARRAY(vertex_c,  lev_gl_verts)
EXTERN_LEVELARRAY(linedef_c, lev_linedefs)
EXTERN_LEVELARRAY(sidedef_c, lev_sidedefs)
EXTERN_LEVELARRAY(sector_c,  lev_sectors)
EXTERN_LEVELARRAY(thing_c,   lev_things)
EXTERN_LEVELARRAY(seg_c,     lev_segs)
EXTERN_LEVELARRAY(subsec_c,  lev_subsecs)
EXTERN_LEVELARRAY(node_c,    lev_nodes)

/* ----- function prototypes ----------------------- */

// load all level data for the current level
void LoadLevel(const char *name);

// free all level data
void FreeLevel(void);

void LevelGetBounds(double *lx, double *ly, double *hx, double *hy);

#endif // __G_LEVEL_H__

//--- editor settings ---
// vi:ts=2:sw=2:expandtab
