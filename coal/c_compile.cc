//----------------------------------------------------------------------
//  COAL COMPILER
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
//
//  Based on QCC (the Quake-C Compiler) and the corresponding
//  execution engine from the Quake source code.
//
//----------------------------------------------------------------------

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
#include "c_compile.h"

#define MAX_ERRORS  1000



compiling_c * COM;


compiling_c::compiling_c() :
	source_file(NULL), source_line(0),
	parse_p(NULL), line_start(NULL),
	error_count(0),
	all_types(NULL), all_defs(NULL),
	temporaries(), constants()
{ }

compiling_c::~compiling_c()
{ }



const char * punctuation[] =
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


void real_vm_c::LEX_NewLine()
{
	// Called when *COM->parse_p == '\n'

	COM->source_line++;

	COM->line_start = COM->parse_p + 1;
	COM->fol_level = 0;
}


//
// Aborts the current function parse
//
void real_vm_c::CompileError(const char *error, ...)
{
	va_list		argptr;
	char		buffer[1024];

	va_start(argptr,error);
	vsprintf(buffer,error,argptr);
	va_end(argptr);

	printf("%s:%i:%s\n", COM->source_file, COM->source_line, buffer);

//  raise(11);
	throw parse_error_x();
}


void real_vm_c::LEX_String()
{
	// Parses a quoted string

	int		c;
	int		len;

	len = 0;
	COM->parse_p++;

	for (;;)
	{
		c = *COM->parse_p++;
		if (!c)
			CompileError("EOF inside quote");
		if (c=='\n')
			CompileError("newline inside quote");
		if (c=='\\')
		{	// escape char
			c = *COM->parse_p++;
			if (!c)
				CompileError("EOF inside quote");
			if (c == 'n')
				c = '\n';
			else if (c == '"')
				c = '"';
			else
				CompileError("Unknown escape char");
		}
		else if (c=='\"')
		{
			COM->token_buf[len] = 0;
			COM->token_type = tt_literal;
			COM->literal_type = &type_string;
			strcpy(COM->literal_buf, COM->token_buf);
			return;
		}
		COM->token_buf[len] = c;
		len++;
	}
}


float real_vm_c::LEX_Number()
{
	int		c;
	int		len;

	len = 0;
	c = *COM->parse_p;
	do
	{
		COM->token_buf[len] = c;
		len++;
		COM->parse_p++;
		c = *COM->parse_p;
	} while ((c >= '0' && c<= '9') || c == '.');

	COM->token_buf[len] = 0;

	return atof(COM->token_buf);
}


void real_vm_c::LEX_Vector()
{
	// Parses a single quoted vector

	int		i;

	COM->parse_p++;
	COM->token_type = tt_literal;
	COM->literal_type = &type_vector;

	for (i=0 ; i<3 ; i++)
	{
		// FIXME: check for digits etc!

		COM->literal_value[i] = LEX_Number();

		while (isspace(*COM->parse_p) && *COM->parse_p != '\n')
			COM->parse_p++;
	}
	if (*COM->parse_p != '\'')
		CompileError("Bad vector");
	COM->parse_p++;
}


