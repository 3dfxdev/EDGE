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


#include "c_local.h"

#include "coal.h"


#include "q_lex.c"  // FIXME


type_t * all_types;
def_t  * all_defs;

def_t * pr_global_defs[MAX_REGS];	// to find def for a global variable

def_t		*pr_scope;		// the function being parsed, or NULL
bool	pr_dumpasm;
string_t	s_file;			// filename for function definition

int			locals_end;		// for tracking local variables vs temps


void PR_ParseFunction (void);
void PR_ParseVariable (void);
void PR_ParseConstant (void);

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
	{"!", OP_NOT_F, -1, false, &type_float, &type_void, &type_float},
	{"!", OP_NOT_V, -1, false, &type_vector, &type_void, &type_float},
	{"!", OP_NOT_S, -1, false, &type_vector, &type_void, &type_float},
	{"!", OP_NOT_FNC, -1, false, &type_function, &type_void, &type_float},

	/* priority 1 is for function calls */

	{"^", OP_POWER_F, 2, false, &type_float, &type_float, &type_float},

	{"*", OP_MUL_F, 2, false, &type_float, &type_float, &type_float},
	{"*", OP_MUL_V, 2, false, &type_vector, &type_vector, &type_float},
	{"*", OP_MUL_FV, 2, false, &type_float, &type_vector, &type_vector},
	{"*", OP_MUL_VF, 2, false, &type_vector, &type_float, &type_vector},

	{"/", OP_DIV_F, 2, false, &type_float, &type_float, &type_float},
	{"/", OP_DIV_V, 2, false, &type_vector, &type_float, &type_vector},
	{"%", OP_MOD_F, 2, false, &type_float, &type_float, &type_float},

	{"+", OP_ADD_F, 3, false, &type_float, &type_float, &type_float},
	{"+", OP_ADD_V, 3, false, &type_vector, &type_vector, &type_vector},

	{"-", OP_SUB_F, 3, false, &type_float, &type_float, &type_float},
	{"-", OP_SUB_V, 3, false, &type_vector, &type_vector, &type_vector},

	{"==", OP_EQ_F, 4, false, &type_float, &type_float, &type_float},
	{"==", OP_EQ_V, 4, false, &type_vector, &type_vector, &type_float},
	{"==", OP_EQ_S, 4, false, &type_string, &type_string, &type_float},
	{"==", OP_EQ_FNC, 4, false, &type_function, &type_function, &type_float},

	{"!=", OP_NE_F, 4, false, &type_float, &type_float, &type_float},
	{"!=", OP_NE_V, 4, false, &type_vector, &type_vector, &type_float},
	{"!=", OP_NE_S, 4, false, &type_string, &type_string, &type_float},
	{"!=", OP_NE_FNC, 4, false, &type_function, &type_function, &type_float},

	{"<=", OP_LE, 4, false, &type_float, &type_float, &type_float},
	{">=", OP_GE, 4, false, &type_float, &type_float, &type_float},
	{"<", OP_LT, 4, false, &type_float, &type_float, &type_float},
	{">", OP_GT, 4, false, &type_float, &type_float, &type_float},

	{"&&", OP_AND, 5, false, &type_float, &type_float, &type_float},
	{"||", OP_OR, 5, false, &type_float, &type_float, &type_float},

	{"&", OP_BITAND, 2, false, &type_float, &type_float, &type_float},
	{"|", OP_BITOR, 2, false, &type_float, &type_float, &type_float},

	{NULL}
};

#define	TOP_PRIORITY	6
#define	NOT_PRIORITY	1

def_t *PR_Expression(int priority, bool *lvalue = NULL);

def_t	junkdef;

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


def_t *PR_NewGlobal(type_t *type)
{
	def_t * var_c = new def_t;
	memset (var_c, 0, sizeof(def_t));

	var_c->ofs = numpr_globals;
	var_c->type = type;

	numpr_globals += type_size[type->type];

	return var_c;
}


def_t *PR_NewLocal(type_t *type)
{
	def_t * var_c = new def_t;
	memset (var_c, 0, sizeof(def_t));

	var_c->ofs = -locals_end;
	var_c->type = type;

	locals_end += type_size[type->type];

	return var_c;
}


/*
============
PR_Statement

Emits a primitive statement, returning the var it places it's value in
============
*/
def_t * PR_Statement(opcode_t *op, def_t *var_a = NULL, def_t *var_b = NULL)
{
	def_t *var_c = NULL;

	if (op->type_c == &type_void || op->right_associative)
	{
		// ifs, gotos, and assignments don't need vars allocated
	}
	else
	{	// allocate return space
		var_c = PR_NewLocal(op->type_c);
	}

	PR_EmitCode(op->op,
    			var_a ? var_a->ofs : 0,
    			var_b ? var_b->ofs : 0,
    			var_c ? var_c->ofs : 0);

	if (op->right_associative)
		return var_a;

	return var_c;
}


