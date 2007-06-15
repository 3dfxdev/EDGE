//----------------------------------------------------------------------------
//  EDGE Console Variable Code
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

#include "i_defs.h"
#include "con_cvar.h"

#include "con_defs.h"
#include "z_zone.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

cvar_t **cvars = NULL;
int num_cvars = 0;

/*** Common ***/
// These are shared between the basic types int, bool and real.
static void GetValue_Val(const void *val, const void **dest)
{
	*dest = val;
}

static void KillValue_Val(cvar_t * cvar)
{
	if (cvar->flags & cf_mem)
		Z_Free(cvar->value);
}

/*** Enumeration ***/
typedef struct cvar_enum_s
{
	int  *value;
	// the name of the enum type (e.g. "enum (foo,bar)"
	// it's called typenam just because "typename" is a keyword in GCC.
	char *typenam;
	// list of the names of the values.
	char **names;
	// number of values
	int  num;
}
cvar_enum_t;

static void GetValueStr_Enum(void *val, char s[1024])
{
	cvar_enum_t *v = (cvar_enum_t *)val;

	strcpy(s, v->names[*(v->value)]);
}

static void GetValue_Enum(const void *val, const void **dest)
{
	const cvar_enum_t *v = (const cvar_enum_t *)val;

	*dest = v->value;
}

static bool SetValue_Enum(cvar_t * cvar, int argc, const char **argv)
{
	cvar_enum_t *v = (cvar_enum_t *)cvar->value;
	long n;
	int i;
	char *end;

	for (i = 0; i < v->num; i++)
		if (0 == stricmp(argv[0], v->names[i]))
		{
			*(v->value) = i;
			return true;
		}

	// Check if the argument can be treated as an int
	n = strtol(argv[0], &end, 10);
	if (*end == 0 &&  // it was a valid number
			(n >= 0 && n < v->num))  // it was in a valid range.

	{
		*(v->value) = i;
		return true;
	}

	return false;
}


static void KillValue_Enum(cvar_t * cvar)
{
	cvar_enum_t *v = (cvar_enum_t *)cvar->value;
	int i;

	if (cvar->flags & cf_mem)
		Z_Free(cvar->value);

	for (i = 0; i < v->num; i++)
		Z_Free(v->names[i]);

	Z_Free(v->names);
	Z_Free(v->typenam);
	Z_Free(v);
}

static const char *GetType_Enum(cvar_t * cvar)
{
	cvar_enum_t *v = (cvar_enum_t *)cvar->value;

	return v->typenam;
}

// Adds a possible value to an enum type
static int CON_AddValueToEnum(cvar_enum_t *e, const char *name)
{
	int oldlen = strlen(e->typenam);
	int newlen = oldlen + strlen(name);

	// Change the type name
	// Alloc two extra chars: One for the terminating 0, and one for the '/'.
	Z_Resize(e->typenam, char, newlen + 2);
	e->typenam[oldlen - 1] = '/';
	// Copy the name.
	Z_MoveData(e->typenam + oldlen, name, char, newlen-oldlen);
	// finish the string
	e->typenam[newlen] = ')';
	e->typenam[newlen + 1] = 0;

	// add the name to the value list
	Z_Resize(e->names, char *, ++e->num);
	e->names[e->num - 1] = Z_StrDup(name);

	return e->num - 1;
}

/*** funclist_t type ***/


/*** bool type ***/
static void GetValueStr_Bool(void *val, char s[1024])
{
	bool *v = (bool *)val;

	if (*v)
		strcpy(s, "true");
	else
		strcpy(s, "false");
}


static bool SetValue_Bool(cvar_t * cvar, int argc, const char **argv)
{
	if (0 == stricmp(argv[0], "true") || 0 == stricmp(argv[0], "yes") ||
			0 == strcmp(argv[0], "1"))
		*(bool *) (cvar->value) = true;
	else if (0 == stricmp(argv[0], "false") || 0 == stricmp(argv[0], "no") ||
			0 == strcmp(argv[0], "0"))
		*(bool *) (cvar->value) = false;
	else
		return false;
	return true;
}

