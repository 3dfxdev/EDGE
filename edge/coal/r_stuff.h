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

#ifndef PROGS_H
#define PROGS_H

typedef union eval_s
{
    string_t string;
    float _float;
    float vector[3];
    func_t function;
    int _int;
}
eval_t;

#define	MAX_ENT_LEAFS	16




//============================================================================

void PR_Init(void);

void PR_ExecuteProgram(func_t fnum);
void PR_LoadProgs(void);

void PR_Profile_f(void);

// returns a copy of the string allocated from the server's string heap



//============================================================================

#define	E_FLOAT(e,o) (((float*)&e->v)[o])
#define	E_INT(e,o) (*(int *)&((float*)&e->v)[o])
#define	E_VECTOR(e,o) (&((float*)&e->v)[o])
#define	E_STRING(e,o) (PR_GetString(*(string_t *)&((float*)&e->v)[o]))

typedef void (*builtin_t) (void);
extern builtin_t *pr_builtins;
extern int pr_numbuiltins;

extern int pr_argc;

extern bool pr_trace;

void PR_RunError(const char *error, ...) __attribute__((format(printf,1,2)));



/*
 * PR Strings stuff
 */
void PR_InitStringTable(void);
char *PR_GetString(int num);
int PR_SetString(char *s);


#endif /* PROGS_H */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
