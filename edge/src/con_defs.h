//----------------------------------------------------------------------------
//  EDGE Console Definitions
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2001  The EDGE Team.
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
// Contains definitions that most cvar users shouldn't need to
// care about.
//

#ifndef __CON_LOCL_H__
#define __CON_LOCL_H__

#include "con_cvar.h"
#include "con_main.h"

struct cvar_s
{
  char *name;
  cflag_t flags;
  const cvartype_t *type;
  void *value;
};

struct cvartype_s
{
  // returns what the type is called, eg "int" or "string" or "boolean"
  const char *(*get_name) (cvar_t * cvar);

  // Converts the value to a string and stores it in s.
  void (*get_value_str) (void *val, char s[1024]);

  // sets *dest to the address of the actual value (the one passed to CreateCVar)
  void (*get_value) (const void *val, const void **dest);

  // Sets the cvar's value according to a parameter list.
  // The parameter list always contains at least one argument.
  // Also use this as callback: it is called whenever the cvar changes.
  boolean_t (*set_value) (cvar_t * cvar, int argc, const char **argv);

  // Called when the cvar is destroyed. Should free the memory of the
  // cvar's value member.
  void (*kill_value) (cvar_t * cvar);
};

struct function_s
{
  // The Function
  void (*func) (void);
  // Called when the function is activated.
  void (*onchange) (funclist_t *);
  // the name of the function. One word.
  char *name;
  // a brief description of the function. Can be displayed as a help message
  char *description;
};

struct funclist_s
{
  // the available functions.
  function_t **funcs;
  int num;
  int current;
  // optionally points to a pointer, that will be changed to the active
  // function whenever it changes.
  void (**dest)(void);
  // optional console variable.
  cvar_t *cvar;
  // for internal use (enum cvar stuff)
  void *donttouch;
};

extern cvar_t **cvars;
extern int num_cvars;

cvar_t *CON_CVarPtrFromName(const char *name);

#endif // __CON_LOCL_H__
//----------------------------------------------------------------------------
//  EDGE Console Definitions
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2001  The EDGE Team.
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
// Contains definitions that most cvar users shouldn't need to
// care about.
//

#ifndef __CON_LOCL_H__
#define __CON_LOCL_H__

#include "con_cvar.h"
#include "con_main.h"

struct cvar_s
{
  char *name;
  cflag_t flags;
  const cvartype_t *type;
  void *value;
};

struct cvartype_s
{
  // returns what the type is called, eg "int" or "string" or "boolean"
  const char *(*get_name) (cvar_t * cvar);

  // Converts the value to a string and stores it in s.
  void (*get_value_str) (void *val, char s[1024]);

  // sets *dest to the address of the actual value (the one passed to CreateCVar)
  void (*get_value) (const void *val, const void **dest);

  // Sets the cvar's value according to a parameter list.
  // The parameter list always contains at least one argument.
  // Also use this as callback: it is called whenever the cvar changes.
  boolean_t (*set_value) (cvar_t * cvar, int argc, const char **argv);

  // Called when the cvar is destroyed. Should free the memory of the
  // cvar's value member.
  void (*kill_value) (cvar_t * cvar);
};

struct function_s
{
  // The Function
  void (*func) (void);
  // Called when the function is activated.
  void (*onchange) (funclist_t *);
  // the name of the function. One word.
  char *name;
  // a brief description of the function. Can be displayed as a help message
  char *description;
};

struct funclist_s
{
  // the available functions.
  function_t **funcs;
  int num;
  int current;
  // optionally points to a pointer, that will be changed to the active
  // function whenever it changes.
  void (**dest)(void);
  // optional console variable.
  cvar_t *cvar;
  // for internal use (enum cvar stuff)
  void *donttouch;
};

extern cvar_t **cvars;
extern int num_cvars;

cvar_t *CON_CVarPtrFromName(const char *name);

#endif // __CON_LOCL_H__
