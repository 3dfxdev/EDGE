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

#ifndef __QCC_H__
#define __QCC_H__


// offset in global data block (if > 0)
// when < 0, it is offset into local stack frame
typedef int	gofs_t;

#define	MAX_PARMS	8

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


//============================================================================

// pr_loc.h -- program local defs

#define	MAX_ERRORS		20

#define	MAX_NAME		64		// chars long

#define	MAX_REGS		16384

//=============================================================================

extern	int		type_size[8];

extern	type_t	type_void, type_string, type_float, type_vector,
                type_entity, type_function;


extern type_t * all_types;  // except built-in types
extern def_t  * all_defs;

typedef struct
{
	char *name;
	int op;  // OP_XXX
	int priority;
	bool right_associative;
	type_t *type_a, *type_b, *type_c;
}
opcode_t;


//============================================================================


extern const char *opcode_names[];

extern bool pr_dumpasm;

extern def_t *pr_global_defs[MAX_REGS];	// to find def for a global variable

typedef enum
{
	tt_eof,			// end of file reached
	tt_name, 		// an alphanumeric name token
	tt_punct, 		// code punctuation
	tt_immediate,	// string, float, vector
}
token_type_t;

extern	char		pr_token[2048];
extern	token_type_t	pr_token_type;
extern	type_t		*pr_immediate_type;
extern	double		pr_immediate[3];

void PR_PrintStatement (statement_t *s);

void PR_Lex (void);
// reads the next token into pr_token and classifies its type

type_t *PR_ParseType (void);
char *PR_ParseName (void);

bool PR_Check (char *string);
void PR_Expect (char *string);
void PR_ParseError (char *error, ...);


class parse_error_x
{
public:
	int foo;

	 parse_error_x() { }
	~parse_error_x() { }
};


extern	int			pr_source_line;
extern	char		*pr_file_p;


#define	OFS_NULL		0
#define	OFS_RETURN		1
#define	OFS_PARM0		4		// leave 3 ofs for each parm to hold vectors
#define	OFS_PARM1		7
#define	OFS_PARM2		10
#define	OFS_PARM3		13
#define	OFS_PARM4		16
#define	RESERVED_OFS	28


extern	def_t	*pr_scope;
extern	int		pr_error_count;

void PR_NewLine (void);
def_t *PR_GetDef (type_t *type, char *name, def_t *scope, bool allocate);

void PR_PrintDefs (void);

void PR_SkipToSemicolon (void);

extern	char		pr_parm_names[MAX_PARMS][MAX_NAME];
extern	bool	pr_trace;

#define	G_FLOAT(o) (pr_globals[o])
#define	G_VECTOR(o) (&pr_globals[o])
#define	G_STRING(o) (strings + (int)pr_globals[o])
#define	G_FUNCTION(o) (pr_globals[o])


bool	PR_CompileFile (char *string, char *filename);

extern	bool	pr_dumpasm;

extern	string_t	s_file;			// filename for function definition

extern	def_t	def_ret, def_parms[MAX_PARMS];

//=============================================================================

#define	MAX_STRINGS		500000
#define	MAX_GLOBALS		16384
#define	MAX_FIELDS		1024
#define	MAX_STATEMENTS	65536
#define	MAX_FUNCTIONS	8192

#define	MAX_DATA_PATH	64

extern	char	strings[MAX_STRINGS];
extern	int		strofs;

extern	statement_t	statements[MAX_STATEMENTS];
extern	int			numstatements;
extern	int			statement_linenums[MAX_STATEMENTS];

extern	function_t  functions[MAX_FUNCTIONS];
extern	int			numfunctions;

extern	double		pr_globals[MAX_REGS];
extern	int			numpr_globals;

extern	char	pr_immediate_string[2048];

int	CopyString (char *str);

#endif

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
