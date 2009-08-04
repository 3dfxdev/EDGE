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

#include "coal.h"

#include <vector>


namespace coal
{

#include "c_local.h"


// FIXME
#define Con_Printf   printf
#define Con_DPrintf  printf

#define MAX_PRINTMSG  1024


double		pr_globals[MAX_REGS];
int			numpr_globals;

statement_t	statements[MAX_STATEMENTS];
int			numstatements;

function_t	functions[MAX_FUNCTIONS];
int			numfunctions;


typedef struct
{
    int s;
    function_t *f;
	int result;
}
prstack_t;

#define	MAX_STACK_DEPTH		96
prstack_t pr_stack[MAX_STACK_DEPTH+1];
int pr_depth;

#define	LOCALSTACK_SIZE		2048
double localstack[LOCALSTACK_SIZE];
int stack_base;

function_t *pr_xfunction;
int pr_xstatement;
int pr_argc;

#define MAX_RUNAWAY  2000000


typedef struct
{
	const char *name;
	native_func_t func;
}
reg_native_func_t;


static std::vector<reg_native_func_t *> native_funcs;


int PR_FindNativeFunc(const char *name)
{
	for (int i = 0; i < (int)native_funcs.size(); i++)
		if (strcmp(native_funcs[i]->name, name) == 0)
			return i;
	
	return -1;  // NOT FOUND
}

void real_vm_c::AddNativeFunction(const char *name, native_func_t func)
{
	// already registered?
	int prev = PR_FindNativeFunc(name);

	if (prev >= 0)
	{
		native_funcs[prev]->func = func;
		return;
	}

	reg_native_func_t *reg = new reg_native_func_t;

	reg->name = strdup(name);
	reg->func = func;

	native_funcs.push_back(reg);
}


void real_vm_c::SetTrace(bool enable)
{
	trace = enable;
}


int real_vm_c::FindFunction(const char *func_name)
{
	for (int i = 1; i < numfunctions; i++)
	{
		function_t *f = &functions[i];

		if (strcmp(f->name, func_name) == 0)
			return i;
	}

	return -1;  // NOT FOUND
}

int real_vm_c::FindVariable(const char *var_name)
{
	// FIXME

	return -1;  // NOT FOUND
}

// returns an offset from the string heap
int	real_vm_c::InternaliseString(const char *new_s)
{
	if (new_s[0] == 0)
		return 0;

	int ofs = string_mem.alloc(strlen(new_s) + 1);
	strcpy((char *)string_mem.deref(ofs), new_s);

	return ofs;
}


/*
===============
PR_String

Returns a string suitable for printing (no newlines, max 60 chars length)
===============
*/
char *PR_String(char *string)
{
	static char buf[80];
	char	*s;

	s = buf;
	*s++ = '"';
	while (string && *string)
	{
		if (s == buf + sizeof(buf) - 2)
			break;
		if (*string == '\n')
		{
			*s++ = '\\';
			*s++ = 'n';
		}
		else if (*string == '"')
		{
			*s++ = '\\';
			*s++ = '"';
		}
		else
			*s++ = *string;
		string++;
		if (s - buf > 60)
		{
			*s++ = '.';
			*s++ = '.';
			*s++ = '.';
			break;
		}
	}
	*s++ = '"';
	*s++ = 0;
	return buf;
}


/*
============
PR_ValueString

Returns a string describing *data in a type specific manner
=============
*/
char *PR_ValueString(etype_t type, double *val)
{
	static char	line[256];
	def_t		*def;
	function_t	*f;

	line[0] = 0;

	switch (type)
	{
	case ev_string:
//!!!!		sprintf(line, "%s", PR_String(REF_STRING((int)*val)));
		break;
//	case ev_entity:
//		sprintf (line, "entity %i", *(int *)val);
//		break;
	case ev_function:
		f = functions + (int)*val;
		if (!f)
			sprintf(line, "undefined function");
//!!!!		else
//!!!!			sprintf(line, "%s()", REF_STRING(f->s_name));
		break;
//	case ev_field:
//		def = PR_DefForFieldOfs ( *(int *)val );
//		sprintf (line, ".%s", def->name);
//		break;
	case ev_void:
		sprintf(line, "void");
		break;
	case ev_float:
		sprintf(line, "%5.1f", (float) *val);
		break;
	case ev_vector:
		sprintf(line, "'%5.1f %5.1f %5.1f'", val[0], val[1], val[2]);
		break;
	case ev_pointer:
		sprintf(line, "pointer");
		break;
	default:
		sprintf(line, "bad type %i", type);
		break;
	}

	return line;
}

/*
============
PR_GlobalString

Returns a string with a description and the contents of a global,
padded to 20 field width
============
*/

#define GVAL(o)  ((o < 0) ? o : pr_globals[o])

char *PR_GlobalStringNoContents(gofs_t ofs)
{
	static char	line[128];

	def_t * def = NULL; // FIXME pr_global_defs[ofs];
	if (!def)
//		Error ("PR_GlobalString: no def for %i", ofs);
		sprintf(line,"%i(?? =%1.2f)", ofs, GVAL(ofs));
	else
		sprintf(line,"%i(%s =%1.2f)", ofs, def->name, GVAL(ofs));

	int i = strlen(line);
	for ( ; i<16 ; i++)
		strcat(line," ");
	strcat(line," ");

	return line;
}

char *PR_GlobalString(gofs_t ofs)
{
	static char	line[128];

	def_t *def = NULL; // FIXME pr_global_defs[ofs];
	if (!def)
		return PR_GlobalStringNoContents(ofs);

	if (false) /// def->initialized && def->type->type != ev_function)
	{
		char *s = PR_ValueString(def->type->type, &pr_globals[ofs]);
		sprintf(line,"%i(%s =%1.2f)", ofs, s, GVAL(ofs));
	}
	else
		sprintf(line,"%i(%s =%1.2f)", ofs, def->name, GVAL(ofs));

	int i = strlen(line);
	for ( ; i<16 ; i++)
		strcat(line," ");
	strcat(line," ");

	return line;
}


#if 0
void PrintStrings(void)
{
	int		i, l, j;

	for (i=0 ; i<strofs ; i += l)
	{
		l = strlen(strings+i) + 1;
		printf("%5i : ",i);

		for (j=0 ; j<l ; j++)
		{
			if (strings[i+j] == '\n')
			{
				putchar('\\');
				putchar('n');
			}
			else
				putchar(strings[i+j]);
		}
		printf("\n");
	}
}


void PrintFunction(char *name)
{
	int		i;
	statement_t	*ds;
	function_t		*df;

	for (i=0 ; i<numfunctions ; i++)
		if (!strcmp(name, strings + functions[i].s_name))
			break;

	if (i==numfunctions)
		RunError("No function names \"%s\"", name);

	df = functions + i;

	printf("Statements for %s:\n", name);
	ds = statements + df->first_statement;

	for (;;)
	{
		PR_PrintStatement(ds);
		if (!ds->op)
			break;
		ds++;
	}
}

void PrintFunctions(void)
{
	int		i,j;
	function_t	*d;

	for (i=0 ; i<numfunctions ; i++)
	{
		d = &functions[i];
		printf("%s : %s : %i %i (", strings + d->s_file, strings + d->s_name, d->first_statement, d->parm_ofs[0]);
		for (j=0 ; j<d->parm_num ; j++)
			printf("%i ",d->parm_size[j]);
		printf(")\n");
	}
}
#endif


double * real_vm_c::AccessParam(int p)
{
	assert(pr_xfunction);

	if (p >= pr_xfunction->parm_num)
		RunError("PR_Parameter: p=%d out of range\n", p);
	
	return &localstack[stack_base + pr_xfunction->parm_ofs[p]];
}


const char * real_vm_c::AccessParamString(int p)
{
	double *d = AccessParam(p);

	return REF_STRING((int) *d);
}



/*
============
RunError

Aborts the currently executing function
============
*/
void real_vm_c::RunError(const char *error, ...)
{
    va_list argptr;
    char buffer[MAX_PRINTMSG];

    va_start(argptr, error);
    vsnprintf(buffer, sizeof(buffer), error, argptr);
    va_end(argptr);

	if (pr_depth > 0)
		StackTrace();

//??	Con_Printf("Last Statement:\n");
//??    PR_PrintStatement(statements + pr_xstatement);

    Con_Printf("%s\n", buffer);

    /* clear the stack so SV/Host_Error can shutdown functions */
    pr_depth = 0;

//  raise(11);
    throw exec_error_x();
}


/*
============================================================================
PR_ExecuteProgram

The interpretation main loop
============================================================================
*/

int real_vm_c::EnterFunction(function_t *f, int result)
{
	// Returns the new program statement counter

    pr_stack[pr_depth].s = pr_xstatement;
    pr_stack[pr_depth].f = pr_xfunction;
    pr_stack[pr_depth].result = result;

    pr_depth++;
	if (pr_depth >= MAX_STACK_DEPTH)
		RunError("stack overflow");

	if (pr_xfunction)
		stack_base += pr_xfunction->locals_end;

	if (stack_base + f->locals_end >= LOCALSTACK_SIZE)
		RunError("PR_ExecuteProgram: locals stack overflow\n");

	pr_xfunction = f;

	return f->first_statement;
}


int real_vm_c::LeaveFunction(int *result)
{
	if (pr_depth <= 0)
		RunError("stack underflow");

	// up stack
	pr_depth--;

	pr_xfunction = pr_stack[pr_depth].f;
	*result      = pr_stack[pr_depth].result;

	if (pr_xfunction)
		stack_base -= pr_xfunction->locals_end;

	return pr_stack[pr_depth].s + 1;  // skip the calling op
}


void real_vm_c::EnterNative(function_t *newf, int result)
{
	int n = -(newf->first_statement + 1);

	assert(n < (int)native_funcs.size());

	function_t *old_prx = pr_xfunction;

	stack_base += pr_xfunction->locals_end;
	pr_xfunction = newf;

	native_funcs[n]->func (this);

	pr_xfunction = old_prx;
	stack_base -= pr_xfunction->locals_end;
}


void real_vm_c::DoExecute(int fnum)
{
    function_t *newf;

	function_t *f = &functions[fnum];

	int runaway = MAX_RUNAWAY;

	// make a stack frame
	int exitdepth = pr_depth;

	int s = EnterFunction(f);

	for (;;)
	{
		statement_t *st = &statements[s];

		pr_xstatement = s;

		if (trace)
			PrintStatement(f, s);

		double * a = (st->a >= 0) ? &pr_globals[st->a] : &localstack[stack_base - (st->a + 1)];
		double * b = (st->b >= 0) ? &pr_globals[st->b] : &localstack[stack_base - (st->b + 1)];
		double * c = (st->c >= 0) ? &pr_globals[st->c] : &localstack[stack_base - (st->c + 1)];

		if (!--runaway)
			RunError("runaway loop error");

		s++;  // next statement

		switch (st->op)
		{
			case OP_ADD_F:
				*c = *a + *b;
				break;

			case OP_ADD_V:
				c[0] = a[0] + b[0];
				c[1] = a[1] + b[1];
				c[2] = a[2] + b[2];
				break;

			case OP_SUB_F:
				*c = *a - *b;
				break;
			case OP_SUB_V:
				c[0] = a[0] - b[0];
				c[1] = a[1] - b[1];
				c[2] = a[2] - b[2];
				break;

			case OP_MUL_F:
				*c = *a * *b;
				break;
			case OP_MUL_V:
				*c = a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
				break;
			case OP_MUL_FV:
				c[0] = a[0] * b[0];
				c[1] = a[0] * b[1];
				c[2] = a[0] * b[2];
				break;
			case OP_MUL_VF:
				c[0] = b[0] * a[0];
				c[1] = b[0] * a[1];
				c[2] = b[0] * a[2];
				break;

			case OP_DIV_F:
				if (*b == 0)
					RunError("Division by zero");
				*c = *a / *b;
				break;

			case OP_DIV_V:
				if (*b == 0)
					RunError("Division by zero");
				c[0] = a[0] / *b;
				c[1] = a[1] / *b;
				c[2] = a[2] / *b;
				break;

			case OP_MOD_F:
				if (*b == 0)
					RunError("Division by zero");
				{
					float d = floorf(*a / *b);
					*c = *a - d * (*b);
				}
				break;

			case OP_POWER_F:
				*c = powf(*a, *b);
				break;

			case OP_BITAND:
				*c = (int)*a & (int)*b;
				break;

			case OP_BITOR:
				*c = (int)*a | (int)*b;
				break;


			case OP_GE:
				*c = *a >= *b;
				break;
			case OP_LE:
				*c = *a <= *b;
				break;
			case OP_GT:
				*c = *a > *b;
				break;
			case OP_LT:
				*c = *a < *b;
				break;
			case OP_AND:
				*c = *a && *b;
				break;
			case OP_OR:
				*c = *a || *b;
				break;

			case OP_NOT_F:
			case OP_NOT_FNC:
				*c = !*a;
				break;
			case OP_NOT_V:
				*c = !a[0] && !a[1] && !a[2];
				break;
			case OP_NOT_S:
				*c = !*a;
				break;

			case OP_EQ_F:
			case OP_EQ_FNC:
				*c = *a == *b;
				break;
			case OP_EQ_V:
				*c = (a[0] == b[0]) && (a[1] == b[1]) && (a[2] == b[2]);
				break;
			case OP_EQ_S:
				*c = (*a == *b) ? 1 :
					!strcmp(REF_STRING((int)*a), REF_STRING((int)*b));
				break;

			case OP_NE_F:
			case OP_NE_FNC:
				*c = *a != *b;
				break;
			case OP_NE_V:
				*c = (a[0] != b[0]) || (a[1] != b[1]) || (a[2] != b[2]);
				break;
			case OP_NE_S:
				*c = (*a == *b) ? 0 :
					!! strcmp(REF_STRING((int)*a), REF_STRING((int)*b));
				break;

				//==================
			case OP_MOVE_F:
			case OP_MOVE_S:
			case OP_MOVE_FNC:	// pointers
				*b = *a;
				break;
			case OP_MOVE_V:
				b[0] = a[0];
				b[1] = a[1];
				b[2] = a[2];
				break;

				//==================

			case OP_IFNOT:
				if (!*a)
					s = st->b;
				break;

			case OP_IF:
				if (*a)
					s = st->b;
				break;

			case OP_GOTO:
				s = st->b;
				break;

			case OP_CALL:
			{
				int fnum = (int)*a;
				if (!fnum)
					RunError("NULL function");
				newf = &functions[fnum];

				pr_argc = st->b;

				/* negative statements are built in functions */
				if (newf->first_statement < 0)
				{
					EnterNative(newf, st->c);
					break;
				}

				s = EnterFunction(newf, st->c);
			}
			break;

			case OP_DONE:
			{
				int result;
				s = LeaveFunction(&result);

				if (pr_depth == exitdepth)
					return;		// all done

				if (result)
				{
					 c = (result > 0) ? &pr_globals[result] : &localstack[stack_base - (result + 1)];
					*c = pr_globals[OFS_RETURN];
				}
			}
			break;

			case OP_DONE_V:
			{
				int result;
				s = LeaveFunction(&result);

				if (pr_depth == exitdepth)
					return;		// all done

				assert(result);
				{
					c = (result > 0) ? &pr_globals[result] : &localstack[stack_base - (result + 1)];

					c[0] = pr_globals[OFS_RETURN+0];
					c[1] = pr_globals[OFS_RETURN+1];
					c[2] = pr_globals[OFS_RETURN+2];
				}
			}
			break;

			case OP_PARM_F:
// printf("OP_PARM_F: %d=%1.2f --> %d+%d+%d\n", st->a, *a, stack_base, pr_xfunction->locals_end, st->b);
				b = &localstack[stack_base + pr_xfunction->locals_end + st->b];

				*b = *a;
				break;

			case OP_PARM_V:
				b = &localstack[stack_base + pr_xfunction->locals_end + st->b];

				b[0] = a[0];
				b[1] = a[1];
				b[2] = a[2];
				break;

			default:
				RunError("Bad opcode %i", st->op);
		}
	}
}

int real_vm_c::Execute(int func_id)
{
	try
	{
		if (func_id < 0 || func_id >= numfunctions)
		{
			RunError("PR_ExecuteProgram: NULL function");
		}

		DoExecute(func_id);
		return 0;
	}
	catch (exec_error_x err)
	{
		return 9;
	}
}


//=================================================================
//  DEBUGGING STUFF
//=================================================================

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
};


