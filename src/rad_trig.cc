//----------------------------------------------------------------------------
//  EDGE Radius Trigger / Tip Code
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2009  The EDGE Team.
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
//
// -KM- 1998/11/25 Fixed problems created by DDF.
//   Radius Triggers can be added to wad files.  RSCRIPT is the lump.
//   Tip function can handle graphics.
//   New functions: ondeath, #version
//   Radius Triggers with radius < 0 affect entire map.
//   Radius triggers used to save compatibility with hacks in Doom/Doom2 
//       (eg MAP07, E2M8, E3M8, MAP32 etc..)
//
// -AJA- 1999/10/23: Began work on a state model for RTS actions.
//
// -AJA- 1999/10/24: Split off actions into rad_act.c, and structures
//       into the rad_main.h file.
//
// -AJA- 2000/01/04: Split off parsing code into rad_pars.c.
//

#include "i_defs.h"

#include "epi/file.h"
#include "epi/filesystem.h"

#include "ddf/language.h"

#include "am_map.h"
#include "e_input.h"
#include "e_keys.h"
#include "e_main.h"
#include "g_state.h"
#include "hu_draw.h"
#include "hu_style.h"
#include "hu_stuff.h"
#include "g_game.h"
#include "m_argv.h"
#include "m_misc.h"
#include "m_random.h"
#include "p_local.h"
#include "p_spec.h"
#include "rad_trig.h"
#include "rad_act.h"
#include "r_defs.h"
#include "r_colormap.h"
#include "hu_draw.h"
#include "r_modes.h"
#include "s_sound.h"
#include "w_wad.h"
#include "z_zone.h"


#define MAXRTSLINE  2048


// Static Scripts.  Never change once all scripts have been read in.
rad_script_t *r_scripts = NULL;

// Dynamic Triggers.  These only exist for the current level.
rad_trigger_t *active_triggers = NULL;


class rts_menu_c
{
private:
	static const int MAX_TITLE  = 24;
	static const int MAX_CHOICE = 9;

	rad_trigger_t *trigger;
	style_c *style;

	int title_num;
	char title_lines[MAX_TITLE][TIP_CHAR_MAX];

	int choice_num;
	char choice_lines[MAX_CHOICE][TIP_CHAR_MAX];

public:
	rts_menu_c(s_show_menu_t *menu, rad_trigger_t *_trigger, style_c *_style) :
		trigger(_trigger), style(_style), title_num(0), choice_num(0)
	{
		int y = 0;

		memset(title_lines, 0, sizeof(title_lines));
		memset(choice_lines, 0, sizeof(choice_lines));

		AddTitle(&y, menu->title, menu->use_ldf);

		bool no_choices = (! menu->options[0] || ! menu->options[1]);

		for (int idx = 0; (idx < 9) && menu->options[idx]; idx++)
			AddChoice(&y, no_choices ? 0 : ('1' + idx), menu->options[idx], menu->use_ldf);

///!!!		// center the menu vertically
///!!!		int adjust_y = (200 - y) / 2;
///!!!
///!!!		for (int t = 0; t < title_num; t++)
///!!!			title_lines[t].y += adjust_y;
///!!!
///!!!		for (int c = 0; c < choice_num; c++)
///!!!			choice_lines[c].y += adjust_y;
	}

	~rts_menu_c() { /* nothing to do */ }

private:
	void AddTitle(int *y, const char *text, bool use_ldf)
	{
		if (use_ldf)
			text = language[text];

		title_num = 0;

		int font_h = 10; /// FIXME remove

		for (int t = 0; t < MAX_TITLE; t++)
		{
			if (! *text)
				break;

			title_num = t + 1;

///			HL_InitTextLine(title_lines + t, 160, *y, style, t_type);
///			title_lines[t].centre = true;

			int pos = 0;

			for (; *text && *text != '\n'; text++)
			{
				title_lines[t][pos++] = *text;
				title_lines[t][pos] = 0;
			}

			if (*text == '\n')
				*text++;

			(*y) += font_h;
		}

		// spacing between two halves (make it changeable ???)
		(*y) += font_h;
	}

