//----------------------------------------------------------------------------
//  EDGE Data Definition File Code (States)
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2004  The EDGE Team.
// 
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//----------------------------------------------------------------------------

#include "i_defs.h"

#include "ddf_main.h"

#include "e_search.h"
#include "ddf_locl.h"
#include "dm_state.h"
#include "dstrings.h"
#include "m_argv.h"
#include "m_inline.h"
#include "m_math.h"
#include "m_misc.h"
#include "p_action.h"
#include "p_mobj.h"
#include "r_state.h"
#include "r_things.h"
#include "z_zone.h"


static const state_t template_state =
{
	0,          // sprite ref
	0,          // frame ref
	0,          // brightness
	-1,         // tics
	NULL,       // label
	NULL,       // routine
	NULL,       // parameter
	0,          // next state ref
	-1          // jump state ref
};

// -KM- 1998/11/25 All weapon related states are out
state_t *states = NULL;
int num_states;

// Until the DDF_StateFinishState() routine is called, the `nextstate'
// field of each state contains a special value.  0 for normal (no
// redirector).  -1 for the #REMOVE redirector.  Otherwise the top 16
// bits is a redirector, and the bottom 16 bits is a positive offset
// from that redirector (usually 0).
//
// Every time a new redirector is used, it is added to this list.  The
// top 16 bits (minus 1) will be an index into this list of redirector
// names.  These labels will be looked for in the states when the
// fixup routine is called.

static char **redirection_names = NULL;
static int num_redirection_names;

static char *stateinfo[NUMSPLIT + 1];


void DDF_StateInit(void)
{
	// setup the 'S_NULL' state
	states = Z_New(state_t, 1);
	states[0] = template_state;
	num_states = 1;

	// setup the 'SPR_NULL' sprite
	R_AddSpriteName("NULL", 1);
}

void DDF_StateCleanUp(void)
{
	/* nothing to do */
}

//
// DDF_MainSplitIntoState
//
// Small procedure that takes the info and splits it into relevant stuff
//
// -KM- 1998/12/21 Rewrote procedure, much cleaner now.
//
// -AJA- 2000/09/03: Rewrote _again_ damn it, in order to handle `:'
//       appearing inside brackets.
//
static int DDF_MainSplitIntoState(const char *info)
{
	char *temp;
	char *first;

	int cur;
	int brackets=0;
	bool done=false;

	// use a buffer, since we modify the string
	char infobuf[512];

	strcpy(infobuf, info);

	memset(stateinfo, 0, sizeof(stateinfo));

	first = temp = infobuf;

	for (cur=0; !done && cur < NUMSPLIT; temp++)
	{
		if (*temp == '(')
		{
			brackets++;
			continue;
		}

		if (*temp == ')')
		{
			if (brackets == 0)
				DDF_Error("Mismatched ) bracket in states: %s\n", info);

			brackets--;
			continue;
		}

		if (*temp && *temp != ':')
			continue;

		if (brackets > 0)
			continue;

		if (*temp == 0)
			done = true;

		*temp = 0;

		if (first[0] == REDIRECTOR)
		{
			// signify that we have found redirector
			stateinfo[0] = Z_StrDup(first + 1);
			stateinfo[1] = NULL;
			stateinfo[2] = NULL;

			if (! done)
				stateinfo[1] = Z_StrDup(temp + 1);

			return -1;
		}

		stateinfo[cur++] = Z_StrDup(first);
		first = temp + 1;
	}

	if (brackets > 0)
		DDF_Error("Unclosed ( bracket in states: %s\n", info);

	return cur;
}

//
// DDF_DestroyStates
//
// Kills a leak.
// -ES- 2000/02/02 Added.
//
static void DestroyStateInfo(void)
{
	int i;
  
	for (i = 0; i < NUMSPLIT; i++)
	{
		if (stateinfo[i])
		{
			Z_Free(stateinfo[i]);
			stateinfo[i] = NULL;
		}
	}
}

//
// DDF_MainSplitActionArg
//
// Small procedure that takes an action like "FOO(BAR)", and splits it
// into two strings "FOO" and "BAR".
//
// -AJA- 1999/08/10: written.
//
static void DDF_MainSplitActionArg(char *info, char *actname, char *actarg)
{
	int len = strlen(info);
	char *mid = strchr(info, '(');

	if (mid && len >= 4 && info[len - 1] == ')')
	{
		int len2 = (mid - info);

		Z_StrNCpy(actname, info, len2);

		len -= len2 + 2;
		Z_StrNCpy(actarg, mid + 1, len);
	}
	else
	{
		strcpy(actname, info);
		actarg[0] = 0;
	}
}

