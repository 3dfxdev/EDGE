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
    int edict;
    union eval_s	*ptr;
}
eval_t;

#define	MAX_ENT_LEAFS	16
//??  typedef struct edict_s {
//??      bool free;
//??      link_t area;		// linked to a division node or leaf
//??  
//??      int num_leafs;
//??      short leafnums[MAX_ENT_LEAFS];
//??  
//??      entity_state_t baseline;
//??  
//??      float freetime;		// sv.time when the object was freed
//??      vars_t v;		// C exported fields from progs
//??  // other fields from progs come immediately after
//??  } edict_t;

typedef int edict_t;  //!!!! TEMP

#define	EDICT_FROM_AREA(l) STRUCT_FROM_LINK(l,edict_t,area)

extern int pr_edict_size;	// in bytes

//============================================================================

void PR_Init(void);

void PR_ExecuteProgram(func_t fnum);
void PR_LoadProgs(void);

void PR_Profile_f(void);

// returns a copy of the string allocated from the server's string heap

void ED_Print(edict_t *ed);



//define EDICT_NUM(n) ((edict_t *)(sv.edicts+ (n)*pr_edict_size))
//define NUM_FOR_EDICT(e) (((byte *)(e) - sv.edicts)/pr_edict_size)

edict_t *EDICT_NUM(int n);
int NUM_FOR_EDICT(edict_t *e);

#define	NEXT_EDICT(e) ((edict_t *)( (byte *)e + pr_edict_size))

#define	EDICT_TO_PROG(e) ((byte *)e - (byte *)sv.edicts)
#define PROG_TO_EDICT(e) ((edict_t *)((byte *)sv.edicts + e))

//============================================================================

#define	G_EDICT(o) ((edict_t *)((byte *)sv.edicts+ *(int *)&pr_globals[o]))
#define G_EDICTNUM(o) NUM_FOR_EDICT(G_EDICT(o))

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

void ED_PrintEdicts(void);
void ED_PrintNum(int ent);


/*
 * PR Strings stuff
 */
void PR_InitStringTable(void);
char *PR_GetString(int num);
int PR_SetString(char *s);


#endif /* PROGS_H */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
