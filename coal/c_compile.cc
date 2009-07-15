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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <errno.h>
#include <assert.h>

// #include <sys/signal.h>

#include <vector>

#include "coal.h"


namespace coal
{

#include "c_local.h"

#define MAX_ERRORS  1000


typedef enum
{
	tt_eof,			// end of file reached
	tt_name, 		// an alphanumeric name token
	tt_punct, 		// code punctuation
	tt_immediate,	// string, float, vector
}
token_type_t;


static int			pr_source_line;

static char		*pr_file_p;
static char		*pr_line_start;		// start of current source line

static int			pr_bracelevel;
static int			pr_parentheses;
static int			pr_fol_level;    // first on line level

static char		pr_token[2048];
static token_type_t	pr_token_type;
static bool		pr_token_is_first;
static type_t		*pr_immediate_type;
static double		pr_immediate[3];

static char	pr_immediate_string[2048];

static int		pr_error_count;

static char	*pr_punctuation[] =
// longer symbols must be before a shorter partial match
{"&&", "||", "<=", ">=","==", "!=", "++", "--", "...", "..",
 ":", ";", ",", "!",
 "*", "/", "%", "^","(", ")", "-", "+", "=",
 "[", "]", "{", "}", ".", "<", ">", "#", "&", "|",
 NULL
};

// simple types.  function types are dynamically allocated
static type_t	type_void = {ev_void};
static type_t	type_string = {ev_string};
static type_t	type_float = {ev_float};
static type_t	type_vector = {ev_vector};
static type_t	type_function = {ev_function, &type_void};

static int		type_size[8] = {1,1,1,3,1,1,1,1};

static def_t	def_void = {&type_void, "VOID_SPACE", 0};


static type_t * all_types;
static def_t  * all_defs;

static def_t		*pr_scope;		// the function being parsed, or NULL

static string_t		s_file;			// filename for function definition

static int			locals_end;		// for tracking local variables vs temps

// all temporaries for current function
static std::vector<def_t *> temporaries;

static std::vector<def_t *> constants;


void PR_ParseStatement(bool allow_def);


void PR_InitData(void)
{
	numstatements = 1;
	strofs = 1;
	numfunctions = 1;
}

void PR_ShowStats(void)
{
	printf("%6i strofs\n", strofs);
	printf("%6i numstatements\n", numstatements);
	printf("%6i numfunctions\n", numfunctions);
	printf("%6i numpr_globals\n", numpr_globals);
}


void PR_NewLine(void)
{
	// Called when *pr_file_p == '\n'

	pr_source_line++;
	pr_line_start = pr_file_p + 1;
	pr_fol_level = 0;
}


/*
============
PR_ParseError

Aborts the current file load
============
*/
void PR_ParseError(char *error, ...)
{
	va_list		argptr;
	char		string[1024];

	va_start(argptr,error);
	vsprintf(string,error,argptr);
	va_end(argptr);

	printf("%s:%i:%s\n", strings + s_file, pr_source_line, string);

//  raise(11);
	throw parse_error_x();
}


/*
==============
PR_LexString

Parses a quoted string
==============
*/
void PR_LexString(void)
{
	int		c;
	int		len;

	len = 0;
	pr_file_p++;

	for (;;)
	{
		c = *pr_file_p++;
		if (!c)
			PR_ParseError("EOF inside quote");
		if (c=='\n')
			PR_ParseError("newline inside quote");
		if (c=='\\')
		{	// escape char
			c = *pr_file_p++;
			if (!c)
				PR_ParseError("EOF inside quote");
			if (c == 'n')
				c = '\n';
			else if (c == '"')
				c = '"';
			else
				PR_ParseError("Unknown escape char");
		}
		else if (c=='\"')
		{
			pr_token[len] = 0;
			pr_token_type = tt_immediate;
			pr_immediate_type = &type_string;
			strcpy(pr_immediate_string, pr_token);
			return;
		}
		pr_token[len] = c;
		len++;
	}
}


float PR_LexNumber(void)
{
	int		c;
	int		len;

	len = 0;
	c = *pr_file_p;
	do
	{
		pr_token[len] = c;
		len++;
		pr_file_p++;
		c = *pr_file_p;
	} while ((c >= '0' && c<= '9') || c == '.');

	pr_token[len] = 0;

	return atof(pr_token);
}


void PR_LexVector(void)
{
	// Parses a single quoted vector

	int		i;

	pr_file_p++;
	pr_token_type = tt_immediate;
	pr_immediate_type = &type_vector;

	for (i=0 ; i<3 ; i++)
	{
		// FIXME: check for digits etc!

		pr_immediate[i] = PR_LexNumber();

		while (isspace(*pr_file_p) && *pr_file_p != '\n')
			pr_file_p++;
	}
	if (*pr_file_p != '\'')
		PR_ParseError("Bad vector");
	pr_file_p++;
}


void PR_LexName(void)
{
	// Parses an identifier

	int		c;
	int		len;

	len = 0;
	c = *pr_file_p;
	do
	{
		pr_token[len] = c;
		len++;
		pr_file_p++;
		c = *pr_file_p;
	} while (   (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'
			 || (c >= '0' && c <= '9'));

	pr_token[len] = 0;
	pr_token_type = tt_name;
}


void PR_LexPunctuation(void)
{
	int		i;
	int		len;
	char	*p;

	pr_token_type = tt_punct;

	char ch = *pr_file_p;

	for (i=0 ; (p = pr_punctuation[i]) != NULL ; i++)
	{
		len = strlen(p);
		if (!strncmp(p, pr_file_p, len) )
		{
			strcpy(pr_token, p);
			if (p[0] == '{')
				pr_bracelevel++;
			else if (p[0] == '}')
				pr_bracelevel--;
			pr_file_p += len;
			return;
		}
	}

	PR_ParseError("Unknown punctuation: %c", ch);
}


void PR_LexWhitespace(void)
{
	int c;

	for (;;)
	{
		// skip whitespace
		while ( (c = *pr_file_p) <= ' ')
		{
			if (c == 0) // end of file?
				return;

			if (c=='\n')
				PR_NewLine();

			pr_file_p++;
		}

		// skip // comments
		if (c=='/' && pr_file_p[1] == '/')
		{
			while (*pr_file_p && *pr_file_p != '\n')
				pr_file_p++;

			PR_NewLine();

			pr_file_p++;
			continue;
		}

		// skip /* */ comments
		if (c=='/' && pr_file_p[1] == '*')
		{
			do
			{
				pr_file_p++;
				
				if (pr_file_p[0]=='\n')
					PR_NewLine();

				if (pr_file_p[1] == 0)
					return;

			} while (pr_file_p[-1] != '*' || pr_file_p[0] != '/');

			pr_file_p++;
			continue;
		}

		break;		// a real character has been found
	}
}

//============================================================================


// just parses text, returning false if an eol is reached
bool PR_SimpleGetToken(void)
{
	int		c;
	int		i;

// skip whitespace
	while ( (c = *pr_file_p) <= ' ')
	{
		if (c=='\n' || c == 0)
			return false;
		pr_file_p++;
	}

	i = 0;
	while ( (c = *pr_file_p) > ' ' && c != ',' && c != ';')
	{
		pr_token[i] = c;
		i++;
		pr_file_p++;
	}
	
	pr_token[i] = 0;

	return true;
}


//============================================================================

/*
==============
PR_Lex

Sets pr_token, pr_token_type, and possibly pr_immediate and pr_immediate_type
==============
*/
void PR_Lex(void)
{
	assert(pr_file_p);

	PR_LexWhitespace();

	pr_token[0] = 0;
	pr_token_is_first = (pr_fol_level == 0);

	pr_fol_level++;

	int c = *pr_file_p;

	if (!c)
	{
		pr_token_type = tt_eof;
		return;
	}

// handle quoted strings as a unit
	if (c == '\"')
	{
		PR_LexString();
		return;
	}

// handle quoted vectors as a unit
	if (c == '\'')
	{
		PR_LexVector();
		return;
	}

// if the first character is a valid identifier, parse until a non-id
// character is reached
	if ( (c >= '0' && c <= '9') || ( c=='-' && pr_file_p[1]>='0' && pr_file_p[1] <='9') )
	{
		pr_token_type = tt_immediate;
		pr_immediate_type = &type_float;
		pr_immediate[0] = PR_LexNumber();
		return;
	}

	if ( (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' )
	{
		PR_LexName();
		return;
	}

// parse symbol strings until a non-symbol is found
	PR_LexPunctuation();
}

//=============================================================================

/*
=============
PR_Expect

Issues an error if the current token isn't equal to string
Gets the next token
=============
*/
void PR_Expect(const char *str)
{
	if (strcmp(pr_token, str) != 0)
		PR_ParseError("expected %s found %s", str, pr_token);

	PR_Lex();
}


/*
=============
PR_Check

Returns true and gets the next token if the current token equals string
Returns false and does nothing otherwise
=============
*/
bool PR_Check(const char *str)
{
	if (strcmp(pr_token, str) != 0)
		return false;

	PR_Lex();

	return true;
}

/*
============
PR_ParseName

Checks to see if the current token is a valid name
============
*/
char *PR_ParseName (void)
{
	static char	ident[MAX_NAME];

	if (pr_token_type != tt_name)
		PR_ParseError("expected identifier");
	if (strlen(pr_token) >= MAX_NAME-1)
		PR_ParseError("identifier too long");
	
	strcpy(ident, pr_token);

	PR_Lex();

	return ident;
}

/*
============
PR_FindType

Returns a preexisting complex type that matches the parm, or allocates
a new one and copies it out.
============
*/
type_t *PR_FindType(type_t *type)
{
	def_t	*def;
	type_t	*check;
	int		i;

	for (check = all_types ; check ; check = check->next)
	{
		if (check->type != type->type
			|| check->aux_type != type->aux_type
			|| check->parm_num != type->parm_num)
			continue;

		for (i=0 ; i< type->parm_num ; i++)
			if (check->parm_types[i] != type->parm_types[i])
				break;

		if (i == type->parm_num)
			return check;
	}

// allocate a new one
	type_t *t_new = new type_t;
	*t_new = *type;

	t_new->next = all_types;
	all_types = t_new;

// allocate a generic def for the type, so fields can reference it
	def = new def_t;
	def->name = "COMPLEX TYPE";
	def->type = t_new;
	
	return t_new;
}


/*
============
PR_SkipToSemicolon

For error recovery, also pops out of nested braces
============
*/
void PR_SkipToSemicolon(void)
{
	do
	{
		if (!pr_bracelevel && PR_Check(";"))
			return;
		PR_Lex();
	}
	while (pr_token_type != tt_eof);
}


/*
============
PR_ParseType

Parses a variable type, including field and functions types
============
*/
char	pr_parm_names[MAX_PARMS][MAX_NAME];

type_t *PR_ParseType(void)
{
	type_t	t_new;
	type_t	*type;
	char	*name;

	if (!strcmp(pr_token, "float") )
		type = &type_float;
	else if (!strcmp(pr_token, "vector") )
		type = &type_vector;
	else if (!strcmp(pr_token, "float") )
		type = &type_float;
//	else if (!strcmp(pr_token, "entity") )
//		type = &type_entity;
	else if (!strcmp(pr_token, "string") )
		type = &type_string;
	else if (!strcmp(pr_token, "void") )
		type = &type_void;
	else
	{
		PR_ParseError("\"%s\" is not a type", pr_token);
		type = &type_float;	// shut up compiler warning
	}
	PR_Lex();

	if (! PR_Check("("))
		return type;

// function type
	memset(&t_new, 0, sizeof(t_new));
	t_new.type = ev_function;
	t_new.aux_type = type;	// return type
	t_new.parm_num = 0;

	if (! PR_Check(")"))
	{
		if (PR_Check("..."))
			t_new.parm_num = -1;	// variable args
		else
			do
			{
				type = PR_ParseType();
				name = PR_ParseName();
				strcpy(pr_parm_names[t_new.parm_num], name);
				t_new.parm_types[t_new.parm_num] = type;
				t_new.parm_num++;
			} while (PR_Check(","));

		PR_Expect(")");
	}

	return PR_FindType(&t_new);
}


//=================================================================//


void PR_ParseFunction(void);
void PR_ParseVariable(void);
void PR_ParseConstant(void);

//========================================


const char * opcode_names[] =
{
	"DONE", "DONE_V",
	"NOT_F", "NOT_V", "NOT_S", "NOT_FNC",
	"POWER", 
	"MUL_F", "MUL_V", "MUL_FV", "MUL_VF",
	"DIV_F", "DIV_V", "MOD_F",
	"ADD_F", "ADD_V",
	"SUB_F", "SUB_V",
	"EQ_F", "EQ_V", "EQ_S", "EQ_FNC",
	"NE_F", "NE_V", "NE_S", "NE_FNC",
	"LE", "GE", "LT", "GT",
	"MOVE_F", "MOVE_V", "MOVE_S", "MOVE_FNC",
	"CALL",
	"IF", "IFNOT", "GOTO",
	"AND", "OR", "BITAND", "BITOR",
	"PARM_F", "PARM_V",

	"???", "???", "???", "???", "???", "???",
	"???", "???", "???", "???", "???", "???",
	"???", "???", "???", "???", "???", "???",
	"???", "???", "???", "???", "???", "???"
};


static opcode_t pr_operators[] =
{
	{"!", OP_NOT_F, -1, &type_float, &type_void, &type_float},
	{"!", OP_NOT_V, -1, &type_vector, &type_void, &type_float},
	{"!", OP_NOT_S, -1, &type_string, &type_void, &type_float},
	{"!", OP_NOT_FNC, -1, &type_function, &type_void, &type_float},

	/* priority 1 is for function calls */

	{"^", OP_POWER_F, 2, &type_float, &type_float, &type_float},

	{"*", OP_MUL_F,  2, &type_float, &type_float, &type_float},
	{"*", OP_MUL_V,  2, &type_vector, &type_vector, &type_float},
	{"*", OP_MUL_FV, 2, &type_float, &type_vector, &type_vector},
	{"*", OP_MUL_VF, 2, &type_vector, &type_float, &type_vector},

	{"/", OP_DIV_F, 2, &type_float, &type_float, &type_float},
	{"/", OP_DIV_V, 2, &type_vector, &type_float, &type_vector},
	{"%", OP_MOD_F, 2, &type_float, &type_float, &type_float},

	{"+", OP_ADD_F, 3, &type_float, &type_float, &type_float},
	{"+", OP_ADD_V, 3, &type_vector, &type_vector, &type_vector},

	{"-", OP_SUB_F, 3, &type_float, &type_float, &type_float},
	{"-", OP_SUB_V, 3, &type_vector, &type_vector, &type_vector},

	{"==", OP_EQ_F, 4, &type_float, &type_float, &type_float},
	{"==", OP_EQ_V, 4, &type_vector, &type_vector, &type_float},
	{"==", OP_EQ_S, 4, &type_string, &type_string, &type_float},
	{"==", OP_EQ_FNC, 4, &type_function, &type_function, &type_float},

	{"!=", OP_NE_F, 4, &type_float, &type_float, &type_float},
	{"!=", OP_NE_V, 4, &type_vector, &type_vector, &type_float},
	{"!=", OP_NE_S, 4, &type_string, &type_string, &type_float},
	{"!=", OP_NE_FNC, 4, &type_function, &type_function, &type_float},

	{"<=", OP_LE, 4, &type_float, &type_float, &type_float},
	{">=", OP_GE, 4, &type_float, &type_float, &type_float},
	{"<",  OP_LT, 4, &type_float, &type_float, &type_float},
	{">",  OP_GT, 4, &type_float, &type_float, &type_float},

	{"&&", OP_AND, 5, &type_float, &type_float, &type_float},
	{"||", OP_OR,  5, &type_float, &type_float, &type_float},

	{"&", OP_BITAND, 2, &type_float, &type_float, &type_float},
	{"|", OP_BITOR,  2, &type_float, &type_float, &type_float},

	{NULL}
};

#define	TOP_PRIORITY	6
#define	NOT_PRIORITY	1

def_t *PR_Expression(int priority, bool *lvalue = NULL);


//===========================================================================


void PR_EmitCode(short op, short a=0, short b=0, short c=0)
{
	statement_linenums[numstatements] = pr_source_line;

	statement_t *st = &statements[numstatements++];

	st->op = op;
	st->a  = a;
	st->b  = b;
	st->c  = c;
}


void PR_DefaultValue(gofs_t ofs, type_t *type)
{
	if (ofs > 0)
	{
		pr_globals[ofs] = 0;

		if (type->type == ev_vector)
		{
			pr_globals[ofs+1] = 0;
			pr_globals[ofs+2] = 0;
		}

		return;
	}

	if (type->type == ev_vector)
		PR_EmitCode(OP_MOVE_V, OFS_DEFAULT, ofs);
	else
		PR_EmitCode(OP_MOVE_F, OFS_DEFAULT, ofs);
}


def_t *PR_NewGlobal(type_t *type)
{
	def_t * var = new def_t;
	memset(var, 0, sizeof(def_t));

	var->ofs = numpr_globals;
	var->type = type;

	numpr_globals += type_size[type->type];

	return var;
}


def_t *PR_NewLocal(type_t *type)
{
	def_t * var = new def_t;
	memset(var, 0, sizeof(def_t));

	var->ofs = -(locals_end+1);
	var->type = type;

	locals_end += type_size[type->type];

	return var;
}


def_t *PR_NewTemporary(type_t *type)
{
	def_t *var;

	std::vector<def_t *>::iterator TI;

	for (TI = temporaries.begin(); TI != temporaries.end(); TI++)
	{
		var = *TI;

		// make sure it fits
		if (type_size[var->type->type] < type_size[type->type])
			continue;

		if (! (var->flags & DF_FreeTemp))
			continue;

		// found a match, so re-use it!

		var->flags &= ~DF_FreeTemp;
		var->type = type;

		return var;
	}

	var = PR_NewLocal(type);

	var->flags |= DF_Temporary;
	temporaries.push_back(var);

	return var;
}


void PR_FreeTemporaries(void)
{
	std::vector<def_t *>::iterator TI;

	for (TI = temporaries.begin(); TI != temporaries.end(); TI++)
	{
		def_t *tvar = *TI;

		tvar->flags |= DF_FreeTemp;
	}
}


def_t * PR_FindConstant(void)
{
	// check for a constant with the same value
	for (int i = 0; i < (int)constants.size(); i++)
	{
		def_t *cn = constants[i];

		if (cn->type != pr_immediate_type)
			continue;

		if (pr_immediate_type == &type_string)
		{
			if (strcmp(G_STRING(cn->ofs), pr_immediate_string) == 0)
				return cn;
		}
		else if (pr_immediate_type == &type_float)
		{
			if (G_FLOAT(cn->ofs) == pr_immediate[0])
				return cn;
		}
		else if	(pr_immediate_type == &type_vector)
		{
			if (G_FLOAT(cn->ofs)   == pr_immediate[0] &&
				G_FLOAT(cn->ofs+1) == pr_immediate[1] &&
				G_FLOAT(cn->ofs+2) == pr_immediate[2])
			{
				return cn;
			}
		}
	}

	return NULL;  // not found
}


void PR_StoreConstant(int ofs)
{
	if (pr_immediate_type == &type_string)
		pr_globals[ofs] = (double) CopyString(pr_immediate_string);
	else if (pr_immediate_type == &type_vector)
		memcpy (pr_globals + ofs, pr_immediate, 3 * sizeof(double));
	else
		pr_globals[ofs] = pr_immediate[0];
}


def_t * PR_ParseImmediate(void)
{
	// Looks for a preexisting constant
	def_t *cn = PR_FindConstant();

	if (! cn)
	{
		// allocate a new one
		cn = PR_NewGlobal(pr_immediate_type);

		cn->name = "CONSTANT VALUE";

		cn->flags |= DF_Constant;
		cn->scope = NULL;   // immediates are always global

		// link into defs list
		cn->next = all_defs;
		all_defs  = cn;

		// copy the immediate to the global area
		PR_StoreConstant(cn->ofs);

		constants.push_back(cn);
	}

	PR_Lex();
	return cn;
}


def_t * PR_ParseFunctionCall(def_t *func)
{
	type_t * t = func->type;

	if (t->type != ev_function)
		PR_ParseError("not a function");
	
	function_t *df = &functions[func->ofs];

	
	// evaluate all parameters
	def_t * exprs[MAX_PARMS];

	int arg = 0;

	if (! PR_Check(")"))
	{
		do
		{
			if (arg >= t->parm_num)
				PR_ParseError("too many parameters (expected %d)", t->parm_num);

			assert(arg < MAX_PARMS);

			def_t * e = PR_Expression(TOP_PRIORITY);

			if (e->type != t->parm_types[arg])
				PR_ParseError("type mismatch on parm %i", arg+1);

			assert (e->type->type != ev_void);

			exprs[arg++] = e;
		}
		while (PR_Check(","));

		if (arg != t->parm_num)
			PR_ParseError("too few parameters (needed %d)", t->parm_num);

		PR_Expect(")");
	}


	def_t *result = NULL;

	if (t->aux_type->type != ev_void)
	{
		result = PR_NewTemporary(t->aux_type);
	}


	// copy parameters
	int parm_ofs = 0;

	for (int k = 0; k < arg; k++)
	{
		if (exprs[k]->type->type == ev_vector)
		{
			PR_EmitCode(OP_PARM_V, exprs[k]->ofs, parm_ofs);
			parm_ofs += 3;
		}
		else
		{
			PR_EmitCode(OP_PARM_F, exprs[k]->ofs, parm_ofs);
			parm_ofs++;
		}
	}


	// Note: local vars are setup where they are declared, and
	//       temporaries do not need any default value.


	if (result)
	{
		PR_EmitCode(OP_CALL, func->ofs, arg, result->ofs);
		return result;
	}

	PR_EmitCode(OP_CALL, func->ofs, arg, 0);

	return &def_void;
}


void PR_ParseReturn(void)
{
	if (pr_token_is_first || pr_token[0] == '}' || PR_Check(";"))
	{
		if (pr_scope->type->aux_type->type != ev_void)
			PR_ParseError("missing value for return");

		PR_EmitCode(OP_DONE);
		return;
	}

	def_t * e = PR_Expression(TOP_PRIORITY);

	if (pr_scope->type->aux_type->type == ev_void)
		PR_ParseError("return with value in void function");

	if (pr_scope->type->aux_type != e->type)
		PR_ParseError("mismatch types for return");

	if (pr_scope->type->aux_type->type == ev_vector)
	{
		PR_EmitCode(OP_MOVE_V, e->ofs, OFS_RETURN);
		PR_EmitCode(OP_DONE_V);
	}
	else
	{
		PR_EmitCode(OP_MOVE_F, e->ofs, OFS_RETURN);
		PR_EmitCode(OP_DONE);
	}

	// -AJA- optional semicolons
	if (! (pr_token_is_first || pr_token[0] == '}'))
		PR_Expect(";");
}


def_t *PR_FindDef(type_t *type, char *name, def_t *scope)
{
	def_t *def;

	for (def = all_defs ; def ; def=def->next)
	{
		if (strcmp(def->name, name) != 0)
			continue;

		if ( def->scope && def->scope != scope)
			continue;		// in a different function

		if (type && def->type != type)
			PR_ParseError("Type mismatch on redeclaration of %s", name);

		return def;
	}

	return NULL;
}


def_t *PR_GetDef(type_t *type, char *name, def_t *scope)
{
	// A new def will be allocated if it can't be found

	assert(type);

	def_t *def = PR_FindDef(type, name, scope);
	if (def)
		return def;

	// allocate a new def

	if (scope)
		def = PR_NewLocal(type);
	else
		def = PR_NewGlobal(type);

	def->name  = strdup(name);
	def->scope = scope;


	// link into list
	def->next = all_defs;
	all_defs = def;

	return def;
}


def_t * PR_ParseValue(void)
{
	char *name = PR_ParseName();

	// look through the defs
	def_t *d = PR_FindDef(NULL, name, pr_scope);
	if (!d)
		PR_ParseError("Unknown identifier '%s'", name);

	return d;
}


def_t * PR_Term(void)
{
	// if the token is an immediate, allocate a constant for it
	if (pr_token_type == tt_immediate)
		return PR_ParseImmediate();

	if (pr_token_type == tt_name)
		return PR_ParseValue();

	if (PR_Check("("))
	{
		def_t *e = PR_Expression(TOP_PRIORITY);
		PR_Expect(")");
		return e;
	}

	// unary operator?

	opcode_t *op;

	for (op=pr_operators ; op->name ; op++)
	{
		if (op->priority != -1)
			continue;

		if (! PR_Check(op->name))
			continue;

		def_t * e = PR_Expression(NOT_PRIORITY);

		for (int i = 0; i == 0 || strcmp(op->name, op[i].name) == 0; i++)
		{
			if (op[i].type_a->type != e->type->type)
				continue;

			def_t *result = PR_NewTemporary(op[i].type_c);

			PR_EmitCode(op[i].op, e->ofs, 0, result->ofs);

			return result;
		}

		PR_ParseError("type mismatch for %s", op->name);
		break;
	}

	PR_ParseError("expected value or unary operator, found %s\n", pr_token);
	return NULL; /* NOT REACHED */
}


def_t * PR_ShortCircuitExp(def_t *e, opcode_t *op)
{
	if (e->type->type != ev_float)
		PR_ParseError("type mismatch for %s", op->name);

	// Instruction stream for &&
	//
	//		... calc a ...
	//		MOVE a --> c
	//		IF c == 0 GOTO label
	//		... calc b ...
	//		MOVE b --> c
	//		label:

	def_t *result = PR_NewTemporary(op->type_c);

	PR_EmitCode(OP_MOVE_F, e->ofs, result->ofs);

	int patch = numstatements;

	if (op->name[0] == '&')
		PR_EmitCode(OP_IFNOT, result->ofs);
	else
		PR_EmitCode(OP_IF, result->ofs);

	def_t *e2 = PR_Expression(op->priority - 1);
	if (e2->type->type != ev_float)
		PR_ParseError("type mismatch for %s", op->name);

	PR_EmitCode(OP_MOVE_F, e2->ofs, result->ofs);

	statements[patch].b = numstatements;

	return result;
}


def_t * PR_ParseFieldQuery(def_t *e, bool lvalue)
{
	// TODO
	PR_ParseError("Operator . not yet implemented\n");
	return NULL;
}


def_t * PR_Expression(int priority, bool *lvalue)
{
	if (priority == 0)
		return PR_Term();

	def_t * e = PR_Expression(priority-1, lvalue);

	for (;;)
	{
		if (priority == 1 && PR_Check("("))
		{
			if (lvalue)
				*lvalue = false;
			return PR_ParseFunctionCall(e);
		}

		if (priority == 1 && PR_Check("."))
			return PR_ParseFieldQuery(e, lvalue);

		if (lvalue)
			return e;

		opcode_t *op;

		for (op=pr_operators ; op->name ; op++)
		{
			if (op->priority != priority)
				continue;

			if (! PR_Check(op->name))
				continue;

			if (strcmp(op->name, "&&") == 0 ||
			    strcmp(op->name, "||") == 0)
			{
				e = PR_ShortCircuitExp(e, op);
				break;
			}

			def_t * e2 = PR_Expression(priority-1);

			// type check

			etype_t type_a = e->type->type;
			etype_t type_b = e2->type->type;
			etype_t type_c = ev_void;
#if 0
			if (op->name[0] == '.')// field access gets type from field
			{
				if (e2->type->aux_type)
					type_c = e2->type->aux_type->type;
				else
					type_c = ev_INVALID;	// not a field
			}
#endif

			opcode_t * oldop = op;

			while (type_a != op->type_a->type
					|| type_b != op->type_b->type
					|| (type_c != ev_void && type_c != op->type_c->type) )
			{
				op++;
				if (!op->name || strcmp(op->name , oldop->name) != 0)
					PR_ParseError("type mismatch for %s", oldop->name);
			}

			if (type_a == ev_pointer && type_b != e->type->aux_type->type)
				PR_ParseError("type mismatch for %s", op->name);

			def_t *result = PR_NewTemporary(op->type_c);

			PR_EmitCode(op->op, e->ofs, e2->ofs, result->ofs);

			e = result;
#if 0
			if (type_c != ev_void)	// field access gets type from field
				e->type = e2->type->aux_type;
#endif
			break;
		}

		if (!op->name)
			break;	// next token isn't at this priority level
	}

	return e;
}


void PR_If_Else(void)
{
	PR_Expect("(");
	def_t * e = PR_Expression(TOP_PRIORITY);
	PR_Expect(")");

	int patch = numstatements;
	PR_EmitCode(OP_IFNOT, e->ofs);

	PR_ParseStatement(false);
	PR_FreeTemporaries();

	if (PR_Check("else"))
	{
		// use GOTO to skip over the else statements
		int patch2 = numstatements;
		PR_EmitCode(OP_GOTO);

		statements[patch].b = numstatements;

		PR_ParseStatement(false);
		PR_FreeTemporaries();

		patch = patch2;
	}
	
	statements[patch].b = numstatements;
}


void PR_WhileLoop(void)
{
	int begin = numstatements;

	PR_Expect("(");
	def_t * e = PR_Expression(TOP_PRIORITY);
	PR_Expect(")");

	int patch = numstatements;
	PR_EmitCode(OP_IFNOT, e->ofs);

	PR_ParseStatement(false);
	PR_FreeTemporaries();

	PR_EmitCode(OP_GOTO, 0, begin);

	statements[patch].b = numstatements;
}


void PR_RepeatLoop(void)
{
	int begin = numstatements;

	PR_ParseStatement(false);
	PR_FreeTemporaries();

	PR_Expect("until");
	PR_Expect("(");

	def_t * e = PR_Expression(TOP_PRIORITY);

	PR_EmitCode(OP_IFNOT, e->ofs, begin);

	PR_Expect(")");

	// -AJA- optional semicolons
	if (! (pr_token_is_first || pr_token[0] == '}'))
		PR_Expect(";");
}


void PR_Assignment(def_t *e)
{
	if (e->flags & DF_Constant)
		PR_ParseError("assignment to a constant.\n");

	def_t *e2 = PR_Expression(TOP_PRIORITY);

	if (e2->type != e->type)
		PR_ParseError("type mismatch in assignment.\n");

	switch (e->type->type)
	{
		case ev_float:
		case ev_string:
		case ev_function:
			PR_EmitCode(OP_MOVE_F, e2->ofs, e->ofs);
			break;

		case ev_vector:
			PR_EmitCode(OP_MOVE_V, e2->ofs, e->ofs);
			break;

		default:
			PR_ParseError("weird type for assignment.\n");
			break;
	}
}


void PR_ParseStatement(bool allow_def)
{
	if (allow_def && PR_Check("var"))
	{
		PR_ParseVariable();
		return;
	}

	if (allow_def && PR_Check("function"))
	{
		PR_ParseError("functions must be global");
		return;
	}

	if (allow_def && PR_Check("constant"))
	{
		PR_ParseError("constants must be global");
		return;
	}

	if (PR_Check("{"))
	{
		do {
			PR_ParseStatement(true);
			PR_FreeTemporaries();
		}
		while (! PR_Check("}"));

		return;
	}

	if (PR_Check("return"))
	{
		PR_ParseReturn();
		return;
	}

	if (PR_Check("if"))
	{
		PR_If_Else();
		return;
	}

	if (PR_Check("while"))
	{
		PR_WhileLoop();
		return;
	}

	if (PR_Check("repeat"))
	{
		PR_RepeatLoop();
		return;
	}

	bool lvalue = true;
	def_t * e = PR_Expression(TOP_PRIORITY, &lvalue);

	// lvalue is false for a plain function call

	if (lvalue)
	{
		PR_Expect("=");

		PR_Assignment(e);
	}

	// -AJA- optional semicolons
	if (! (pr_token_is_first || pr_token[0] == '}'))
		PR_Expect(";");
}


int PR_ParseFunctionBody(type_t *type, const char *func_name)
{
	temporaries.clear();

	//
	// check for builtin function definition #1, #2, etc
	//
	if (PR_Check("#"))
	{
		if (pr_token_type != tt_immediate
			|| pr_immediate_type != &type_float
			|| pr_immediate[0] != (int)pr_immediate[0])
		{
			PR_ParseError("Bad builtin immediate");
		}
		int builtin = (int)pr_immediate[0];
		PR_Lex();
		return -builtin;
	}

	//
	// create the parmeters as locals
	//
	def_t *defs[MAX_PARMS];

	for (int i=0 ; i < type->parm_num ; i++)
	{
		if (PR_FindDef(type->parm_types[i], pr_parm_names[i], pr_scope))
			PR_ParseError("parameter %s redeclared", pr_parm_names[i]);

		defs[i] = PR_GetDef(type->parm_types[i], pr_parm_names[i], pr_scope);
	}

	int code = numstatements;

	//
	// parse regular statements
	//
	PR_Expect("{");

	while (! PR_Check("}"))
	{
		PR_ParseStatement(true);
		PR_FreeTemporaries();
	}

	if (code == numstatements ||
		! (statements[numstatements-1].op == OP_DONE ||
		   statements[numstatements-1].op == OP_DONE_V))
	{
		if (type->aux_type->type == ev_void)
			PR_EmitCode(OP_DONE);
		else
			PR_ParseError("missing return at end of '%s' function", func_name);
	}

	return code;
}


void PR_ParseFunction(void)
{
	char *func_name = strdup(PR_ParseName());

	PR_Expect("(");

	type_t t_new;

	memset(&t_new, 0, sizeof(t_new));
	t_new.type = ev_function;
	t_new.parm_num = 0;
	t_new.aux_type = &type_void;

	if (! PR_Check(")"))
	{
		do
		{
			if (t_new.parm_num >= MAX_PARMS)
				PR_ParseError("too many parameters (over %d)", MAX_PARMS);

			char *name = PR_ParseName();

			strcpy(pr_parm_names[t_new.parm_num], name);

			// parameter type (defaults to float)
			if (PR_Check(":"))
				t_new.parm_types[t_new.parm_num] = PR_ParseType();
			else
				t_new.parm_types[t_new.parm_num] = &type_float;

			t_new.parm_num++;
		}
		while (PR_Check(","));

		PR_Expect(")");
	}

	// return type (defaults to void)
	if (PR_Check(":"))
	{
		t_new.aux_type = PR_ParseType();
	}

	type_t *func_type = PR_FindType(&t_new);

	def_t *def = PR_GetDef(func_type, func_name, pr_scope);

	if (def->flags & DF_Initialized)
		PR_ParseError("%s redeclared", func_name);


	PR_Expect("=");

	assert(func_type->type == ev_function);


	G_FUNCTION(def->ofs) = (func_t)numfunctions;
	numfunctions++;


	// fill in the dfunction
	function_t *df = &functions[(int)G_FUNCTION(def->ofs)];
	memset(df, 0, sizeof(function_t));

	df->s_name = CopyString(func_name);
	df->s_file = s_file;

	int stack_ofs = 0;

	df->return_size = type_size[def->type->aux_type->type];
	if (def->type->aux_type->type == ev_void) df->return_size = 0;
///---	stack_ofs += df->return_size;

	df->parm_num = def->type->parm_num;

	for (int i=0 ; i < df->parm_num ; i++)
	{
		df->parm_ofs[i]  = stack_ofs;
		df->parm_size[i] = type_size[def->type->parm_types[i]->type];
		if (def->type->parm_types[i]->type == ev_void) df->parm_size[i] = 0;

		stack_ofs += df->parm_size[i];
	}


	df->locals_ofs = stack_ofs;

	// parms are "realloc'd" by GetDef in PR_ParseFunctionBody (FIXME)
	locals_end = 0;


	pr_scope = def;
	//  { 
		df->first_statement = PR_ParseFunctionBody(func_type, func_name);
	//  }
	pr_scope = NULL;

	def->flags |= DF_Initialized;

	df->locals_size = locals_end - df->locals_ofs;
	df->locals_end  = locals_end;

// fprintf(stderr, "FUNCTION %s locals:%d\n", func_name, locals_end);
}


void PR_ParseVariable(void)
{
	char *var_name = strdup(PR_ParseName());

	type_t *type = &type_float;

	if (PR_Check(":"))
	{
		type = PR_ParseType();
	}

	// TODO
	// if (PR_Check("="))
	// 	 get default value

	def_t * def = PR_GetDef(type, var_name, pr_scope);

	PR_DefaultValue(def->ofs, type);

	// -AJA- optional semicolons
	if (! (pr_token_is_first || pr_token[0] == '}'))
		PR_Expect(";");
}


void PR_ParseConstant(void)
{
	char *const_name = strdup(PR_ParseName());

	PR_Expect("=");

	if (pr_token_type != tt_immediate)
		PR_ParseError("expected value for constant, got %s", pr_token);


	if (PR_FindDef(NULL, const_name, NULL))
		PR_ParseError("name already used: %s", const_name);

	/// TODO: reuse existing constant
	/// def_t * exist_cn = PR_FindConstant();

	def_t * cn = PR_NewGlobal(pr_immediate_type);

	cn->name  = const_name;
	cn->flags |= DF_Constant;

	// link into list
	cn->next = all_defs;
	all_defs = cn;

	PR_StoreConstant(cn->ofs);

	constants.push_back(cn);

	PR_Lex();

	// -AJA- optional semicolons
	if (! (pr_token_is_first || pr_token[0] == '}'))
		PR_Expect(";");
}


void PR_ParseGlobals(void)
{
	if (PR_Check("function"))
	{
		PR_ParseFunction();
		return;
	}

	if (PR_Check("var"))
	{
		PR_ParseVariable();
		return;
	}

	if (PR_Check("constant"))
	{
		PR_ParseConstant();
		return;
	}


	PR_ParseError("Expected global definition, found %s", pr_token);
}


/*
============
PR_CompileFile

compiles the 0 terminated text, adding definitions to the pr structure
============
*/
bool PR_CompileFile(char *string, char *filename)
{
	s_file = CopyString(filename);

	pr_file_p = string;
	pr_line_start = string;
	pr_source_line = 1;
	pr_fol_level = 0;

	PR_Lex();	// read first token

	while (pr_token_type != tt_eof)
	{
		try
		{
			pr_scope = NULL;	// outside all functions

			PR_ParseGlobals();
		}
		catch (parse_error_x err)
		{
			if (++pr_error_count > MAX_ERRORS)
				return false;

			PR_SkipToSemicolon();

			if (pr_token_type == tt_eof)
				return false;
		}
	}

	return (pr_error_count == 0);
}

/*
==============
PR_BeginCompilation

called before compiling a batch of files, clears the pr struct
==============
*/
void PR_BeginCompilation(void)
{
	numpr_globals = RESERVED_OFS;

	all_defs = NULL;

// link the function type in so state forward declarations match proper type
	all_types = &type_function;
	type_function.next = NULL;

	constants.clear();

	pr_error_count = 0;
}

/*
==============
PR_FinishCompilation

called after all files are compiled to check for errors
Returns false if errors were detected.
==============
*/
bool PR_FinishCompilation(void)
{
	def_t *d;
	bool errors = false;

// check to make sure all functions prototyped have code
	for (d=all_defs ; d ; d=d->next)
	{
		if (d->type->type == ev_function && !d->scope)// function parms are ok
		{
//			f = G_FUNCTION(d->ofs);
//			if (!f || (!f->code && !f->builtin) )
			if (! (d->flags & DF_Initialized))
			{
				printf("function %s was not defined\n",d->name);
				errors = true;
			}
		}
	}

	return !errors;
}

}  // namespace coal

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