	void AddChoice(int *y, char key, const char *text, bool use_ldf)
	{
		if (use_ldf)
			text = language[text];

		int c = choice_num;
		choice_num++;

		int font_h = 10; /// FIXME remove

///		HL_InitTextLine(choice_lines + c, 160, *y, style, t_type);
///		choice_lines[c].centre = true;

		int pos = 0;

		if (key)
		{
			choice_lines[c][pos++] = key;
			choice_lines[c][pos++] = '.';
			choice_lines[c][pos++] = ' ';

			choice_lines[c][pos] = 0;
		}

		for (; *text && *text != '\n'; text++)
		{
			choice_lines[c][pos++] = *text;
			choice_lines[c][pos] = 0;
		}

		(*y) += font_h;
	}

public:
	int NumChoices() const { return choice_num; }

	void NotifyResult(int result)
	{
		trigger->menu_result = result;
	}

	void Drawer()
	{
		HUD_Reset();

		HUD_SetAlpha(0.64f);
		HUD_SolidBox(0, 0, 320, 200, T_BLACK);
		HUD_Reset("a");

		HUD_SetTextColor(T_WHITE);  // FIXME

		float font_h = HUD_FontHeight();

		// FIXME: !!!!!! POSITION
		for (int t = 0; t < title_num; t++)
			HUD_DrawText(0, t*font_h, title_lines[t]);

		HUD_SetTextColor(T_LTBLUE);  // FIXME

		for (int c = 0; c < choice_num; c++)
			HUD_DrawText(0, (title_num+1+c)*font_h, choice_lines[c]);
	}

	int CheckKey(int key)
	{
		if ('a' <= key && key <= 'z')
			key = toupper(key);

		if (key == 'Q' || key == 'X')  // cancel
			return 0;

		if ('1' <= key && key <= ('0' + choice_num))
			return key - '0';

		if (choice_num < 2 &&
			(key == KEYD_SPACE || key == KEYD_ENTER || key == 'Y'))
			return 1;

		return -1;  /* invalid */
	}
};

// RTS menu active ?
bool rts_menuactive = false;
static rts_menu_c *rts_curr_menu = NULL;

// Current RTS file or lump being parsed.
static byte *rad_memfile;
static byte *rad_memfile_end;
static byte *rad_memptr;
static int rad_memfile_size;


rad_script_t * RAD_FindScriptByName(const char *map_name, const char *name)
{
	rad_script_t *scr;

	for (scr=r_scripts; scr; scr=scr->next)
	{
		if (scr->script_name == NULL)
			continue;

		if (strcmp(scr->mapid, map_name) != 0)
			continue;

		if (DDF_CompareName(scr->script_name, name) == 0)
			return scr;
	}

	I_Error("RTS: No such script `%s' on map %s.\n", name, map_name);
	return NULL;
}


rad_trigger_t * RAD_FindTriggerByName(const char *name)
{
	rad_trigger_t *trig;

	for (trig=active_triggers; trig; trig=trig->next)
	{
		if (trig->info->script_name == NULL)
			continue;

		if (DDF_CompareName(trig->info->script_name, name) == 0)
			return trig;
	}

	I_Warning("RTS: No such trigger `%s'.\n", name);
	return NULL;
}


rad_trigger_t * RAD_FindTriggerByScript(const rad_script_t *scr)
{
	rad_trigger_t *trig;

	for (trig=active_triggers; trig; trig=trig->next)
	{
		if (trig->info == scr)
			return trig;
	}

	return NULL;  // no worries if none.
}


rts_state_t * RAD_FindStateByLabel(rad_script_t *scr, char *label)
{
	rts_state_t *st;

	for (st=scr->first_state; st; st=st->next)
	{
		if (st->label == NULL)
			continue;

		if (DDF_CompareName(st->label, label) == 0)
			return st;
	}

	// NOTE: no error message, unlike the other Find funcs
	return NULL;
}

