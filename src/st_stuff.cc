//----------------------------------------------------------------------------
//  EDGE Status Bar Code
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
//
//  Based on the DOOM source code, released by Id Software under the
//  following copyright:
//
//    Copyright (C) 1993-1996 by id Software, Inc.
//
//----------------------------------------------------------------------------
//
// -KM- 1998/07/21 Removed Cheats from this file to m_cheat.c.
//

#include "i_defs.h"
#include "st_stuff.h"

#include "am_map.h"
#include "dm_defs.h"
#include "dm_state.h"
#include "dstrings.h"
#include "ddf_main.h"
#include "g_game.h"
#include "m_fixed.h"
#include "m_random.h"
#include "st_lib.h"
#include "r_local.h"
#include "p_local.h"
#include "p_mobj.h"
#include "m_cheat.h"
#include "m_menu.h"
#include "s_sound.h"
#include "v_ctx.h"
#include "v_res.h"
#include "v_colour.h"
#include "w_image.h"
#include "w_wad.h"
#include "z_zone.h"

//
// STATUS BAR DATA
//

// N/256*100% probability that the normal face state will change
#define ST_FACEPROBABILITY   96

#define ST_FX  143
#define ST_FY  169

#define ST_FACESX  143
#define ST_FACESY  168

// Should be set to patch width
//  for tall numbers later on
#define ST_TALLNUMWIDTH         (tallnum[0]->width)

// Number of status faces.
#define ST_NUMPAINFACES         5
#define ST_NUMSTRAIGHTFACES     3
#define ST_NUMTURNFACES         2
#define ST_NUMSPECIALFACES      3

#define ST_FACESTRIDE \
	(ST_NUMSTRAIGHTFACES+ST_NUMTURNFACES+ST_NUMSPECIALFACES)

#define ST_NUMEXTRAFACES        2

#define ST_NUMFACES \
	(ST_FACESTRIDE*ST_NUMPAINFACES+ST_NUMEXTRAFACES)

#define ST_TURNOFFSET           (ST_NUMSTRAIGHTFACES)
#define ST_OUCHOFFSET           (ST_TURNOFFSET + ST_NUMTURNFACES)
#define ST_EVILGRINOFFSET       (ST_OUCHOFFSET + 1)
#define ST_RAMPAGEOFFSET        (ST_EVILGRINOFFSET + 1)
#define ST_GODFACE              (ST_NUMPAINFACES*ST_FACESTRIDE)
#define ST_DEADFACE             (ST_GODFACE+1)

#define ST_STRAIGHTFACECOUNT    (TICRATE/2)
#define ST_TURNCOUNT            (1*TICRATE)
#define ST_OUCHCOUNT            (1*TICRATE)
#define ST_RAMPAGEDELAY         (2*TICRATE)
#define ST_SHORT_DELAY          (TICRATE/5)

#define ST_MUCHPAIN   20

// Location and size of statistics,
//  justified according to widget type.
// Problem is, within which space? STbar? Screen?
// Note: this could be read in by a lump.
//       Problem is, is the stuff rendered
//       into a buffer,
//       or into the frame buffer?

// AMMO number pos.
#define ST_AMMOWIDTH            3
#define ST_AMMOX                (ST_X+44)
#define ST_AMMOY                (171)

// HEALTH number pos.
#define ST_HEALTHWIDTH          3
#define ST_HEALTHX              (ST_X+90)
#define ST_HEALTHY              ((171))

// Weapon pos.
#define ST_ARMSX                (ST_X+111)
#define ST_ARMSY                ((172))
#define ST_ARMSBGX              (ST_X+104)
#define ST_ARMSBGY              ((168))
#define ST_ARMSXSPACE           12
#define ST_ARMSYSPACE           10

// Frags pos.
#define ST_FRAGSX               (ST_X+138)
#define ST_FRAGSY               ((171))
#define ST_FRAGSWIDTH           2

// ARMOUR number pos.
#define ST_ARMOURWIDTH          3
#define ST_ARMOURX              (ST_X+221)
#define ST_ARMOURY              ((171))

