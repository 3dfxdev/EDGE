//------------------------------------------------------------------------
//  BASIC OBJECT HANDLING
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor (C) 2001-2009 Andrew Apted
//                     (C) 1997-2003 Andr� Majorel et al
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

#include <algorithm>

#include "e_basis.h"
#include "m_strings.h"


extern int MadeChanges;


std::vector<Thing *>   Things;
std::vector<Vertex *>  Vertices;
std::vector<Sector *>  Sectors;
std::vector<SideDef *> SideDefs;
std::vector<LineDef *> LineDefs;
std::vector<RadTrig *> RadTrigs;


string_table_c basis_strtab;


const char * Sector::FloorTex() const
{
	return basis_strtab.get(floor_tex);
}

const char * Sector::CeilTex() const
{
	return basis_strtab.get(ceil_tex);
}

const char * SideDef::UpperTex() const
{
	return basis_strtab.get(upper_tex);
}

const char * SideDef::MidTex() const
{
	return basis_strtab.get(mid_tex);
}

const char * SideDef::LowerTex() const
{
	return basis_strtab.get(lower_tex);
}


Sector * SideDef::SecRef() const
{
	return Sectors[sector];
}

Vertex * LineDef::Start() const
{
	return Vertices[start];
}

Vertex * LineDef::End() const
{
	return Vertices[end];
}

SideDef * LineDef::Right() const
{
	return (right >= 0) ? SideDefs[right] : NULL;
}

SideDef * LineDef::Left() const
{
	return (left >= 0) ? SideDefs[left] : NULL;
}


//------------------------------------------------------------------------

static void RawInsertThing(int objnum, int *ptr)
{
	SYS_ASSERT(0 <= objnum && objnum <= NumThings);

	Things.push_back(NULL);

	for (int n = NumThings-1; n > objnum; n--)
		Things[n] = Things[n - 1];

	Things[objnum] = (Thing*) ptr;
}

static void RawInsertLineDef(int objnum, int *ptr)
{
	SYS_ASSERT(0 <= objnum && objnum <= NumThings);

	LineDefs.push_back(NULL);

	for (int n = NumLineDefs-1; n > objnum; n--)
		LineDefs[n] = LineDefs[n - 1];

	LineDefs[objnum] = (LineDef*) ptr;
}

static void RawInsertVertex(int objnum, int *ptr)
{
	SYS_ASSERT(0 <= objnum && objnum <= NumVertices);

	Vertices.push_back(NULL);

	for (int n = NumVertices-1; n > objnum; n--)
		Vertices[n] = Vertices[n - 1];

	Vertices[objnum] = (Vertex*) ptr;

	// fix references in linedefs

	if (objnum+1 < NumVertices)
	{
		for (int n = NumLineDefs-1; n >= 0; n--)
		{
			LineDef *L = LineDefs[n];

			if (L->start >= objnum)
				L->start++;

			if (L->end >= objnum)
				L->end++;
		}
	}
}

static void RawInsertSideDef(int objnum, int *ptr)
{
	SYS_ASSERT(0 <= objnum && objnum <= NumSideDefs);

	SideDefs.push_back(NULL);

	for (int n = NumSideDefs-1; n > objnum; n--)
		SideDefs[n] = SideDefs[n - 1];

	SideDefs[objnum] = (SideDef*) ptr;

	// fix the linedefs references

	if (objnum+1 < NumSideDefs)
	{
		for (int n = NumLineDefs-1; n >= 0; n--)
		{
			LineDef *L = LineDefs[n];

			if (L->right >= objnum)
				L->right++;

			if (L->left >= objnum)
				L->left++;
		}
	}
}

static void RawInsertSector(int objnum, int *ptr)
{
	SYS_ASSERT(0 <= objnum && objnum <= NumSectors);

	Sectors.push_back(NULL);

	for (int n = NumSectors-1; n > objnum; n--)
		Sectors[n] = Sectors[n - 1];

	Sectors[objnum] = (Sector*) ptr;

	// fix all sidedef references

	if (objnum+1 < NumSectors)
	{
		for (int n = NumSideDefs-1; n >= 0; n--)
		{
			SideDef *S = SideDefs[n];

			if (S->sector >= objnum)
				S->sector++;
		}
	}
}

static void RawInsertRadTrig(int objnum, int *ptr)
{
	SYS_ASSERT(0 <= objnum && objnum <= NumRadTrigs);

	RadTrigs.push_back(NULL);

	for (int n = NumRadTrigs-1; n > objnum; n--)
		RadTrigs[n] = RadTrigs[n - 1];

	RadTrigs[objnum] = (RadTrig*) ptr;
}


static int * RawDeleteThing(int objnum)
{
	SYS_ASSERT(0 <= objnum && objnum < NumThings);

	int * result = (int*) Things[objnum];

	for (int n = objnum; n < NumThings-1; n++)
		Things[n] = Things[n + 1];

	Things.pop_back();

	return result;
}