//
// Looks for all current triggers with the given tag number, and
// either enables them or disables them (based on `disable').
// Actor can be NULL.
//
void RAD_EnableByTag(mobj_t *actor, int tag, bool disable)
{
	rad_trigger_t *trig;

	if (tag <= 0)
		I_Error("INTERNAL ERROR: RAD_EnableByTag: bad tag %d\n", tag);

	for (trig=active_triggers; trig; trig=trig->next)
	{
		if (trig->info->tag == tag)
			break;
	}

	// were there any ?
	if (! trig)
		return;

	for (; trig; trig=trig->tag_next)
	{
		if (disable)
			trig->disabled = true;
		else
			trig->disabled = false;
	}
}

bool RAD_WithinRadius(mobj_t * mo, rad_script_t * r)
{
	if (r->rad_x >= 0 && fabs(r->x - mo->x) > r->rad_x + mo->radius)
		return false;

	if (r->rad_y >= 0 && fabs(r->y - mo->y) > r->rad_y + mo->radius)
		return false;

	if (r->rad_z >= 0 && fabs(r->z - MO_MIDZ(mo)) > r->rad_z + mo->height/2)
	{
		return false;
	}

	return true;
}


static int RAD_AlivePlayers(void)
{
	int result = 0;

	for (int pnum = 0; pnum < MAXPLAYERS; pnum++)
	{
		player_t *p = players[pnum];

		if (p && p->playerstate != PST_DEAD)
			result |= (1 << pnum);
	}

	return result;
}

static int RAD_AllPlayersInRadius(rad_script_t * r, int mask)
{
	int result = 0;

	for (int pnum = 0; pnum < MAXPLAYERS; pnum++)
	{
		player_t *p = players[pnum];

		if (p && (mask & (1 << pnum)) && RAD_WithinRadius(p->mo, r))
			result |= (1 << pnum);
	}

	return result;
}

static int RAD_AllPlayersUsing(int mask)
{
	int result = 0;

	for (int pnum = 0; pnum < MAXPLAYERS; pnum++)
	{
		player_t *p = players[pnum];

		if (p && p->usedown)
			result |= (1 << pnum);
	}

	return result & mask;
}

static int RAD_AllPlayersCheckCond(rad_script_t * r, int mask)
{
	int result = 0;

	for (int pnum = 0; pnum < MAXPLAYERS; pnum++)
	{
		player_t *p = players[pnum];

		if (p && (mask & (1 << pnum)) && G_CheckConditions(p->mo, r->cond_trig))
			result |= (1 << pnum);
	}

	return result;
}


static bool RAD_CheckBossTrig(rad_trigger_t *trig, s_ondeath_t *cond)
{
	mobj_t *mo;

	int count = 0;

	// lookup thing type if we haven't already done so
	if (! cond->cached_info)
	{
		if (cond->thing_name)
			cond->cached_info = mobjtypes.Lookup(cond->thing_name);
		else
		{
			cond->cached_info = mobjtypes.Lookup(cond->thing_type);

			if (cond->cached_info == NULL)
				I_Error("RTS ONDEATH: Unknown thing type %d.\n",
						cond->thing_type);
		}
	}

	// scan the remaining mobjs to see if all bosses are dead
	for (mo=mobjlisthead; mo != NULL; mo=mo->next)
	{
		if (mo->info == cond->cached_info && mo->health > 0)
		{
			count++;

			if (count > cond->threshhold)
				return false;
		}
	}

	return true;
}


static bool RAD_CheckHeightTrig(rad_trigger_t *trig, 
		s_onheight_t *cond)
{
	float h;

	// lookup sector if we haven't already done so
	if (! cond->cached_sector)
	{
		if (cond->sec_num >= 0)
		{
			if (cond->sec_num >= numsectors)
				I_Error("RTS ONHEIGHT: no such sector %d.\n", cond->sec_num);

			cond->cached_sector = & sectors[cond->sec_num];
		}
		else
		{
			cond->cached_sector = R_PointInSubsector(trig->info->x, 
					trig->info->y)->sector;
		}
	}

	if (cond->is_ceil)
		h = cond->cached_sector->c_h;
	else
		h = cond->cached_sector->f_h;

	return (cond->z1 <= h && h <= cond->z2);
}

