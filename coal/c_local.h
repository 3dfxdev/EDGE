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


typedef struct
{
	int op;
	int a, b, c;
}
statement_t;


enum
{
	OP_DONE,
	OP_DONE_V,

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

	OP_IF,
	OP_IFNOT,

	OP_GOTO,
	OP_AND,
	OP_OR,

	OP_BITAND,
	OP_BITOR,

	OP_PARM_F,
	OP_PARM_V
};


#define	MAX_PARMS	16

typedef struct
{
	// these two are offsets into the strings[] buffer
	int		s_name;
	int		s_file;			// source file defined in

	int		return_size;

	int		parm_num;
	short	parm_ofs[MAX_PARMS];
	short	parm_size[MAX_PARMS];

	int		locals_ofs;
	int		locals_size;
	int		locals_end;

	int		first_statement;	// negative numbers are builtins
}
function_t;

// offset in global data block (if > 0)
// when < 0, it is offset into local stack frame
typedef int	gofs_t;


#define	MAX_NAME		64


typedef struct type_s
{
	etype_t			type;

// function types are more complex
	struct type_s	*aux_type;	// return type or field type

	int				parm_num;	// -1 = variable args
	struct type_s	*parm_types[MAX_PARMS];	// only [parm_num] allocated

	struct type_s	*next;
}
type_t;

typedef struct def_s
{
	type_t		*type;
	char		*name;

	gofs_t		ofs;
	struct def_s	*scope;		// function the var was defined in, or NULL
	int			initialized;	// 1 when a declaration included "= immediate"

	struct def_s	*next;
}
def_t;


//=============================================================================

extern	int		type_size[8];

extern	type_t	type_void, type_string, type_float, type_vector,
                type_entity, type_function;


extern type_t * all_types;  // except built-in types
extern def_t  * all_defs;

typedef struct
{
	const char *name;
	int op;  // OP_XXX
	int priority;
	bool right_associative;
	type_t *type_a, *type_b, *type_c;
}
opcode_t;


//=== COMPILER STUFF =========================================//

extern const char *opcode_names[];

void PR_Lex(void);
// reads the next token into pr_token and classifies its type

type_t *PR_ParseType(void);
char *PR_ParseName(void);

bool PR_Check(char *string);
void PR_Expect(char *string);
void PR_ParseError(char *error, ...);


class parse_error_x
{
public:
	int foo;

	 parse_error_x() { }
	~parse_error_x() { }
};


extern	int			pr_source_line;
extern	char		*pr_file_p;

extern	char	pr_immediate_string[2048];

extern	def_t	*pr_scope;
extern	int		pr_error_count;

void PR_NewLine(void);
def_t *PR_GetDef(type_t *type, char *name, def_t *scope, bool allocate);

void PR_SkipToSemicolon(void);

extern char pr_parm_names[MAX_PARMS][MAX_NAME];

extern	string_t	s_file;			// filename for function definition


//=== EXECUTION STUFF ========================================//


typedef void (*builtin_t) (void);
extern builtin_t *pr_builtins;
extern int pr_numbuiltins;

extern int pr_argc;

extern bool pr_trace;


class exec_error_x
{
public:
	int foo;

	 exec_error_x() { }
	~exec_error_x() { }
};


void PR_RunError(const char *error, ...) __attribute__((format(printf,1,2)));

char *PR_GetString(int num);

#endif // __DEFS_CC_H__

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