static const char *GetType_Bool(cvar_t * cvar)
{
	return "Boolean";
}

/*** Real type ***/
static void GetValueStr_Real(void *val, char s[1024])
{
	float *v = (float *)val;

	sprintf(s, "%f", (double)*v);
}

static bool SetValue_Real(cvar_t * cvar, int argc, const char **argv)
{
	float *v = (float *)cvar->value;
	double f;

	if (1 != sscanf(argv[0], "%lf", &f))
		return false;

	*v = (float)f;

	return true;
}

static const char *GetType_Real(cvar_t * cvar)
{
	return "Real Number";
}

/*** String type ***/
typedef struct
{
	int maxlen;
	char *val;
}
cvar_str_t;

static void GetValueStr_Str(void *val, char s[1024])
{
	cvar_str_t *v = (cvar_str_t *)val;

	Z_StrNCpy(s, v->val, v->maxlen);
}

static void GetValue_Str(const void *val, const void **dest)
{
	const cvar_str_t *v = (const cvar_str_t *)val;

	*dest = v->val;
}

static bool SetValue_Str(cvar_t * cvar, int argc, const char **argv)
{
	cvar_str_t *v = (cvar_str_t *)cvar->value;
	char *s;
	int len = -1;
	int curlen;
	int i;

	if (v->maxlen < 0)
	{  // dynamically reallocate value

		for (i = 0; i < argc; i++)
			len += strlen(argv[i]);
		Z_Resize(v->val, char, len + 1);
		s = v->val;
		for (i = 0; i < argc; i++)
		{
			curlen = strlen(argv[i]);
			Z_MoveData(s, argv[i], char, curlen);
			s += curlen;
			*s++ = ' ';
		}
		s[-1] = 0;
		return true;
	}

	s = v->val;

	for (i = 0; i < argc; i++)
	{
		curlen = strlen(argv[i]);
		len += curlen + 1;
		if (len > v->maxlen || i == argc - 1)
		{  // last time
			Z_StrNCpy(s, argv[i], v->maxlen - len + curlen + 1);
			break;
		}
		sprintf(s, "%s ", argv[i]);
		s += curlen + 1;
	}

	return true;
}

static void KillValue_Str(cvar_t * cvar)
{
	cvar_str_t *v = (cvar_str_t *)cvar->value;

	if (cvar->flags & cf_mem)
		Z_Free(v->val);
	Z_Free(v);
}

static const char *GetType_Str(cvar_t * cvar)
{
	return "String";
}


/*** Integer type ***/
static void GetValueStr_Int(void *val, char s[1024])
{
	int *v = (int*)val;

	sprintf(s, "%d", *v);
}

static bool SetValue_Int(cvar_t * cvar, int argc, const char **argv)
{
	int *v = (int*)cvar->value;

	if (1 != sscanf(argv[0], "%d", v))
		return false;

	return true;
}

static const char *GetType_Int(cvar_t * cvar)
{
	return "Integer";
}

/*** Callbacks ***/
// stores the old values
typedef struct cvar_callback_s
{
	const cvartype_t *type;
	void *value;
	void (*callback) (cvar_t * var, void *user);
	// user data
	void *user;
	// destructor for user data
	void (*kill_user)(void *);
}
cvar_callback_t;

static const char *GetType_Callback(cvar_t * cvar)
{
	const char *s;
	cvar_callback_t *v = (cvar_callback_t *)cvar->value;
	const cvartype_t *type = cvar->type;

	cvar->value = v->value;
	cvar->type = v->type;
	s = cvar->type->get_name(cvar);
	cvar->value = v;
	cvar->type = type;
	return s;
}

static void GetValueStr_Callback(void *val, char s[1024])
{
	cvar_callback_t *v = (cvar_callback_t *)val;

	v->type->get_value_str(v->value, s);
}

static void GetValue_Callback(const void *val, const void **dest)
{
	const cvar_callback_t *v = (const cvar_callback_t *)val;

	v->type->get_value(v->value, dest);
}

