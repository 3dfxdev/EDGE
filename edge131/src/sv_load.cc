//----------------------------------------------------------------------------
//  EDGE New SaveGame Handling (Loading)
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2005  The EDGE Team.
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
//----------------------------------------------------------------------------
//
// See the file "docs/save_sys.txt" for a complete description of the
// new savegame system.
//

#include "i_defs.h"
#include "sv_chunk.h"

#include "dm_state.h"
#include "e_main.h"
#include "g_game.h"
#include "m_math.h"
#include "m_random.h"
#include "p_local.h"
#include "p_spec.h"
#include "r_state.h"
#include "sv_chunk.h"
#include "sv_main.h"
#include "w_wad.h"
#include "wi_stuff.h"
#include "z_zone.h"

#include <string.h>

static savestruct_t *loaded_struct_list;
static savearray_t  *loaded_array_list;


//----------------------------------------------------------------------------
//
//  ADMININISTRATION
//

static void AddLoadedStruct(savestruct_t *S)
{
	S->next = loaded_struct_list;
	loaded_struct_list = S;
}

static void AddLoadedArray(savearray_t *A)
{
	A->next = loaded_array_list;
	loaded_array_list = A;
}

//
// SV_LookupLoadedStruct
//
savestruct_t *SV_LookupLoadedStruct(const char *name)
{
	savestruct_t *cur;

	for (cur=loaded_struct_list; cur; cur=cur->next)
		if (strcmp(cur->struct_name, name) == 0)
			return cur;

	// not found
	return NULL;
}

//
// SV_LookupLoadedArray
//
savearray_t *SV_LookupLoadedArray(const char *name)
{
	savearray_t *cur;

	for (cur=loaded_array_list; cur; cur=cur->next)
		if (strcmp(cur->array_name, name) == 0)
			return cur;

	// not found
	return NULL;
}


//----------------------------------------------------------------------------
//
//  LOADING STUFF
//

//
// SV_BeginLoad
//
// Prepare main code for loading, e.g. initialise some lists.
//
void SV_BeginLoad(void)
{
	savestruct_t *S;
	savearray_t *A;

	L_WriteDebug("SV_BeginLoad...\n");

	loaded_struct_list = NULL;
	loaded_array_list  = NULL;

	// clear counterpart fields
	for (S=sv_known_structs; S; S=S->next)
		S->counterpart = NULL;

	for (A=sv_known_arrays; A; A=A->next)
		A->counterpart = NULL;
}

static void LoadFreeStruct(savestruct_t *cur)
{
	Z_Free((char *)cur->struct_name);
	Z_Free((char *)cur->marker);
	Z_Free(cur->fields);
	Z_Free(cur);
}

static void LoadFreeArray(savearray_t *cur)
{
	Z_Free((char *)cur->array_name);
	Z_Free(cur);
}

//
// SV_FinishLoad
//
// Finalise all the arrays, and free some stuff after loading has
// finished.
//
void SV_FinishLoad(void)
{
	L_WriteDebug("SV_FinishLoad...\n");

	while (loaded_struct_list)
	{
		savestruct_t *cur = loaded_struct_list;
		loaded_struct_list = cur->next;

		LoadFreeStruct(cur);
	}

	while (loaded_array_list)
	{
		savearray_t *cur = loaded_array_list;
		loaded_array_list = cur->next;

		if (cur->counterpart)
		{
			(* cur->counterpart->finalise_elems)();
		}

		LoadFreeArray(cur);
	}
}

static savefield_t *StructFindField(savestruct_t *info, const char *name)
{
	savefield_t *cur;

	for (cur=info->fields; cur->type.kind != SFKIND_Invalid; cur++)
	{
		if (strcmp(name, cur->field_name) == 0)
			return cur;
	}

	return NULL;
}

static void StructSkipField(savefield_t *field)
{
	char marker[6];
	const char *str;
	int i;

	switch (field->type.kind)
	{
	case SFKIND_Struct:
		SV_GetMarker(marker);
		//!!! compare marker with field->type.name
		SV_SkipReadChunk(marker);
		break;

	case SFKIND_String:
		str = SV_GetString();
		Z_Free((char *)str);
		break;

	case SFKIND_Numeric:
	case SFKIND_Index:
		for (i=0; i < field->type.size; i++)
			SV_GetByte();
		break;

	default:
		I_Error("SV_LoadStruct: BAD TYPE IN FIELD.\n");
	}
}

//
// SV_LoadStruct
//
// The savestruct_t here is the "loaded" one.
//
bool SV_LoadStruct(void *base, savestruct_t *info)
{
	char marker[6];
	savefield_t *cur, *actual;
	char *storage;
	int offset;
	int i;

	SV_GetMarker(marker);

	if (strcmp(marker, info->marker) != 0 || ! SV_PushReadChunk(marker))
		return false;

	for (cur=info->fields; cur->type.kind != SFKIND_Invalid; cur++)
	{
		actual = cur->known_field;

		// if this field no longer exists, ignore it
		if (! actual)
		{
			for (i=0; i < cur->count; i++)
				StructSkipField(cur);
			continue;
		}

		SYS_ASSERT(actual->field_get);
		SYS_ASSERT(info->counterpart);

		offset = actual->offset_p - info->counterpart->dummy_base;

		storage = ((char *) base) + offset;

		for (i=0; i < cur->count; i++)
		{
			// if there are extra elements in the savegame, ignore them
			if (i >= actual->count)
			{
				StructSkipField(cur);
				continue;
			}
			switch (actual->type.kind)
			{
			case SFKIND_Struct:
			case SFKIND_Index:
				(* actual->field_get)(storage, i, (char *)actual->type.name);
				break;

			default:
				(* actual->field_get)(storage, i, NULL);
				break;
			}
		}
	}

	SV_PopReadChunk();

	return true;
}