static void RawInsert(obj_type_t objtype, int objnum, int *ptr)
{
	switch (objtype)
	{
		case OBJ_THINGS:
			RawInsertThing(objnum, ptr);

		case OBJ_LINEDEFS:
			RawInsertLineDef(objnum, ptr);

		case OBJ_VERTICES:
			RawInsertVertex(objnum, ptr);

		case OBJ_SIDEDEFS:
			RawInsertSideDef(objnum, ptr);
		
		case OBJ_SECTORS:
			RawInsertSector(objnum, ptr);

		case OBJ_RADTRIGS:
			RawInsertRadTrig(objnum, ptr);

		default:
			nf_bug ("RawInsert: bad objtype %d", (int) objtype);
	}
}


static int * RawDeleteLineDef(int objnum)
{
	SYS_ASSERT(0 <= objnum && objnum < NumLineDefs);

	int * result = (int*) LineDefs[objnum];

	for (int n = objnum; n < NumLineDefs-1; n++)
		LineDefs[n] = LineDefs[n + 1];
	
	LineDefs.pop_back();

	return result;
}

static int * RawDeleteVertex(int objnum)
{
	SYS_ASSERT(0 <= objnum && objnum < NumVertices);

	int * result = (int*) Vertices[objnum];

	for (int n = objnum; n < NumVertices-1; n++)
		Vertices[n] = Vertices[n + 1];

	Vertices.pop_back();

	// fix the linedef references

	if (objnum < NumVertices)
	{
		for (int n = NumLineDefs-1; n >= 0; n--)
		{
			LineDef *L = LineDefs[n];

			if (L->start > objnum)
				L->start--;

			if (L->end > objnum)
				L->end--;
		}
	}

	return result;
}

static int * RawDeleteSideDef(int objnum)
{
	SYS_ASSERT(0 <= objnum && objnum < NumSideDefs);

	int * result = (int*) SideDefs[objnum];

	for (int n = objnum; n < NumSideDefs-1; n++)
		SideDefs[n] = SideDefs[n + 1];

	SideDefs.pop_back();

	// fix the linedefs references

	if (objnum < NumSideDefs)
	{
		for (int n = NumLineDefs-1; n >= 0; n--)
		{
			LineDef *L = LineDefs[n];

			if (L->right > objnum)
				L->right--;

			if (L->left > objnum)
				L->left--;
		}
	}

	return result;
}

static int * RawDeleteSector(int objnum)
{
	SYS_ASSERT(0 <= objnum && objnum < NumSectors);

	int * result = (int*) Sectors[objnum];

	for (int n = objnum; n < NumSectors-1; n++)
		Sectors[n] = Sectors[n + 1];

	Sectors.pop_back();

	// fix sidedef references

	if (objnum < NumSectors)
	{
		for (int n = NumSideDefs-1; n >= 0; n--)
		{
			SideDef *S = SideDefs[n];

			if (S->sector > objnum)
				S->sector--;
		}
	}

	return result;
}

static int * RawDeleteRadTrig(int objnum)
{
	SYS_ASSERT(0 <= objnum && objnum < NumRadTrigs);

	int * result = (int*) RadTrigs[objnum];

	for (int n = objnum; n < NumRadTrigs-1; n++)
		RadTrigs[n] = RadTrigs[n + 1];

	RadTrigs.pop_back();

	return result;
}

static int * RawDelete(obj_type_t objtype, int objnum)
{
	switch (objtype)
	{
		case OBJ_THINGS:
			return RawDeleteThing(objnum);

		case OBJ_LINEDEFS:
			return RawDeleteLineDef(objnum);

		case OBJ_VERTICES:
			return RawDeleteVertex(objnum);

		case OBJ_SIDEDEFS:
			return RawDeleteSideDef(objnum);
		
		case OBJ_SECTORS:
			return RawDeleteSector(objnum);

		case OBJ_RADTRIGS:
			return RawDeleteRadTrig(objnum);

		default:
			nf_bug ("RawDelete: bad objtype %d", (int) objtype);
	}
}


void edit_op_c::Apply()
{
	int * pos;

	switch (action)
	{
		case OP_CHANGE:
			pos = RawGetBase(objtype, objnum);
			std::swap(pos[field], value);
			return;

		case OP_DELETE:
			ptr = RawDelete(objtype, objnum);
			action = OP_INSERT;  // reverse the operation
			return;

		case OP_INSERT:
			RawInsert(objtype, objnum, ptr);
			ptr = NULL;
			action = OP_DELETE;  // reverse the operation
			return;

		default:
			nf_bug("edit_op_c::Apply");
	}
}


void ApplyGroup(op_group_c& grp)
{
	int total = (int)grp.size();

	for (int i = 0; i < total; i++)
		grp[i].Apply();

	// reverse order of all operations
	
	for (int i = 0; i < total/2; i++)
		std::swap(grp[i], grp[total-1-i]);
}


//------------------------------------------------------------------------