static bool SetValue_Callback(cvar_t * cvar, int argc, const char **argv)
{
	bool ok;
	cvar_callback_t *v = (cvar_callback_t *)cvar->value;
	const cvartype_t *type = cvar->type;

	cvar->value = v->value;
	cvar->type = v->type;
	ok = cvar->type->set_value(cvar, argc, argv);
	if (ok)
	{
		v->callback(cvar, v->user);
	}
	v->value = cvar->value;
	v->type = cvar->type;
	cvar->value = v;
	cvar->type = type;
	return ok;
}

static void KillValue_Callback(cvar_t * cvar)
{
	cvar_callback_t *v = (cvar_callback_t *)cvar->value;

	cvar->value = v->value;
	cvar->type = v->type;
	cvar->type->kill_value(cvar);

	if (v->kill_user)
		v->kill_user(v->user);

	Z_Free(v);
}

cvartype_t cvar_bool = {GetType_Bool, GetValueStr_Bool, GetValue_Val, SetValue_Bool, KillValue_Val};
cvartype_t cvar_int = {GetType_Int, GetValueStr_Int, GetValue_Val, SetValue_Int, KillValue_Val};
cvartype_t cvar_real = {GetType_Real, GetValueStr_Real, GetValue_Val, SetValue_Real, KillValue_Val};
cvartype_t cvar_str = {GetType_Str, GetValueStr_Str, GetValue_Str, SetValue_Str, KillValue_Str};
cvartype_t cvar_enum = {GetType_Enum, GetValueStr_Enum, GetValue_Enum, SetValue_Enum, KillValue_Enum};
cvartype_t cvar_callback = {GetType_Callback, GetValueStr_Callback, GetValue_Callback, SetValue_Callback, KillValue_Callback};

//
// CON_CVarFromName
//
// Returns a pointer to the console variable with the given name, or -1
// if it doesn't exist.
// 
static int CON_CVarIndexFromName(const char *name)
{
	int i;

	for (i = 0; i < num_cvars; i++)
		if (!stricmp(name, cvars[i]->name))
			return i;

	return -1;
}
//
// CON_CVarFromName
//
// Returns a pointer to the console variable with the given name, or NULL if
// it doesn't exist.
//
cvar_t *CON_CVarPtrFromName(const char *name)
{
	int i;

	for (i = 0; i < num_cvars; i++)
		if (!stricmp(name, cvars[i]->name))
			return cvars[i];

	return NULL;
}

const void *CON_CVarGetValue(const cvar_t * var)
{
	const void *ret;

	var->type->get_value(var->value, &ret);

	return ret;
}

bool CON_GetCVar(const char *name, const void **value)
{
	cvar_t *var;

	var = CON_CVarPtrFromName(name);

	if (!var || !(var->flags & cf_read))
	{
		*value = NULL;
		return false;
	}

	*value = CON_CVarGetValue(var);

	return true;
}

bool CON_CreateCVar(const char *name, cflag_t flags, const cvartype_t * type, void *value)
{
	cvar_t *var = CON_CVarPtrFromName(name);

	if (var)
	{
		// the variable already existed, check if we can redefine it
		if (!(var->flags & cf_write))
			return false;

		Z_Free(var->name);
	}
	else
	{
		// it didn't already exist, so we create a new one
		Z_Resize(cvars, cvar_t *, ++num_cvars);
		var = cvars[num_cvars - 1] = Z_New(cvar_t, 1);
	}

	var->name = Z_StrDup(name);

	var->flags = flags;
	var->value = value;
	var->type = type;

	return true;
}

bool CON_SetCVar(const char *name, const char *value)
{
	cvar_t *var;

	var = CON_CVarPtrFromName(name);
	if (!var)
		return false;

	if (var->flags & cf_write)
	{
		return var->type->set_value(var, 1, &value);
	}

	return false;
}

bool CON_DeleteCVar(const char *name)
{
	int i = CON_CVarIndexFromName(name);

	if (i == -1 || !(cvars[i]->flags & cf_delete))
		return false;

	cvars[i]->type->kill_value(cvars[i]);

	if (cvars[i]->flags & cf_mem)
		Z_Free(cvars[i]->value);

	Z_Free(cvars[i]->name);
	Z_Free(cvars[i]);

	Z_MoveData(&cvars[i], &cvars[i + 1], cvar_t *, --num_cvars - i);

	return true;
}

