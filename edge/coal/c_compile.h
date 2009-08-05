//----------------------------------------------------------------------
//  COAL EXECUTION ENGINE
//----------------------------------------------------------------------
// 
//  Copyright (C)      2009  Andrew Apted
//  Copyright (C) 1996-1997  Id Software, Inc.
//
//  Coal is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as
//  published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  Coal is distributed in the hope that it will be useful, but
//  WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
//  the GNU General Public License for more details.
//
//----------------------------------------------------------------------

#ifndef __COAL_COMPILER_BITS_H__
#define __COAL_COMPILER_BITS_H__


typedef enum
{
	tt_eof,			// end of file reached
	tt_name, 		// an alphanumeric name token
	tt_punct, 		// code punctuation
	tt_literal,  	// string, float, vector
}
token_e;


class compiling_c
{
public:
	const char *source_file;
	int source_line;
	int function_line;

	// current parsing position
	char *parse_p;
	char *line_start;	// start of current source line
	int  bracelevel;
	int  parentheses;
	int  fol_level;    // fol = first on line

	// current token (from LEX_Next)
	char    token_buf[2048];
	token_e token_type;
	bool    token_is_first;

	char    literal_buf[2048];
	type_t *literal_type;
	double  literal_value[3];

	// parameter names (when parsing a function def)
	char parm_names[MAX_PARMS][MAX_NAME];

	int error_count;

	type_t * all_types;
	def_t  * all_defs;

	// all temporaries for current function
	std::vector<def_t *> temporaries;

	std::vector<def_t *> constants;

	// the function being parsed, or NULL
	def_t  * scope;

	// for tracking local variables vs temps
	int locals_end;

public:
	 compiling_c();
	~compiling_c();
};

#endif /* __COAL_COMPILER_BITS_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
