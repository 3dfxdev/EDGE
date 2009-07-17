//------------------------------------------------------------------------
//  BASIC OBJECT HANDLING
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

#include "main.h"

#include <algorithm>
#include <list>

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


int BA_InternaliseString(const char *str)
{
	return basis_strtab.add(str);
}

const char *BA_GetString(int offset)
{
	return basis_strtab.get(offset);
}


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
			break;

		case OBJ_LINEDEFS:
			RawInsertLineDef(objnum, ptr);
			break;

		case OBJ_VERTICES:
			RawInsertVertex(objnum, ptr);
			break;

		case OBJ_SIDEDEFS:
			RawInsertSideDef(objnum, ptr);
			break;
		
		case OBJ_SECTORS:
			RawInsertSector(objnum, ptr);
			break;

		case OBJ_RADTRIGS:
			RawInsertRadTrig(objnum, ptr);
			break;

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

		case OBJ_VERTICES:
			return RawDeleteVertex(objnum);

		case OBJ_SECTORS:
			return RawDeleteSector(objnum);

		case OBJ_SIDEDEFS:
			return RawDeleteSideDef(objnum);
		
		case OBJ_LINEDEFS:
			return RawDeleteLineDef(objnum);

		case OBJ_RADTRIGS:
			return RawDeleteRadTrig(objnum);

		default:
			nf_bug ("RawDelete: bad objtype %d", (int) objtype);
	}
}


static int * RawGetBase(obj_type_t objtype, int objnum)
{
	switch (objtype)
	{
		case OBJ_THINGS:
			return (int*) Things[objnum];

		case OBJ_VERTICES:
			return (int*) Vertices[objnum];

		case OBJ_SECTORS:
			return (int*) Sectors[objnum];

		case OBJ_SIDEDEFS:
			return (int*) SideDefs[objnum];
		
		case OBJ_LINEDEFS:
			return (int*) LineDefs[objnum];

		case OBJ_RADTRIGS:
			return (int*) RadTrigs[objnum];

		default:
			nf_bug ("RawGetBase: bad objtype %d", (int) objtype);
	}
}

static void DeleteFinally(obj_type_t objtype, int *ptr)
{
	switch (objtype)
	{
		case OBJ_THINGS:   delete (Thing *)   ptr; break;
		case OBJ_VERTICES: delete (Vertex *)  ptr; break;
		case OBJ_SECTORS:  delete (Sector *)  ptr; break;
		case OBJ_SIDEDEFS: delete (SideDef *) ptr; break;
		case OBJ_LINEDEFS: delete (LineDef *) ptr; break;
		case OBJ_RADTRIGS: delete (RadTrig *) ptr; break;

		default:
			nf_bug ("DeleteFinally: bad objtype %d", (int) objtype);
	}
}


//------------------------------------------------------------------------
//  BASIS API IMPLEMENTATION
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
	edit_op_c() : action(0), objtype(0), field(0), objnum(0), ptr(NULL), value(0)
	{ }

	~edit_op_c()
	{
		if (action == OP_INSERT)
		{
			SYS_ASSERT(ptr);
			DeleteFinally((obj_type_t) objtype, ptr);
		}
		else
		{
			SYS_ASSERT(! ptr);
		}
	}

	void Apply()
	{
		int * pos;

		switch (action)
		{
			case OP_CHANGE:
				pos = RawGetBase((obj_type_t)objtype, objnum);
				std::swap(pos[field], value);
				return;

			case OP_DELETE:
				ptr = RawDelete((obj_type_t)objtype, objnum);
				action = OP_INSERT;  // reverse the operation
				return;

			case OP_INSERT:
				RawInsert((obj_type_t)objtype, objnum, ptr);
				ptr = NULL;
				action = OP_DELETE;  // reverse the operation
				return;

			default:
				nf_bug("edit_op_c::Apply");
		}
	}
};


class undo_group_c
{
private:
	std::vector<edit_op_c> ops;

	int dir;

public:
	undo_group_c() : ops(), dir(+1)
	{ }

	~undo_group_c()
	{ }

	void Add_Apply(edit_op_c& op)
	{
		ops.push_back(op);
		ops.back().Apply();
	}

	void End()
	{
		dir = -1;
	}

	void ReApply()
	{
		int total = (int)ops.size();

		if (dir > 0)
			for (int i = 0; i < total; i++)
				ops[i].Apply();
		else
			for (int i = total-1; i >= 0; i--)
				ops[i].Apply();

		// reverse the order for next time
		dir = -dir;
	}
};


static undo_group_c *cur_group;

static std::list<undo_group_c *> undo_history;
static std::list<undo_group_c *> redo_future;


static void ClearUndoHistory()
{
	std::list<undo_group_c *>::iterator LI;

	for (LI = undo_history.begin(); LI != undo_history.end(); LI++)
		delete *LI;
}

static void ClearRedoFuture()
{
	std::list<undo_group_c *>::iterator LI;

	for (LI = redo_future.begin(); LI != redo_future.end(); LI++)
		delete *LI;
}


void BA_Begin()
{
	if (cur_group)
		nf_bug("BA_Begin called twice without BA_End\n");

	ClearRedoFuture();

	cur_group = new undo_group_c();
}

void BA_End()
{
	if (! cur_group)
		nf_bug("BA_End called without a previous BA_Begin\n");

	cur_group->End();

	undo_history.push_front(cur_group);

	cur_group = NULL;
}