// Key icon positions.
#define ST_KEY0WIDTH            8
#define ST_KEY0HEIGHT           5
#define ST_KEY0X                (ST_X+239)
#define ST_KEY0Y                ((171))
#define ST_KEY1WIDTH            ST_KEY0WIDTH
#define ST_KEY1X                (ST_X+239)
#define ST_KEY1Y                ((181))
#define ST_KEY2WIDTH            ST_KEY0WIDTH
#define ST_KEY2X                (ST_X+239)
#define ST_KEY2Y                ((191))

// Ammunition counter.
#define ST_AMMO0WIDTH           3
#define ST_AMMO0HEIGHT          6
#define ST_AMMO0X               (ST_X+288)
#define ST_AMMO0Y               ((173))
#define ST_AMMO1WIDTH           ST_AMMO0WIDTH
#define ST_AMMO1X               (ST_X+288)
#define ST_AMMO1Y               ((179))
#define ST_AMMO2WIDTH           ST_AMMO0WIDTH
#define ST_AMMO2X               (ST_X+288)
#define ST_AMMO2Y               ((185))
#define ST_AMMO3WIDTH           ST_AMMO0WIDTH
#define ST_AMMO3X               (ST_X+288)
#define ST_AMMO3Y               ((191))

// Indicate maximum ammunition.
// Only needed because backpack exists.
#define ST_MAXAMMO0WIDTH        3
#define ST_MAXAMMO0HEIGHT       5
#define ST_MAXAMMO0X            (ST_X+314)
#define ST_MAXAMMO0Y            ((173))
#define ST_MAXAMMO1WIDTH        ST_MAXAMMO0WIDTH
#define ST_MAXAMMO1X            (ST_X+314)
#define ST_MAXAMMO1Y            ((179))
#define ST_MAXAMMO2WIDTH        ST_MAXAMMO0WIDTH
#define ST_MAXAMMO2X            (ST_X+314)
#define ST_MAXAMMO2Y            ((185))
#define ST_MAXAMMO3WIDTH        ST_MAXAMMO0WIDTH
#define ST_MAXAMMO3X            (ST_X+314)
#define ST_MAXAMMO3Y            ((191))

// used to update the whole status bar
bool stbar_update = true;

// colorise health/ammo/armour
bool stbar_colours = false;

// used to execute ST_Init() only once
static int veryfirsttime = 1;

// used for making messages go away
static int st_msgcounter = 0;

// used when in chat 
static st_chatstateenum_t st_chatstate;

// whether left-side main status bar is active
static bool st_statusbaron;
static bool st_baron_not_overlay;

// whether status bar chat is active
static bool st_chat;

// value of st_chat before message popped up
static bool st_oldchat;

// whether chat window has the cursor on
static bool st_cursoron;

// !deathmatch
static bool st_notdeathmatch;

// !deathmatch && st_statusbaron
static bool st_armson;

// !deathmatch
static bool st_fragson;

// main bar left
static const image_t *sbar_image;

// 0-9, tall numbers
static const image_t *tallnum[10];

// tall % sign
static const image_t *tallpercent;

static const image_t *sttminus;

// 0-9, short, yellow (,different!) numbers
static const image_t *shortnum[10];

// 3 key-cards, 3 skulls & 3 Combination -ACB- 1998/09/11
static const image_t *keys[9];

// face status patches
static const image_t *faces[ST_NUMFACES];

// face background
static const image_t *faceback;

// main bar right
static const image_t *armsbg;

// weapon ownership patches
static const image_t *arms[6][2];

// ready-weapon widget
static st_number_t w_ready;

// in deathmatch only, summary of frags stats
static st_number_t w_frags;

// health widget
static st_percent_t w_health;

// arms background
static st_binicon_t w_armsbg;

// weapon ownership widgets
static st_multicon_t w_arms[6];

// face status widget
static st_multicon_t w_faces;

// keycard widgets
static st_multicon_t w_keyboxes[3];

// armour widget
static st_percent_t w_armour;

// ammo widgets
static st_number_t w_ammo[4];

// max ammo widgets
static st_number_t w_maxammo[4];

// number of frags so far in deathmatch
static int st_fragscount;

// holds key-type for each key box on bar
static int keyboxes[3];

static bool st_stopped = true;


//
// STATUS BAR CODE
//
void ST_Stop(void);

