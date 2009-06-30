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


#define MAX_PRINTMSG  1024

#define Con_Printf   printf
#define Con_DPrintf  printf


// typedef int  bool;
// #define false   0
// #define true    1


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <errno.h>
#include <assert.h>

#include <sys/signal.h>


#include "defs.h"
#include "r_stuff.h"
#include "coal.h"


void Error (char *error, ...)
{
	va_list argptr;

	printf ("************ ERROR ************\n");

	va_start (argptr,error);
	vprintf (error,argptr);
	va_end (argptr);
	printf ("\n");
	exit (1);
}



#include "q_lex.c"
#include "q_comp.c"

#include "r_cmds.c"
#include "r_exec.c"


char		sourcedir[1024];

double		pr_globals[MAX_REGS];
int			numpr_globals;

char		strings[MAX_STRINGS];
int			strofs;

statement_t	statements[MAX_STATEMENTS];
int			numstatements;
int			statement_linenums[MAX_STATEMENTS];

function_t	functions[MAX_FUNCTIONS];
int			numfunctions;



// CopyString returns an offset from the string heap
int	CopyString (char *str)
{
	int		old;

	old = strofs;
	strcpy (strings+strofs, str);
	strofs += strlen(str)+1;
	return old;
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


void PrintFunctions (void)
{
	int		i,j;
	function_t	*d;

	for (i=0 ; i<numfunctions ; i++)
	{
		d = &functions[i];
		printf ("%s : %s : %i %i (", strings + d->s_file, strings + d->s_name, d->first_statement, d->parm_start);
		for (j=0 ; j<d->parm_num ; j++)
			printf ("%i ",d->parm_size[j]);
		printf (")\n");
	}
}


void InitData (void)
{
	numstatements = 1;
	strofs = 1;
	numfunctions = 1;

	def_ret.ofs = OFS_RETURN;

	for (int i=0 ; i<MAX_PARMS ; i++)
		def_parms[i].ofs = OFS_PARM0 + 3*i;
}


static void WriteData (void)
{

//PrintStrings ();
//PrintFunctions ();

strofs = (strofs+3)&~3;

	printf ("%6i strofs\n", strofs);
	printf ("%6i numstatements\n", numstatements);
	printf ("%6i numfunctions\n", numfunctions);
	printf ("%6i numpr_globals\n", numpr_globals);

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
char *PR_GlobalStringNoContents (gofs_t ofs)
{
	static char	line[128];

	def_t * def = pr_global_defs[ofs];
	if (!def)
//		Error ("PR_GlobalString: no def for %i", ofs);
		sprintf (line,"%i(?!?)", ofs);
	else
		sprintf (line,"%i(%s)", ofs, def->name);

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
		sprintf (line,"%i(%s)", ofs, s);
	}
	else
		sprintf (line,"%i(%s)", ofs, def->name);

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
	int		i;

	printf ("%4i : %4i : %s ", (int)(s - statements), statement_linenums[s-statements], pr_opcodes[s->op].opname);
	i = strlen(pr_opcodes[s->op].opname);
	for ( ; i<10 ; i++)
		printf (" ");

	if (s->op == OP_IF || s->op == OP_IFNOT)
		printf ("%sbranch %i",PR_GlobalString(s->a),s->b);
	else if (s->op == OP_GOTO)
	{
		printf ("branch %i",s->a);
	}
	else if ( (unsigned)(s->op - OP_MOVE_F) < 6)
	{
		printf ("%s",PR_GlobalString(s->a));
		printf ("%s", PR_GlobalStringNoContents(s->b));
	}
	else
	{
		if (s->a)
			printf ("%s",PR_GlobalString(s->a));
		if (s->b)
			printf ("%s",PR_GlobalString(s->b));
		if (s->c)
			printf ("%s", PR_GlobalStringNoContents(s->c));
	}
	printf ("\n");
}


void PR_PrintDefs (void)
{
	def_t *d;

	for (d=pr.defs ; d ; d=d->next)
		PR_PrintOfs (d->ofs);
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



/*
==============
PR_BeginCompilation

called before compiling a batch of files, clears the pr struct
==============
*/
void PR_BeginCompilation (void)
{
	int i;

	static def_t  def_void = {&type_void, "VOID_CRUD"};

	numpr_globals = RESERVED_OFS;
	pr.defs = NULL;

	for (i=0 ; i<RESERVED_OFS ; i++)
		pr_global_defs[i] = &def_void;

// link the function type in so state forward declarations match proper type
	pr.types = &type_function;
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
	for (d=pr.defs ; d ; d=d->next)
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


//============================================================================



static int filelength (FILE *f)
{
	int pos = ftell (f);

	fseek (f, 0, SEEK_END);

	int end = ftell (f);

	fseek (f, pos, SEEK_SET);

	return end;
}


int LoadFile (char *filename, char **bufptr)
{
	FILE *f = fopen(filename, "rb");

	if (!f)
		Error ("Error opening %s: %s", filename, strerror(errno));

	int length = filelength (f);
	*bufptr = (char *) malloc (length+1);

	if ( fread (*bufptr, 1, length, f) != (size_t)length)
		Error ("File read failure");

	fclose (f);

	(*bufptr)[length] = 0;

	return length;
}


//============================================================================

/*
============
main
============
*/
int main (int argc, char **argv)
{
	char	*src2;
	char	filename[1024];

	int   k;

	if (argc <= 1 ||
	    (strcmp(argv[1], "-?") == 0) || (strcmp(argv[1], "-h") == 0) ||
		(strcmp(argv[1], "-help") == 0) || (strcmp(argv[1], "--help") == 0))
	{
		printf ("USAGE: coal [OPTIONS] filename.qc ...\n");
		return 0;
	}

  strcpy (sourcedir, "");

	InitData ();

	pr_dumpasm = false;


	PR_BeginCompilation();

// compile all the files
  for (k = 1; k < argc; k++)
	{
    if (argv[k][0] == '-')
      Error("Bad filename: %s\n", argv[k]);

		sprintf (filename, "%s%s", sourcedir, argv[k]);
		printf ("compiling %s\n", filename);

		LoadFile (filename, &src2);

		if (!PR_CompileFile (src2, filename) )
			exit (1);

    // FIXME: FreeFile(src2);

	}

	if (!PR_FinishCompilation ())
		Error ("compilation errors");

  WriteData();


  // find 'main' function
  func_t main_func = 0;
  {
    int i;
    for (i = 0; i < numfunctions; i++)
    {
      function_t *f = &functions[i];

      const char *name = strings + f->s_name;

      if (strcmp(name, "main") == 0)
      {
        main_func = (func_t)i;
        break;
      }
    }
  }

  if (! (int)main_func)
    Error("No main function!\n");

  fprintf(stderr, "numfunctions = %d  main = %d\n", numfunctions, (int)main_func);

  PR_ExecuteProgram(main_func);


  return 0;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
