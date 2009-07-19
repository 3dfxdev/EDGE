//----------------------------------------------------------------------------
// EDGE Finale Code on Game Completion
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2008  The EDGE Team.
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
//  Based on the DOOM source code, released by Id Software under the
//  following copyright:
//
//    Copyright (C) 1993-1996 by id Software, Inc.
//
//----------------------------------------------------------------------------
//
// -KM- 1998/07/21 Clear the background behind those end pics.
// -KM- 1998/09/27 sounds.ddf stuff: seesound -> DDF_LookupSound(seesound)
// -KM- 1998/11/25 Finale generalised.
//

#include "i_defs.h"
#include "i_defs_gl.h"

#include "ddf/thing.h"
#include "ddf/attack.h"
#include "ddf/language.h"

#include "e_input.h"
#include "g_state.h"
#include "g_game.h"
#include "m_strings.h"
#include "e_main.h"
#include "f_finale.h"
#include "f_interm.h"
#include "hu_lib.h"
#include "hu_stuff.h"
#include "hu_style.h"
#include "m_menu.h"
#include "m_random.h"
#include "p_action.h"
#include "am_map.h"
#include "r_colormap.h"
#include "r_draw.h"
#include "r_md2.h"
#include "r_modes.h"
#include "r_state.h"
#include "r_gldefs.h"
#include "s_sound.h"
#include "s_music.h"
#include "w_wad.h"
#include "w_model.h"


typedef enum
{
	f_text,
	f_pic,
	f_bunny,
	f_cast,
	f_done
}
finalestage_e;

void operator++ (finalestage_e& f, int blah)
{
	f = (finalestage_e)(f + 1);
}

// Stage of animation:
//  0 = text, 1 = art screen, 2 = character cast
static finalestage_e finalestage;

// -ES- 2000/03/11 skip to the next finale stage
static bool skip_finale;
static int finalecount;
static int picnum;

#define TEXTSPEED 3
#define TEXTWAIT  250

static const char *finaletext;

static gameaction_e newgameaction;
static const map_finaledef_c *finale;
static rgbcol_t finale_textcol;

static void CastInitNew(int num);
static void CastTicker(void);
static void CastSkip(void);

static const image_c *finale_textback;
static float finale_textbackscale;

static style_c *cast_style;
static style_c *finale_hack_style;


static bool HasFinale(const map_finaledef_c *F, finalestage_e cur)
{
	SYS_ASSERT(F);

	switch (cur)
	{
		case f_text:
			return ! F->text.empty();

		case f_pic:
			return (F->pics.GetSize() > 0);

		case f_bunny:
			return F->dobunny;

		case f_cast:
			return F->docast;

		default:
			I_Error("Bad parameter passed to HasFinale().\n");
	}

	return false; /* NOT REACHED */
}

// returns f_done if nothing found
static finalestage_e FindValidFinale(const map_finaledef_c *F, finalestage_e cur)
{
	SYS_ASSERT(F);

	for (; cur != f_done; cur++)
	{
		if (HasFinale(F, cur))
			return cur;
	}

	return f_done;
}

static void DoStartFinale(void)
{
	finalecount = 0;

	switch (finalestage)
	{
		case f_text:
			finaletext = language[finale->text.c_str()];
			S_ChangeMusic(finale->music, true);
			break;

		case f_pic:
			picnum = 0;
			break;

		case f_bunny:
			if (currmap->episode)
				S_ChangeMusic(currmap->episode->special_music, true);
			break;

		case f_cast:
			CastInitNew(2);
			if (currmap->episode)
				S_ChangeMusic(currmap->episode->special_music, true);
			break;

		default:
			I_Error("DoStartFinale: bad stage #%d\n", (int)finalestage);
			break;
	}

	for (int pnum = 0; pnum < MAXPLAYERS; pnum++)
		if (players[pnum])
			players[pnum]->cmd.buttons = 0;
}