static void RefreshBackground(void)
{
	if (st_baron_not_overlay)
	{
#ifdef USE_GL  // -AJA- hack fix for red line at bottom of screen
		VCTX_Image320(ST_X, ST_Y, ST_WIDTH, ST_HEIGHT+1, sbar_image);
#else
		VCTX_Image320(ST_X, ST_Y, ST_WIDTH, ST_HEIGHT, sbar_image);
#endif
	}
}

//
// ST_Responder
//
bool ST_Responder(event_t * ev)
{
	// does nothing at the moment
	return false;
}

static int ST_CalcPainOffset(void)
{
	float base, health;
	int index;

	if (! consoleplayer->mo)
		return 0;

	base = consoleplayer->mo->info->spawnhealth;

	DEV_ASSERT2(base > 0);

	health = (base - MIN(base, consoleplayer->health)) / base;
	index = MIN(ST_NUMPAINFACES-1, (int)(health * ST_NUMPAINFACES));

	return ST_FACESTRIDE * index;
}

static void DrawWidgets(bool refresh)
{
	int i;

	// used by w_arms[] widgets
	st_armson = st_baron_not_overlay && !deathmatch;

	// used by w_frags widget
	st_fragson = st_baron_not_overlay && deathmatch;

	STLIB_UpdateNum(&w_ready, refresh);

	for (i = 0; i < 4; i++)
	{
		STLIB_UpdateNum(&w_ammo[i], refresh);
		STLIB_UpdateNum(&w_maxammo[i], refresh);
	}

	STLIB_UpdatePercent(&w_health, refresh);
	STLIB_UpdatePercent(&w_armour, refresh);

	STLIB_UpdateBinIcon(&w_armsbg, refresh);

	for (i = 0; i < 6; i++)
		STLIB_UpdateMultIcon(&w_arms[i], refresh);

	STLIB_UpdateMultIcon(&w_faces, refresh);

	for (i = 0; i < 3; i++)
		STLIB_UpdateMultIcon(&w_keyboxes[i], refresh);

	STLIB_UpdateNum(&w_frags, refresh);
}

//
// ST_UpdateFaceWidget
//
// This routine handles the face states and their timing.
// The precedence of expressions is:
//
//    dead > evil grin > turned head > straight ahead
//
static void ST_UpdateFaceWidget(void)
{
	angle_t badguyangle;
	angle_t diffang;

	if (consoleplayer->face_count > 0)
	{
		consoleplayer->face_count--;
		return;
	}

	// dead ?
	if (consoleplayer->health <= 0)
	{
		consoleplayer->face_index = ST_DEADFACE;
		consoleplayer->face_count = TICRATE;
		return;
	}

	// evil grin if just picked up weapon
	if (consoleplayer->grin_count)
	{
		consoleplayer->face_index = ST_CalcPainOffset() + ST_EVILGRINOFFSET;
		consoleplayer->face_count = ST_SHORT_DELAY;
		return;
	}

	// being attacked ?
	if (consoleplayer->damagecount && consoleplayer->attacker && consoleplayer->attacker != consoleplayer->mo)
	{
		if ((consoleplayer->old_health - consoleplayer->health) > ST_MUCHPAIN)
		{
			consoleplayer->face_index = ST_CalcPainOffset() + ST_OUCHOFFSET;
			consoleplayer->face_count = ST_TURNCOUNT;
			return;
		}

		badguyangle = R_PointToAngle(consoleplayer->mo->x, consoleplayer->mo->y,
			consoleplayer->attacker->x, consoleplayer->attacker->y);

		diffang = badguyangle - consoleplayer->mo->angle;

		consoleplayer->face_index = ST_CalcPainOffset();
		consoleplayer->face_count = ST_TURNCOUNT;

		if (diffang < ANG45 || diffang > ANG315 ||
			(diffang > ANG135 && diffang < ANG225))
		{
			// head-on  
			consoleplayer->face_index += ST_RAMPAGEOFFSET;
		}
		else if (diffang >= ANG45 && diffang <= ANG135)
		{
			// turn face left
			consoleplayer->face_index += ST_TURNOFFSET + 1;
		}
		else
		{
			// turn face right
			consoleplayer->face_index += ST_TURNOFFSET;
		}
		return;
	}

	// getting hurt because of your own damn stupidity
	if (consoleplayer->damagecount)
	{
		if ((consoleplayer->old_health - consoleplayer->health) > ST_MUCHPAIN)
		{
			consoleplayer->face_index = ST_CalcPainOffset() + ST_OUCHOFFSET;
			consoleplayer->face_count = ST_TURNCOUNT;
			return;
		}

		consoleplayer->face_index = ST_CalcPainOffset() + ST_RAMPAGEOFFSET;
		consoleplayer->face_count = ST_TURNCOUNT;
		return;
	}

	// rapid firing
	if (consoleplayer->attackdown_count > ST_RAMPAGEDELAY)
	{
		consoleplayer->face_index = ST_CalcPainOffset() + ST_RAMPAGEOFFSET;
		consoleplayer->face_count = ST_SHORT_DELAY;
		return;
	}

	// invulnerability
	if ((consoleplayer->cheats & CF_GODMODE)
		|| consoleplayer->powers[PW_Invulnerable] > 0)
	{
		consoleplayer->face_index = ST_GODFACE;
		consoleplayer->face_count = ST_SHORT_DELAY;
		return;
	}

	// default: look about the place...
	consoleplayer->face_index = ST_CalcPainOffset() + (M_Random() % 3);
	consoleplayer->face_count = ST_STRAIGHTFACECOUNT;
}