static bool SV_LoadSTRU(void)
{
	savestruct_t *S;
	savefield_t *F;

	int i, numfields;

	S = Z_ClearNew(savestruct_t, 1);

	numfields = SV_GetInt();

	S->struct_name = SV_GetString();
	S->counterpart = SV_MainLookupStruct(S->struct_name);

	// make the counterparts refer to each other
	if (S->counterpart)
	{
		SYS_ASSERT(S->counterpart->counterpart == NULL);
		S->counterpart->counterpart = S;
	}

	S->marker = SV_GetString();

	if (strlen(S->marker) != 4)
		I_Error("LOADGAME: Corrupt savegame (STRU bad marker)\n");

	S->fields = Z_ClearNew(savefield_t, numfields+1);

	//
	// -- now load in all the fields --
	//

	for (i=0, F=S->fields; i < numfields; i++, F++)
	{
		F->type.kind = (savefieldkind_e) SV_GetByte();
		F->type.size = SV_GetByte();
		F->count = SV_GetShort();
		F->field_name = SV_GetString();

		if (F->type.kind == SFKIND_Struct ||
			F->type.kind == SFKIND_Index)
		{
			F->type.name = SV_GetString();
		}

		F->known_field = NULL;

		if (S->counterpart)
			F->known_field = StructFindField(S->counterpart, F->field_name);

		// ??? compare names for STRUCT and INDEX
	}

	// terminate the array
	F->type.kind = SFKIND_Invalid;

	AddLoadedStruct(S);

	return true;
}

static bool SV_LoadARRY(void)
{
	const char *struct_name;

	savearray_t *A;

	A = Z_ClearNew(savearray_t, 1);

	A->loaded_size = SV_GetInt();

	A->array_name = SV_GetString();
	A->counterpart = SV_MainLookupArray(A->array_name);

	// make the counterparts refer to each other
	if (A->counterpart)
	{
		SYS_ASSERT(A->counterpart->counterpart == NULL);
		A->counterpart->counterpart = A;
	}

	struct_name = SV_GetString();
	A->sdef = SV_LookupLoadedStruct(struct_name);

	if (A->sdef == NULL)
		I_Error("LOADGAME: Coding Error ! (no STRU `%s' for ARRY)\n", struct_name);

	Z_Free((char *)struct_name);

	// create array
	if (A->counterpart)
	{
		(* A->counterpart->create_elems)(A->loaded_size);
	}

	AddLoadedArray(A);

	return true;
}

static bool SV_LoadDATA(void)
{
	const char *array_name;
	savearray_t *A;

	int i;

	array_name = SV_GetString();

	A = SV_LookupLoadedArray(array_name);

	if (! A)
		I_Error("LOADGAME: Coding Error ! (no ARRY `%s' for DATA)\n", array_name);

	Z_Free((char *)array_name);

	// nothing to load if not known
	if (! A->counterpart)
		return true;

	for (i=0; i < A->loaded_size; i++)
	{
		//??? check error too ???
		if (SV_RemainingChunkSize() == 0)
			return false;

		sv_current_elem = (* A->counterpart->get_elem)(i);

		if (! sv_current_elem)
			I_Error("SV_LoadDATA: FIXME: skip elems\n");

		if (! SV_LoadStruct(sv_current_elem, A->sdef))
			return false;
	}

	///  if (SV_RemainingChunkSize() != 0)   //???
	///    return false;

	return true;
}

//
// SV_LoadEverything
//
bool SV_LoadEverything(void)
{
	char marker[6];
	bool result;

	for (;;)
	{
		if (SV_GetError() != 0)
			break;  /// FIXME: set error !!

		SV_GetMarker(marker);

		if (strcmp(marker, DATA_END_MARKER) == 0)
			break;

		// Structure Area
		if (strcmp(marker, "Stru") == 0)
		{
			SV_PushReadChunk("Stru");
			result = SV_LoadSTRU();
			result = SV_PopReadChunk() && result;

			if (! result)
				return false;

			continue;
		}

		// Array Area
		if (strcmp(marker, "Arry") == 0)
		{
			SV_PushReadChunk("Arry");
			result = SV_LoadARRY();
			result = SV_PopReadChunk() && result;

			if (! result)
				return false;

			continue;
		}

		// Data Area
		if (strcmp(marker, "Data") == 0)
		{
			SV_PushReadChunk("Data");
			result = SV_LoadDATA();
			result = SV_PopReadChunk() && result;

			if (! result)
				return false;

			continue;
		}

		I_Warning("LOADGAME: Unexpected top-level chunk [%s]\n", marker);

		if (! SV_SkipReadChunk(marker))
			return false;
	}

	return true;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