bool RAD_CheckReachedTrigger(mobj_t * thing)
{
	rad_script_t * scr = (rad_script_t *) thing->path_trigger;
	rad_trigger_t * trig;

	rts_path_t *path;
	int choice;

	if (! RAD_WithinRadius(thing, scr))
		return false;

	// Thing has reached this path node. Update so it starts following
	// the next node.  Handle any PATH_EVENT too.  Enable the associated
	// trigger (could be none if there were no states).

	trig = RAD_FindTriggerByScript(scr);

	if (trig)
		trig->disabled = false;

	if (scr->path_event_label)
	{
		int state = P_MobjFindLabel(thing, scr->path_event_label);

		state += scr->path_event_offset;

		// verify new state number
		if (state >= (int)thing->info->states.size())
			state =  (int)thing->info->states.size()-1;
		else if (state < 1)
			state = 1;

		if (state)
			P_SetMobjStateDeferred(thing, state, 0);
	}

	if (scr->next_path_total == 0)
	{
		thing->path_trigger = NULL;
		return true;
	}
	else if (scr->next_path_total == 1)
		choice = 0;
	else
		choice = P_Random() % scr->next_path_total;

	path = scr->next_in_path;
	SYS_ASSERT(path);

	for (; choice > 0; choice--)
	{
		path = path->next;
		SYS_ASSERT(path);
	}

	if (! path->cached_scr)
		path->cached_scr = RAD_FindScriptByName(scr->mapid, path->name);

	SYS_ASSERT(path->cached_scr);

	thing->path_trigger = path->cached_scr;
	return true;
}

static void DoRemoveTrigger(rad_trigger_t *trig)
{
	// handle tag linkage
	if (trig->tag_next)
		trig->tag_next->tag_prev = trig->tag_prev;

	if (trig->tag_prev)
		trig->tag_prev->tag_next = trig->tag_next;

	// unlink and free it
	if (trig->next)
		trig->next->prev = trig->prev;

	if (trig->prev)
		trig->prev->next = trig->next;
	else
		active_triggers = trig->next;

    S_StopFX(&trig->sfx_origin);

	// FIXME: delete !
    Z_Free(trig);
}

//
// Radius Trigger Event handler.
//
void RAD_RunTriggers(void)
{
	rad_trigger_t *trig, *next;

	// Start looking through the trigger list.
	for (trig=active_triggers; trig; trig=next)
	{
		next = trig->next;

		// stop running all triggers when an RTS menu becomes active
		if (rts_menuactive)
			break;

		// Don't process, if disabled
		if (trig->disabled)
			continue;

		// Handle repeat delay (from TAGGED_REPEATABLE).  This must be
		// done *before* all the condition checks, and that's what makes
		// it different from `wait_tics'.
		//
		if (trig->repeat_delay > 0)
		{
			trig->repeat_delay--;
			continue;
		}

		// Independent, means you don't have to stay within the trigger
		// radius for it to operate, It will operate on it's own.

		if (! (trig->info->tagged_independent && trig->activated))
		{
			int mask = RAD_AlivePlayers();

			// Immediate triggers are just that. Immediate.
			// Not within range so skip it.

			if (!trig->info->tagged_immediate)
			{
				mask = RAD_AllPlayersInRadius(trig->info, mask);
				if (mask == 0)
					continue;
			}

			// Check for use key trigger.
			if (trig->info->tagged_use)
			{
				mask = RAD_AllPlayersUsing(mask);
				if (mask == 0)
					continue;
			}

			// height check...
			if (trig->info->height_trig)
			{
				s_onheight_t *cur;

				for (cur=trig->info->height_trig; cur; cur=cur->next)
					if (! RAD_CheckHeightTrig(trig, cur))
						break;

				// if they all succeeded, then cur will be NULL...
				if (cur)
					continue;
			}

			// ondeath check...
			if (trig->info->boss_trig)
			{
				s_ondeath_t *cur;

				for (cur=trig->info->boss_trig; cur; cur=cur->next)
					if (! RAD_CheckBossTrig(trig, cur))
						break;

				// if they all succeeded, then cur will be NULL...
				if (cur)
					continue;
			}

			// condition check...
			if (trig->info->cond_trig)
			{
				mask = RAD_AllPlayersCheckCond(trig->info, mask);
				if (mask == 0)
					continue;
			}

			trig->activated = true;
			trig->acti_players = mask;
		}

		// If we are waiting, decrement count and skip it.
		// Note that we must do this *after* all the condition checks.
		//
		if (trig->wait_tics > 0)
		{
			trig->wait_tics--;
			continue;
		}

		// Execute the commands
		while (trig->wait_tics == 0)
		{
			rts_state_t *state = trig->state;

			// move to next state.  We do this NOW since the action itself
			// may want to change the trigger's state (to support GOTO type
			// actions and other possibilities).
			//
			trig->state = trig->state->next;

			(*state->action)(trig, state->param);

			if (trig->state == NULL)
				break;

			trig->wait_tics += trig->state->tics;

			if (trig->disabled || rts_menuactive)
				break;
		}

		if (trig->state)
			continue;

		// we've reached the end of the states.  Delete the trigger unless
		// it is Tagged_Repeatable and has some more repeats left.
		//
		if (trig->info->repeat_count != REPEAT_FOREVER)
			trig->repeats_left--;

		if (trig->repeats_left > 0)
		{
			trig->state = trig->info->first_state;
			trig->wait_tics = trig->state->tics;
			trig->repeat_delay = trig->info->repeat_delay;
			continue;
		}

		DoRemoveTrigger(trig);
	}
}