static keys_e st_key_list[6] =
{
	KF_BlueCard, KF_YellowCard, KF_RedCard,
		KF_BlueSkull, KF_YellowSkull, KF_RedSkull
};

static void UpdateWidgets(void)
{
	static int largeammo = 1994;  // means "n/a"

	bool is_overlay = hud_overlay && (viewwindowheight == SCREENHEIGHT);

	int i;
	keys_e cards;

	// set health colour, as in BOOM.  -AJA- Experimental !!
	if (stbar_colours)
	{
		if (consoleplayer->health < 25.0f)
			w_health.f.num.colmap = text_red_map;
		else if (consoleplayer->health < 60.0f)
			w_health.f.num.colmap = text_green_map;
		else if (consoleplayer->health <= 100.0f)
			w_health.f.num.colmap = text_yellow_map;
		else
			w_health.f.num.colmap = text_blue_map;
	}
	else
	{
		// the original digits have a grey shadow that doesn't look nice
		// on the overlay hud.  Using the TEXT_RED map improves it.
		w_health.f.num.colmap = is_overlay ? text_red_map : NULL;
	}

	// find ammo amount to show -- or leave it blank
	w_ready.num = &largeammo;
	w_ready.colmap = is_overlay ? text_yellow_map : NULL;

	if (consoleplayer->ready_wp >= 0)
	{
		playerweapon_t *pw = &consoleplayer->weapons[consoleplayer->ready_wp];

		if (pw->info->ammo != AM_NoAmmo)
		{
			if (pw->info->show_clip)
			{
				w_ready.num = &pw->clip_size;
				w_ready.colmap = text_brown_map;
			}
			else
			{
				w_ready.num = &consoleplayer->ammo[pw->info->ammo].num;

				// set ammo colour as in BOOM.  -AJA- Experimental !!
				if (stbar_colours)
				{
					if (*(w_ready.num) < 20)
						w_ready.colmap = text_red_map;
					else if (*(w_ready.num) <= 50)
						w_ready.colmap = text_green_map;
					else
						w_ready.colmap = text_yellow_map;
				}
			}
		}
	}

	// show total armour, where colour is the best we've got

	for (i=NUMARMOUR-1; i >= 0; i--)
	{
		if (consoleplayer->armours[i] > 0)
		{
			w_armour.f.num.colmap =
				(i == ARMOUR_Green)  ? text_green_map :
				(i == ARMOUR_Blue)   ? text_blue_map :
				(i == ARMOUR_Yellow) ? text_yellow_map : text_red_map;
			break;
		}
	}

	if (consoleplayer->totalarmour < 1)
		w_armour.f.num.colmap = is_overlay ? text_green_map : NULL;

	cards = consoleplayer->cards;

	// update keycard multiple widgets
	// -ACB- 1998/09/11 Include Combo Cards
	for (i = 0; i < 3; i++)
	{
		if ((cards & st_key_list[i]) && (cards & st_key_list[i + 3]))
			keyboxes[i] = i + 6;
		else if (cards & st_key_list[i + 3])
			keyboxes[i] = i + 3;
		else if (cards & st_key_list[i])
			keyboxes[i] = i;
		else
			keyboxes[i] = -1;
	}

	// refresh everything if this is him coming back to life
	ST_UpdateFaceWidget();

	// used by the w_armsbg widget
	st_notdeathmatch = !deathmatch;

	// used by w_arms[] widgets
	st_armson = st_baron_not_overlay && !deathmatch;

	// used by w_frags widget
	st_fragson = st_baron_not_overlay && deathmatch;
	st_fragscount = 0;

	st_fragscount = consoleplayer->frags;

	// get rid of chat window if up because of message
	if (!--st_msgcounter)
		st_chat = st_oldchat;
}

