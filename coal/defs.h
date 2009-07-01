/*  Copyright (C) 1996-1997  Id Software, Inc.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

    See file, 'COPYING', for details.
*/


#ifndef __DEFS_CC_H__
#define __DEFS_CC_H__

typedef unsigned char byte;

typedef int	func_t;
typedef int	string_t;

typedef enum
{
	ev_INVALID = -1,

	ev_void = 0,
	ev_string,
	ev_float,
	ev_vector,
	ev_entity,
	ev_field,
	ev_function,
	ev_pointer
}
etype_t;


#define	OFS_NULL		0
#define	OFS_RETURN		1
#define	OFS_PARM0		4		// leave 3 ofs for each parm to hold vectors
#define	OFS_PARM1		7
#define	OFS_PARM2		10
#define	OFS_PARM3		13
#define	OFS_PARM4		16
#define	OFS_PARM5		19
#define	OFS_PARM6		22
#define	OFS_PARM7		25
#define	RESERVED_OFS	28


enum
{
	OP_DONE,

	OP_NOT_F,
	OP_NOT_V,
	OP_NOT_S,
	OP_NOT_FNC,

	OP_POWER_F,
	OP_MUL_F,
	OP_MUL_V,
	OP_MUL_FV,
	OP_MUL_VF,

	OP_DIV_F,
	OP_DIV_V,
	OP_MOD_F,

	OP_ADD_F,
	OP_ADD_V,
	OP_SUB_F,
	OP_SUB_V,

	OP_EQ_F,
	OP_EQ_V,
	OP_EQ_S,
	OP_EQ_FNC,

	OP_NE_F,
	OP_NE_V,
	OP_NE_S,
	OP_NE_FNC,

	OP_LE,
	OP_GE,
	OP_LT,
	OP_GT,

	OP_MOVE_F,
	OP_MOVE_V,
	OP_MOVE_S,
	OP_MOVE_FNC,

	OP_CALL,
	OP_RETURN,

	OP_IF,
	OP_IFNOT,

	OP_GOTO,
	OP_AND,
	OP_OR,

	OP_BITAND,
	OP_BITOR
};


#define	MAX_PARMS	8

typedef struct
{
	short op;
	short a, b, c;
}
statement_t;


typedef struct
{
	// these two are offsets into the strings[] buffer
	int		s_name;
	int		s_file;			// source file defined in

	int		parm_start;
	int		parm_num;
	byte	parm_size[MAX_PARMS];

	int		locals;				// total ints of parms + locals

	int		first_statement;	// negative numbers are builtins
}
function_t;

#endif // __DEFS_CC_H__

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