//
// Called from RAD_SpawnTriggers to set the tag_next & tag_prev fields
// of each rad_trigger_t, keeping all triggers with the same tag in a
// linked list for faster handling.
//
void RAD_GroupTriggerTags(rad_trigger_t *trig)
{
	rad_trigger_t *cur;

	trig->tag_next = trig->tag_prev = NULL;

	// find first trigger with the same tag #
	for (cur=active_triggers; cur; cur=cur->next)
	{
		if (cur == trig)
			continue;

		if (cur->info->tag == trig->info->tag)
			break;
	}

	if (! cur)
		return;

	// link it in

	trig->tag_next = cur;
	trig->tag_prev = cur->tag_prev;

	if (cur->tag_prev)
		cur->tag_prev->tag_next = trig;

	cur->tag_prev = trig;
}


void RAD_SpawnTriggers(const char *map_name)
{
	rad_script_t *scr;
	rad_trigger_t *trig;

#ifdef DEVELOPERS
	if (active_triggers)
		I_Error("RAD_SpawnTriggers without RAD_ClearTriggers\n");
#endif

	for (scr=r_scripts; scr; scr=scr->next)
	{
		// This is from a different map!
		if (strcmp(map_name, scr->mapid) != 0 && strcmp(scr->mapid, "ALL") != 0)
			continue;

		// -AJA- 1999/09/25: Added skill checks.
		if (! G_CheckWhenAppear(scr->appear))
			continue;

		// -AJA- 2000/02/03: Added player num checks.
		if (numplayers < scr->min_players || numplayers > scr->max_players)
			continue;

		// ignore empty scripts (e.g. path nodes)
		if (! scr->first_state)
			continue;

		// OK, spawn new dynamic trigger
		trig = Z_New(rad_trigger_t, 1);

		Z_Clear(trig, rad_trigger_t, 1);

		trig->info = scr;
		trig->disabled = scr->tagged_disabled;
		trig->repeats_left = (scr->repeat_count < 0 || 
				scr->repeat_count == REPEAT_FOREVER) ? 1 : scr->repeat_count;
		trig->repeat_delay = 0;
		trig->tip_slot = 0;

		RAD_GroupTriggerTags(trig);

		// initialise state machine
		trig->state = scr->first_state;
		trig->wait_tics = scr->first_state->tics;

		// link it in
		trig->next = active_triggers;
		trig->prev = NULL;

		if (active_triggers)
			active_triggers->prev = trig;

		active_triggers = trig;
	}
}


static void RAD_ClearCachedInfo(void)
{
	rad_script_t *scr;
	s_ondeath_t *d_cur;
	s_onheight_t *h_cur;

	for (scr=r_scripts; scr; scr=scr->next)
	{
		// clear ONDEATH cached info
		for (d_cur=scr->boss_trig; d_cur; d_cur=d_cur->next)
		{
			d_cur->cached_info = NULL;
		}

		// clear ONHEIGHT cached info
		for (h_cur=scr->height_trig; h_cur; h_cur=h_cur->next)
		{
			h_cur->cached_sector = NULL;
		}
	}
}


