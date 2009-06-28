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
    dfunction_t *f;
}
prstack_t;

#define	MAX_STACK_DEPTH		32
prstack_t pr_stack[MAX_STACK_DEPTH];
int pr_depth;

#define	LOCALSTACK_SIZE		2048
int localstack[LOCALSTACK_SIZE];
int localstack_used;

bool pr_trace;
dfunction_t *pr_xfunction;
int pr_xstatement;
int pr_argc;

char *pr_opnames[] =
{
    "DONE",

    "MUL_F",
    "MUL_V",
    "MUL_FV",
    "MUL_VF",

    "DIV",
    "MOD",

    "ADD_F",
    "ADD_V",

    "SUB_F",
    "SUB_V",

    "EQ_F",
    "EQ_V",
    "EQ_S",
    "EQ_E",
    "EQ_FNC",

    "NE_F",
    "NE_V",
    "NE_S",
    "NE_E",
    "NE_FNC",

    "LE",
    "GE",
    "LT",
    "GT",

    "INDIRECT",
    "INDIRECT",
    "INDIRECT",
    "INDIRECT",
    "INDIRECT",
    "INDIRECT",

    "ADDRESS",

    "STORE_F",
    "STORE_V",
    "STORE_S",
    "STORE_ENT",
    "STORE_FLD",
    "STORE_FNC",

    "STOREP_F",
    "STOREP_V",
    "STOREP_S",
    "STOREP_ENT",
    "STOREP_FLD",
    "STOREP_FNC",

    "RETURN",

    "NOT_F",
    "NOT_V",
    "NOT_S",
    "NOT_ENT",
    "NOT_FNC",

    "IF",
    "IFNOT",

    "CALL0",
    "CALL1",
    "CALL2",
    "CALL3",
    "CALL4",
    "CALL5",
    "CALL6",
    "CALL7",
    "CALL8",

    "STATE",

    "GOTO",

    "AND",
    "OR",

    "BITAND",
    "BITOR"
};