def_t * PR_ParseImmediate(void)
{
	// Looks for a preexisting constant

	def_t *cn;

	char im_name[32];

	// check for a constant with the same value
	for (cn=all_defs ; cn ; cn=cn->next)
	{
		if (!cn->initialized)
			continue;
		if (cn->type != pr_immediate_type)
			continue;

		if (pr_immediate_type == &type_string)
		{
			if (!strcmp(G_STRING(cn->ofs), pr_immediate_string) )
			{
				PR_Lex ();
				return cn;
			}
		}
		else if (pr_immediate_type == &type_float)
		{
			if ( G_FLOAT(cn->ofs) == pr_immediate[0] )
			{
				PR_Lex ();
				return cn;
			}
		}
		else if	(pr_immediate_type == &type_vector)
		{
			if ( ( G_FLOAT(cn->ofs)     == pr_immediate[0] )
				&& ( G_FLOAT(cn->ofs+1) == pr_immediate[1] )
				&& ( G_FLOAT(cn->ofs+2) == pr_immediate[2] ) )
			{
				PR_Lex ();
				return cn;
			}
		}
		else
			PR_ParseError ("weird immediate type");
	}

	// allocate a new one
	cn = new def_t;

	cn->next = all_defs;
	all_defs  = cn;

	sprintf(im_name, "IMMED_%p", cn);

	cn->type = pr_immediate_type;
	cn->name = strdup(im_name);
	cn->initialized = 1;
	cn->scope = NULL;		// always share immediates

	// copy the immediate to the global area
	cn->ofs = numpr_globals;
	pr_global_defs[cn->ofs] = cn;
	numpr_globals += type_size[pr_immediate_type->type];

	if (pr_immediate_type == &type_string)
		pr_immediate[0] = CopyString (pr_immediate_string);

	memcpy (pr_globals + cn->ofs, pr_immediate, sizeof(double) * type_size[pr_immediate_type->type]);

	PR_Lex ();

	return cn;
}


def_t * PR_ParseFunctionCall(def_t *func)
{
	type_t * t = func->type;

	if (t->type != ev_function)
		PR_ParseError ("not a function");
	
	function_t *df = &functions[func->ofs];

	
	// evaluate all parameters
	def_t * exprs[8];

	int arg = 0;

	if (! PR_Check(")"))
	{
		do
		{
			if (arg >= t->parm_num || arg >= 8)
				PR_ParseError ("too many parameters");

			def_t * e = PR_Expression(TOP_PRIORITY);

			if (e->type != t->parm_types[arg])
				PR_ParseError ("type mismatch on parm %i", arg+1);

			assert (e->type->type != ev_void);

			exprs[arg++] = e;
		}
		while (PR_Check(","));

		if (arg != t->parm_num)
			PR_ParseError("too few parameters");

		PR_Expect(")");
	}


	def_t *result = NULL;

	if (t->aux_type->type != ev_void)
	{
		result = PR_NewLocal(t->aux_type);

		// FIXME: set result to default value
		// PR_EmitCode(OP_MOVE_F, default_value, result->ofs)
	}


	int parm_ofs = 0;


	// copy parameters
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


	// FIXME: setup locals
	// for (int j = 0; j < df->local_size; j++)
	// 	  PR_EmitCode(OP_PARM_F, default_value, parm_ofs)
	// 	  parm_ofs++;

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
	if (PR_Check(";"))
	{
		if (pr_scope->type->aux_type->type != ev_void)
			PR_ParseError("missing value for return");

		PR_EmitCode(OP_DONE);
		return;
	}

	def_t * e = PR_Expression(TOP_PRIORITY);
	PR_Expect(";");

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
}


def_t *PR_FindDef(type_t *type, char *name, def_t *scope)
{
	def_t *def;

	for (def = all_defs ; def ; def=def->next)
	{
		if (strcmp(def->name,name) != 0)
			continue;

		if ( def->scope && def->scope != scope)
			continue;		// in a different function

		if (type && def->type != type)
			PR_ParseError ("Type mismatch on redeclaration of %s", name);

		return def;
	}

	return NULL;
}