static void DoBumpFinale(void)
{
	// find next valid Finale stage
	finalestage_e stage = finalestage;
	stage++;
	stage = FindValidFinale(finale, stage);

	if (stage != f_done)
	{
		if (gamestate != GS_INTERMISSION)
			E_ForceWipe();

		finalestage = stage;

		DoStartFinale();
		return;
	}

	// capture the screen _before_ changing any global state
	if (newgameaction != ga_nothing)
	{
		E_ForceWipe();
		gameaction = newgameaction;
	}

	gamestate = GS_NOTHING;  // hack ???  (cannot leave as GS_FINALE)
}

static void LookupFinaleStuff(void)
{
	// here is where we lookup the required images
	if (! cast_style)
	{
		styledef_c *def = styledefs.Lookup("CAST SCREEN");
		if (! def)
			def = default_style;
		cast_style = HU_LookupStyle(def);
	}

	if (! finale_hack_style)
		finale_hack_style = HU_LookupStyle(default_style); //???

	if (finale->text_flat[0])
	{
		finale_textback = W_ImageLookup(finale->text_flat, INS_Flat);
		finale_textbackscale = 5.0f;
	}
	else if (finale->text_back[0])
	{
		finale_textback = W_ImageLookup(finale->text_back, INS_Graphic);
		finale_textbackscale = 1.0f;
	}
	else
	{
		finale_textback = NULL;
	}

	// text color
	finale_textcol = finale->text_rgb;

	if (finale->text_colmap)
		finale_textcol = V_GetFontColor(finale->text_colmap);
}


void F_StartFinale(const map_finaledef_c *F, gameaction_e newaction)
{
	SYS_ASSERT(F);

	newgameaction = newaction;
	automapactive = false;

	finalestage_e stage = FindValidFinale(F, f_text);

	if (stage == f_done)
	{
		if (newgameaction != ga_nothing)
			gameaction = newgameaction;

		return /* false */;
	}

	// capture the screen _before_ changing any global state
	//--- E_ForceWipe();   // CRASH with IDCLEV

	finale = F;
	finalestage = stage;

	LookupFinaleStuff();

	gamestate = GS_FINALE;

	DoStartFinale();
}


bool F_Responder(event_t * event)
{
	SYS_ASSERT(gamestate == GS_FINALE);

	// FIXME: use WI_CheckAccelerate() in netgames
	if (event->type != ev_keydown)
		return false;

	if (finalecount > TICRATE)
	{
		skip_finale = true;
		return true;
	}

	return false;
}


void F_Ticker(void)
{
	SYS_ASSERT(gamestate == GS_FINALE);

	// advance animation
	finalecount++;

	switch (finalestage)
	{
		case f_text:
			if (skip_finale && finalecount < (int)strlen(finaletext) * TEXTSPEED)
			{
				finalecount = TEXTSPEED * strlen(finaletext);
				skip_finale = false;
			}
			else if (skip_finale || finalecount > TEXTWAIT + (int)strlen(finaletext) * TEXTSPEED)
			{
				DoBumpFinale();
				skip_finale = false;
			}
			break;

		case f_pic:
			if (skip_finale || finalecount > (int)finale->picwait)
			{
				picnum++;
				finalecount = 0;
				skip_finale = false;
			}
			if (picnum >= finale->pics.GetSize())
			{
				DoBumpFinale();
			}
			break;

		case f_bunny:
			if (skip_finale && finalecount < 1100)
			{
				finalecount = 1100;
				skip_finale = false;
			}
			break;

		case f_cast:
			if (skip_finale)
			{
				CastSkip();
				skip_finale = false;
			}
			else
				CastTicker();

			break;

		default:
			I_Error("F_Ticker: bad finalestage #%d\n", (int)finalestage);
			break;
	}


	if (finalestage == f_done)
	{
		if (newgameaction != ga_nothing)
		{
			gameaction = newgameaction;

			// don't come here again (for E_ForceWipe)
			newgameaction = ga_nothing;

			if (gamestate == GS_FINALE)
				E_ForceWipe();
		}
	}
}


