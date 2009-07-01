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


#include "coal.h"


pr_info_t	pr;
def_t		*pr_global_defs[MAX_REGS];	// to find def for a global variable

//========================================

def_t		*pr_scope;		// the function being parsed, or NULL
bool	pr_dumpasm;
string_t	s_file;			// filename for function definition

int			locals_end;		// for tracking local variables vs temps


void PR_ParseFunction (void);
void PR_ParseVariable (void);
void PR_ParseConstant (void);

//========================================


opcode_t pr_opcodes[] =
{
	{"<DONE>", "DONE", -1, false, &type_void, &type_void, &type_void},

	{"!", "NOT_F", -1, false, &type_float, &type_void, &type_float},
	{"!", "NOT_V", -1, false, &type_vector, &type_void, &type_float},
	{"!", "NOT_S", -1, false, &type_vector, &type_void, &type_float},
	{"!", "NOT_FNC", -1, false, &type_function, &type_void, &type_float},

	/* priority 1 is for function calls */

	{"^", "POWER", 2, false, &type_float, &type_float, &type_float},

	{"*", "MUL_F", 2, false, &type_float, &type_float, &type_float},
	{"*", "MUL_V", 2, false, &type_vector, &type_vector, &type_float},
	{"*", "MUL_FV", 2, false, &type_float, &type_vector, &type_vector},
	{"*", "MUL_VF", 2, false, &type_vector, &type_float, &type_vector},

	{"/", "DIV_F", 2, false, &type_float, &type_float, &type_float},
	{"/", "DIV_V", 2, false, &type_vector, &type_float, &type_vector},
	{"%", "MOD_F", 2, false, &type_float, &type_float, &type_float},

	{"+", "ADD_F", 3, false, &type_float, &type_float, &type_float},
	{"+", "ADD_V", 3, false, &type_vector, &type_vector, &type_vector},

	{"-", "SUB_F", 3, false, &type_float, &type_float, &type_float},
	{"-", "SUB_V", 3, false, &type_vector, &type_vector, &type_vector},

	{"==", "EQ_F", 4, false, &type_float, &type_float, &type_float},
	{"==", "EQ_V", 4, false, &type_vector, &type_vector, &type_float},
	{"==", "EQ_S", 4, false, &type_string, &type_string, &type_float},
	{"==", "EQ_FNC", 4, false, &type_function, &type_function, &type_float},

	{"!=", "NE_F", 4, false, &type_float, &type_float, &type_float},
	{"!=", "NE_V", 4, false, &type_vector, &type_vector, &type_float},
	{"!=", "NE_S", 4, false, &type_string, &type_string, &type_float},
	{"!=", "NE_FNC", 4, false, &type_function, &type_function, &type_float},

	{"<=", "LE", 4, false, &type_float, &type_float, &type_float},
	{">=", "GE", 4, false, &type_float, &type_float, &type_float},
	{"<",  "LT", 4, false, &type_float, &type_float, &type_float},
	{">",  "GT", 4, false, &type_float, &type_float, &type_float},

	{"=", "MOVE_F", 6, true, &type_float, &type_float, &type_float},
	{"=", "MOVE_V", 6, true, &type_vector, &type_vector, &type_vector},
	{"=", "MOVE_S", 6, true, &type_string, &type_string, &type_string},
	{"=", "MOVE_FNC", 6, true, &type_function, &type_function, &type_function},

// calls returns REG_RETURN
	{"<CALL>",  "CALL", -1, false, &type_function, &type_void, &type_void},
	{"<RETURN>", "RETURN", -1, false, &type_void, &type_void, &type_void},

	{"<IF>", "IF", -1, false, &type_float, &type_float, &type_void},
	{"<IFNOT>", "IFNOT", -1, false, &type_float, &type_float, &type_void},

	{"<GOTO>", "GOTO", -1, false, &type_float, &type_void, &type_void},

	{"&&", "AND", 5, false, &type_float, &type_float, &type_float},
	{"||", "OR",  5, false, &type_float, &type_float, &type_float},

	{"&", "BITAND", 2, false, &type_float, &type_float, &type_float},
	{"|", "BITOR", 2, false, &type_float, &type_float, &type_float},

	{NULL}
};

