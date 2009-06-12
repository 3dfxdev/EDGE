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


#include "lib.h"
#include "lib.c"

#include "coal.h"

#include "q_lex.c"
#include "q_comp.c"

#include "r_cmds.c"
#include "r_exec.c"


char		sourcedir[1024];

float		pr_globals[MAX_REGS];
int			numpr_globals;

char		strings[MAX_STRINGS];
int			strofs;

dstatement_t	statements[MAX_STATEMENTS];
int			numstatements;
int			statement_linenums[MAX_STATEMENTS];

dfunction_t	functions[MAX_FUNCTIONS];
int			numfunctions;

ddef_t		globals[MAX_GLOBALS];
int			numglobaldefs;

ddef_t		fields[MAX_FIELDS];
int			numfielddefs;


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
	dfunction_t	*d;
	
	for (i=0 ; i<numfunctions ; i++)
	{
		d = &functions[i];
		printf ("%s : %s : %i %i (", strings + d->s_file, strings + d->s_name, d->first_statement, d->parm_start);
		for (j=0 ; j<d->numparms ; j++)
			printf ("%i ",d->parm_size[j]);
		printf (")\n");
	}
}

void PrintFields (void)
{
	int		i;
	ddef_t	*d;
	
	for (i=0 ; i<numfielddefs ; i++)
	{
		d = &fields[i];
		printf ("%5i : (%i) %s\n", d->ofs, d->type, strings + d->s_name);
	}
}

void PrintGlobals (void)
{
	int		i;
	ddef_t	*d;
	
	for (i=0 ; i<numglobaldefs ; i++)
	{
		d = &globals[i];
		printf ("%5i : (%i) %s\n", d->ofs, d->type, strings + d->s_name);
	}
}


void InitData (void)
{
	int		i;
	
	numstatements = 1;
	strofs = 1;
	numfunctions = 1;
	numglobaldefs = 1;
	numfielddefs = 1;
	
	def_ret.ofs = OFS_RETURN;
	for (i=0 ; i<MAX_PARMS ; i++)
		def_parms[i].ofs = OFS_PARM0 + 3*i;
}