static void TextWrite(void)
{
	// 98-7-10 KM erase the entire screen to a tiled background
	if (finale_textback)
	{
		RGL_DrawImage(0, 0, SCREENWIDTH, SCREENHEIGHT, finale_textback,
				0.0f, 0.0f, IM_RIGHT(finale_textback) * finale_textbackscale,
				IM_TOP(finale_textback) * finale_textbackscale);
	}
	
	// draw some of the text onto the screen
	int cx = 10;
	int cy = 10;

	const char *ch = finaletext;

	int count = (int) ((finalecount - 10) / finale->text_speed);
	if (count < 0)
		count = 0;

	hu_textline_t L;

	SYS_ASSERT(finale_hack_style);

	HL_InitTextLine(&L, cx, cy, finale_hack_style, 0);

	for (;;)
	{
		SYS_ASSERT(finale);

		if (count == 0 || *ch == 0)
		{
			HL_DrawTextLineAlpha(&L, false, finale_textcol, 1.0f);
			break;
		}

		int c = *ch++; count--;

		if (c == '\n')
		{
			cy += 11;
			HL_DrawTextLineAlpha(&L, false, finale_textcol, 1.0f);
			HL_InitTextLine(&L, cx, cy, finale_hack_style, 0);
			continue;
		}

		HL_AddCharToTextLine(&L, c);
	}
}


//
// Final DOOM 2 animation
// Casting by id Software.
//   in order of appearance
//

static const mobjtype_c *cast_info;
static const char *cast_title;
static const state_t *cast_state;
static int cast_tics;
static bool cast_death;
static int cast_frames;
static int cast_onmelee;
static bool cast_attacking;

//
// CastSetState, CastPerformAction
// 
// -AJA- 2001/05/28: separated this out from CastTicker
// 
static void CastSetState(int index)
{
	SYS_ASSERT(index < (int)cast_info->states.size());

	// we just ignore the #REMOVE directive
	if (index == S_NULL)
	{
		cast_tics = 15;
		return;
	}

	cast_state = &cast_info->states[index];

	cast_tics = cast_state->tics;
	if (cast_tics < 0)
		cast_tics = 15;
}

static void CAST_RangeAttack(const atkdef_c *range)
{
	sfx_t *sfx = NULL;

	SYS_ASSERT(range);

	if (range->attackstyle == ATK_SHOT)
	{
		sfx = range->sound;
	}
	else if (range->attackstyle == ATK_SKULLFLY)
	{
		sfx = range->initsound;
	}
	else if (range->attackstyle == ATK_SPAWNER)
	{
		if (range->spawnedobj && range->spawnedobj->rangeattack)
			sfx = range->spawnedobj->rangeattack->initsound;
	}
	else if (range->attackstyle == ATK_TRACKER)
	{
		sfx = range->initsound;
	}
	else if (range->atk_mobj)
	{
		sfx = range->atk_mobj->seesound;
	}

	S_StartFX(sfx);
}

static void CastPerformAction(void)
{
	sfx_t *sfx = NULL;

	// Yuk, handles sounds

	if (cast_state->action == A_MakeCloseAttemptSound)
	{
		if (cast_info->closecombat)
			sfx = cast_info->closecombat->initsound;
	}
	else if (cast_state->action == A_MeleeAttack)
	{
		if (cast_info->closecombat)
			sfx = cast_info->closecombat->sound;
	}
	else if (cast_state->action == A_MakeRangeAttemptSound)
	{
		if (cast_info->rangeattack)
			sfx = cast_info->rangeattack->initsound;
	}
	else if (cast_state->action == A_RangeAttack)
	{
		if (cast_info->rangeattack)
			CAST_RangeAttack(cast_info->rangeattack);
	}
	else if (cast_state->action == A_ComboAttack)
	{
		if (cast_onmelee && cast_info->closecombat)
		{
			sfx = cast_info->closecombat->sound;
		}
		else if (cast_info->rangeattack)
		{
			CAST_RangeAttack(cast_info->rangeattack);
		}
	}
	else if (cast_info->activesound && (M_Random() < 2) && !cast_death)
	{
		sfx = cast_info->activesound;
	}
	else if (cast_state->action == A_WalkSoundChase)
	{
		sfx = cast_info->walksound;
	}

	S_StartFX(sfx);
}