//
// StateGetRedirector
//
static int StateGetRedirector(const char *redir)
{
	int i;

	for (i=0; i < num_redirection_names; i++)
	{
		if (DDF_CompareName(redirection_names[i], redir) == 0)
			return i;
	}

	Z_Resize(redirection_names, char *, ++num_redirection_names);
  
	redirection_names[num_redirection_names-1] = Z_StrDup(redir);

	return num_redirection_names-1;
}

//
// StateFindLabel
//
int StateFindLabel(int first, int last, const char *label)
{
	int i;

	for (i=first; i <= last; i++)
	{
		if (!states[i].label)
			continue;

		if (DDF_CompareName(states[i].label, label) == 0)
			return i;
	}

	// compatibility hack:
	if (DDF_CompareName(label, "IDLE") == 0)
	{
		return StateFindLabel(first, last, "SPAWN");
	}
  
	DDF_Error("Unknown label `%s' (object has no such frames).\n", label);
	return 0;
}

//
// DDF_StateReadState
//
void DDF_StateReadState(const char *info, const char *label,
						int *first, int *last, int *state_num, int index,
						const char *redir, const actioncode_t *action_list)
{
	int i, j;

	char action_name[128];
	char action_arg[128];

	state_t *cur;

	// Split the state info into component parts
	// -ACB- 1998/07/26 New Procedure, for cleaner code.

	i = DDF_MainSplitIntoState(info);
	if (i < 5 && i >= 0)
	{
		if (strchr(info, '['))
		{
			// -ES- 2000/02/02 Probably unterminated state.
			DDF_Error("DDF_MainLoadStates: Bad state '%s', possibly missing ';'\n");
		}
		DDF_Error("Bad state '%s'\n", info);
	}

	if (stateinfo[0] == NULL)
		DDF_Error("Missing sprite in state frames: `%s'\n", info);

	//--------------------------------------------------
	//----------------REDIRECTOR HANDLING---------------
	//--------------------------------------------------

	if (stateinfo[2] == NULL)
	{
		if ((*first) == 0)
			DDF_Error("Redirector used without any states (`%s')\n", info);

		cur = &states[(*last)];

		DEV_ASSERT2(stateinfo[0]);

		if (DDF_CompareName(stateinfo[0], "REMOVE") == 0)
		{
			cur->nextstate = -1;
			DestroyStateInfo();
			return;
		}  

		cur->nextstate = (StateGetRedirector(stateinfo[0]) + 1) << 16;

		if (stateinfo[1] != NULL)
			cur->nextstate += MAX(0, atoi(stateinfo[1]) - 1);

		DestroyStateInfo();
		return;
	}

	//--------------------------------------------------
	//---------------- ALLOCATE NEW STATE --------------
	//--------------------------------------------------

	Z_Resize(states, state_t, ++num_states);

	cur = &states[num_states-1];

	// initialise with defaults
	cur[0] = template_state;
  
	if ((*first) == 0)
	{
		// very first state for thing/weapon
		(*first) = num_states-1;
	}

	(*last) = num_states-1;
  
	if (index == 0)
	{
		// first state in this set of states
		if (state_num)
			state_num[0] = num_states-1;
    
		// ...therefore copy the label
		cur->label = Z_StrDup(label);
	}

	if (redir && cur->nextstate == 0)
	{
		if (DDF_CompareName("REMOVE", redir) == 0)
			cur->nextstate = -1;
		else
			cur->nextstate = (StateGetRedirector(redir) + 1) << 16;
	}

	//--------------------------------------------------
	//----------------SPRITE NAME HANDLING--------------
	//--------------------------------------------------

	if (!stateinfo[1] || !stateinfo[2] || !stateinfo[3])
		DDF_Error("Bad state frame, missing fields: %s\n", info);
  
	if (strlen(stateinfo[0]) != 4)
		DDF_Error("DDF_MainLoadStates: Sprite names must be 4 "
				  "characters long '%s'.\n", stateinfo[0]);

	//--------------------------------------------------
	//--------------SPRITE INDEX HANDLING---------------
	//--------------------------------------------------

	// look at the first character
	j = stateinfo[1][0];

	// check for bugger up
	if (j < 'A' || j > ']')
		DDF_Error("DDF_MainLoadStates: Illegal sprite index: %s\n", stateinfo[1]);

	cur->frame = (short)(j - 65);
	cur->sprite = R_AddSpriteName(stateinfo[0], cur->frame);  

	//--------------------------------------------------
	//------------STATE TIC COUNT HANDLING--------------
	//--------------------------------------------------

	cur->tics = atol(stateinfo[2]);

	//--------------------------------------------------
	//------------STATE BRIGHTNESS LEVEL----------------
	//--------------------------------------------------

	if (!strcmp("BRIGHT", stateinfo[3]))
		cur->bright = 1;
	else if (strcmp("NORMAL", stateinfo[3]))
		DDF_WarnError2(0x128, "DDF_MainLoadStates: Lighting is not BRIGHT or NORMAL\n");

	//--------------------------------------------------
	//------------STATE ACTION CODE HANDLING------------
	//--------------------------------------------------

	if (stateinfo[4])
	{
		// Get Action Code Ref (Using remainder of the string).
		// Go through all the actions, end if terminator or action found
		//
		// -AJA- 1999/08/10: Updated to handle a special argument.

		DDF_MainSplitActionArg(stateinfo[4], action_name, action_arg);

		for (i=0; action_list[i].actionname; i++)
		{
			const char *current = action_list[i].actionname;
			bool obsolete = false;

			if (current[0] == '!')
			{
				current++;
				obsolete = true;
			}
    
			if (DDF_CompareName(current, action_name) == 0)
			{
				if (obsolete && !no_obsoletes)
					DDF_Warning("The ddf %s action is obsolete !\n", current);

				break;
			}
		}

		if (!action_list[i].actionname)
			DDF_WarnError("Unknown code pointer: %s\n", stateinfo[4]);
		else
		{
			cur->action = action_list[i].action;
			cur->action_par = NULL;

			if (action_list[i].handle_arg)
				(* action_list[i].handle_arg)(action_arg, cur);
		}
	}

	//--------------------------------------------------
	//---------------- MISC1 and MISC2 -----------------
	//--------------------------------------------------

	if ((stateinfo[5] || stateinfo[6]) && !no_obsoletes)
		DDF_WarnError("Misc1 and Misc2 are no longer used.\n");

	DestroyStateInfo();
}

