//----------------------------------------------------------------------------
//  EDGE Console Variable Code
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2004  The EDGE Team.
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

#ifndef __CON_CVAR__
#define __CON_CVAR__

typedef struct cvartype_s cvartype_t;
typedef struct cvar_s cvar_t;
typedef struct function_s function_t;
typedef struct funclist_s funclist_t;

// Console variables (cvars)
typedef enum
{
	// Readable
	cf_read = 1,
	// Writable.  Player properties are not writable!
	cf_write = 2,
	// Normal: cf_read|cf_write
	cf_normal = 3,
	// cvar is in z_memory.  If it is deleted, it will be Z_Freed
	// this is handled individually by each type
	cf_mem = 4,
	// Delete: cvar can be deleted.
	cf_delete = 8
}
cflag_t;

extern cvartype_t cvar_bool;
extern cvartype_t cvar_int;
extern cvartype_t cvar_real;
extern cvartype_t cvar_str;
extern cvartype_t cvar_enum;

void CON_AddFunctionToList(funclist_t * fl, const char *name, const char *description, void (*func) (void), void (*onchange) (funclist_t *));
void CON_InitFunctionList(funclist_t * fl, const char *cvarname, void (*default_func) (void), void (*onchange) (funclist_t *));
void CON_SetFunclistDest(funclist_t * fl, void (**dest) (void));

// Sets an existing cvar (if writable)
bool CON_SetCVar(const char *name, const char *value);

// Gets an existing cvar value (if readable) pass the address of a
// pointer to the data you want, eg int *p; CON_GetCVar("health", &p);
// Must be done this way because using a void
bool CON_GetCVar(const char *name, const void **value);

// more low-level version of GetCVar.
// Usable in callbacks and similar, when it's simpler.
// Never use this if the cvar is referenced by name, since this doesn't do
// any read-only test.
const void *CON_CVarGetValue(const cvar_t * var);

// Creates a new value.
bool CON_CreateCVar(const char *name, cflag_t flags, const cvartype_t * type, void *value);

// Deletes a cvar.
bool CON_DeleteCVar(const char *name);

// Some special create routines for common types.
// The value pointer points to the variable that is changed whenever the
// cvar is changed. If NULL is passed, a new value will be created internally
bool CON_CreateCVarInt(const char *name, cflag_t flags, int *value);
bool CON_CreateCVarStr(const char *name, cflag_t flags, char *value, int maxlen);
bool CON_CreateCVarBool(const char *name, cflag_t flags, bool * value);
bool CON_CreateCVarReal(const char *name, cflag_t flags, float * value);
bool CON_CreateCVarEnum(const char *name, cflag_t flags, void *value, const char *names, int num);

// Adds a callback hook, which will be called whenever the value is changed.
// The callback will get the cvar as parameter. Normally, it should only
// read from var->value.
void CON_AddCVarCallback(cvar_t * var, void (*callback) (cvar_t * var, void *user), void *user, void (*kill_user) (void *));

void CON_ChooseFunctionFromList(funclist_t *fl, const char *funcname);

#endif // __CON_CVAR__