static void CastInitNew(int num)
{
	cast_info = mobjtypes.LookupCastMember(num);

	// FIXME!!! Better handling of the finale
	if (!cast_info)
		cast_info = mobjtypes[0];

	cast_title = cast_info->cast_title.empty() ?
		cast_info->ddf.name.c_str() :
		language[cast_info->cast_title.c_str()];

	cast_death = false;
	cast_frames = 0;
	cast_onmelee = 0;
	cast_attacking = false;

	SYS_ASSERT(cast_info->chase_state);  // checked in ddf_mobj.c
	CastSetState(cast_info->chase_state);
}


//
// -KM- 1998/10/29 Use sfx_t.
//      Known bug: Chaingun/Spiderdemon's sounds aren't stopped.
//
static void CastTicker(void)
{
	// time to change state yet ?
	cast_tics--;
	if (cast_tics > 0)
		return;

	// switch from deathstate to next monster
	if (cast_state->tics == -1 || cast_state->nextstate == S_NULL ||
			(cast_death && cast_frames >= 30))
	{
		CastInitNew(cast_info->castorder + 1);

		if (cast_info->seesound)
			S_StartFX(cast_info->seesound);

		return;
	}

	CastPerformAction();

	// advance to next state in animation
	// -AJA- if there's a jumpstate, enter it occasionally
	int st;

	if (cast_state->action == A_Jump && cast_state->jumpstate &&
		(M_Random() < 64))
		CastSetState(cast_state->jumpstate);
	else
		CastSetState(cast_state->nextstate);

	cast_frames++;

	// go into attack frame
	if (cast_frames == 24 && !cast_death)
	{
		cast_onmelee ^= 1;
		st = cast_onmelee ? cast_info->melee_state : cast_info->missile_state;

		if (st == S_NULL)
		{
			cast_onmelee ^= 1;
			st = cast_onmelee ? cast_info->melee_state : cast_info->missile_state;
		}

		// check if missing both melee and missile states
		if (st != S_NULL)
		{
			CastSetState(st);
			cast_attacking = true;

			if (cast_info->attacksound)
				S_StartFX(cast_info->attacksound);
		}
	}

	// leave attack frames after a certain time
	if (cast_attacking &&
	    (cast_frames == 48 || 
		 cast_state == &cast_info->states[cast_info->chase_state]))
	{
		cast_attacking = false;
		cast_frames = 0;

		CastSetState(cast_info->chase_state);
	}
}


static void CastSkip(void)
{
	if (cast_death)
		return;  // already in dying frames

	// go into death frame
	cast_death = true;

	if (cast_info->overkill_state && (M_Random() < 32))
		cast_state = &cast_info->states[cast_info->overkill_state];
	else
	{
		SYS_ASSERT(cast_info->death_state);  // checked in ddf_mobj.c
		cast_state = &cast_info->states[cast_info->death_state];
	}

	cast_tics = cast_state->tics;
	cast_frames = 0;
	cast_attacking = false;

	if (cast_info->deathsound)
		S_StartFX(cast_info->deathsound);
}

static void CastPrint(const char *text)
{
	HL_WriteText(cast_style, 0, 160 - cast_style->fonts[0]->StringWidth(text)/2,
		180, text);
}