void ST_Ticker(void)
{
	UpdateWidgets();

	consoleplayer->old_health = consoleplayer->health;
}

// -AJA- 1999/07/03: Rewrote this routine, since the palette handling
// has been moved to v_colour.c/h (and made more flexible).  Later on it
// might be good to DDF-ify all this, allowing other palette lumps and
// being able to set priorities for the different effects.

static void DoPaletteStuff(void)
{
	int palette = PALETTE_NORMAL;
	float amount = 0;
	int cnt;
	int bzc;

	cnt = consoleplayer->damagecount;

	if (consoleplayer->powers[PW_Berserk] > 0)
	{
		// slowly fade the berzerk out
		bzc = MIN(20, (int) consoleplayer->powers[PW_Berserk]);

		if (bzc > cnt)
			cnt = bzc;
	}

	if (cnt)
	{
		palette = PALETTE_PAIN;
		amount = (cnt + 7) / 64.0f;
	}
	else if (consoleplayer->bonuscount)
	{
		palette = PALETTE_BONUS;
		amount = (consoleplayer->bonuscount + 7) / 32.0f;
	}
	else if (consoleplayer->powers[PW_AcidSuit] > 4 * 32 ||
		fmod(consoleplayer->powers[PW_AcidSuit], 16) >= 8)
	{
		palette = PALETTE_SUIT;
		amount = 1.0f;
	}

	// This routine will limit `amount' to acceptable values, and will
	// only update the video palette/colourmaps when the palette actually
	// changes.
	V_SetPalette(palette, amount);
}

static void DoRefresh(void)
{
	// draw status bar background to off-screen buff
	RefreshBackground();

	// and refresh all widgets
	DrawWidgets(true);

	stbar_update = false;
}

static void DiffDraw(void)
{
	// update all widgets
	DrawWidgets(false);
}

void ST_Drawer(bool fullscreen, bool refresh)
{
	st_statusbaron       = (!fullscreen) || automapactive || hud_overlay;
	st_baron_not_overlay = (!fullscreen) || automapactive;

#if 1 
	// -AJA- one small hack for GL, one *giant* HACK for software 
	refresh = true;
#endif

	// Do red-/gold-shifts from damage/items
	DoPaletteStuff();

	// If just after ST_Start(), refresh all
	if (stbar_update || refresh)
	{
		DoRefresh();
		return;
	}

	// Otherwise, update as little as possible
	DiffDraw();
}