static const char *OpcodeName(short op)
{
	if (op < 0 || op > OP_PARM_V)
		return "???";
	
	return opcode_names[op];
}


void real_vm_c::StackTrace()
{
	Con_Printf("Stack Trace:\n");

	pr_stack[pr_depth].f = pr_xfunction;
	pr_stack[pr_depth].s = pr_xstatement;

	for (int i = pr_depth; i >= 1; i--)
	{
		int back = (pr_depth - i) + 1;

		function_t * f = pr_stack[i].f;
		statement_t *st = &statements[pr_stack[i].s];

		if (f)
			Con_Printf("%-2d %s() at %s:%d\n", back, f->name, f->source_file, f->source_line + st->line);
		else
			Con_Printf("%-2d ????\n", back);
	}

	Con_Printf("\n");
}


const char * real_vm_c::RegString(statement_t *st, int who)
{
	static char buffer[100];

	int val = (who == 1) ? st->a : (who == 2) ? st->b : st->c;

	sprintf(buffer, "%s[%d]", (val < 0) ? "stack" : "glob", abs(val));

	return buffer;
}

void real_vm_c::PrintStatement(function_t *f, int s)
{
	statement_t *st = &statements[s];

	const char *op_name = OpcodeName(st->op);

	Con_Printf("%06x: %-9s ", s, op_name);

	switch (st->op)
	{
		case OP_DONE:
		case OP_DONE_V:
			break;
	
		case OP_MOVE_F:
		case OP_MOVE_S:
		case OP_MOVE_FNC:	// pointers
		case OP_MOVE_V:
			Con_Printf("%s ",    RegString(st, 1));
			Con_Printf("-> %s",  RegString(st, 2));
			break;

		case OP_IFNOT:
		case OP_IF:
			Con_Printf("%s, %08x", RegString(st, 1), st->b);
			break;

		case OP_GOTO:
			Con_Printf("%08x", st->b);
			// TODO
			break;

		case OP_CALL:
			Con_Printf("%s, %d ", RegString(st, 1), st->b);

			if (! st->c)
				Con_Printf(" ");
			else
				Con_Printf("-> %s",   RegString(st, 3));
			break;

		case OP_PARM_F:
		case OP_PARM_V:
			Con_Printf("%s -> future[%d]", RegString(st, 1), st->b);
			break;

		case OP_NOT_F:
		case OP_NOT_FNC:
		case OP_NOT_V:
		case OP_NOT_S:
			Con_Printf("%s ",    RegString(st, 1));
			Con_Printf("-> %s",  RegString(st, 3));
			break;

		default:
			Con_Printf("%s, ",   RegString(st, 1));
			Con_Printf("%s ",    RegString(st, 2));
			Con_Printf("-> %s",  RegString(st, 3));
			break;
	}

	Con_Printf("\n");

#if 0
	int i;

	const char *opname = opcode_names[s->op];
	printf("%4i : %4i : %s ", (int)(s - statements),
	        statement_linenums[s-statements], opname);
	i = strlen(opname);
	for ( ; i<10 ; i++)
		printf(" ");

	if (s->op == OP_IF || s->op == OP_IFNOT)
		printf("%sbranch %i",PR_GlobalString(s->a),s->b);
	else if (s->op == OP_GOTO)
	{
		printf("branch %i",s->a);
	}
	else if ( (unsigned)(s->op - OP_MOVE_F) < 4)
	{
		printf("%s",PR_GlobalString(s->a));
		printf("%s", PR_GlobalStringNoContents(s->b));
	}
	else if (s->op == OP_CALL)
	{
		function_t *f = &functions[(int)G_FUNCTION(s->a)];

		printf("a:%d(%s) ", s->a, REF_STRING(f->s_name));

		if (s->b)
			printf("b:%s",PR_GlobalString(s->b));
		if (s->c)
			printf("c:%s", PR_GlobalStringNoContents(s->c));
	}
	else
	{
		if (s->a)
			printf("a:%s",PR_GlobalString(s->a));
		if (s->b)
			printf("b:%s",PR_GlobalString(s->b));
		if (s->c)
			printf("c:%s", PR_GlobalStringNoContents(s->c));
	}
	printf("\n");
#endif
}



}  // namespace coal

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
