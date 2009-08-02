//------------------------------------------------------------------------
//  OBJECT EDITING
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


void DisplayObjectInfo (const Editor_State_c *e, int);
void input_objid (Objid& objid, const Objid& init, int x0, int y0);

int InputObjectNumber (int, int, int, int);
int InputObjectXRef (int, int, int, bool, int);
bool Input2VertexNumbers (int, int, const char *, int *, int *);
void EditObjectsInfo (int, int, int, SelPtr);
void InsertStandardObject (int, int, int choice);
void MiscOperations (int, SelPtr *, int choice);

void TransferLinedefProperties (int src_linedef, SelPtr linedefs);
void TransferSectorProperties (int src_sector, SelPtr sectors);
void TransferThingProperties (int src_thing, SelPtr things);

const char *GetObjectTypeName (int);
const char *GetEditModeName (int);
const char *GetLineDefFlagsName (int);
const char *GetLineDefFlagsLongName (int);


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
