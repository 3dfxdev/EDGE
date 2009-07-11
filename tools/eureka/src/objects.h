//------------------------------------------------------------------------
//  OBJECT STUFF
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


#ifndef YH_OBJECTS  /* DO NOT INSERT ANYTHING BEFORE THIS LINE */
#define YH_OBJECTS


// Object types
#define OBJ_NONE  0
#define OBJ_THINGS  1
#define OBJ_LINEDEFS  2
#define OBJ_SIDEDEFS  3
#define OBJ_VERTICES  4
#define OBJ_SEGS  5         // UNUSED
#define OBJ_SSECTORS  6         // UNUSED
#define OBJ_NODES 7         // UNUSED
#define OBJ_SECTORS 8
#define OBJ_REJECT  9      // UNUSED
#define OBJ_BLOCKMAP  10   // UNUSED
#define OBJ_RSCRIPT  11

// Special object numbers
typedef s16_t  obj_no_t;
typedef char obj_type_t;
#define OBJ_NO_NONE    -1
#define OBJ_NO_CANVAS  -2
#define is_obj(n)      ((n) >= 0)
#define is_linedef(n)  ((n) >= 0 && (n) < NumLineDefs)
#define is_sector(n)   ((n) >= 0 && (n) < NumSectors )
#define is_sidedef(n)  ((n) >= 0 && (n) < NumSideDefs)
#define is_thing(n)    ((n) >= 0 && (n) < NumThings  )
#define is_vertex(n)   ((n) >= 0 && (n) < NumVertices)

class Objid
{
  public :
    Objid () { num = -1; type = OBJ_NONE; }
    Objid (obj_type_t t, obj_no_t n) { type = t; num = n; }
    bool operator== (const Objid& other) const
    {
      return other.type == type && other.num == num;
    }
    bool _is_linedef () const { return type == OBJ_LINEDEFS && num >= 0; }
    bool _is_sector  () const { return type == OBJ_SECTORS  && num >= 0; }
    bool _is_thing   () const { return type == OBJ_THINGS   && num >= 0; }
    bool _is_vertex  () const { return type == OBJ_VERTICES && num >= 0; }
    bool is_nil     () const { return num <  0 || type == OBJ_NONE; }
    bool operator() () const { return num >= 0 && type != OBJ_NONE; } 
    void nil () { num = -1; type = OBJ_NONE; }
    obj_type_t type;
    obj_no_t   num;
};


void  HighlightSelection (int, SelPtr);
obj_no_t GetMaxObjectNum (int);
void  GetCurObject (Objid& o, int objtype, int x, int y);
void  SelectObjectsInBox (SelPtr *list, int, int, int, int, int);
void  HighlightObject (int, int, int);
void  DeleteObject (const Objid&);
void  DeleteObjects (int, SelPtr *);
void  InsertObject (obj_type_t, obj_no_t, int, int);
bool  IsLineDefInside (int, int, int, int, int);
int GetOppositeSector (int, bool);
void  CopyObjects (int, SelPtr);
bool  MoveObjectsToCoords (int, SelPtr, int, int, int);
void  GetObjectCoords (int, int, int *, int *);
int FindFreeTag (void);


#endif  /* DO NOT ADD ANYTHING AFTER THIS LINE */
//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