static void LoadGraphics(void)
{
	int i;
	int j;
	int facenum;

	char namebuf[9];

	// Load the numbers, tall and short
	for (i = 0; i < 10; i++)
	{
		sprintf(namebuf, "STTNUM%d", i);
		tallnum[i] = W_ImageFromFont(namebuf);

		sprintf(namebuf, "STYSNUM%d", i);
		shortnum[i] = W_ImageFromFont(namebuf);
	}

	// Load percent key.
	tallpercent = W_ImageFromFont("STTPRCNT");

	// Load '-'
	sttminus = W_ImageFromFont("STTMINUS");

	// key cards
	// -ACB- 1998/09/11 Include dual card/skull graphics
	for (i = 0; i < 9; i++)
	{
		sprintf(namebuf, "STKEYS%d", i);
		keys[i] = W_ImageFromPatch(namebuf);
	}

	// arms background
	armsbg = W_ImageFromPatch("STARMS");

	// arms ownership widgets
	for (i = 0; i < 6; i++)
	{
		sprintf(namebuf, "STGNUM%d", i + 2);

		// gray #
		arms[i][0] = W_ImageFromPatch(namebuf);

		// yellow #
		arms[i][1] = shortnum[i + 2];
	}

	// face backgrounds for different colour players
	sprintf(namebuf, "STFB%d", consoleplayer->pnum);
	faceback = W_ImageFromPatch(namebuf);

	// status bar background bits
	sbar_image = W_ImageFromPatch("STBAR");

	// face states
	facenum = 0;
	for (i = 0; i < ST_NUMPAINFACES; i++)
	{
		for (j = 0; j < ST_NUMSTRAIGHTFACES; j++)
		{
			sprintf(namebuf, "STFST%d%d", i, j);
			faces[facenum++] = W_ImageFromPatch(namebuf);
		}

		// turn right
		sprintf(namebuf, "STFTR%d0", i);
		faces[facenum++] = W_ImageFromPatch(namebuf);

		// turn left
		sprintf(namebuf, "STFTL%d0", i);
		faces[facenum++] = W_ImageFromPatch(namebuf);

		// ouch!
		sprintf(namebuf, "STFOUCH%d", i);
		faces[facenum++] = W_ImageFromPatch(namebuf);

		// evil grin ;)
		sprintf(namebuf, "STFEVL%d", i);
		faces[facenum++] = W_ImageFromPatch(namebuf);

		// pissed off
		sprintf(namebuf, "STFKILL%d", i);
		faces[facenum++] = W_ImageFromPatch(namebuf);
	}

	faces[facenum++] = W_ImageFromPatch("STFGOD0");
	faces[facenum]   = W_ImageFromPatch("STFDEAD0");
}

static void LoadData(void)
{
	LoadGraphics();
}

#if 0  // NOT YET USED ?
static void UnloadGraphics(void)
{
	int i;

	// unload the numbers, tall and short
	for (i = 0; i < 10; i++)
	{
		W_DoneWithLump(tallnum[i]);
		W_DoneWithLump(shortnum[i]);
	}
	// unload tall percent
	W_DoneWithLump(tallpercent);

	// unload '-'
	W_DoneWithLump(sttminus);

	// unload the key cards (include combo widgets) -ACB- 1998/09/11
	for (i = 0; i < 9; i++)
		W_DoneWithLump(keys[i]);

	// unload arms background
	W_DoneWithLump(armsbg);

	// unload gray #'s
	for (i = 0; i < 6; i++)
		W_DoneWithLump(arms[i][0]);

	W_DoneWithLump(faceback);
	W_DoneWithLump(sbar);

	for (i = 0; i < ST_NUMFACES; i++)
		W_DoneWithLump(faces[i]);
}

static void UnloadData(void)
{
	UnloadGraphics();
}
#endif

static void InitData(void)
{
	int i;

	st_chatstate = StartChatState;
	st_statusbaron = st_baron_not_overlay = true;
	st_oldchat = st_chat = false;
	st_cursoron = false;
	consoleplayer->face_index = 0;

	for (i = 0; i < 3; i++)
		keyboxes[i] = -1;

	STLIB_Init();
}

