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

#ifndef __QCC_H__
#define __QCC_H__


void PR_BeginCompilation(void);
bool PR_FinishCompilation(void);

void PR_ExecuteProgram(func_t fnum);


//============================================================================

// pr_loc.h -- program local defs

#define	MAX_ERRORS		20

#define	MAX_NAME		64		// chars long

#define	MAX_REGS		16384

//============================================================================


extern const char *opcode_names[];

extern def_t *pr_global_defs[MAX_REGS];	// to find def for a global variable

void PR_PrintStatement (statement_t *s);

void PR_Lex (void);
// reads the next token into pr_token and classifies its type

type_t *PR_ParseType (void);
char *PR_ParseName (void);

bool PR_Check (char *string);
void PR_Expect (char *string);
void PR_ParseError (char *error, ...);


class parse_error_x
{
public:
	int foo;

	 parse_error_x() { }
	~parse_error_x() { }
};


extern	int			pr_source_line;
extern	char		*pr_file_p;


#define	OFS_NULL		0
#define	OFS_RETURN		1

#define	RESERVED_OFS	10


extern	def_t	*pr_scope;
extern	int		pr_error_count;

void PR_NewLine (void);
def_t *PR_GetDef (type_t *type, char *name, def_t *scope, bool allocate);

void PR_PrintDefs (void);

void PR_SkipToSemicolon (void);

extern	char		pr_parm_names[MAX_PARMS][MAX_NAME];
extern	bool	pr_trace;

#define	G_FLOAT(o) (pr_globals[o])
#define	G_VECTOR(o) (&pr_globals[o])
#define	G_STRING(o) (strings + (int)pr_globals[o])
#define	G_FUNCTION(o) (pr_globals[o])


bool	PR_CompileFile (char *string, char *filename);

extern	string_t	s_file;			// filename for function definition


//=============================================================================

#define	MAX_STRINGS		500000
#define	MAX_GLOBALS		16384
#define	MAX_FIELDS		1024
#define	MAX_STATEMENTS	65536
#define	MAX_FUNCTIONS	8192

#define	MAX_DATA_PATH	64

extern	char	strings[MAX_STRINGS];
extern	int		strofs;

extern	statement_t	statements[MAX_STATEMENTS];
extern	int			numstatements;
extern	int			statement_linenums[MAX_STATEMENTS];

extern	function_t  functions[MAX_FUNCTIONS];
extern	int			numfunctions;

extern	double		pr_globals[MAX_REGS];
extern	int			numpr_globals;

extern	char	pr_immediate_string[2048];

int	CopyString (char *str);

double * PR_Parameter(int p);

#endif

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