bool CON_CreateCVarBool(const char *name, cflag_t flags, bool * value)
{
	if (!value)
	{
		value = Z_New(bool, 1);
		flags = (cflag_t)(flags | cf_mem);
		*value = false;
	}

	return CON_CreateCVar(name, flags, &cvar_bool, value);
}

bool CON_CreateCVarInt(const char *name, cflag_t flags, int *value)
{
	if (!value)
	{
		value = Z_New(int, 1);

		flags = (cflag_t)(flags|cf_mem);
		*value = 0;
	}

	return CON_CreateCVar(name, flags, &cvar_int, value);
}

bool CON_CreateCVarReal(const char *name, cflag_t flags, float * value)
{
	if (!value)
	{
		value = Z_New(float, 1);
		*value = 0;
		flags = (cflag_t)(flags|cf_mem);
	}

	return CON_CreateCVar(name, flags, &cvar_real, value);
}

//
// CON_CreateCVarStr
//
// maxlen shows the maximum string length of value. At least maxlen+1
// characters must be allocated.
//
bool CON_CreateCVarStr(const char *name, cflag_t flags, char *value, int maxlen)
{
	cvar_str_t *v;

	v = Z_New(cvar_str_t, 1);

	if (value)
	{
		v->maxlen = maxlen;
		v->val = value;
	}
	else
	{
		flags = (cflag_t)(flags|cf_mem);
		v->maxlen = -1;
		v->val = Z_New(char, 16);
		v->val[0] = 0;
	}

	return CON_CreateCVar(name, flags, &cvar_str, v);
}

//
// CON_CreateCVarEnum
//
// Names: comma-separated list of names for the enum values.
// Num: Number of possible enum values.
//
// For example: If the enum direction_e would be defined like
// typedef enum {left,right,forward,backward,NUM_DIRECTIONS} direction_e;
// then names would be "left,right,forward,backward" and num would be
// NUM_DIRECTIONS.
//
// -ACB- 1999/09/23 Removed alloca() references (not portable).
//                  Replaced with malloc with Z_New. Don't waste mem.
//
bool CON_CreateCVarEnum(const char *name, cflag_t flags, void *value, const char *names, int num)
{
	cvar_enum_t *v;
	int i;
	char *s;
	char *n;

	n = Z_StrDup(names);

	v = Z_New(cvar_enum_t, 1);

	v->typenam = Z_New(char, strlen(names) + 8);
	sprintf(v->typenam, "enum (%s)", names);

	v->num = num;
	v->names = Z_New(char *, num);

	for (i = 0, s = strtok(n, "/"); s; s = strtok(NULL, "/"), i++)
	{
		if (i >= num)
			I_Error("CON_CreateCVarEnum: %s has more than %d elements!", v->typenam, num);

		v->names[i] = Z_StrDup(s);
	}
	if (i < num)
		I_Error("CON_CreateCVarEnum: %s has less than %d elements!", v->typenam, num);

	if (value)
		v->value = (int *)value;
	else
	{
		v->value = Z_New(int, 1);

		*(v->value) = 0;
		flags = (cflag_t)(flags|cf_mem);
	}

	// free mem after use
	// FIXME: use heap as much as possible. strtok functions to be rewritten
	Z_Free(n);

	return CON_CreateCVar(name, flags, &cvar_enum, v);
}

//
// CON_AddCVarCallback
//
void CON_AddCVarCallback(cvar_t * var, void (*callback) (cvar_t * var, void *user), void *user, void (*kill_user) (void *))
{
	cvar_callback_t *data;

	data = Z_New(cvar_callback_t, 1);
	data->type = var->type;
	data->value = var->value;
	data->callback = callback;

	data->user = user;
	data->kill_user = kill_user;

	var->type = &cvar_callback;
	var->value = data;
}

