/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

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


// FIXME
#define Con_Printf   printf
#define Con_DPrintf  printf

#define MAX_PRINTMSG  1024

extern void Error (char *error, ...);  // FIXME


double		pr_globals[MAX_REGS];
int			numpr_globals;

char		strings[MAX_STRINGS];
int			strofs;

statement_t	statements[MAX_STATEMENTS];
int			numstatements;
int			statement_linenums[MAX_STATEMENTS];

function_t	functions[MAX_FUNCTIONS];
int			numfunctions;


typedef struct
{
    int s;
    function_t *f;
	int result;
}
prstack_t;

#define	MAX_STACK_DEPTH		128
prstack_t pr_stack[MAX_STACK_DEPTH];
int pr_depth;

#define	LOCALSTACK_SIZE		2048
double localstack[LOCALSTACK_SIZE];
int stack_base;

bool pr_trace;
function_t *pr_xfunction;
int pr_xstatement;
int pr_argc;



/*
============
PR_StackTrace
============
*/
void PR_StackTrace(void)
{
    function_t *f;
    int i;

	if (pr_depth == 0)
	{
		Con_Printf("<NO STACK>\n");
		return;
	}

	pr_stack[pr_depth].f = pr_xfunction;

	for (i = pr_depth; i >= 0; i--)
	{
		f = pr_stack[i].f;
		if (!f)
			Con_Printf("<NO FUNCTION>\n");
		else
			Con_Printf("%12s : %s\n", PR_GetString(f->s_file),
					PR_GetString(f->s_name));
	}
}


/*
============
PR_RunError

Aborts the currently executing function
============
*/
void PR_RunError(const char *error, ...)
{
    va_list argptr;
    char string[MAX_PRINTMSG];

    va_start(argptr, error);
    vsnprintf(string, sizeof(string), error, argptr);
    va_end(argptr);

    PR_PrintStatement(statements + pr_xstatement);
    PR_StackTrace();
    Con_Printf("%s\n", string);

    /* dump the stack so SV/Host_Error can shutdown functions */
    pr_depth = 0;

    Error("Program error");
}


/*
============================================================================
PR_ExecuteProgram

The interpretation main loop
============================================================================
*/

int PR_EnterFunction(function_t *f, int result = 0)
{
	// Returns the new program statement counter

    pr_stack[pr_depth].s = pr_xstatement;
    pr_stack[pr_depth].f = pr_xfunction;
    pr_stack[pr_depth].result = result;

    pr_depth++;
	if (pr_depth >= MAX_STACK_DEPTH)
		PR_RunError("stack overflow");

	if (pr_xfunction)
		stack_base += pr_xfunction->locals_end;

	if (stack_base + f->locals_end >= LOCALSTACK_SIZE)
		PR_RunError("PR_ExecuteProgram: locals stack overflow\n");

	pr_xfunction = f;

	return f->first_statement;
}


int PR_LeaveFunction(int *result)
{
	if (pr_depth <= 0)
		Error("prog stack underflow");

	// up stack
	pr_depth--;

	pr_xfunction = pr_stack[pr_depth].f;
	*result      = pr_stack[pr_depth].result;

	if (pr_xfunction)
		stack_base -= pr_xfunction->locals_end;

	return pr_stack[pr_depth].s + 1;  // skip the calling op
}


void PR_EnterBuiltin(function_t *newf, int result)
{
	int i = -newf->first_statement;

	if (i >= pr_numbuiltins)
		PR_RunError("Bad builtin call number");

	function_t *old_prx = pr_xfunction;

	stack_base += pr_xfunction->locals_end;
	pr_xfunction = newf;

	pr_builtins[i] ();

	pr_xfunction = old_prx;
	stack_base -= pr_xfunction->locals_end;
}

/*
====================
PR_ExecuteProgram
====================
*/
void PR_ExecuteProgram(func_t fnum)
{
    function_t *newf;

	if (!fnum || fnum >= numfunctions)
	{
		Error("PR_ExecuteProgram: NULL function");
	}

	function_t *f = &functions[fnum];

	int runaway = 100000;

	// make a stack frame
	int exitdepth = pr_depth;

	int s = PR_EnterFunction(f);

	while (1)
	{
		statement_t *st = &statements[s];

		double * a = (st->a >= 0) ? &pr_globals[st->a] : &localstack[stack_base - (st->a + 1)];
		double * b = (st->b >= 0) ? &pr_globals[st->b] : &localstack[stack_base - (st->b + 1)];
		double * c = (st->c >= 0) ? &pr_globals[st->c] : &localstack[stack_base - (st->c + 1)];

		if (!--runaway)
			PR_RunError("runaway loop error");

		// pr_xfunction->profile++;
		pr_xstatement = s;

		if (pr_trace)
			PR_PrintStatement(st);

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
					Error("Division by zero");
				*c = *a / *b;
				break;

			case OP_DIV_V:
				if (*b == 0)
					Error("Division by zero");
				c[0] = a[0] / *b;
				c[1] = a[1] / *b;
				c[2] = a[2] / *b;
				break;

			case OP_MOD_F:
				if (*b == 0)
					Error("Division by zero");
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
				*c = !*a || !*PR_GetString((int)*a);
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
					!strcmp(PR_GetString((int)*a), PR_GetString((int)*b));
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
					!! strcmp(PR_GetString((int)*a), PR_GetString((int)*b));
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
					PR_RunError("NULL function");
				newf = &functions[fnum];

				pr_argc = st->b;

				/* negative statements are built in functions */
				if (newf->first_statement < 0)
				{
					PR_EnterBuiltin(newf, st->c);
					break;
				}

				s = PR_EnterFunction(newf, st->c);
			}
			break;

			case OP_DONE:
			{
				int result;
				s = PR_LeaveFunction(&result);

				if (pr_depth == exitdepth)
					return;		// all done

				if (result)
				{
					double * c = (result >= 0) ? &pr_globals[result] : &localstack[stack_base - (result + 1)];
					*c = pr_globals[OFS_RETURN];
				}
			}
			break;

			case OP_DONE_V:
			{
				int result;
				s = PR_LeaveFunction(&result);

				if (pr_depth == exitdepth)
					return;		// all done

				assert(result);
				{
					double * c = (result >= 0) ? &pr_globals[result] : &localstack[stack_base - (result + 1)];

					c[0] = pr_globals[OFS_RETURN+0];
					c[1] = pr_globals[OFS_RETURN+1];
					c[2] = pr_globals[OFS_RETURN+2];
				}
			}
			break;

			case OP_PARM_F:
// printf("OP_PARM_F: %d=%1.2f --> %d+%d+%d\n", st->a, *a, stack_base, pr_xfunction->locals_end, st->b);
				c = &localstack[stack_base + pr_xfunction->locals_end + st->b];

				*c = *a;
				break;

			case OP_PARM_V:
				c = &localstack[stack_base + pr_xfunction->locals_end + st->b];

				c[0] = a[0];
				c[1] = a[1];
				c[2] = a[2];
				break;

			default:
				PR_RunError("Bad opcode %i", st->op);
		}
	}
}


char * PR_GetString(int num)
{
	if (num >= 0)
		return strings + num;
	else
		Error("invalid string offset %d\n", num);

	return "";
}



// CopyString returns an offset from the string heap
int	CopyString (char *str)
{
	int old;

	old = strofs;
	strcpy (strings+strofs, str);
	strofs += strlen(str)+1;

	return old;
}