//
// DDF_StateFinishStates
//
// Check through the states on an mobj and attempts to dereference any
// encoded state redirectors.
//
void DDF_StateFinishStates(int first, int last)
{
	int i;

	for (i=first; i <= last; i++)
	{
		// handle next state ref
		if (states[i].nextstate == -1)
		{
			states[i].nextstate = S_NULL;
		}
		else if ((states[i].nextstate >> 16) == 0)
		{
			states[i].nextstate = (i == last) ? S_NULL : i+1;
		}
		else
		{
			states[i].nextstate = StateFindLabel(first, last,
												 redirection_names[(states[i].nextstate >> 16) - 1]) +
				(states[i].nextstate & 0xFFFF);
		}

		// handle jump state ref
		if (states[i].jumpstate == -1)
		{
			states[i].jumpstate = S_NULL;
		}
		else if ((states[i].jumpstate >> 16) == 0)
		{
			states[i].jumpstate = (i == last) ? S_NULL : i+1;
		}
		else
		{
			states[i].jumpstate = StateFindLabel(first, last,
												 redirection_names[(states[i].jumpstate >> 16) - 1]) +
				(states[i].jumpstate & 0xFFFF);
		}
	}
  
	// FIXME: can free the redirection name list here
}

//
// DDF_StateGetAttack
//
// Parse the special argument for the state as an attack.
//
// -AJA- 1999/08/10: written.
//
void DDF_StateGetAttack(const char *arg, state_t * cur_state)
{
	if (!arg || !arg[0])
		return;

	cur_state->action_par = (void *)atkdefs.Lookup(arg);
	if (cur_state->action_par == NULL)
		DDF_WarnError2(0x128, "Unknown Attack (States): %s\n", arg);
}

//
// DDF_StateGetMobj
//
void DDF_StateGetMobj(const char *arg, state_t * cur_state)
{
	if (!arg || !arg[0])
		return;

	cur_state->action_par = (void *)mobjdefs.Lookup(arg);
}

//
// DDF_StateGetSound
//
void DDF_StateGetSound(const char *arg, state_t * cur_state)
{
	if (!arg || !arg[0])
		return;

	cur_state->action_par = (void *)DDF_SfxLookupSound(arg);
}

