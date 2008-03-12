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

#ifndef __CON_VAR_H__
#define __CON_VAR_H__

class cvar_c
{
public:
	// current value
	int d;
	float f;
	const char *str;

private:
	enum bufsize_e { BUFSIZE = 32 };

	// local buffer used for integers, floats and small strings.
	// in use whenever (s == buffer).  Otherwise s is on the heap,
	// using strdup() and free().
	char buffer[BUFSIZE];

public:
	cvar_c() : d(0), f(0.0f), str(buffer)
	{
		buffer[0] = '0';
		buffer[1] =  0;
	}

	cvar_c(int value);
	cvar_c(float value);
	cvar_c(const char *value);
	cvar_c(const cvar_c& other);

	~cvar_c();

	cvar_c& operator= (int value);
	cvar_c& operator= (float value);
	cvar_c& operator= (const char *value);
	cvar_c& operator= (std::string value);
	cvar_c& operator= (const cvar_c& other);

private:
	inline bool Allocd()
	{
		return (str != NULL) && (str != buffer);
	}

	void FmtFloat(float value);
	void DoStr(const char *value);
};


typedef enum
{
	CV_NONE = 0,

	CV_Config   = (1 << 0),  // saved in user's config file
	CV_Option   = (1 << 1),  // settable from the command line
	CV_ReadOnly = (1 << 2),  // cannot change in console
}
cvar_flag_e;


typedef struct cvar_link_s
{
	// the console variable
	cvar_c *var;

	// various flags
	int flags;

	// name of variable (aliases are separated by commas)
	const char *names;
}
cvar_link_t;


extern cvar_link_t all_cvars[];


#endif // __CON_VAR_H__

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