static void CastDrawer(void)
{
	const image_c *image;
	bool flip;

	image = W_ImageLookup("BOSSBACK");

	RGL_Image320(0, 0, 320, 200, image);

	CastPrint(cast_title);

	if (cast_state->flags & SFF_Model)
	{
		modeldef_c *md = W_GetModel(cast_state->sprite, ((mobjtype_c*)cast_info)->states);

		const image_c *skin_img = md->skins[cast_info->model_skin];

		if (! skin_img)
			skin_img = W_ImageForDummySkin();

		glClear(GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);

		MD2_RenderModel_2D(md->model, skin_img, cast_state->frame,
						   SCREENWIDTH/2.0, FROM_200(30),
						   SCREENWIDTH / 320.0, SCREENHEIGHT / 200.0,
						   cast_info);

		glDisable(GL_DEPTH_TEST);
		return;
	}

	// draw the current frame in the middle of the screen
	image = R2_GetOtherSprite(cast_state->sprite, cast_state->frame, &flip);

	if (! image)
		return;

	float xscale = cast_info->scale * cast_info->aspect;
	float yscale = cast_info->scale;

	float width    = IM_WIDTH(image);
	float height   = IM_HEIGHT(image);

	float offset_x = IM_OFFSETX(image);
	float offset_y = IM_OFFSETY(image);

	if (flip)
		offset_x = -offset_x;

	float tx1 = 160 - (width/2.0f + offset_x) * xscale;

	float gzb = 170 - offset_y * yscale;

	width  *= xscale;
	height *= yscale;

	// TODO: support FUZZY effect (glColor4f 0/0/0/0.5).

	RGL_DrawImage(FROM_320(tx1), SCREENHEIGHT - FROM_200(gzb),
			      FROM_320(width), FROM_200(height),
				  image, 
				  flip ? IM_RIGHT(image) : 0, 0,
				  flip ? 0 : IM_RIGHT(image), IM_TOP(image),
				  1.0f, RGB_NO_VALUE, cast_info->palremap);
}

//
// BunnyScroll
//
// -KM- 1998/07/31 Made our bunny friend take up more screen space.
// -KM- 1998/12/16 Removed fading routine.
//
static void BunnyScroll(void)
{
	int scrolled;
	const image_c *p1;
	const image_c *p2;
	char name[10];
	int stage;
	static int laststage;

	p1 = W_ImageLookup("PFUB2");
	p2 = W_ImageLookup("PFUB1");

	scrolled = 320 - (finalecount - 230) / 2;

	if (scrolled > 320)
		scrolled = 320;
	if (scrolled < 0)
		scrolled = 0;

	RGL_Image320(0   - scrolled, 0, 320, 200, p1);
	RGL_Image320(320 - scrolled, 0, 320, 200, p2);

	if (finalecount < 1130)
		return;

	if (finalecount < 1180)
	{
		p1 = W_ImageLookup("END0");

		RGL_ImageEasy320((320 - 13 * 8) / 2, (200 - 8 * 8) / 2, p1);
		laststage = 0;
		return;
	}

	stage = (finalecount - 1180) / 5;

	if (stage > 6)
		stage = 6;

	if (stage > laststage)
	{
		S_StartFX(sfx_pistol);
		laststage = stage;
	}

	sprintf(name, "END%i", stage);

	p1 = W_ImageLookup(name);

	RGL_ImageEasy320((320 - 13 * 8) / 2, (200 - 8 * 8) / 2, p1);
}


void F_Drawer(void)
{
	SYS_ASSERT(gamestate == GS_FINALE);

	switch (finalestage)
	{
		case f_text:
			TextWrite();
			break;

		case f_pic:
			{
				const image_c *image = W_ImageLookup(finale->pics[picnum]);

				RGL_Image320(0, 0, 320, 200, image);
			}
			break;

		case f_bunny:
			BunnyScroll();
			break;

		case f_cast:
			CastDrawer();
			break;

		default:
			I_Error("F_Drawer: bad finalestage #%d\n", (int)finalestage);
			break;
	}
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