//
// DDF_StateGetInteger
//
void DDF_StateGetInteger(const char *arg, state_t * cur_state)
{
	int *value;

	if (!arg || !arg[0])
		return;

	value = Z_New(int, 1);

	if (sscanf(arg, " %i ", value) != 1)
		DDF_Error("DDF_StateGetInteger: bad value: %s\n", arg);

	cur_state->action_par = value;
}

//
// DDF_StateGetIntPair
//
// Parses two integers separated by commas.
//
void DDF_StateGetIntPair(const char *arg, state_t * cur_state)
{
	int *values;

	if (!arg || !arg[0])
		return;

	values = Z_New(int, 2);

	if (sscanf(arg, " %i , %i ", &values[0], &values[1]) != 2)
		DDF_Error("DDF_StateGetIntPair: bad values: %s\n", arg);

	cur_state->action_par = values;
}

//
// DDF_StateGetFloat
//
void DDF_StateGetFloat(const char *arg, state_t * cur_state)
{
	float *value;

	if (!arg || !arg[0])
		return;

	value = Z_New(float, 1);

	if (sscanf(arg, " %f ", value) != 1)
		DDF_Error("DDF_StateGetFloat: bad value: %s\n", arg);

	cur_state->action_par = value;
}

//
// DDF_StateGetPercent
//
void DDF_StateGetPercent(const char *arg, state_t * cur_state)
{
	percent_t *value;

	if (!arg || !arg[0])
		return;

	value = Z_New(percent_t, 1);

	if (sscanf(arg, " %f%% ", value) != 1 || (*value) < 0)
		DDF_Error("DDF_StateGetPercent: Bad percentage: %s\n", arg);

	(* value) /= 100.0f;

	cur_state->action_par = value;
}

//
// DDF_StateGetJump
//
void DDF_StateGetJump(const char *arg, state_t * cur_state)
{
	// JUMP(label)
	// JUMP(label,chance)
  
	const char *s;
	act_jump_info_t *jump;

	char buffer[80];
	int len;
	int offset = 0;

	if (!arg || !arg[0])
		return;

	jump = Z_New(act_jump_info_t, 1);
	jump->chance = 1.0f;                  // -ACB- 2001/02/04 tis a precent_t

	s = strchr(arg, ',');

	if (! s)
	{
		len = strlen(arg);
	}
	else
	{
		// convert chance value
		DDF_MainGetPercent(s+1, &jump->chance);
    
		len = s - arg;
	}

	if (len == 0)
		DDF_Error("DDF_StateGetJump: missing label !\n");

	if (len > 75)
		DDF_Error("DDF_StateGetJump: label name too long !\n");

	// copy label name
	for (len=0; *arg && (*arg != ':') && (*arg != ','); len++, arg++)
		buffer[len] = *arg;

	buffer[len] = 0;

	if (*arg == ':')
		offset = MAX(0, atoi(arg+1) - 1);

	// set the jump state   
	cur_state->jumpstate = ((StateGetRedirector(buffer) + 1) << 16) + offset;
	cur_state->action_par = jump;
}

//
// DDF_StateGetAngle
//
void DDF_StateGetAngle(const char *arg, state_t * cur_state)
{
	angle_t *value;
	float tmp;

	if (!arg || !arg[0])
		return;

	value = Z_New(angle_t, 1);

	if (sscanf(arg, " %f ", &tmp) != 1)
		DDF_Error("DDF_StateGetAngle: bad value: %s\n", arg);

	*value = FLOAT_2_ANG(tmp);

	cur_state->action_par = value;
}

//
// DDF_StateGetSlope
//
void DDF_StateGetSlope(const char *arg, state_t * cur_state)
{
	float *value, tmp;

	if (!arg || !arg[0])
		return;

	value = Z_New(float, 1);

	if (sscanf(arg, " %f ", &tmp) != 1)
		DDF_Error("DDF_StateGetSlope: bad value: %s\n", arg);

	if (tmp > +89.5f)
		tmp = +89.5f;
	if (tmp < -89.5f)
		tmp = -89.5f;

	*value = M_Tan(FLOAT_2_ANG(tmp));

	cur_state->action_par = value;
}

bool DDF_CheckSprites(int st_low, int st_high)
{
	if (st_low == S_NULL)
		return true;
	
	DEV_ASSERT2(st_low <= st_high);

	while (st_low <= st_high &&	states[st_low].sprite == SPR_NULL)
		st_low++;

	if (st_low > st_high)
		return true;

	if (sprites[states[st_low].sprite].frames > 0)
		return true;	

	return false;
}

