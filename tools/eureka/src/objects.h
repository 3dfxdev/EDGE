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


void  HighlightSelection (int, SelPtr);
obj_no_t GetMaxObjectNum (int);
void  GetCurObject (Objid& o, int objtype, int x, int y);
void  SelectObjectsInBox (selection_c *list, int, int, int, int, int);
void  HighlightObject (int, int, int);

void  DeleteObject (const Objid& obj);
void  DeleteObjects (selection_c * list);
int   InsertObject (obj_type_t, obj_no_t, int, int);
void  CopyObjects (selection_c * list);

bool  IsLineDefInside (int, int, int, int, int);
int GetOppositeSector (int, bool);
bool  MoveObjectsToCoords (int, SelPtr, int, int, int);
void  GetObjectCoords (int, int, int *, int *);
int FindFreeTag (void);
void GoToObject (const Objid& objid);


#endif  /* DO NOT ADD ANYTHING AFTER THIS LINE */
//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
