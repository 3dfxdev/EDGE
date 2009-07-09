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


// COMPILER:

void PR_InitData(void);
void PR_BeginCompilation(void);
bool PR_FinishCompilation(void);
void PR_ShowStats(void);

// EXECUTOR:

void PR_SetTrace(bool enable);
func_t PR_FindFunction(const char *func_name);
void PR_ExecuteProgram(func_t fnum);


//============================================================================

#define	MAX_ERRORS		20

#define	MAX_REGS		16384

//============================================================================


extern def_t *pr_global_defs[MAX_REGS];	// to find def for a global variable

void PR_PrintStatement(statement_t *s);


#define	OFS_NULL		0
#define	OFS_RETURN		1

#define	RESERVED_OFS	10


void PR_PrintDefs(void);


#define	G_FLOAT(o) (pr_globals[o])
#define	G_VECTOR(o) (&pr_globals[o])
#define	G_STRING(o) (strings + (int)pr_globals[o])
#define	G_FUNCTION(o) (pr_globals[o])


bool PR_CompileFile(char *string, char *filename);


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

int	CopyString(char *str);

double * PR_Parameter(int p);

#endif

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