/*
============
PR_GetDef

a new def will be allocated if it can't be found
============
*/
def_t *PR_GetDef(type_t *type, char *name, def_t *scope)
{
	assert(type);

	def_t *def = PR_FindDef(type, name, scope);

	// allocate a new def
	def = new def_t;
	memset(def, 0, sizeof(*def));

	def->next = all_defs;
	all_defs = def;

	def->name = strdup(name);
	def->type = type;

	def->scope = scope;

	if (! scope)
	{
printf("GetDef scope:%p --> %d\n", scope, numpr_globals);
		def->ofs = numpr_globals;
		pr_global_defs[numpr_globals] = def;

		numpr_globals += type_size[type->type];
	}
	else
	{
printf("GetDef scope:%p --> %d\n", scope, -locals_end);
		def->ofs = -locals_end;

		locals_end += type_size[type->type];
	}

	return def;
}


def_t * PR_ParseValue(void)
{
	// if the token is an immediate, allocate a constant for it
	if (pr_token_type == tt_immediate)
		return PR_ParseImmediate();

	char *name = PR_ParseName();

	// look through the defs
	def_t *d = PR_FindDef(NULL, name, pr_scope);
	if (!d)
		PR_ParseError ("Unknown identifier '%s'", name);

	return d;
}


def_t * PR_Term(void)
{
	if (PR_Check("!"))
	{
		def_t * e = PR_Expression(NOT_PRIORITY);
		etype_t t = e->type->type;

		def_t *e2 = NULL;

		if (t == ev_float)
			e2 = PR_Statement (&pr_operators[OP_NOT_F], e);
		else if (t == ev_string)
			e2 = PR_Statement (&pr_operators[OP_NOT_S], e);
		else if (t == ev_vector)
			e2 = PR_Statement (&pr_operators[OP_NOT_V], e);
		else if (t == ev_function)
			e2 = PR_Statement (&pr_operators[OP_NOT_FNC], e);
		else
			PR_ParseError("type mismatch for !");

		return e2;
	}

	if (PR_Check("("))
	{
		def_t *e = PR_Expression(TOP_PRIORITY);
		PR_Expect(")");
		return e;
	}

	return PR_ParseValue();
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

	def_t *result = PR_NewLocal(op->type_c);

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

	while (1)
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

			def_t * e2;

			if (op->right_associative)
				e2 = PR_Expression(priority);
			else
				e2 = PR_Expression(priority-1);

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


			assert(! op->right_associative);

			if (op->right_associative)
				e = PR_Statement (op, e2, e);
			else
				e = PR_Statement (op, e, e2);

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


void PR_ParseStatement(void)
{
	if (PR_Check("function"))
	{
		PR_ParseError("Functions must be global");
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

	if (PR_Check("{"))
	{
		do {
			PR_ParseStatement();
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
		PR_Expect("(");
		def_t * e = PR_Expression(TOP_PRIORITY);
		PR_Expect(")");

		int patch = numstatements;
		PR_EmitCode(OP_IFNOT, e->ofs);

		PR_ParseStatement();

		if (PR_Check("else"))
		{
			// use GOTO to skip over the else statements
			int patch2 = numstatements;
			PR_EmitCode(OP_GOTO);

			statements[patch].b = numstatements;

			PR_ParseStatement ();

			patch = patch2;
		}
		
		statements[patch].b = numstatements;
		return;
	}

	if (PR_Check("while"))
	{
		int begin = numstatements;

		PR_Expect ("(");
		def_t * e = PR_Expression(TOP_PRIORITY);
		PR_Expect (")");

		int patch = numstatements;
		PR_EmitCode(OP_IFNOT, e->ofs);

		PR_ParseStatement ();

		PR_EmitCode(OP_GOTO, 0, begin);

		statements[patch].b = numstatements;
		return;
	}

	if (PR_Check("do"))
	{
		int begin = numstatements;

		PR_ParseStatement ();

		PR_Expect ("while");
		PR_Expect ("(");
		def_t * e = PR_Expression(TOP_PRIORITY);
		PR_Expect (")");
		PR_Expect (";");

		PR_EmitCode(OP_IF, e->ofs, begin);
		return;
	}

	bool lvalue = true;
	def_t * e = PR_Expression(TOP_PRIORITY, &lvalue);

	// lvalue is false for a plain function call

	if (lvalue)
	{
		// FIXME: proper constant check
		if (strncmp(e->name, "IMMED_", 6) == 0)
			PR_ParseError("assignment to a constant.\n");

		PR_Expect("=");

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

	PR_Expect (";");
}


int PR_ParseFunctionBody(type_t *type)
{
	//
	// check for builtin function definition #1, #2, etc
	//
	if (PR_Check ("#"))
	{
		if (pr_token_type != tt_immediate
			|| pr_immediate_type != &type_float
			|| pr_immediate[0] != (int)pr_immediate[0])
		{
			PR_ParseError ("Bad builtin immediate");
		}
		int builtin = (int)pr_immediate[0];
		PR_Lex ();
		return -builtin;
	}

	//
	// define the parms
	//
	def_t *defs[MAX_PARMS];

	for (int i=0 ; i<type->parm_num ; i++)
	{
		defs[i] = PR_GetDef (type->parm_types[i], pr_parm_names[i], pr_scope);
	}

	int code = numstatements;

	//
	// parse regular statements
	//
	PR_Expect ("{");

	while (! PR_Check("}"))
	{
		PR_ParseStatement();
	}

	PR_EmitCode(OP_DONE);

	return code;
}


void PR_ParseFunction(void)
{
	char *func_name = strdup(PR_ParseName());

	PR_Expect("(");

	type_t t_new;

	memset (&t_new, 0, sizeof(t_new));
	t_new.type = ev_function;
	t_new.parm_num = 0;
	t_new.aux_type = &type_void;

	if (! PR_Check(")"))
	{
		do
		{
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

	if (def->initialized)
		PR_ParseError ("%s redeclared", func_name);


	PR_Expect("=");

	assert(func_type->type == ev_function);


	G_FUNCTION(def->ofs) = (func_t)numfunctions;
printf("Setting func '%s' ofs:%d --> %d\n", func_name, def->ofs, numfunctions);
	numfunctions++;


	// fill in the dfunction
	function_t *df = &functions[(int)G_FUNCTION(def->ofs)];
	memset(df, 0, sizeof(function_t));

	df->s_name = CopyString (func_name);
	df->s_file = s_file;

	int stack_ofs = 1;

	df->return_size = type_size[def->type->aux_type->type];
	if (def->type->aux_type->type == ev_void) df->return_size = 0;
	stack_ofs += df->return_size;

	df->parm_num = def->type->parm_num;

	for (int i=0 ; i < df->parm_num ; i++)
	{
		df->parm_ofs[i]  = stack_ofs;
		df->parm_size[i] = type_size[def->type->parm_types[i]->type];
		if (def->type->parm_types[i]->type == ev_void) df->parm_size[i] = 0;

		stack_ofs += df->parm_size[i];
	}


	df->locals_ofs = stack_ofs;

	locals_end = df->locals_ofs;


	pr_scope = def;
	//  { 
		df->first_statement = PR_ParseFunctionBody(func_type);
	//  }
	pr_scope = NULL;

	def->initialized = 1;

	df->locals_size = locals_end - df->locals_ofs;
	df->locals_end  = locals_end;
}


void PR_ParseVariable(void)
{
	// FIXME

	char *var_name = strdup(PR_ParseName());

	type_t *type = &type_float;

	if (PR_Check(":"))
	{
		type = PR_ParseType();
	}

	// TODO
	// if (PR_Check("="))
	// 	 get default value

	PR_GetDef(type, var_name, pr_scope);

	PR_Expect(";");  // FIXME: allow EOL as well
}


void PR_ParseConstant(void)
{
	char *const_name = strdup(PR_ParseName());

	PR_Expect("=");

	if (pr_token_type != tt_immediate)
		PR_ParseError("Expected value for constant");

	// FIXME: create constant
	  PR_ParseError("Constants not yet implemented");
	
	PR_Lex();
	PR_Expect(";");  // FIXME: allow EOL as well
}


void PR_ParseGlobals (void)
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

	if (PR_Check ("constant"))
	{
		PR_ParseConstant();
		return;
	}


	PR_ParseError("Syntax error");
	{ return; }

}


/*
============
PR_CompileFile

compiles the 0 terminated text, adding definitions to the pr structure
============
*/
bool PR_CompileFile (char *string, char *filename)
{
	pr_file_p = string;
	s_file = CopyString (filename);

	pr_source_line = 0;

	PR_NewLine ();

	PR_Lex ();	// read first token

	while (pr_token_type != tt_eof)
	{
		try
		{
			pr_scope = NULL;	// outside all functions

			PR_ParseGlobals ();
		}
		catch (parse_error_x err)
		{
			if (++pr_error_count > MAX_ERRORS)
				return false;

			PR_SkipToSemicolon ();

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
void PR_BeginCompilation (void)
{
	int i;

	numpr_globals = RESERVED_OFS;
	all_defs = NULL;

	for (i=0 ; i<RESERVED_OFS ; i++)
		pr_global_defs[i] = &def_void;

// link the function type in so state forward declarations match proper type
	all_types = &type_function;
	type_function.next = NULL;

	pr_error_count = 0;
}

/*
==============
PR_FinishCompilation

called after all files are compiled to check for errors
Returns false if errors were detected.
==============
*/
bool PR_FinishCompilation (void)
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
			if (!d->initialized)
			{
				printf ("function %s was not defined\n",d->name);
				errors = true;
			}
		}
	}

	return !errors;
}



//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab