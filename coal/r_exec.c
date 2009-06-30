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

typedef struct
{
    int s;
    function_t *f;
}
prstack_t;

#define	MAX_STACK_DEPTH		128
prstack_t pr_stack[MAX_STACK_DEPTH];
int pr_depth;

#define	LOCALSTACK_SIZE		2048
double localstack[LOCALSTACK_SIZE];
int localstack_used;

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

int PR_EnterFunction(function_t *f)
{
	// Returns the new program statement counter

    pr_stack[pr_depth].s = pr_xstatement;
    pr_stack[pr_depth].f = pr_xfunction;

    pr_depth++;
	if (pr_depth >= MAX_STACK_DEPTH)
		PR_RunError("stack overflow");

	// save off any locals that the new function steps on
	int c = f->locals;
	if (localstack_used + c > LOCALSTACK_SIZE)
		PR_RunError("PR_ExecuteProgram: locals stack overflow\n");

	for (int i = 0; i < c; i++)
		localstack[localstack_used++] = pr_globals[f->parm_start + i];

	// copy parameters
	int o = f->parm_start;
	for (int i = 0; i < f->parm_num; i++)
		for (int j = 0; j < f->parm_size[i]; j++)
			pr_globals[o++] = pr_globals[OFS_PARM0 + i * 3 + j];

	pr_xfunction = f;
	return f->first_statement - 1;	// offset the s++
}


int PR_LeaveFunction(void)
{
	if (pr_depth <= 0)
		Error("prog stack underflow");

	// restore locals from the stack
	int c = pr_xfunction->locals;
	localstack_used -= c;
	if (localstack_used < 0)
		PR_RunError("PR_ExecuteProgram: locals stack underflow\n");

	for (int i = 0; i < c; i++)
	{
		pr_globals[pr_xfunction->parm_start + i] = localstack[localstack_used + i];
	}

	// up stack
	pr_depth--;
	pr_xfunction = pr_stack[pr_depth].f;

	return pr_stack[pr_depth].s;
}


/*
====================
PR_ExecuteProgram
====================
*/
void PR_ExecuteProgram(func_t fnum)
{
    function_t *newf;
    int i;

	if (!fnum || fnum >= numfunctions)
	{
		Error("PR_ExecuteProgram: NULL function");
	}

	function_t *f = &functions[fnum];

	pr_trace = false;

	int runaway = 100000;

	// make a stack frame
	int exitdepth = pr_depth;

	int s = PR_EnterFunction(f);

	while (1)
	{
		s++;			// next statement

		statement_t *st = &statements[s];

		double * a = &pr_globals[st->a];
		double * b = &pr_globals[st->b];
		double * c = &pr_globals[st->c];

		if (!--runaway)
			PR_RunError("runaway loop error");

		// pr_xfunction->profile++;
		pr_xstatement = s;

		if (pr_trace)
			PR_PrintStatement(st);

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
			case OP_ASSIGN_F:
			case OP_ASSIGN_S:
			case OP_ASSIGN_FNC:	// pointers
				*b = *a;
				break;
			case OP_ASSIGN_V:
				b[0] = a[0];
				b[1] = a[1];
				b[2] = a[2];
				break;

				//==================

			case OP_IFNOT:
				if (!*a)
					s += st->b - 1;	// offset the s++
				break;

			case OP_IF:
				if (*a)
					s += st->b - 1;	// offset the s++
				break;

			case OP_GOTO:
				s += st->a - 1;	// offset the s++
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
					i = -newf->first_statement;
					if (i >= pr_numbuiltins)
						PR_RunError("Bad builtin call number");
					pr_builtins[i] ();
					break;
				}

				s = PR_EnterFunction(newf);
			}
			break;

			case OP_DONE:
			case OP_RETURN:
				pr_globals[OFS_RETURN]     = pr_globals[st->a];
				pr_globals[OFS_RETURN + 1] = pr_globals[st->a + 1];
				pr_globals[OFS_RETURN + 2] = pr_globals[st->a + 2];

				s = PR_LeaveFunction();
				if (pr_depth == exitdepth)
					return;		// all done
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


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