static void CreateWidgets(void)
{
	int i;

	// ready weapon ammo
	STLIB_InitNum(&w_ready, ST_AMMOX, ST_AMMOY, tallnum, sttminus,
		&consoleplayer->ammo[0].num,  // FIXME
		&st_statusbaron, ST_AMMOWIDTH);

	// health percentage
	STLIB_InitPercent(&w_health, ST_HEALTHX, ST_HEALTHY, tallnum, tallpercent,
		&consoleplayer->health, &st_statusbaron);

	// arms background
	STLIB_InitBinIcon(&w_armsbg, ST_ARMSBGX, ST_ARMSBGY, armsbg,
		&st_notdeathmatch, &st_baron_not_overlay);

	// weapons owned
	for (i = 0; i < 6; i++)
	{
		STLIB_InitMultIcon(&w_arms[i],
			ST_ARMSX + (i % 3) * ST_ARMSXSPACE,
			ST_ARMSY + (i / 3) * ST_ARMSYSPACE,
			arms[i], (int *)&consoleplayer->avail_weapons[2 + i], &st_armson);
	}

	// frags sum
	STLIB_InitNum(&w_frags, ST_FRAGSX, ST_FRAGSY, tallnum, sttminus,
		&st_fragscount, &st_fragson, ST_FRAGSWIDTH);

	// faces
	STLIB_InitMultIcon(&w_faces, ST_FACESX, ST_FACESY, faces,
		&consoleplayer->face_index, &st_baron_not_overlay);

	// armour percentage - should be coloured later
	STLIB_InitPercent(&w_armour, ST_ARMOURX, ST_ARMOURY, tallnum, tallpercent,
		&consoleplayer->totalarmour, &st_statusbaron);

	// keyboxes 0-2
	STLIB_InitMultIcon(&w_keyboxes[0], ST_KEY0X, ST_KEY0Y, keys,
		&keyboxes[0], &st_statusbaron);

	STLIB_InitMultIcon(&w_keyboxes[1], ST_KEY1X, ST_KEY1Y, keys,
		&keyboxes[1], &st_statusbaron);

	STLIB_InitMultIcon(&w_keyboxes[2], ST_KEY2X, ST_KEY2Y, keys,
		&keyboxes[2], &st_statusbaron);

	// ammo count (all four kinds)
	STLIB_InitNum(&w_ammo[0], ST_AMMO0X, ST_AMMO0Y, shortnum, NULL,
		&consoleplayer->ammo[0].num, &st_statusbaron, ST_AMMO0WIDTH);

	STLIB_InitNum(&w_ammo[1], ST_AMMO1X, ST_AMMO1Y, shortnum, NULL,
		&consoleplayer->ammo[1].num, &st_statusbaron, ST_AMMO1WIDTH);

	STLIB_InitNum(&w_ammo[2], ST_AMMO2X, ST_AMMO2Y, shortnum, NULL,
		&consoleplayer->ammo[2].num, &st_statusbaron, ST_AMMO2WIDTH);

	STLIB_InitNum(&w_ammo[3], ST_AMMO3X, ST_AMMO3Y, shortnum, NULL,
		&consoleplayer->ammo[3].num, &st_statusbaron, ST_AMMO3WIDTH);

	// max ammo count (all four kinds)
	STLIB_InitNum(&w_maxammo[0], ST_MAXAMMO0X, ST_MAXAMMO0Y, shortnum, NULL,
		&consoleplayer->ammo[0].max, &st_statusbaron, ST_MAXAMMO0WIDTH);

	STLIB_InitNum(&w_maxammo[1], ST_MAXAMMO1X, ST_MAXAMMO1Y, shortnum, NULL,
		&consoleplayer->ammo[1].max, &st_statusbaron, ST_MAXAMMO1WIDTH);

	STLIB_InitNum(&w_maxammo[2], ST_MAXAMMO2X, ST_MAXAMMO2Y, shortnum, NULL,
		&consoleplayer->ammo[2].max, &st_statusbaron, ST_MAXAMMO2WIDTH);

	STLIB_InitNum(&w_maxammo[3], ST_MAXAMMO3X, ST_MAXAMMO3Y, shortnum, NULL,
		&consoleplayer->ammo[3].max, &st_statusbaron, ST_MAXAMMO3WIDTH);

	for (i=0; i < 4; i++)
		w_ammo[i].colmap = w_maxammo[i].colmap = text_yellow_map;
}

void ST_Start(void)
{
	if (!st_stopped)
		ST_Stop();

	InitData();
	CreateWidgets();
	UpdateWidgets();  // make sure colours are set right

	st_stopped = false;
}

void ST_Stop(void)
{
	if (st_stopped)
		return;

	// -AJA- 1999/07/03: removed PLAYPAL reference.
	V_SetPalette(PALETTE_NORMAL, 0);

	st_stopped = true;
}

//
// ST_ReInit
// Re-inits status bar after a resolution change.
// -ES- 1999/03/29 Fixed Low Resolutions
//
void ST_ReInit(void)
{
	//!!! -AJA- FIXME: this gets called too early
	//!!! ST_Start();
}

//
// ST_Init
//
// Called once at startup
//
bool ST_Init(void)
{
	DEV_ASSERT2(veryfirsttime);

	veryfirsttime = 0;

	LoadData();
	M_CheatInit();

	return true;
}
