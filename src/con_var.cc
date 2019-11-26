//----------------------------------------------------------------------------
//  EDGE Console Variables
//----------------------------------------------------------------------------
// 
//  Copyright (c) 2007-2009  The EDGE Team.
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

#include "system/i_defs.h"

#include "con_var.h"
#include "con_main.h"

#include "m_argv.h"

#include <stdio.h>

#include "z_zone.h"  // Noooooooooooooo!

cvar_link_t *all_cvars_list = NULL;

cvar_c::~cvar_c()
{

}

cvar_c& cvar_c::operator= (int value)
{
	if(type == cvar_type_e::CVT_FLOAT) {
		*f = value;
	} else if(type == cvar_type_e::CVT_INT) {
		*d = value;		
	} else if(type == cvar_type_e::CVT_STRING) {
		*str = std::to_string(value);
	}

	modified++;
	return *this;
}

cvar_c& cvar_c::operator= (float value)
{
	if(type == cvar_type_e::CVT_FLOAT) {
		*f = value;
	} else if(type == cvar_type_e::CVT_INT) {
		*d = I_ROUND(value);		
	} else if(type == cvar_type_e::CVT_STRING) {
		*str = std::to_string(value);
	}

	modified++;
	return *this;
}

cvar_c& cvar_c::operator= (const char *value)
{
	if(type == cvar_type_e::CVT_STRING) {
		*str = value;
	} else if(type == cvar_type_e::CVT_INT) {
		*d = atoi(value);
	} else if(type == cvar_type_e::CVT_FLOAT) {
		*f = atof(value);
	}

	modified++;
	return *this;
}

cvar_c& cvar_c::operator= (std::string value)
{
	return operator=(value.c_str());
}

//----------------------------------------------------------------------------

static bool CON_MatchFlags(const char *var_f, const char *want_f)
{
	for (; *want_f; want_f++)
	{
		if (isupper(*want_f))
		{
			if (strchr(var_f, tolower(*want_f)))
				return false;
		}
		else
		{
			if (! strchr(var_f, *want_f))
				return false;
		}
	}

	return true;
}


void CON_ResetAllVars(bool initial)
{
	cvar_link_t *link = all_cvars_list;
	while(link) {
		link->var->reset_to_default();
		/**all_cvars[i].var = all_cvars[i].def_val;*/

		// this function is equivalent to construction,
		// hence ensure the modified count is zero.
		if (initial)
			link->var->modified = 0;

		link = link->next;
	}
}


cvar_link_t * CON_FindVar(const char *name)
{
	cvar_link_t *link = all_cvars_list;
	while(link) {
		if (stricmp(link->name, name) == 0)
			return link;
		link = link->next;
	}

	return NULL;
}


bool CON_MatchPattern(const char *name, const char *pat)
{
	while (*name && *pat)
	{
		if (*name != *pat)
			return false;

		name++;
		pat++;
	}

	return (*pat == 0);
}


int CON_MatchAllVars(std::vector<const char *>& list,
                     const char *pattern, const char *flags)
{
	list.clear();

	cvar_link_t *link = all_cvars_list;
	while(link) {
		if (! CON_MatchPattern(link->name, pattern)) {
			link = link->next;

			continue;
		}

		if (! CON_MatchFlags(link->flags, flags)) {
			link = link->next;

			continue;
		}

		list.push_back(link->name);

		link = link->next;
	}

	return (int)list.size();
}


bool CON_SetVar(const char *name, const char *flags, const char *value)
{
	bool no_alias = false;

	if (*flags == 'A')
	{
		no_alias = true;
		flags++;
	}

	cvar_link_t *L = CON_FindVar(name);

	if (! L)
	{
		CON_Printf("No such cvar: %s\n", name);
		return false;
	}

	if (! CON_MatchFlags(L->flags, flags))
	{
		CON_Printf("Cannot set cvar: %s\n", name);
		return false;
	}

	*L->var = value;

	return true;
}


void CON_HandleProgramArgs(void)
{
	for (int p = 1; p < M_GetArgCount(); p++)
	{
		const char *s = M_GetArgument(p);

		if (s[0] != '-')
			continue;

		cvar_link_t *link = CON_FindVar(s+1);

		if (! link)
			continue;

		p++;

		if (p >= M_GetArgCount() || M_GetArgument(p)[0] == '-')
		{
			I_Error("Missing value for option: %s\n", s);
			continue;
		}

		*link->var = M_GetArgument(p);
	}
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