//
// Function list:
// This is used for handling functions which there can be different versions
// of to choose between. The typical example is the 8-bit column renderers
// (R_DrawColumn8_*), which there both are different C versions and different
// Assembler optimised versions of. This is an attempt to a general way of
// handling such functions.
// There are restrictions on the function: It should not have any parameters,
// and it should not return anything. That should instead be done via global
// variables.
// The function list will mostly be stored in a cvar, which will be an
// extendable enum, with an extra callback hooked in.
//

static void UpdateFunctionList(cvar_t *cvar, void *data)
{
	funclist_t *fl = (funclist_t *)data;
	function_t *f = fl->funcs[fl->current];

	if (fl->dest)
		*(fl->dest) = f->func;

	if (f->onchange)
		f->onchange(fl);
}

//
// CON_ChooseFunctionFromList
//
// Sets the list's active function to the one named funcname.
//
void CON_ChooseFunctionFromList(funclist_t *fl, const char *funcname)
{
	int i;

	if (fl->cvar)
	{
		// this will update the cvar, but also choose the right function through
		// a callback
		CON_SetCVar(fl->cvar->name, funcname);
	}
	else
	{
		// search for it
		for (i = 0; stricmp(fl->funcs[i]->name, funcname); i++)
			if (i >= fl->num)
				I_Error("CON_ChooseFunctionFromList: No function called \"%s\"", funcname);

		fl->current = i;
		UpdateFunctionList(NULL, fl);
	}
}

//
// CON_InitFunctionList
//
// Initialises a newly allocated function list fl, so it can be used.
// There must be at least one function, which also is the default one.
// This is typically the simplest C version of the routine, e.g.
// R_DrawColumn8_CVersion.
// If cvarname is not NULL, a cvar with that name will be created, as an
// enum with all the function names.
//
void CON_InitFunctionList(funclist_t * fl, const char *cvarname, void (*default_func) (void), void (*onchange) (funclist_t *))
{
	function_t *f;

	fl->funcs = Z_New(function_t *, 1);
	f = Z_New(function_t, 1);
	f->func = default_func;
	f->onchange = onchange;
	f->name = Z_StrDup("default");
	f->description = Z_StrDup("The default routine");
	fl->funcs[0] = f;
	// start without destination ptr
	fl->dest = NULL;
	fl->num = 1;
	// start with the default function.
	fl->current = 0;

	if (cvarname && CON_CreateCVarEnum(cvarname, cf_normal, &fl->current, "default", 1))
	{
		fl->cvar = CON_CVarPtrFromName(cvarname);
		fl->donttouch = fl->cvar->value;
		CON_AddCVarCallback(fl->cvar, UpdateFunctionList, fl, NULL);
	}
	else
	{
		fl->cvar = NULL;
		fl->donttouch = NULL;
	}
}

//
// CON_SetFunclistDest
//
// Sets the destination pointer, which always will be updated to
// the active function. If NULL is passed, no funcion pointer will be updated.
// 
void CON_SetFunclistDest(funclist_t * fl, void (**dest) (void))
{
	fl->dest = dest;
	UpdateFunctionList(NULL, fl);
}

//
// CON_AddFunctionToList
//
// Adds a func to the list fl. It will be referenced as the passed name.
// 
void CON_AddFunctionToList(funclist_t * fl, const char *name, const char *description, void (*func) (void), void (*onchange) (funclist_t *))
{
	function_t *f;

#ifdef DEVELOPERS
	int i;

	// check if it already exists in the list
	for (i = 0; i < fl->num; i++)
	{
		if (!stricmp(fl->funcs[i]->name, name))
			I_Error("CON_AddFunctionToList: %s already exists!",fl->funcs[i]->name);
	}
#endif

	Z_Resize(fl->funcs, function_t *, ++fl->num);
	f = Z_New(function_t, 1);
	f->func = func;
	f->onchange = onchange;
	f->name = Z_StrDup(name);
	f->description = Z_StrDup(description);

	fl->funcs[fl->num - 1] = f;

	if (fl->cvar)
	{
		// here's the only place we may touch 'donttouch'
		CON_AddValueToEnum((cvar_enum_t *)fl->donttouch, name);
	}
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