void real_vm_c::LEX_Name()
{
	// Parses an identifier

	int		c;
	int		len;

	len = 0;
	c = *COM->parse_p;
	do
	{
		COM->token_buf[len] = c;
		len++;
		COM->parse_p++;
		c = *COM->parse_p;
	} while (   (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'
			 || (c >= '0' && c <= '9'));

	COM->token_buf[len] = 0;
	COM->token_type = tt_name;
}


void real_vm_c::LEX_Punctuation()
{
	COM->token_type = tt_punct;

	const char *p;

	char ch = *COM->parse_p;

	for (int i=0 ; (p = punctuation[i]) != NULL ; i++)
	{
		int len = strlen(p);

		if (strncmp(p, COM->parse_p, len) == 0)
		{
			strcpy(COM->token_buf, p);

			if (p[0] == '{')
				COM->bracelevel++;
			else if (p[0] == '}')
				COM->bracelevel--;

			if (p[0] == '(')
				COM->parentheses++;
			else if (p[0] == ')')
				COM->parentheses--;

			COM->parse_p += len;
			return;
		}
	}

	CompileError("Unknown punctuation: %c", ch);
}


void real_vm_c::LEX_Whitespace(void)
{
	int c;

	for (;;)
	{
		// skip whitespace
		while ( (c = *COM->parse_p) <= ' ')
		{
			if (c == 0) // end of file?
				return;

			if (c=='\n')
				LEX_NewLine();

			COM->parse_p++;
		}

		// skip // comments
		if (c=='/' && COM->parse_p[1] == '/')
		{
			while (*COM->parse_p && *COM->parse_p != '\n')
				COM->parse_p++;

			LEX_NewLine();

			COM->parse_p++;
			continue;
		}

		// skip /* */ comments
		if (c=='/' && COM->parse_p[1] == '*')
		{
			do
			{
				COM->parse_p++;
				
				if (COM->parse_p[0]=='\n')
					LEX_NewLine();

				if (COM->parse_p[1] == 0)
					return;

			} while (COM->parse_p[-1] != '*' || COM->parse_p[0] != '/');

			COM->parse_p++;
			continue;
		}

		break;		// a real character has been found
	}
}



/*
==============
LEX_Next

Sets COM->token_buf, COM->token_type, and possibly COM->literal_value and COM->literal_type
==============
*/
void real_vm_c::LEX_Next()
{
	assert(COM->parse_p);

	LEX_Whitespace();

	COM->token_buf[0] = 0;
	COM->token_is_first = (COM->fol_level == 0);

	COM->fol_level++;

	int c = *COM->parse_p;

	if (!c)
	{
		COM->token_type = tt_eof;
		return;
	}

// handle quoted strings as a unit
	if (c == '\"')
	{
		LEX_String();
		return;
	}

// handle quoted vectors as a unit
	if (c == '\'')
	{
		LEX_Vector();
		return;
	}

// if the first character is a valid identifier, parse until a non-id
// character is reached
	if ( (c >= '0' && c <= '9') || ( c=='-' && COM->parse_p[1]>='0' && COM->parse_p[1] <='9') )
	{
		COM->token_type = tt_literal;
		COM->literal_type = &type_float;
		COM->literal_value[0] = LEX_Number();
		return;
	}

	if ( (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' )
	{
		LEX_Name();
		return;
	}

// parse symbol strings until a non-symbol is found
	LEX_Punctuation();
}


/*
=============
LEX_Expect

Issues an error if the current token isn't equal to string
Gets the next token
=============
*/
void real_vm_c::LEX_Expect(const char *str)
{
	if (strcmp(COM->token_buf, str) != 0)
		CompileError("expected %s found %s", str, COM->token_buf);

	LEX_Next();
}


/*
=============
LEX_Check

Returns true and gets the next token if the current token equals string
Returns false and does nothing otherwise
=============
*/
bool real_vm_c::LEX_Check(const char *str)
{
	if (strcmp(COM->token_buf, str) != 0)
		return false;

	LEX_Next();

	return true;
}

/*
============
LEX_SkipToSemicolon

For error recovery, also pops out of nested braces
============
*/
void real_vm_c::LEX_SkipToSemicolon()
{
	do
	{
		if (!COM->bracelevel && LEX_Check(";"))
			return;
		LEX_Next();
	}
	while (COM->token_type != tt_eof);
}


/*
============
ParseName

Checks to see if the current token is a valid name
============
*/
char * real_vm_c::ParseName()
{
	static char	ident[MAX_NAME];

	if (COM->token_type != tt_name)
		CompileError("expected identifier");

	if (strlen(COM->token_buf) >= MAX_NAME-1)
		CompileError("identifier too long");

	strcpy(ident, COM->token_buf);

	LEX_Next();

	return ident;
}


//=============================================================================

/*
============
FindType

Returns a preexisting complex type that matches the parm, or allocates
a new one and copies it out.
============
*/
type_t * real_vm_c::FindType(type_t *type)
{
	def_t	*def;
	type_t	*check;
	int		i;

	for (check = COM->all_types ; check ; check = check->next)
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

	t_new->next = COM->all_types;
	COM->all_types = t_new;

// allocate a generic def for the type, so fields can reference it
	def = new def_t;
	def->name = "COMPLEX TYPE";
	def->type = t_new;
	
	return t_new;
}


/*
============
ParseType

Parses a variable type, including field and functions types
============
*/
char	pr_parm_names[MAX_PARMS][MAX_NAME];

type_t * real_vm_c::ParseType()
{
	type_t	t_new;
	type_t	*type;
	char	*name;

	if (!strcmp(COM->token_buf, "float") )
		type = &type_float;
	else if (!strcmp(COM->token_buf, "vector") )
		type = &type_vector;
	else if (!strcmp(COM->token_buf, "float") )
		type = &type_float;
//	else if (!strcmp(COM->token_buf, "entity") )
//		type = &type_entity;
	else if (!strcmp(COM->token_buf, "string") )
		type = &type_string;
	else if (!strcmp(COM->token_buf, "void") )
		type = &type_void;
	else
	{
		CompileError("\"%s\" is not a type", COM->token_buf);
		type = &type_float;	// shut up compiler warning
	}
	LEX_Next();

	if (! LEX_Check("("))
		return type;

// function type
	memset(&t_new, 0, sizeof(t_new));
	t_new.type = ev_function;
	t_new.aux_type = type;	// return type
	t_new.parm_num = 0;

	if (! LEX_Check(")"))
	{
		if (LEX_Check("..."))
			t_new.parm_num = -1;	// variable args
		else
			do
			{
				type = ParseType();
				name = ParseName();
				strcpy(pr_parm_names[t_new.parm_num], name);
				t_new.parm_types[t_new.parm_num] = type;
				t_new.parm_num++;
			} while (LEX_Check(","));

		LEX_Expect(")");
	}

	return FindType(&t_new);
}


//===========================================================================


void real_vm_c::EmitCode(short op, short a, short b, short c)
{
	statement_t *st = &statements[numstatements++];

	st->op = op;
	st->line = COM->source_line - COM->function_line;

	st->a  = a;
	st->b  = b;
	st->c  = c;
}


void real_vm_c::DefaultValue(gofs_t ofs, type_t *type)
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
		EmitCode(OP_MOVE_V, OFS_DEFAULT, ofs);
	else
		EmitCode(OP_MOVE_F, OFS_DEFAULT, ofs);
}


def_t * real_vm_c::NewGlobal(type_t *type)
{
	def_t * var = new def_t;
	memset(var, 0, sizeof(def_t));

	var->ofs = numpr_globals;
	var->type = type;

	numpr_globals += type_size[type->type];

	return var;
}


def_t * real_vm_c::NewLocal(type_t *type)
{
	def_t * var = new def_t;
	memset(var, 0, sizeof(def_t));

	var->ofs = -(COM->locals_end+1);
	var->type = type;

	COM->locals_end += type_size[type->type];

	return var;
}


def_t * real_vm_c::NewTemporary(type_t *type)
{
	def_t *var;

	std::vector<def_t *>::iterator TI;

	for (TI = COM->temporaries.begin(); TI != COM->temporaries.end(); TI++)
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

	var = NewLocal(type);

	var->flags |= DF_Temporary;
	COM->temporaries.push_back(var);

	return var;
}


void real_vm_c::FreeTemporaries()
{
	std::vector<def_t *>::iterator TI;

	for (TI = COM->temporaries.begin(); TI != COM->temporaries.end(); TI++)
	{
		def_t *tvar = *TI;

		tvar->flags |= DF_FreeTemp;
	}
}


def_t * real_vm_c::FindConstant()
{
	// check for a constant with the same value
	for (int i = 0; i < (int)COM->constants.size(); i++)
	{
		def_t *cn = COM->constants[i];

		if (cn->type != COM->literal_type)
			continue;

		if (COM->literal_type == &type_string)
		{
			if (strcmp(G_STRING(cn->ofs), COM->literal_buf) == 0)
				return cn;
		}
		else if (COM->literal_type == &type_float)
		{
			if (G_FLOAT(cn->ofs) == COM->literal_value[0])
				return cn;
		}
		else if	(COM->literal_type == &type_vector)
		{
			if (G_FLOAT(cn->ofs)   == COM->literal_value[0] &&
				G_FLOAT(cn->ofs+1) == COM->literal_value[1] &&
				G_FLOAT(cn->ofs+2) == COM->literal_value[2])
			{
				return cn;
			}
		}
	}

	return NULL;  // not found
}


void real_vm_c::StoreConstant(int ofs)
{
	if (COM->literal_type == &type_string)
		pr_globals[ofs] = (double) InternaliseString(COM->literal_buf);
	else if (COM->literal_type == &type_vector)
		memcpy (pr_globals + ofs, COM->literal_value, 3 * sizeof(double));
	else
		pr_globals[ofs] = COM->literal_value[0];
}


def_t * real_vm_c::EXP_Constant()
{
	// Looks for a preexisting constant
	def_t *cn = FindConstant();

	if (! cn)
	{
		// allocate a new one
		cn = NewGlobal(COM->literal_type);

		cn->name = "CONSTANT VALUE";

		cn->flags |= DF_Constant;
		cn->scope = NULL;   // literals are always global

		// link into defs list
		cn->next = COM->all_defs;
		COM->all_defs  = cn;

		// copy the literal to the global area
		StoreConstant(cn->ofs);

		COM->constants.push_back(cn);
	}

	LEX_Next();
	return cn;
}


def_t * real_vm_c::EXP_FunctionCall(def_t *func)
{
	type_t * t = func->type;

	if (t->type != ev_function)
		CompileError("not a function");
	
//??	function_t *df = &functions[func->ofs];

	
	// evaluate all parameters
	def_t * exprs[MAX_PARMS];

	int arg = 0;

	if (! LEX_Check(")"))
	{
		do
		{
			if (arg >= t->parm_num)
				CompileError("too many parameters (expected %d)", t->parm_num);

			assert(arg < MAX_PARMS);

			def_t * e = EXP_Expression(TOP_PRIORITY);

			if (e->type != t->parm_types[arg])
				CompileError("type mismatch on parm %i", arg+1);

			assert (e->type->type != ev_void);

			exprs[arg++] = e;
		}
		while (LEX_Check(","));

		if (arg != t->parm_num)
			CompileError("too few parameters (needed %d)", t->parm_num);

		LEX_Expect(")");
	}


	def_t *result = NULL;

	if (t->aux_type->type != ev_void)
	{
		result = NewTemporary(t->aux_type);
	}


	// copy parameters
	int parm_ofs = 0;

	for (int k = 0; k < arg; k++)
	{
		if (exprs[k]->type->type == ev_vector)
		{
			EmitCode(OP_PARM_V, exprs[k]->ofs, parm_ofs);
			parm_ofs += 3;
		}
		else
		{
			EmitCode(OP_PARM_F, exprs[k]->ofs, parm_ofs);
			parm_ofs++;
		}
	}


	// Note: local vars are setup where they are declared, and
	//       temporaries do not need any default value.


	if (result)
	{
		EmitCode(OP_CALL, func->ofs, arg, result->ofs);
		return result;
	}

	EmitCode(OP_CALL, func->ofs, arg, 0);

	return &def_void;
}


void real_vm_c::STAT_Return(void)
{
	if (COM->token_is_first || COM->token_buf[0] == '}' || LEX_Check(";"))
	{
		if (COM->scope->type->aux_type->type != ev_void)
			CompileError("missing value for return");

		EmitCode(OP_DONE);
		return;
	}

	def_t * e = EXP_Expression(TOP_PRIORITY);

	if (COM->scope->type->aux_type->type == ev_void)
		CompileError("return with value in void function");

	if (COM->scope->type->aux_type != e->type)
		CompileError("mismatch types for return");

	if (COM->scope->type->aux_type->type == ev_vector)
	{
		EmitCode(OP_MOVE_V, e->ofs, OFS_RETURN);
		EmitCode(OP_DONE_V);
	}
	else
	{
		EmitCode(OP_MOVE_F, e->ofs, OFS_RETURN);
		EmitCode(OP_DONE);
	}

	// -AJA- optional semicolons
	if (! (COM->token_is_first || COM->token_buf[0] == '}'))
		LEX_Expect(";");
}


def_t * real_vm_c::FindDef(type_t *type, char *name, def_t *scope)
{
	def_t *def;

	for (def = COM->all_defs ; def ; def=def->next)
	{
		if (strcmp(def->name, name) != 0)
			continue;

		if ( def->scope && def->scope != scope)
			continue;		// in a different function

		if (type && def->type != type)
			CompileError("Type mismatch on redeclaration of %s", name);

		return def;
	}

	return NULL;
}


def_t * real_vm_c::GetDef(type_t *type, char *name, def_t *scope)
{
	// A new def will be allocated if it can't be found

	assert(type);

	def_t *def = FindDef(type, name, scope);
	if (def)
		return def;

	// allocate a new def

	if (scope)
		def = NewLocal(type);
	else
		def = NewGlobal(type);

	def->name  = strdup(name);
	def->scope = scope;


	// link into list
	def->next = COM->all_defs;
	COM->all_defs = def;

	return def;
}


def_t * real_vm_c::EXP_VarValue()
{
	char *name = ParseName();

	// look through the defs
	def_t *d = FindDef(NULL, name, COM->scope);
	if (!d)
		CompileError("Unknown identifier '%s'", name);

	return d;
}


def_t * real_vm_c::EXP_Term()
{
	// if the token is a literal, allocate a constant for it
	if (COM->token_type == tt_literal)
		return EXP_Constant();

	if (COM->token_type == tt_name)
		return EXP_VarValue();

	if (LEX_Check("("))
	{
		def_t *e = EXP_Expression(TOP_PRIORITY);
		LEX_Expect(")");
		return e;
	}

	// unary operator?

	opcode_t *op;

	for (op=pr_operators ; op->name ; op++)
	{
		if (op->priority != -1)
			continue;

		if (! LEX_Check(op->name))
			continue;

		def_t * e = EXP_Expression(NOT_PRIORITY);

		for (int i = 0; i == 0 || strcmp(op->name, op[i].name) == 0; i++)
		{
			if (op[i].type_a->type != e->type->type)
				continue;

			def_t *result = NewTemporary(op[i].type_c);

			EmitCode(op[i].op, e->ofs, 0, result->ofs);

			return result;
		}

		CompileError("type mismatch for %s", op->name);
		break;
	}

	CompileError("expected value or unary operator, found %s\n", COM->token_buf);
	return NULL; /* NOT REACHED */
}


def_t * real_vm_c::EXP_ShortCircuit(def_t *e, opcode_t *op)
{
	if (e->type->type != ev_float)
		CompileError("type mismatch for %s", op->name);

	// Instruction stream for &&
	//
	//		... calc a ...
	//		MOVE a --> c
	//		IF c == 0 GOTO label
	//		... calc b ...
	//		MOVE b --> c
	//		label:

	def_t *result = NewTemporary(op->type_c);

	EmitCode(OP_MOVE_F, e->ofs, result->ofs);

	int patch = numstatements;

	if (op->name[0] == '&')
		EmitCode(OP_IFNOT, result->ofs);
	else
		EmitCode(OP_IF, result->ofs);

	def_t *e2 = EXP_Expression(op->priority - 1);
	if (e2->type->type != ev_float)
		CompileError("type mismatch for %s", op->name);

	EmitCode(OP_MOVE_F, e2->ofs, result->ofs);

	statements[patch].b = numstatements;

	return result;
}


def_t * real_vm_c::EXP_FieldQuery(def_t *e, bool lvalue)
{
	// TODO
	CompileError("Operator . not yet implemented\n");
	return NULL;
}


def_t * real_vm_c::EXP_Expression(int priority, bool *lvalue)
{
	if (priority == 0)
		return EXP_Term();

	def_t * e = EXP_Expression(priority-1, lvalue);

	for (;;)
	{
		if (priority == 1 && LEX_Check("("))
		{
			if (lvalue)
				*lvalue = false;
			return EXP_FunctionCall(e);
		}

		if (priority == 1 && LEX_Check("."))
			return EXP_FieldQuery(e, lvalue);

		if (lvalue)
			return e;

		opcode_t *op;

		for (op=pr_operators ; op->name ; op++)
		{
			if (op->priority != priority)
				continue;

			if (! LEX_Check(op->name))
				continue;

			if (strcmp(op->name, "&&") == 0 ||
			    strcmp(op->name, "||") == 0)
			{
				e = EXP_ShortCircuit(e, op);
				break;
			}

			def_t * e2 = EXP_Expression(priority-1);

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
					CompileError("type mismatch for %s", oldop->name);
			}

			if (type_a == ev_pointer && type_b != e->type->aux_type->type)
				CompileError("type mismatch for %s", op->name);

			def_t *result = NewTemporary(op->type_c);

			EmitCode(op->op, e->ofs, e2->ofs, result->ofs);

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


void real_vm_c::STAT_If_Else()
{
	LEX_Expect("(");
	def_t * e = EXP_Expression(TOP_PRIORITY);
	LEX_Expect(")");

	int patch = numstatements;
	EmitCode(OP_IFNOT, e->ofs);

	STAT_Statement(false);
	FreeTemporaries();

	if (LEX_Check("else"))
	{
		// use GOTO to skip over the else statements
		int patch2 = numstatements;
		EmitCode(OP_GOTO);

		statements[patch].b = numstatements;

		STAT_Statement(false);
		FreeTemporaries();

		patch = patch2;
	}
	
	statements[patch].b = numstatements;
}


void real_vm_c::STAT_WhileLoop()
{
	int begin = numstatements;

	LEX_Expect("(");
	def_t * e = EXP_Expression(TOP_PRIORITY);
	LEX_Expect(")");

	int patch = numstatements;
	EmitCode(OP_IFNOT, e->ofs);

	STAT_Statement(false);
	FreeTemporaries();

	EmitCode(OP_GOTO, 0, begin);

	statements[patch].b = numstatements;
}


void real_vm_c::STAT_RepeatLoop()
{
	int begin = numstatements;

	STAT_Statement(false);
	FreeTemporaries();

	LEX_Expect("until");
	LEX_Expect("(");

	def_t * e = EXP_Expression(TOP_PRIORITY);

	EmitCode(OP_IFNOT, e->ofs, begin);

	LEX_Expect(")");

	// -AJA- optional semicolons
	if (! (COM->token_is_first || COM->token_buf[0] == '}'))
		LEX_Expect(";");
}


void real_vm_c::STAT_Assignment(def_t *e)
{
	if (e->flags & DF_Constant)
		CompileError("assignment to a constant.\n");

	def_t *e2 = EXP_Expression(TOP_PRIORITY);

	if (e2->type != e->type)
		CompileError("type mismatch in assignment.\n");

	switch (e->type->type)
	{
		case ev_float:
		case ev_string:
		case ev_function:
			EmitCode(OP_MOVE_F, e2->ofs, e->ofs);
			break;

		case ev_vector:
			EmitCode(OP_MOVE_V, e2->ofs, e->ofs);
			break;

		default:
			CompileError("weird type for assignment.\n");
			break;
	}
}


void real_vm_c::STAT_Statement(bool allow_def)
{
	if (allow_def && LEX_Check("var"))
	{
		GLOB_Variable();
		return;
	}

	if (allow_def && LEX_Check("function"))
	{
		CompileError("functions must be global");
		return;
	}

	if (allow_def && LEX_Check("constant"))
	{
		CompileError("constants must be global");
		return;
	}

	if (LEX_Check("{"))
	{
		do {
			STAT_Statement(true);
			FreeTemporaries();
		}
		while (! LEX_Check("}"));

		return;
	}

	if (LEX_Check("return"))
	{
		STAT_Return();
		return;
	}

	if (LEX_Check("if"))
	{
		STAT_If_Else();
		return;
	}

	if (LEX_Check("while"))
	{
		STAT_WhileLoop();
		return;
	}

	if (LEX_Check("repeat"))
	{
		STAT_RepeatLoop();
		return;
	}

	bool lvalue = true;
	def_t * e = EXP_Expression(TOP_PRIORITY, &lvalue);

	// lvalue is false for a plain function call

	if (lvalue)
	{
		LEX_Expect("=");

		STAT_Assignment(e);
	}

	// -AJA- optional semicolons
	if (! (COM->token_is_first || COM->token_buf[0] == '}'))
		LEX_Expect(";");
}


int real_vm_c::GLOB_FunctionBody(type_t *type, const char *func_name)
{
	COM->temporaries.clear();

	COM->function_line = COM->source_line;

	//
	// check for native function definition
	//
	if (LEX_Check("native"))
	{
		int native = PR_FindNativeFunc(func_name);

		if (native < 0)
			CompileError("No such native function: %s\n", func_name);

		return -(native + 1);
	}

	//
	// create the parmeters as locals
	//
	def_t *defs[MAX_PARMS];

	for (int i=0 ; i < type->parm_num ; i++)
	{
		if (FindDef(type->parm_types[i], pr_parm_names[i], COM->scope))
			CompileError("parameter %s redeclared", pr_parm_names[i]);

		defs[i] = GetDef(type->parm_types[i], pr_parm_names[i], COM->scope);
	}

	int code = numstatements;

	//
	// parse regular statements
	//
	LEX_Expect("{");

	while (! LEX_Check("}"))
	{
		STAT_Statement(true);
		FreeTemporaries();
	}

	if (code == numstatements ||
		! (statements[numstatements-1].op == OP_DONE ||
		   statements[numstatements-1].op == OP_DONE_V))
	{
		if (type->aux_type->type == ev_void)
			EmitCode(OP_DONE);
		else
			CompileError("missing return at end of '%s' function", func_name);
	}

	return code;
}


void real_vm_c::GLOB_Function()
{
	char *func_name = strdup(ParseName());

	LEX_Expect("(");

	type_t t_new;

	memset(&t_new, 0, sizeof(t_new));
	t_new.type = ev_function;
	t_new.parm_num = 0;
	t_new.aux_type = &type_void;

	if (! LEX_Check(")"))
	{
		do
		{
			if (t_new.parm_num >= MAX_PARMS)
				CompileError("too many parameters (over %d)", MAX_PARMS);

			char *name = ParseName();

			strcpy(pr_parm_names[t_new.parm_num], name);

			// parameter type (defaults to float)
			if (LEX_Check(":"))
				t_new.parm_types[t_new.parm_num] = ParseType();
			else
				t_new.parm_types[t_new.parm_num] = &type_float;

			t_new.parm_num++;
		}
		while (LEX_Check(","));

		LEX_Expect(")");
	}

	// return type (defaults to void)
	if (LEX_Check(":"))
	{
		t_new.aux_type = ParseType();
	}

	type_t *func_type = FindType(&t_new);

	def_t *def = GetDef(func_type, func_name, COM->scope);

	if (def->flags & DF_Initialized)
		CompileError("%s redeclared", func_name);


	LEX_Expect("=");

	assert(func_type->type == ev_function);


	G_FUNCTION(def->ofs) = (func_t)functions.size();


	// fill in the dfunction
	function_t *df = new function_t;
	memset(df, 0, sizeof(function_t));

	functions.push_back(df);

	df->name = func_name;  // already strdup'd
	df->source_file = strdup(COM->source_file);
	df->source_line = COM->source_line;

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

	// parms are "realloc'd" by GetDef in FunctionBody (FIXME)
	COM->locals_end = 0;


	COM->scope = def;
	//  { 
		df->first_statement = GLOB_FunctionBody(func_type, func_name);
		df->last_statement  = numstatements-1;
	//  }
	COM->scope = NULL;

	def->flags |= DF_Initialized;

	df->locals_size = COM->locals_end - df->locals_ofs;
	df->locals_end  = COM->locals_end;

// fprintf(stderr, "FUNCTION %s locals:%d\n", func_name, COM->locals_end);
}


void real_vm_c::GLOB_Variable()
{
	char *var_name = strdup(ParseName());

	type_t *type = &type_float;

	if (LEX_Check(":"))
	{
		type = ParseType();
	}

	// TODO
	// if (LEX_Check("="))
	// 	 get default value

	def_t * def = GetDef(type, var_name, COM->scope);

	DefaultValue(def->ofs, type);

	// -AJA- optional semicolons
	if (! (COM->token_is_first || COM->token_buf[0] == '}'))
		LEX_Expect(";");
}


void real_vm_c::GLOB_Constant()
{
	char *const_name = strdup(ParseName());

	LEX_Expect("=");

	if (COM->token_type != tt_literal)
		CompileError("expected value for constant, got %s", COM->token_buf);


	if (FindDef(NULL, const_name, NULL))
		CompileError("name already used: %s", const_name);

	/// TODO: reuse existing constant
	/// def_t * exist_cn = FindConstant();

	def_t * cn = NewGlobal(COM->literal_type);

	cn->name  = const_name;
	cn->flags |= DF_Constant;

	// link into list
	cn->next = COM->all_defs;
	COM->all_defs = cn;

	StoreConstant(cn->ofs);

	COM->constants.push_back(cn);

	LEX_Next();

	// -AJA- optional semicolons
	if (! (COM->token_is_first || COM->token_buf[0] == '}'))
		LEX_Expect(";");
}


void real_vm_c::GLOB_Globals()
{
	if (LEX_Check("function"))
	{
		GLOB_Function();
		return;
	}

	if (LEX_Check("var"))
	{
		GLOB_Variable();
		return;
	}

	if (LEX_Check("constant"))
	{
		GLOB_Constant();
		return;
	}

	CompileError("Expected global definition, found %s", COM->token_buf);
}


//
// compiles the NUL terminated text, adding definitions to the pr structure
//
bool real_vm_c::CompileFile(char *buffer, const char *filename)
{
	COM->source_file = filename;
	COM->source_line = 1;
	COM->function_line = 0;

	COM->parse_p = buffer;
	COM->line_start = buffer;
	COM->bracelevel  = 0;
	COM->parentheses = 0;
	COM->fol_level   = 0;

	LEX_Next();	// read first token

	while (COM->token_type != tt_eof)
	{
		try
		{
			COM->scope = NULL;	// outside all functions

			GLOB_Globals();
		}
		catch (parse_error_x err)
		{
			if (++COM->error_count > MAX_ERRORS)
				return false;

			LEX_SkipToSemicolon();

			if (COM->token_type == tt_eof)
				return false;
		}
	}

	COM->source_file = NULL;

	return (COM->error_count == 0);
}

void real_vm_c::ShowStats()
{
	printf("string memory: %d / %d\n", string_mem.totalUsed(), string_mem.totalMemory());
	printf("%6i numstatements\n", numstatements);
	printf("%6u numfunctions\n", functions.size());
	printf("%6i numpr_globals\n", numpr_globals);
}


real_vm_c::real_vm_c() :
	global_mem(), string_mem(), op_mem(), functions()
{
// FIXME TEMP HACK !!!!!!
COM = new compiling_c;


	// string #0 must be the empty string
	int ofs = string_mem.alloc(2);
	assert(ofs == 0);
	strcpy((char *)string_mem.deref(0), "");


	// function #0 is the "null function" 
	function_t *df = new function_t;
	memset(df, 0, sizeof(function_t));

	functions.push_back(df);


	// FIXME: clear native functions


	numstatements = 1;


	numpr_globals = RESERVED_OFS;


// FIXME NEEDED ????
// link the function type in so state forward declarations match proper type
	COM->all_types = &type_function;
	type_function.next = NULL;
}

real_vm_c::~real_vm_c()
{
	// FIXME !!!!
}


void real_vm_c::SetErrorFunc(print_func_t func)
{
	// TODO
}

void real_vm_c::SetPrintFunc(print_func_t func)
{
	// TODO
}


vm_c * CreateVM()
{
	return new real_vm_c;
}

}  // namespace coal

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