static void WriteData (void)
{
	def_t		*def;
	ddef_t		*dd;
	int			i;

	for (def = pr.def_head.next ; def ; def = def->next)
	{
		if (def->type->type == ev_function)
		{
//			df = &functions[numfunctions];
//			numfunctions++;

		}
    else if (def->type->type == ev_field)
    {
      dd = &fields[numfielddefs];
      numfielddefs++;
      dd->type = def->type->aux_type->type;
      dd->s_name = CopyString (def->name);
      dd->ofs = G_INT(def->ofs);
    }
    dd = &globals[numglobaldefs];
    numglobaldefs++;
    dd->type = def->type->type;
    if ( !def->initialized
        && def->type->type != ev_function
        && def->type->type != ev_field
        && def->scope == NULL)
      dd->type |= DEF_SAVEGLOBAL;
    dd->s_name = CopyString (def->name);
    dd->ofs = def->ofs;
	}

//PrintStrings ();
//PrintFunctions ();
//PrintFields ();
//PrintGlobals ();

strofs = (strofs+3)&~3;

	printf ("%6i strofs\n", strofs);
	printf ("%6i numstatements\n", numstatements);
	printf ("%6i numfunctions\n", numfunctions);
	printf ("%6i numglobaldefs\n", numglobaldefs);
	printf ("%6i numfielddefs\n", numfielddefs);
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



def_t	*PR_DefForFieldOfs (gofs_t ofs)
{
	def_t	*d;
	
	for (d=pr.def_head.next ; d ; d=d->next)
	{
		if (d->type->type != ev_field)
			continue;
		if (*((int *)&pr_globals[d->ofs]) == ofs)
			return d;
	}
	Error ("PR_DefForFieldOfs: couldn't find %i",ofs);
	return NULL;
}

/*
============
PR_ValueString

Returns a string describing *data in a type specific manner
=============
*/
char *PR_ValueString (etype_t type, eval_t *val)
{
	static char	line[256];
	def_t		*def;
	dfunction_t	*f;
	
	switch (type)
	{
	case ev_string:
		sprintf (line, "%s", PR_String(strings + *(int *)val));
		break;
	case ev_entity:	
		sprintf (line, "entity %i", *(int *)val);
		break;
	case ev_function:
		f = functions + *(int *)val;
		if (!f)
			sprintf (line, "undefined function");
		else
			sprintf (line, "%s()", strings + f->s_name);
		break;
	case ev_field:
		def = PR_DefForFieldOfs ( *(int *)val );
		sprintf (line, ".%s", def->name);
		break;
	case ev_void:
		sprintf (line, "void");
		break;
	case ev_float:
		sprintf (line, "%5.1f", *(float *)val);
		break;
	case ev_vector:
		sprintf (line, "'%5.1f %5.1f %5.1f'", ((float *)val)[0], ((float *)val)[1], ((float *)val)[2]);
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
	int		i;
	def_t	*def;
	void	*val;
	static char	line[128];
	
	val = (void *)&pr_globals[ofs];
	def = pr_global_defs[ofs];
	if (!def)
//		Error ("PR_GlobalString: no def for %i", ofs);
		sprintf (line,"%i(?!?)", ofs);
	else
		sprintf (line,"%i(%s)", ofs, def->name);
	
	i = strlen(line);
	for ( ; i<16 ; i++)
		strcat (line," ");
	strcat (line," ");
		
	return line;
}

char *PR_GlobalString (gofs_t ofs)
{
	char	*s;
	int		i;
	def_t	*def;
	void	*val;
	static char	line[128];
	
	val = (void *)&pr_globals[ofs];
	def = pr_global_defs[ofs];
	if (!def)
		return PR_GlobalStringNoContents(ofs);
	if (def->initialized && def->type->type != ev_function)
	{
		s = PR_ValueString (def->type->type, (eval_t*)&pr_globals[ofs]);
		sprintf (line,"%i(%s)", ofs, s);
	}
	else
		sprintf (line,"%i(%s)", ofs, def->name);
	
	i = strlen(line);
	for ( ; i<16 ; i++)
		strcat (line," ");
	strcat (line," ");
		
	return line;
}

/*
============
PR_PrintOfs
============
*/
void PR_PrintOfs (gofs_t ofs)
{
	printf ("%s\n",PR_GlobalString(ofs));
}

/*
=================
PR_PrintStatement
=================
*/
void PR_PrintStatement (dstatement_t *s)
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
	else if ( (unsigned)(s->op - OP_STORE_F) < 6)
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


/*
============
PR_PrintDefs
============
*/
void PR_PrintDefs (void)
{
	def_t	*d;
	
	for (d=pr.def_head.next ; d ; d=d->next)
		PR_PrintOfs (d->ofs);
}


/*
==============
PR_BeginCompilation

called before compiling a batch of files, clears the pr struct
==============
*/
void	PR_BeginCompilation (void *memory, int memsize)
{
	int		i;
	
	pr.memory = memory;
	pr.max_memory = memsize;
	
	numpr_globals = RESERVED_OFS;
	pr.def_tail = &pr.def_head;
	
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
qboolean	PR_FinishCompilation (void)
{
	def_t		*d;
	qboolean	errors;
	
	errors = false;
	
// check to make sure all functions prototyped have code
	for (d=pr.def_head.next ; d ; d=d->next)
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

//=============================================================================

/*
============
PR_WriteProgdefs

Writes the global and entity structures out
Returns a crc of the header, to be stored in the progs file for comparison
at load time.
============
*/
int	PR_WriteProgdefs (char *filename)
{
	def_t	*d;
	FILE	*f;
	unsigned short		crc;
	int		c;
	
	printf ("writing %s\n", filename);
	f = fopen (filename, "w");
	
// print global vars until the first field is defined
	fprintf (f,"\n/* file generated by coal, do not modify */\n\ntypedef struct\n{\tint\tpad[%i];\n", RESERVED_OFS);
	for (d=pr.def_head.next ; d ; d=d->next)
	{
		if (!strcmp (d->name, "end_sys_globals"))
			break;
			
		switch (d->type->type)
		{
		case ev_float:
			fprintf (f, "\tfloat\t%s;\n",d->name);
			break;
		case ev_vector:
			fprintf (f, "\tvec3_t\t%s;\n",d->name);
			d=d->next->next->next;	// skip the elements
			break;
		case ev_string:
			fprintf (f,"\tstring_t\t%s;\n",d->name);
			break;
		case ev_function:
			fprintf (f,"\tfunc_t\t%s;\n",d->name);
			break;
		case ev_entity:
			fprintf (f,"\tint\t%s;\n",d->name);
			break;
		default:
			fprintf (f,"\tint\t%s;\n",d->name);
			break;
		}
	}
	fprintf (f,"} globalvars_t;\n\n");

// print all fields
	fprintf (f,"typedef struct\n{\n");
	for (d=pr.def_head.next ; d ; d=d->next)
	{
		if (!strcmp (d->name, "end_sys_fields"))
			break;
			
		if (d->type->type != ev_field)
			continue;
			
		switch (d->type->aux_type->type)
		{
		case ev_float:
			fprintf (f,"\tfloat\t%s;\n",d->name);
			break;
		case ev_vector:
			fprintf (f,"\tvec3_t\t%s;\n",d->name);
			d=d->next->next->next;	// skip the elements
			break;
		case ev_string:
			fprintf (f,"\tstring_t\t%s;\n",d->name);
			break;
		case ev_function:
			fprintf (f,"\tfunc_t\t%s;\n",d->name);
			break;
		case ev_entity:
			fprintf (f,"\tint\t%s;\n",d->name);
			break;
		default:
			fprintf (f,"\tint\t%s;\n",d->name);
			break;
		}
	}
	fprintf (f,"} entvars_t;\n\n");
	
	fclose (f);
	
// do a crc of the file
  crc = 0;

	return crc;
}


void PrintFunction (char *name)
{
	int		i;
	dstatement_t	*ds;
	dfunction_t		*df;
	
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


//============================================================================

/*
============
main
============
*/
int main (int argc, char **argv)
{
	char	*src;
	char	*src2;
	char	filename[1024];
	int		p;
  int   k;
	double	start, stop;

	start = I_FloatTime ();

	myargc = argc;
	myargv = argv;
	
	if (argc <= 1 || CheckParm ("-?") || CheckParm ("-help"))
	{
		printf ("USAGE: coal [OPTIONS] file ...\n");
		return 0;
	}
	
  strcpy (sourcedir, "");

	InitData ();
	
	pr_dumpasm = false;


	PR_BeginCompilation (malloc (0x100000), 0x100000);

// compile all the files
  for (k = 1; k < argc; k++)
	{
    if (argv[k][0] == '-')
      Error("Bad filename: %s\n", argv[k]);

		sprintf (filename, "%s%s", sourcedir, argv[k]);
		printf ("compiling %s\n", filename);

		LoadFile (filename, (void *)&src2);

		if (!PR_CompileFile (src2, filename) )
			exit (1);
		
    // FIXME: FreeFile(src2);

	}
	
	if (!PR_FinishCompilation ())
		Error ("compilation errors");

  WriteData();


	stop = I_FloatTime ();
	printf ("%i seconds elapsed.\n", (int)(stop-start));
  printf("\n\n");


  // find 'main' function
  func_t main_func = 0;
  {
    int i;
    for (i = 0; i < numfunctions; i++)
    {
      dfunction_t *f = &functions[i];

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