void RAD_ClearTriggers(void)
{
	// remove all dynamic triggers
	while (active_triggers)
	{
		rad_trigger_t *trig = active_triggers;
		active_triggers = trig->next;

		Z_Free(trig);
	}

	RAD_ClearCachedInfo();
	RAD_ResetTips();
}


static int ReadScriptLine(char *buf, int max)
{
	int real_num = 1;

	while (rad_memptr < rad_memfile_end)
	{
		if (rad_memptr[0] == '\n')
		{
			// skip trailing EOLN
			rad_memptr++;
			break;
		}

		// line concatenation
		if (rad_memptr+2 < rad_memfile_end && rad_memptr[0] == '\\')
		{
			if (rad_memptr[1] == '\n' ||
			    (rad_memptr[1] == '\r' && rad_memptr[2] == '\n'))
			{
				real_num++;
				rad_memptr += (rad_memptr[1] == '\n') ? 2 : 3;
				continue;
			}
		}

		// ignore carriage returns
		if (rad_memptr[0] == '\r')
		{
			rad_memptr++;
			continue;
		}

		if (max <= 2)
			I_Error("RTS script: line %d too long !!\n", rad_cur_linenum);

		*buf++ = *rad_memptr++;  max--;
	}

	*buf = 0;

	return real_num;
}


void RAD_Parse(void *data, int size, const char *name)
{
	// NOTE WELL: we assume buffer is NUL terminated !
	SYS_ASSERT(data);

	L_WriteDebug("RTS: Loading %s (size=%d)\n", name, size);

	rad_cur_filename = name;
	rad_cur_linenum = 1;

	rad_memfile_size = size;
	rad_memfile = (byte *)data;
	rad_memfile_end = &rad_memfile[size];

	// OK we have the file in memory.  Parse it to death :-)
	RAD_ParserBegin();

	rad_memptr = rad_memfile;

	char linebuf[MAXRTSLINE];

	while (rad_memptr < rad_memfile_end)
	{
		int real_num = ReadScriptLine(linebuf, MAXRTSLINE);

#if (DEBUG_RTS)
		L_WriteDebug("RTS LINE: '%s'\n", linebuf);
#endif

		RAD_ParseLine(linebuf);

		rad_cur_linenum += real_num;
	}

	RAD_ParserDone();
}


void RAD_Init(void)
{
	RAD_InitTips();
}

void RAD_StartMenu(rad_trigger_t *R, s_show_menu_t *menu)
{
	SYS_ASSERT(! rts_menuactive);

	// find the right style
	styledef_c *def = NULL;

	if (R->menu_style_name)
		def = styledefs.Lookup(R->menu_style_name);

	if (! def) def = styledefs.Lookup("RTS MENU");
	if (! def) def = styledefs.Lookup("MENU");
	if (! def) def = default_style;

	rts_curr_menu = new rts_menu_c(menu, R, HU_LookupStyle(def));
	rts_menuactive = true;
}

void RAD_FinishMenu(int result)
{
	if (! rts_menuactive)
		return;
	
	SYS_ASSERT(rts_curr_menu);

	// zero is cancelled, otherwise result is 1..N
	if (result < 0 || result > MAX(1, rts_curr_menu->NumChoices()))
		return;

	rts_curr_menu->NotifyResult(result);

	delete rts_curr_menu;

	rts_curr_menu = NULL;
	rts_menuactive = false;
}

static void RAD_MenuDrawer(void)
{
	SYS_ASSERT(rts_curr_menu);

	rts_curr_menu->Drawer();
}


void RAD_Drawer(void)
{
	if (! automapactive)
		RAD_DisplayTips();
	
	if (rts_menuactive)
		RAD_MenuDrawer();
}


bool RAD_Responder(event_t * ev)
{
	if (ev->type != ev_keydown)
		return false;

	if (! rts_menuactive)
		return false;
		
	SYS_ASSERT(rts_curr_menu);

	int check = rts_curr_menu->CheckKey(ev->value.key.sym);

	if (check >= 0)
	{
		RAD_FinishMenu(check);
		return true;
	}

	return false;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