/*
============
PR_StackTrace
============
*/
void
PR_StackTrace(void)
{
    dfunction_t *f;
    int i;

    if (pr_depth == 0) {
	Con_Printf("<NO STACK>\n");
	return;
    }

    pr_stack[pr_depth].f = pr_xfunction;
    for (i = pr_depth; i >= 0; i--) {
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
void
PR_RunError(const char *error, ...)
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

/*
====================
PR_EnterFunction

Returns the new program statement counter
====================
*/
int
PR_EnterFunction(dfunction_t *f)
{
    int i, j, c, o;

    pr_stack[pr_depth].s = pr_xstatement;
    pr_stack[pr_depth].f = pr_xfunction;
    pr_depth++;
    if (pr_depth >= MAX_STACK_DEPTH)
	PR_RunError("stack overflow");

// save off any locals that the new function steps on
    c = f->locals;
    if (localstack_used + c > LOCALSTACK_SIZE)
	PR_RunError("PR_ExecuteProgram: locals stack overflow\n");

    for (i = 0; i < c; i++)
	localstack[localstack_used + i] =
	    ((int *)pr_globals)[f->parm_start + i];
    localstack_used += c;

// copy parameters
    o = f->parm_start;
    for (i = 0; i < f->numparms; i++) {
	for (j = 0; j < f->parm_size[i]; j++) {
	    ((int *)pr_globals)[o] =
		((int *)pr_globals)[OFS_PARM0 + i * 3 + j];
	    o++;
	}
    }

    pr_xfunction = f;
    return f->first_statement - 1;	// offset the s++
}

/*
====================
PR_LeaveFunction
====================
*/
int
PR_LeaveFunction(void)
{
    int i, c;

    if (pr_depth <= 0)
	Error("prog stack underflow");

// restore locals from the stack
    c = pr_xfunction->locals;
    localstack_used -= c;
    if (localstack_used < 0)
	PR_RunError("PR_ExecuteProgram: locals stack underflow\n");

    for (i = 0; i < c; i++)
	((int *)pr_globals)[pr_xfunction->parm_start + i] =
	    localstack[localstack_used + i];

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
void
PR_ExecuteProgram(func_t fnum)
{
    eval_t *a, *b, *c;
    int s;
    dstatement_t *st;
    dfunction_t *f, *newf;
    int runaway;
    int i;
//   edict_t *ed;
    int exitdepth;
//    eval_t *ptr;

    if (!fnum || fnum >= numfunctions)
    {
//??	if (pr_global_struct->self)
//??	    ED_Print(PROG_TO_EDICT(pr_global_struct->self));
      Error("PR_ExecuteProgram: NULL function");
    }

    f = &functions[fnum];

    runaway = 100000;
    pr_trace = false;

// make a stack frame
    exitdepth = pr_depth;

    s = PR_EnterFunction(f);

    while (1) {
	s++;			// next statement

	st = &statements[s];
	a = (eval_t *)&pr_globals[st->a];
	b = (eval_t *)&pr_globals[st->b];
	c = (eval_t *)&pr_globals[st->c];

	if (!--runaway)
	    PR_RunError("runaway loop error");
	if (runaway <= 50000 && !(runaway % 5000))
	    Con_DPrintf("%s: progs execution running away (%i left)\n",
			__func__, runaway);

	// pr_xfunction->profile++;
	pr_xstatement = s;

	if (pr_trace)
	    PR_PrintStatement(st);

	switch (st->op) {
	case OP_ADD_F:
	    c->_float = a->_float + b->_float;
	    break;
	case OP_ADD_V:
	    c->vector[0] = a->vector[0] + b->vector[0];
	    c->vector[1] = a->vector[1] + b->vector[1];
	    c->vector[2] = a->vector[2] + b->vector[2];
	    break;

	case OP_SUB_F:
	    c->_float = a->_float - b->_float;
	    break;
	case OP_SUB_V:
	    c->vector[0] = a->vector[0] - b->vector[0];
	    c->vector[1] = a->vector[1] - b->vector[1];
	    c->vector[2] = a->vector[2] - b->vector[2];
	    break;

	case OP_MUL_F:
	    c->_float = a->_float * b->_float;
	    break;
	case OP_MUL_V:
	    c->_float = a->vector[0] * b->vector[0]
		+ a->vector[1] * b->vector[1]
		+ a->vector[2] * b->vector[2];
	    break;
	case OP_MUL_FV:
	    c->vector[0] = a->_float * b->vector[0];
	    c->vector[1] = a->_float * b->vector[1];
	    c->vector[2] = a->_float * b->vector[2];
	    break;
	case OP_MUL_VF:
	    c->vector[0] = b->_float * a->vector[0];
	    c->vector[1] = b->_float * a->vector[1];
	    c->vector[2] = b->_float * a->vector[2];
	    break;

	case OP_DIV_F:
      if (b->_float == 0)
        Error("Division by zero");
	    c->_float = a->_float / b->_float;
	    break;

	case OP_MOD_F:
      if (b->_float == 0)
        Error("Division by zero");
      {
        float d = floorf(a->_float / b->_float);
	      c->_float = a->_float - d * b->_float;
      }
	    break;

	case OP_POWER_F:
	    c->_float = powf(a->_float, b->_float);
	    break;

	case OP_BITAND:
	    c->_float = (int)a->_float & (int)b->_float;
	    break;

	case OP_BITOR:
	    c->_float = (int)a->_float | (int)b->_float;
	    break;


	case OP_GE:
	    c->_float = a->_float >= b->_float;
	    break;
	case OP_LE:
	    c->_float = a->_float <= b->_float;
	    break;
	case OP_GT:
	    c->_float = a->_float > b->_float;
	    break;
	case OP_LT:
	    c->_float = a->_float < b->_float;
	    break;
	case OP_AND:
	    c->_float = a->_float && b->_float;
	    break;
	case OP_OR:
	    c->_float = a->_float || b->_float;
	    break;

	case OP_NOT_F:
	    c->_float = !a->_float;
	    break;
	case OP_NOT_V:
	    c->_float = !a->vector[0] && !a->vector[1] && !a->vector[2];
	    break;
	case OP_NOT_S:
	    c->_float = !a->string || !*PR_GetString(a->string);
	    break;
	case OP_NOT_FNC:
	    c->_float = !a->function;
	    break;
	case OP_NOT_ENT:
	    c->_float = 1; //FIXME   (PROG_TO_EDICT(a->edict) == sv.edicts);
	    break;

	case OP_EQ_F:
	    c->_float = a->_float == b->_float;
	    break;
	case OP_EQ_V:
	    c->_float = (a->vector[0] == b->vector[0]) &&
		(a->vector[1] == b->vector[1]) &&
		(a->vector[2] == b->vector[2]);
	    break;
	case OP_EQ_S:
	    c->_float =
		!strcmp(PR_GetString(a->string), PR_GetString(b->string));
	    break;
	case OP_EQ_E:
	    c->_float = a->_int == b->_int;
	    break;
	case OP_EQ_FNC:
	    c->_float = a->function == b->function;
	    break;

	case OP_NE_F:
	    c->_float = a->_float != b->_float;
	    break;
	case OP_NE_V:
	    c->_float = (a->vector[0] != b->vector[0]) ||
		(a->vector[1] != b->vector[1]) ||
		(a->vector[2] != b->vector[2]);
	    break;
	case OP_NE_S:
	    c->_float =
		strcmp(PR_GetString(a->string), PR_GetString(b->string));
	    break;
	case OP_NE_E:
	    c->_float = a->_int != b->_int;
	    break;
	case OP_NE_FNC:
	    c->_float = a->function != b->function;
	    break;

//==================
	case OP_STORE_F:
	case OP_STORE_ENT:
	case OP_STORE_FLD:	// integers
	case OP_STORE_S:
	case OP_STORE_FNC:	// pointers
	    b->_int = a->_int;
	    break;
	case OP_STORE_V:
	    b->vector[0] = a->vector[0];
	    b->vector[1] = a->vector[1];
	    b->vector[2] = a->vector[2];
	    break;

	case OP_STOREP_F:
	case OP_STOREP_ENT:
	case OP_STOREP_FLD:	// integers
	case OP_STOREP_S:
	case OP_STOREP_FNC:	// pointers
      fprintf(stderr, "ignoring OP_STOREP_F/ENT/FLD/S/FNC\n");
      /*
	    ptr = (eval_t *)((byte *)sv.edicts + b->_int);
	    ptr->_int = a->_int;
      */
	    break;
	case OP_STOREP_V:
      fprintf(stderr, "ignoring OP_STOREP_V\n");
      /*
	    ptr = (eval_t *)((byte *)sv.edicts + b->_int);
	    ptr->vector[0] = a->vector[0];
	    ptr->vector[1] = a->vector[1];
	    ptr->vector[2] = a->vector[2];
      */
	    break;

	case OP_ADDRESS:
      fprintf(stderr, "ignoring OP_ADDRESS\n");
      /*
	    ed = PROG_TO_EDICT(a->edict);
#ifdef PARANOID
	    NUM_FOR_EDICT(ed);	// make sure it's in range
#endif
	    if (ed == (edict_t *)sv.edicts && sv.state == ss_active)
		    Error("assignment to world entity");
	    c->_int = (byte *)((int *)&ed->v + b->_int) - (byte *)sv.edicts;
      */
	    break;

	case OP_LOAD_F:
	case OP_LOAD_FLD:
	case OP_LOAD_ENT:
	case OP_LOAD_S:
	case OP_LOAD_FNC:
      fprintf(stderr, "ignoring OP_LOAD_F/FLD/ENT/S/FNC\n");
/*
	    ed = PROG_TO_EDICT(a->edict);
#ifdef PARANOID
	    NUM_FOR_EDICT(ed);	// make sure it's in range
#endif
	    a = (eval_t *)((int *)&ed->v + b->_int);
	    c->_int = a->_int; */
	    break;

	case OP_LOAD_V:
      fprintf(stderr, "ignoring OP_LOAD_V\n");
      /*
	    ed = PROG_TO_EDICT(a->edict);
#ifdef PARANOID
	    NUM_FOR_EDICT(ed);	// make sure it's in range
#endif
	    a = (eval_t *)((int *)&ed->v + b->_int);
	    c->vector[0] = a->vector[0];
	    c->vector[1] = a->vector[1];
	    c->vector[2] = a->vector[2];
      */
	    break;

//==================

	case OP_IFNOT:
	    if (!a->_int)
		s += st->b - 1;	// offset the s++
	    break;

	case OP_IF:
	    if (a->_int)
		s += st->b - 1;	// offset the s++
	    break;

	case OP_GOTO:
	    s += st->a - 1;	// offset the s++
	    break;

	case OP_CALL0:
	case OP_CALL1:
	case OP_CALL2:
	case OP_CALL3:
	case OP_CALL4:
	case OP_CALL5:
	case OP_CALL6:
	case OP_CALL7:
	case OP_CALL8:
	    pr_argc = st->op - OP_CALL0;
	    if (!a->function)
		PR_RunError("NULL function");

	    newf = &functions[a->function];

	    /* negative statements are built in functions */
	    if (newf->first_statement < 0) {
		i = -newf->first_statement;
		if (i >= pr_numbuiltins)
		    PR_RunError("Bad builtin call number");
		pr_builtins[i] ();
		break;
	    }

	    s = PR_EnterFunction(newf);
	    break;

	case OP_DONE:
	case OP_RETURN:
	    pr_globals[OFS_RETURN] = pr_globals[st->a];
	    pr_globals[OFS_RETURN + 1] = pr_globals[st->a + 1];
	    pr_globals[OFS_RETURN + 2] = pr_globals[st->a + 2];

	    s = PR_LeaveFunction();
	    if (pr_depth == exitdepth)
		return;		// all done
	    break;

	case OP_STATE:
      fprintf(stderr, "ignoring OP_STATE\n");
      /*
	    ed = PROG_TO_EDICT(pr_global_struct->self);
	    ed->v.nextthink = pr_global_struct->time + 0.1;
	    if (a->_float != ed->v.frame) {
        ed->v.frame = a->_float;
	    }
	    ed->v.think = b->function;
      */
	    break;

	default:
	    PR_RunError("Bad opcode %i", st->op);
	}
    }
}

/*----------------------*/

#define PR_STRTBL_CHUNK 256
static char **pr_strtbl = NULL;
static int pr_strtbl_size;
static int num_prstr;

void
PR_InitStringTable(void)
{
    if (pr_strtbl) {
/// 	Z_Free(pr_strtbl);
	pr_strtbl = NULL;
    }
    pr_strtbl_size = 0;
    num_prstr = 0;
}

char *
PR_GetString(int num)
{
    char *s = "";

int pr_strings_size = 99999; // FIXME

    if (num >= 0 && num < pr_strings_size - 1)
	s = strings + num;
    else if (num < 0 && num >= -num_prstr)
	s = pr_strtbl[-num - 1];
    else
      Error("invalid string offset %d\n", num);

    return s;
}

int
PR_SetString(char *s)
{
fprintf(stderr, "PR_SetString : IGNORED\n");
return 0;
/*
    int i;

    if (s - pr_strings < 0 || s - pr_strings > pr_strings_size - 2) {
	for (i = 0; i < num_prstr; i++)
	    if (pr_strtbl[i] == s)
		break;
	if (i < num_prstr)
	    return -i - 1;
	if (num_prstr == pr_strtbl_size) {
	    pr_strtbl_size += PR_STRTBL_CHUNK;
	    pr_strtbl = Z_Realloc(pr_strtbl, pr_strtbl_size * sizeof(char *));
	    Con_DPrintf("%s: Progs string table grew to %d entries.\n",
			__func__, pr_strtbl_size);
	}
	pr_strtbl[num_prstr] = s;
	num_prstr++;
	return -num_prstr;
    }
    return (int)(s - pr_strings);
*/
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