/*
===============
PR_String

Returns a string suitable for printing (no newlines, max 60 chars length)
===============
*/
char *PR_String (char *string)
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
char *PR_ValueString (etype_t type, double *val)
{
	static char	line[256];
	def_t		*def;
	function_t	*f;

	switch (type)
	{
	case ev_string:
		sprintf (line, "%s", PR_String(strings + (int)*val));
		break;
//	case ev_entity:
//		sprintf (line, "entity %i", *(int *)val);
//		break;
	case ev_function:
		f = functions + (int)*val;
		if (!f)
			sprintf (line, "undefined function");
		else
			sprintf (line, "%s()", strings + f->s_name);
		break;
//	case ev_field:
//		def = PR_DefForFieldOfs ( *(int *)val );
//		sprintf (line, ".%s", def->name);
//		break;
	case ev_void:
		sprintf (line, "void");
		break;
	case ev_float:
		sprintf (line, "%5.1f", (float) *val);
		break;
	case ev_vector:
		sprintf (line, "'%5.1f %5.1f %5.1f'", val[0], val[1], val[2]);
		break;
	case ev_pointer:
		sprintf (line, "pointer");
		break;
	default:
		sprintf (line, "bad type %i", type);
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

char *PR_GlobalStringNoContents (gofs_t ofs)
{
	static char	line[128];

	def_t * def = pr_global_defs[ofs];
	if (!def)
//		Error ("PR_GlobalString: no def for %i", ofs);
		sprintf (line,"%i(?? =%1.2f)", ofs, GVAL(ofs));
	else
		sprintf (line,"%i(%s =%1.2f)", ofs, def->name, GVAL(ofs));

	int i = strlen(line);
	for ( ; i<16 ; i++)
		strcat (line," ");
	strcat (line," ");

	return line;
}

char *PR_GlobalString (gofs_t ofs)
{
	static char	line[128];

	def_t *def = pr_global_defs[ofs];
	if (!def)
		return PR_GlobalStringNoContents(ofs);

	if (def->initialized && def->type->type != ev_function)
	{
		char *s = PR_ValueString (def->type->type, &pr_globals[ofs]);
		sprintf (line,"%i(%s =%1.2f)", ofs, s, GVAL(ofs));
	}
	else
		sprintf (line,"%i(%s =%1.2f)", ofs, def->name, GVAL(ofs));

	int i = strlen(line);
	for ( ; i<16 ; i++)
		strcat (line," ");
	strcat (line," ");

	return line;
}


void PR_PrintOfs (gofs_t ofs)
{
	printf ("%s\n",PR_GlobalString(ofs));
}


void PR_PrintStatement (statement_t *s)
{
	int i;

	const char *opname = opcode_names[s->op];
	printf ("%4i : %4i : %s ", (int)(s - statements),
	        statement_linenums[s-statements], opname);
	i = strlen(opname);
	for ( ; i<10 ; i++)
		printf (" ");

	if (s->op == OP_IF || s->op == OP_IFNOT)
		printf ("%sbranch %i",PR_GlobalString(s->a),s->b);
	else if (s->op == OP_GOTO)
	{
		printf ("branch %i",s->a);
	}
	else if ( (unsigned)(s->op - OP_MOVE_F) < 4)
	{
		printf ("%s",PR_GlobalString(s->a));
		printf ("%s", PR_GlobalStringNoContents(s->b));
	}
	else if (s->op == OP_CALL)
	{
		function_t *f = &functions[(int)G_FUNCTION(s->a)];

		printf("a:%d(%s) ", s->a, strings + f->s_name);

		if (s->b)
			printf ("b:%s",PR_GlobalString(s->b));
		if (s->c)
			printf ("c:%s", PR_GlobalStringNoContents(s->c));
	}
	else
	{
		if (s->a)
			printf ("a:%s",PR_GlobalString(s->a));
		if (s->b)
			printf ("b:%s",PR_GlobalString(s->b));
		if (s->c)
			printf ("c:%s", PR_GlobalStringNoContents(s->c));
	}
	printf ("\n");
}


void PR_PrintDefs (void)
{
	def_t *d;

	for (d=all_defs ; d ; d=d->next)
		PR_PrintOfs (d->ofs);
}


void PrintStrings (void)
{
	int		i, l, j;

	for (i=0 ; i<strofs ; i += l)
	{
		l = strlen(strings+i) + 1;
		printf ("%5i : ",i);
		for (j=0 ; j<l ; j++)
		{
			if (strings[i+j] == '\n')
			{
				putchar ('\\');
				putchar ('n');
			}
			else
				putchar (strings[i+j]);
		}
		printf ("\n");
	}
}


void PrintFunction (char *name)
{
	int		i;
	statement_t	*ds;
	function_t		*df;

	for (i=0 ; i<numfunctions ; i++)
		if (!strcmp (name, strings + functions[i].s_name))
			break;

	if (i==numfunctions)
		Error ("No function names \"%s\"", name);

	df = functions + i;

	printf ("Statements for %s:\n", name);
	ds = statements + df->first_statement;

	while (1)
	{
		PR_PrintStatement (ds);
		if (!ds->op)
			break;
		ds++;
	}
}

void PrintFunctions (void)
{
	int		i,j;
	function_t	*d;

	for (i=0 ; i<numfunctions ; i++)
	{
		d = &functions[i];
		printf ("%s : %s : %i %i (", strings + d->s_file, strings + d->s_name, d->first_statement, d->parm_ofs[0]);
		for (j=0 ; j<d->parm_num ; j++)
			printf ("%i ",d->parm_size[j]);
		printf (")\n");
	}
}


double * PR_Parameter(int p)
{
	assert(pr_xfunction);

	if (p >= pr_xfunction->parm_num)
		PR_RunError("PR_Parameter: p=%d out of range\n", p);
	
	return &localstack[stack_base + pr_xfunction->parm_ofs[p]];
}


#include "r_cmds.c"  // FIXME


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