int BA_New(obj_type_t type)
{
	edit_op_c op;

	op.action  = OP_INSERT;
	op.objtype = type;

	switch (type)
	{
		case OBJ_THINGS:
			op.objnum = NumThings;
			op.ptr = (int*) new Thing;
			break;

		case OBJ_VERTICES:
			op.objnum = NumVertices;
			op.ptr = (int*) new Vertex;
			break;

		case OBJ_SIDEDEFS:
			op.objnum = NumSideDefs;
			op.ptr = (int*) new SideDef;
			break;

		case OBJ_LINEDEFS:
			op.objnum = NumLineDefs;
			op.ptr = (int*) new LineDef;
			break;

		case OBJ_SECTORS:
			op.objnum = NumSectors;
			op.ptr = (int*) new Sector;
			break;

		case OBJ_RADTRIGS:
			op.objnum = NumRadTrigs;
			op.ptr = (int*) new RadTrig;
			break;

		default: nf_bug("BA_New");
	}

	SYS_ASSERT(cur_group);

	cur_group->Add_Apply(op);
}


void BA_Delete(obj_type_t type, int objnum)
{
	edit_op_c op;

	op.action  = OP_DELETE;
	op.objtype = type;
	op.objnum  = objnum;

	// this must happen  _before_ doing the deletion (otherwise
	// when we undo, the insertion will mess up the references).
	if (type == OBJ_SIDEDEFS)
	{
		// unbind sidedef from any linedefs using it
		for (int n = NumLineDefs-1; n >= 0; n--)
		{
			LineDef *L = LineDefs[n];

			if (L->right == objnum)
				BA_ChangeLD(n, LineDef::F_RIGHT, -1);

			if (L->left == objnum)
				BA_ChangeLD(n, LineDef::F_LEFT, -1);
		}
	}

	SYS_ASSERT(cur_group);

	cur_group->Add_Apply(op);

	/* these must be done after the deletion */

	if (type == OBJ_VERTICES)
	{
		// delete any linedefs bound to this vertex
		for (int n = NumLineDefs-1; n >= 0; n--)
		{
			LineDef *L = LineDefs[n];

			if (L->start == objnum || L->end == objnum)
				BA_Delete(OBJ_LINEDEFS, n);
		}
	}
	else if (type == OBJ_SECTORS)
	{
		// delete the sidedefs bound to this sector
		for (int n = NumSideDefs-1; n >= 0; n--)
		{
			if (SideDefs[n]->sector == objnum)
				BA_Delete(OBJ_SIDEDEFS, n);
		}
	}
}


bool BA_Change(obj_type_t type, int objnum, byte field, int value)
{
	edit_op_c op;

	op.action  = OP_CHANGE;
	op.objtype = type;
	op.field   = field;
	op.objnum  = objnum;
	op.value   = value;

	SYS_ASSERT(cur_group);

	cur_group->Add_Apply(op);
}


bool BA_Undo()
{
	if (undo_history.empty())
		return false;

	undo_group_c * grp = undo_history.front();
	undo_history.pop_front();

	grp->ReApply();

	redo_future.push_front(grp);
	return true;
}

bool BA_Redo()
{
	if (redo_future.empty())
		return false;
	
	undo_group_c * grp = redo_future.front();
	redo_future.pop_front();

	grp->ReApply();

	undo_history.push_front(grp);
	return true;
}


void BA_ClearAll()
{
	int i;

	for (i = 0; i < NumThings;   i++) delete Things[i];
	for (i = 0; i < NumVertices; i++) delete Vertices[i];
	for (i = 0; i < NumSectors;  i++) delete Sectors[i];
	for (i = 0; i < NumSideDefs; i++) delete SideDefs[i];
	for (i = 0; i < NumLineDefs; i++) delete LineDefs[i];
	for (i = 0; i < NumRadTrigs; i++) delete RadTrigs[i];

	Things.clear();
	Vertices.clear();
	Sectors.clear();
	SideDefs.clear();
	LineDefs.clear();
	RadTrigs.clear();

	basis_strtab.clear();

	ClearUndoHistory();
	ClearRedoFuture();
}


/* HELPERS */

bool BA_ChangeTH(int thing, byte field, int value)
{
	SYS_ASSERT(is_thing(thing));
	SYS_ASSERT(field <= Thing::F_OPTIONS);

	return BA_Change(OBJ_THINGS, thing, field, value);
}

bool BA_ChangeVT(int vert, byte field, int value)
{
	SYS_ASSERT(is_vertex(vert));
	SYS_ASSERT(field <= Vertex::F_Y);

	return BA_Change(OBJ_VERTICES, vert, field, value);
}

bool BA_ChangeSEC(int sec, byte field, int value)
{
	SYS_ASSERT(is_sector(sec));
	SYS_ASSERT(field <= Sector::F_TAG);

	return BA_Change(OBJ_SECTORS, sec, field, value);
}

bool BA_ChangeSD(int side, byte field, int value)
{
	SYS_ASSERT(is_sidedef(side));
	SYS_ASSERT(field <= SideDef::F_SECTOR);

	return BA_Change(OBJ_SIDEDEFS, side, field, value);
}

bool BA_ChangeLD(int line, byte field, int value)
{
	SYS_ASSERT(is_linedef(line));
	SYS_ASSERT(field <= LineDef::F_LEFT);

	return BA_Change(OBJ_LINEDEFS, line, field, value);
}

bool BA_ChangeRAD(int rad, byte field, int value)
{
	SYS_ASSERT(is_radtrig(rad));
	SYS_ASSERT(field <= RadTrig::F_CODE);

	return BA_Change(OBJ_RADTRIGS, rad, field, value);
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