#define	TOP_PRIORITY	6
#define	NOT_PRIORITY	1

def_t *PR_Expression (int priority);

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


def_t *PR_NewVar(type_t *type)
{
	def_t * var_c = new def_t;
	memset (var_c, 0, sizeof(def_t));

	var_c->ofs = numpr_globals;
	var_c->type = type;

	numpr_globals += type_size[type->type];

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
	{	// allocate result space
		var_c = PR_NewVar(op->type_c);
	}

	PR_EmitCode(op - pr_opcodes,
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
	for (cn=pr.defs ; cn ; cn=cn->next)
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

	cn->next = pr.defs;
	pr.defs  = cn;

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

	// copy the arguments to the global parameter variables
	int arg = 0;

	if (! PR_Check(")"))
	{
		do
		{
			if (t->parm_num != -1 && arg >= t->parm_num)
				PR_ParseError ("too many parameters");

			def_t * e = PR_Expression(TOP_PRIORITY);

			if (t->parm_num != -1 && ( e->type != t->parm_types[arg] ) )
				PR_ParseError ("type mismatch on parm %i", arg+1);

			// FIXME HACK : a vector copy will copy everything
			def_parms[arg].type = t->parm_types[arg];
			PR_Statement (&pr_opcodes[OP_MOVE_V], e, &def_parms[arg]);
			arg++;
		}
		while (PR_Check(","));

		if (t->parm_num != -1 && arg != t->parm_num)
			PR_ParseError("too few parameters");
		PR_Expect(")");
	}

	if (arg >8)
		PR_ParseError ("More than eight parameters");

	PR_EmitCode(OP_CALL, func->ofs, arg);

	if (t->aux_type->type == ev_void)
		return &def_void;

	def_ret.type = t->aux_type;
	return &def_ret;
}


def_t * PR_ParseValue(void)
{
	// if the token is an immediate, allocate a constant for it
	if (pr_token_type == tt_immediate)
		return PR_ParseImmediate();

	char *name = PR_ParseName();

	// look through the defs
	def_t *d = PR_GetDef (NULL, name, pr_scope, false);
	if (!d)
		PR_ParseError ("Unknown value \"%s\"", name);
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
			e2 = PR_Statement (&pr_opcodes[OP_NOT_F], e);
		else if (t == ev_string)
			e2 = PR_Statement (&pr_opcodes[OP_NOT_S], e);
		else if (t == ev_vector)
			e2 = PR_Statement (&pr_opcodes[OP_NOT_V], e);
		else if (t == ev_function)
			e2 = PR_Statement (&pr_opcodes[OP_NOT_FNC], e);
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

	def_t *result = PR_NewVar(op->type_c);

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


def_t * PR_Expression(int priority)
{
	if (priority == 0)
		return PR_Term();

	def_t * e = PR_Expression(priority-1);

	while (1)
	{
		if (priority == 1 && PR_Check("("))
			return PR_ParseFunctionCall(e);

		opcode_t *op;

		for (op=pr_opcodes ; op->name ; op++)
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

			if (op->name[0] == '.')// field access gets type from field
			{
				if (e2->type->aux_type)
					type_c = e2->type->aux_type->type;
				else
					type_c = ev_INVALID;	// not a field
			}


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


			if (op->right_associative)
				e = PR_Statement (op, e2, e);
			else
				e = PR_Statement (op, e, e2);

			if (type_c != ev_void)	// field access gets type from field
				e->type = e2->type->aux_type;

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
		PR_ParseError ("Functions must be global");
		return;
	}

	if (PR_Check("var"))
	{
		PR_ParseVariable ();
		locals_end = numpr_globals;
		return;
	}

	if (PR_Check("constant"))
	{
		PR_ParseConstant ();
		locals_end = numpr_globals;
		return;
	}

	if (PR_Check ("{"))
	{
		do {
			PR_ParseStatement();
		}
		while (! PR_Check("}"));

		return;
	}

	if (PR_Check("return"))
	{
		if (PR_Check (";"))
		{
			if (pr_scope->type->aux_type->type != ev_void)
				PR_ParseError("missing value for return");

			PR_EmitCode(OP_RETURN);
			return;
		}

		def_t * e = PR_Expression(TOP_PRIORITY);
		PR_Expect (";");

		if (pr_scope->type->aux_type->type == ev_void)
			PR_ParseError("return with value in void function");

		if (pr_scope->type->aux_type != e->type)
			PR_ParseError("mismatch types for return");

		PR_EmitCode(OP_RETURN, e->ofs);
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

	PR_Expression(TOP_PRIORITY);
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

	int parm_ofs[MAX_PARMS];

	for (int i=0 ; i<type->parm_num ; i++)
	{
		defs[i] = PR_GetDef (type->parm_types[i], pr_parm_names[i], pr_scope, true);

		parm_ofs[i] = defs[i]->ofs;

		if (i > 0 && parm_ofs[i] < parm_ofs[i-1])
			Error ("bad parm order");
	}

	int code = numstatements;

	//
	// parse regular statements
	//
	PR_Expect ("{");

	while (! PR_Check("}"))
		PR_ParseStatement();

	// emit an end of statements opcode
	PR_Statement(&pr_opcodes[OP_DONE]);

	return code;
}

/*
============
PR_GetDef

If type is NULL, it will match any type
If allocate is true, a new def will be allocated if it can't be found
============
*/
def_t *PR_GetDef (type_t *type, char *name, def_t *scope, bool allocate)
{
	def_t *def;
	char element[MAX_NAME];

	// see if the name is already in use
	for (def = pr.defs ; def ; def=def->next)
		if (!strcmp(def->name,name) )
		{
			if ( def->scope && def->scope != scope)
				continue;		// in a different function

			if (type && def->type != type)
				PR_ParseError ("Type mismatch on redeclaration of %s",name);

			return def;
		}

	if (!allocate)
		return NULL;

	// allocate a new def
	def = new def_t;
	memset (def, 0, sizeof(*def));

	def->next = pr.defs;
	pr.defs = def;

	def->name = strdup(name);
	def->type = type;

	def->scope = scope;

	def->ofs = numpr_globals;
	pr_global_defs[numpr_globals] = def;

	numpr_globals += type_size[type->type];

	return def;
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

	def_t *def = PR_GetDef (func_type, func_name, pr_scope, true);

	if (def->initialized)
		PR_ParseError ("%s redeclared", func_name);


	PR_Expect("=");

	assert(func_type->type == ev_function);

	int locals_start;
	locals_start = locals_end = numpr_globals;

	pr_scope = def;
	//  { 
		int code = PR_ParseFunctionBody(func_type);
	//  }
	pr_scope = NULL;

	def->initialized = 1;
	G_FUNCTION(def->ofs) = (func_t)numfunctions;


	// fill in the dfunction
	function_t	*df = &functions[numfunctions++];

	df->first_statement = code;

	df->s_name = CopyString (def->name);
	df->s_file = s_file;
	df->parm_num =  def->type->parm_num;
	df->locals = locals_end - locals_start;
	df->parm_start = locals_start;

	for (int i=0 ; i<df->parm_num ; i++)
		df->parm_size[i] = type_size[def->type->parm_types[i]->type];
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

	PR_GetDef (type, var_name, pr_scope, true);

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


	type_t *type = PR_ParseType ();

	do
	{
		char *name = PR_ParseName ();

		def_t *def = PR_GetDef (type, name, pr_scope, true);

// check for an initialization
		if ( PR_Check ("=") )
		{
			if (def->initialized)
				PR_ParseError ("%s redeclared", name);

			if (type->type == ev_function)
				PR_ParseError("Cannot initialise function");

			if (pr_immediate_type != type)
				PR_ParseError ("wrong immediate type for %s", name);

			def->initialized = 1;
			memcpy (pr_globals + def->ofs, pr_immediate, sizeof(double) * type_size[pr_immediate_type->type]);
			PR_Lex ();
		}
	}
	while (PR_Check (","));

	PR_Expect (";");
}

/*
============
PR_CompileFile

compiles the 0 terminated text, adding defintions to the pr structure
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


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
