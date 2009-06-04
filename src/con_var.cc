//----------------------------------------------------------------------------
//  EDGE Console Variables
//----------------------------------------------------------------------------
// 
//  Copyright (c) 2007-2008  The EDGE Team.
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

#include "con_var.h"
#include "con_main.h"


cvar_c::cvar_c(int value) : d(value), f(value), str(buffer)
{
	sprintf(buffer, "%d", value);
}

cvar_c::cvar_c(float value) : d(I_ROUND(value)), f(value), str(buffer)
{
	FmtFloat(value);
}

cvar_c::cvar_c(const char *value) : str(buffer)
{
	DoStr(value);
}

cvar_c::cvar_c(const cvar_c& other) : str(buffer)
{
	DoStr(other.str);
}

cvar_c::~cvar_c()
{
	if (Allocd())
	{
		free((void *) str);
	}
}


cvar_c& cvar_c::operator= (int value)
{
	if (Allocd())
	{
		free((void *) str);
	}

	d = value;
	f = value;

	str = buffer;
	sprintf(buffer, "%d", value);

	return *this;
}

cvar_c& cvar_c::operator= (float value)
{
	if (Allocd())
	{
		free((void *) str);
	}

	d = I_ROUND(value);
	f = value;

	str = buffer;
	FmtFloat(value);

	return *this;
}

cvar_c& cvar_c::operator= (const char *value)
{
	DoStr(value);

	return *this;
}

cvar_c& cvar_c::operator= (std::string value)
{
	DoStr(value.c_str());

	return *this;
}

cvar_c& cvar_c::operator= (const cvar_c& other)
{
	if (&other != this)
	{
		DoStr(other.str);
	}

	return *this;
}

void cvar_c::FmtFloat(float value)
{
	float ab = fabs(value);

	if (ab >= 1e10)  // handle huge numbers
		sprintf(buffer, "%1.5e", value);
	else if (ab >= 1e5)
		sprintf(buffer, "%1.1f", value);
	else if (ab >= 1e3)
		sprintf(buffer, "%1.3f", value);
	else if (ab >= 1.0)
		sprintf(buffer, "%1.5f", value);
	else
		sprintf(buffer, "%1.7f", value);
}

void cvar_c::DoStr(const char *value)
{
	if (Allocd())
	{
		free((void *) str);
	}

	if (strlen(value)+1 <= BUFSIZE)
	{
		str = buffer;
		strcpy(buffer, value);
	}
	else
	{
		str = strdup(value);
	}

	d = atoi(str);
	f = atof(str);
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


static bool CON_MatchPattern(const char *name, const char *pat)
{
	// FIXME !!!
	return false;
}


void CON_ResetAllVars(void)
{
	for (int i = 0; all_cvars[i].var; i++)
	{
		*all_cvars[i].var = all_cvars[i].def_val;
	}
}


cvar_link_t * CON_FindVar(const char *name)
{
	for (int i = 0; all_cvars[i].var; i++)
	{
		if (stricmp(all_cvars[i].name, name) == 0)
			return &all_cvars[i];
	}

	return NULL;
}


int CON_FindMultiVar(std::vector<cvar_link_t *>& list,
                     const char *pattern, const char *flags)
{
	list.clear();

	for (int i = 0; all_cvars[i].var; i++)
	{
		if (! CON_MatchPattern(all_cvars[i].name, pattern))
			continue;

		if (! CON_MatchFlags(all_cvars[i].flags, flags))
			continue;

		list.push_back(&all_cvars[i]);
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


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
