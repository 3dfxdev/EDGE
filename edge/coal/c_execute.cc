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
#include "c_execute.h"


// FIXME
#define Con_Printf   printf
#define Con_DPrintf  printf

#define MAX_PRINTMSG  1024


double		pr_globals[MAX_REGS];
int			numpr_globals;


execution_c * EXE;


#define MAX_RUNAWAY  (1000*1000)


typedef struct
{
	const char *name;
	native_func_t func;
}
reg_native_func_t;


static std::vector<reg_native_func_t *> native_funcs;


execution_c::execution_c() :
	s(0), func(0), tracing(false), stack_depth(0), call_depth(0)
{ }

execution_c::~execution_c()
{ }


void real_vm_c::default_printer(const char *msg, ...)
{
	// does nothing
}

void real_vm_c::default_aborter(const char *msg, ...)
{
	exit(66);
}


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
	// FIXME: EXE->tracing = enable;
}


int real_vm_c::FindFunction(const char *func_name)
{
	for (int i = 1; i < (int)functions.size(); i++)
	{
		function_t *f = functions[i];

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


double * real_vm_c::AccessParam(int p)
{
	assert(EXE->func);

	if (p >= functions[EXE->func]->parm_num)
		RunError("PR_Parameter: p=%d out of range\n", p);

	return &EXE->stack[EXE->stack_depth + functions[EXE->func]->parm_ofs[p]];
}


const char * real_vm_c::AccessParamString(int p)
{
	double *d = AccessParam(p);

	return REF_STRING((int) *d);
}


//
// Aborts the currently executing functions
//
void real_vm_c::RunError(const char *error, ...)
{
    va_list argptr;
    char buffer[MAX_PRINTMSG];

    va_start(argptr, error);
    vsnprintf(buffer, sizeof(buffer), error, argptr);
    va_end(argptr);

	if (EXE->call_depth > 0)
		StackTrace();

//??	Con_Printf("Last Statement:\n");
//??    PR_PrintStatement(statements + pr_xstatement);

    Con_Printf("%s\n", buffer);

    /* clear the stack so SV/Host_Error can shutdown functions */
    EXE->call_depth = 0;

//  raise(11);
    throw exec_error_x();
}


//================================================================
//  EXECUTION ENGINE
//================================================================

void real_vm_c::EnterFunction(int func, int result)
{
	assert(func > 0);

	function_t *new_f = functions[func];

	// NOTE: the saved 's' value points to the instruction _after_ OP_CALL

    EXE->call_stack[EXE->call_depth].s      = EXE->s;
    EXE->call_stack[EXE->call_depth].func   = EXE->func;
    EXE->call_stack[EXE->call_depth].result = result;

    EXE->call_depth++;
	if (EXE->call_depth >= MAX_CALL_STACK)
		RunError("stack overflow");

	if (EXE->func)
		EXE->stack_depth += functions[EXE->func]->locals_end;

	if (EXE->stack_depth + new_f->locals_end >= MAX_LOCAL_STACK)
		RunError("PR_ExecuteProgram: locals stack overflow\n");

	EXE->s    = new_f->first_statement;
	EXE->func = func;
}


void real_vm_c::LeaveFunction(int *result)
{
	if (EXE->call_depth <= 0)
		RunError("stack underflow");

	EXE->call_depth--;

	EXE->s    = EXE->call_stack[EXE->call_depth].s;
	EXE->func = EXE->call_stack[EXE->call_depth].func;
	*result   = EXE->call_stack[EXE->call_depth].result;

	if (EXE->func)
		EXE->stack_depth -= functions[EXE->func]->locals_end;

///---	// skip the OP_CALL instruction
///---	EXE->s += sizeof(statement_t);
}


void real_vm_c::EnterNative(int func, int result)
{
	function_t *newf = functions[func];

	int n = -(newf->first_statement + 1);
	assert(n < (int)native_funcs.size());

	EXE->stack_depth += functions[EXE->func]->locals_end;
	{
		int old_func = EXE->func;
		{
			EXE->func = func;
			native_funcs[n]->func (this);
		}
		EXE->func = old_func;
	}
	EXE->stack_depth -= functions[EXE->func]->locals_end;
}


void real_vm_c::DoExecute(int fnum)
{
    function_t *newf;

	function_t *f = functions[fnum];

	int runaway = MAX_RUNAWAY;

	// make a stack frame
	int exitdepth = EXE->call_depth;

	EnterFunction(fnum);

	for (;;)
	{
		statement_t *st = REF_OP(EXE->s);

		if (EXE->tracing)
			PrintStatement(f, EXE->s);

		double * a = (st->a >= 0) ? &pr_globals[st->a] : &EXE->stack[EXE->stack_depth - (st->a + 1)];
		double * b = (st->b >= 0) ? &pr_globals[st->b] : &EXE->stack[EXE->stack_depth - (st->b + 1)];
		double * c = (st->c >= 0) ? &pr_globals[st->c] : &EXE->stack[EXE->stack_depth - (st->c + 1)];

		if (!--runaway)
			RunError("runaway loop error");

		// move code pointer to next statement
		EXE->s += sizeof(statement_t);

		switch (st->op)
		{
			case OP_NULL:
				// no operation
				break;

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
					EXE->s = st->b;
				break;

			case OP_IF:
				if (*a)
					EXE->s = st->b;
				break;

			case OP_GOTO:
				EXE->s = st->b;
				break;

			case OP_CALL:
			{
				int fnum = (int)*a;
				if (!fnum)
					RunError("NULL function");
				newf = functions[fnum];

				//??  pr_argc = st->b;

				/* negative statements are built in functions */
				if (newf->first_statement < 0)
				{
					EnterNative(fnum, st->c);
					break;
				}

				EnterFunction(fnum, st->c);
			}
			break;

			case OP_DONE:
			{
				int result;
				LeaveFunction(&result);

				if (EXE->call_depth == exitdepth)
					return;		// all done

				if (result)
				{
					 c = (result > 0) ? &pr_globals[result] : &EXE->stack[EXE->stack_depth - (result + 1)];
					*c = pr_globals[OFS_RETURN];
				}
			}
			break;

			case OP_DONE_V:
			{
				int result;
				LeaveFunction(&result);

				if (EXE->call_depth == exitdepth)
					return;		// all done

				assert(result);
				{
					c = (result > 0) ? &pr_globals[result] : &EXE->stack[EXE->stack_depth - (result + 1)];

					c[0] = pr_globals[OFS_RETURN+0];
					c[1] = pr_globals[OFS_RETURN+1];
					c[2] = pr_globals[OFS_RETURN+2];
				}
			}
			break;

			case OP_PARM_F:
				b = &EXE->stack[EXE->stack_depth + functions[EXE->func]->locals_end + st->b];

				*b = *a;
				break;

			case OP_PARM_V:
				b = &EXE->stack[EXE->stack_depth + functions[EXE->func]->locals_end + st->b];

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
if (! EXE) EXE = new execution_c();

	try
	{
		if (func_id < 1 || func_id >= (int)functions.size())
		{
			RunError("vm_c::Execute: NULL function");
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
	"NULL",
	"RET", "RET_V",
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


#define GVAL(o)  ((o < 0) ? o : pr_globals[o])

//
// Returns a string suitable for printing (no newlines, max 60 chars length)
// TODO: reimplement this
#if 0
char *DebugString(char *string)
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
#endif

//
// Returns a string describing *data in a type specific manner
// TODO: reimplement this
#if 0
char *Debug_ValueString(etype_t type, double *val)
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
#endif


void real_vm_c::StackTrace()
{
	Con_Printf("Stack Trace:\n");

	EXE->call_stack[EXE->call_depth].func = EXE->func;
	EXE->call_stack[EXE->call_depth].s    = EXE->s;

	for (int i = EXE->call_depth; i >= 1; i--)
	{
		int back = (EXE->call_depth - i) + 1;

		function_t * f = functions[EXE->call_stack[i].func];

		statement_t *st = REF_OP(EXE->call_stack[i].s);

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

	if (val == 1)
		return "result";

	sprintf(buffer, "%s[%d]", (val < 0) ? "stack" : "glob", abs(val));
	return buffer;
}

void real_vm_c::PrintStatement(function_t *f, int s)
{
	statement_t *st = REF_OP(s);

	const char *op_name = OpcodeName(st->op);

	Con_Printf("  %06x: %-9s ", s, op_name);

	switch (st->op)
	{
		case OP_NULL:
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
			Con_Printf("%s %08x", RegString(st, 1), st->b);
			break;

		case OP_GOTO:
			Con_Printf("%08x", st->b);
			// TODO
			break;

		case OP_CALL:
			Con_Printf("%s (%d) ", RegString(st, 1), st->b);

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
			Con_Printf("%s + ",  RegString(st, 1));
			Con_Printf("%s ",    RegString(st, 2));
			Con_Printf("-> %s",  RegString(st, 3));
			break;
	}

	Con_Printf("\n");
}


void real_vm_c::ASM_DumpFunction(function_t *f)
{
	Con_Printf("Function %s()\n", f->name);

	if (f->first_statement < 0)
	{
		Con_Printf("  native #%d\n\n", - f->first_statement);
		return;
	}

	for (int s = f->first_statement; s <= f->last_statement; s += sizeof(statement_t))
	{
		PrintStatement(f, s);
	}

	Con_Printf("\n");
}

void real_vm_c::ASM_DumpAll()
{
	for (int i = 1; i < (int)functions.size(); i++)
	{
		function_t *f = functions[i];

		ASM_DumpFunction(f);
	}
}


}  // namespace coal

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