void AnalyseChange(op_group_c& grp, obj_type_t type, int objnum,
                   byte field, int new_val)
{
	edit_op_c op;

	op.action  = OP_CHANGE;
	op.objtype = type;
	op.field   = field;
	op.objnum  = objnum;
	op.value   = new_val;

	grp.push_back(op);

	op->Apply();
}


void AnalyseDelete(op_group_c& grp, obj_type_t type, int objnum);


static void AnalyseDeleteVertex(op_group_c& grp, int objnum)
{
	// delete any linedefs bound to this vertex

	for (int n = NumLineDefs-1; n >= 0; n--)
	{
		LineDef *L = LineDefs[n];

		if (L->start == objnum || L->end == objnum)
			AnalyseDelete(grp, OBJ_LINEDEFS, n);
	}
}

static void AnalyseDeleteSideDef(op_group_c& grp, int objnum)
{
	// unbind from linedefs

	for (int n = NumLineDefs-1; n >= 0; n--)
	{
		LineDef *L = LineDefs[n];

		if (L->right == objnum)
			AnalyseChange(grp, OBJ_LINEDEFS, n, LineDef::F_RIGHT, -1);

		if (L->left == objnum)
			AnalyseChange(grp, OBJ_LINEDEFS, n, LineDef::F_LEFT, -1);
	}
}

static void AnalyseDeleteSector(op_group_c& grp, int objnum)
{
	// delete the sidedefs bound to this sector

	for (int n = NumSideDefs-1; n >= 0; n--)
	{
		SideDef *S = SideDefs[n];

		if (S->sector == objnum)
			AnalyseDelete(grp, OBJ_SIDEDEFS, n);
	}
}


void AnalyseDelete(op_group_c& grp, obj_type_t type, int objnum)
{
	edit_op_c op;

	op.action  = OP_DELETE;
	op.objtype = type;
	op.objnum  = objnum;

	// must analyse sidedefs before the deletion
	if (type == OBJ_SIDEDEFS)
		AnalyseDeleteSideDef(grp, objnum);

	grp.push_back(op);

	op->Apply();

	// these must be done after the deletion
	if (type == OBJ_VERTICES)
		AnalyseDeleteVertex(grp, objnum);

	if (type == OBJ_SECTORS)
		AnalyseDeleteSector(grp, objnum);
}


void AnalyseCreate(op_group_c& grp, obj_type_t type)
{
	edit_op_c op;

	op.action  = OP_INSERT;
	op.objtype = type;

	switch (type)
	{
		case OBJ_THINGS:   op.objnum = NumThings;   break;
		case OBJ_VERTICES: op.objnum = NumVertices; break;
		case OBJ_SIDEDEFS: op.objnum = NumSideDefs; break;
		case OBJ_LINEDEFS: op.objnum = NumLineDefs; break;
		case OBJ_SECTORS:  op.objnum = NumSectors;  break;
		case OBJ_RADTRIGS: op.objnum = NumRadTrigs; break;
	}

	grp.push_back(op);

	op->Apply();
}


//------------------------------------------------------------------------

enum
{
	OP_CHANGE = 'c',
	OP_INSERT = 'i',
	OP_DELETE = 'd',
};


class edit_op_c
{
public:
	char action;
	byte objtype;
	byte field;
	byte _pad;

	int objnum;

	int *ptr;
	int value;

public:
	 edit_op_c() : action(0), objtype(0), field(0), objnum(0), ptr(NULL), value(0) { }
	~edit_op_c() { }

	void Apply();

private:
	int *GetStructBase();
};


typedef std::vector<edit_op_c> op_group_c;


void BA_Begin();
{
	// TODO
}

void BA_End();
{
	// TODO
}

int BA_New(obj_type_t type);
{
	// TODO
}

void BA_Delete(obj_type_t type, int objnum);
{
	// TODO
}

bool BA_Change(obj_type_t type, int objnum, byte field, int value);
{
	// TODO
}

bool BA_Undo();
{
	// TODO
}

bool BA_Redo();
{
	// TODO
}


int BA_InternaliseString(const char *str);
{
	return basis_strtab.add(str);
}

const char *BA_GetString(int offset);
{
	return basis_strtab.get(offset);
}



/* HELPERS */

bool BA_ChangeTH(int thing, byte field, int value);
{
	SYS_ASSERT(is_thing(thing));

	return BA_Change(OBJ_THINGS, thing, field, value);
}

bool BA_ChangeLD(int line,  byte field, int value);
{
	SYS_ASSERT(is_linedef(line));
	// TODO
}

bool BA_ChangeSD(int side,  byte field, int value);
{
	SYS_ASSERT(is_sidedef(side));
	// TODO
}

bool BA_ChangeSEC(int sec,  byte field, int value);
{
	SYS_ASSERT(is_sector(sec));
	// TODO
}

bool BA_ChangeVT(int vert,  byte field, int value);
{
	SYS_ASSERT(is_vertex(vert));
	// TODO
}

bool BA_ChangeRAD(int rad,   byte field, int value);
{
	SYS_ASSERT(is_radtrig(rad));
	// TODO
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
