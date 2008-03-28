//------------------------------------------------------------------------
//  LUA HUD code
//------------------------------------------------------------------------
//
//  Copyright (c) 2006-2008  The EDGE Team.
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
//------------------------------------------------------------------------

#include "i_defs.h"
#include "i_luainc.h"

#include "ddf/main.h"
#include "ddf/font.h"

#include "dm_state.h"
#include "e_main.h"
#include "l_lua.h"
#include "version.h"

#include "e_player.h"
#include "hu_font.h"
#include "r_draw.h"
#include "r_modes.h"
#include "w_wad.h"
#include "z_zone.h"

#include "r_misc.h"     //  R_Render
#include "r_automap.h"  // AM_Drawer


static lua_State *HUD_ST;

static bool has_loaded = false;

static player_t *cur_player = NULL;

static font_c *cur_font = NULL;
static colourmap_c *cur_colmap = NULL;
static float cur_scale;
static float cur_alpha;

static int hud_last_time = -1;

extern std::string w_map_title;


static void FrameSetup(void)
{
	fontdef_c *DEF = fontdefs.Lookup("DOOM");  // FIXME allow other default
	SYS_ASSERT(DEF);

	cur_font = hu_fonts.Lookup(DEF);
	SYS_ASSERT(cur_font);

	cur_colmap = NULL;
	cur_scale  = 1.0f;
	cur_alpha  = 1.0f;

	cur_player = players[displayplayer];


	int now_time = I_GetTime();

	if (hud_last_time <= 0 || hud_last_time > now_time)
		hud_last_time = now_time;

	int tics = MIN(now_time - hud_last_time, TICRATE);


	// setup some fields in 'player' module

	lua_getglobal(HUD_ST, "player");

	// TODO

	lua_pop(HUD_ST, 1);


	// setup some fields in 'hud' module

	lua_getglobal(HUD_ST, "hud");

	lua_pushinteger(HUD_ST, screen_hud);
	lua_setfield(HUD_ST, -2, "which");

	lua_pushboolean(HUD_ST, automapactive);
	lua_setfield(HUD_ST, -2, "automap");

	lua_pushinteger(HUD_ST, now_time);
	lua_setfield(HUD_ST, -2, "now_time");

	lua_pushinteger(HUD_ST, tics);
	lua_setfield(HUD_ST, -2, "passed_time");

	lua_pop(HUD_ST, 1);
}


static void DoWriteText(float x, float y, const char *str)
{
	float cx = x;
	float cy = y;

	for (; *str; str++)
	{
		char ch = *str;

		if (ch == '\n')
		{
			cx = x;
			cy += 12.0f * cur_scale;  // FIXME: use font's height
			continue;
		}

		if (cx >= 320.0f)
			continue;

		cur_font->DrawChar(cx, cy, ch, cur_scale,1.0f, cur_colmap, cur_alpha);

		cx += cur_font->CharWidth(ch) * cur_scale;
	}
}

static void DoWriteText_RightAlign(float x, float y, const char *str)
{
	float cx = x;
	float cy = y;

	for (int pos = strlen(str)-1; pos >= 0; pos--)
	{
		char ch = str[pos];

		if (ch == '\n')
		{
			cx = x;
			cy += 12.0f * cur_scale;  // FIXME: use font's height
			continue;
		}

		if (cx >= 320.0f)
			continue;

		cx -= cur_font->CharWidth(ch) * cur_scale;

		cur_font->DrawChar(cx, cy, ch, cur_scale,1.0f, cur_colmap, cur_alpha);
	}
}


//------------------------------------------------------------------------
//  HUD MODULE
//------------------------------------------------------------------------


static rgbcol_t ParseColor(lua_State *L, int index)
{
	if (lua_isstring(L, index))
	{
		const char *name = lua_tostring(L, index);

		if (name[0] != '#')
			I_Error("Bad color in lua script (missing #) : %s\n", name);

		return (rgbcol_t) strtol(name+1, NULL, 16);
	}

	if (lua_istable(L, index))
	{
		// parse 'r', 'g', 'b' fields
		int r, g, b;

		lua_getfield(L, index, "r");
		if (! lua_isnumber(L, -1))
			I_Error("Bad color table in lua script (missing \"r\")\n");
		r = lua_tointeger(L, -1);

		lua_getfield(L, index, "g");
		if (! lua_isnumber(L, -1))
			I_Error("Bad color table in lua script (missing \"g\")\n");
		g = lua_tointeger(L, -1);

		lua_getfield(L, index, "b");
		if (! lua_isnumber(L, -1))
			I_Error("Bad color table in lua script (missing \"b\")\n");
		b = lua_tointeger(L, -1);

		lua_pop(L, 3);

		return RGB_MAKE(r, g, b);
	}

	I_Error("Bad color value in lua script!\n");
	return 0; /* NOT REACHED */
}


// hud.raw_debug_print(str)
//
static int HD_raw_debug_print(lua_State *L)
{
	int nargs = lua_gettop(L);

	if (nargs >= 1)
	{
		const char *res = luaL_checkstring(L,1);
		SYS_ASSERT(res);

		I_Debugf("%s", res);
	}

	return 0;
}


// hud.coord_sys(w, h)
//
static int HD_coord_sys(lua_State *L)
{
	int w = luaL_checkint(L, 1);
	int h = luaL_checkint(L, 2);

	if (w <= 80 || h <= 50)
		I_Error("Bad hud.coord_sys size: %dx%d\n", w, h);

	//... FIXME

	return 0;
}


// hud.game_mode()
//
static int HD_game_mode(lua_State *L)
{
	if (DEATHMATCH())
		lua_pushstring(L, "dm");
	else if (COOP_MATCH())
		lua_pushstring(L, "coop");
	else
		lua_pushstring(L, "sp");

	return 1;
}


// hud.map_title()
//
static int HD_map_title(lua_State *L)
{
	lua_pushstring(L, w_map_title.c_str());
	return 1;
}


// hud.text_font(name)
//
static int HD_text_font(lua_State *L)
{
	const char *font_name = luaL_checkstring(L, 1);

	fontdef_c *DEF = fontdefs.Lookup(font_name);
	SYS_ASSERT(DEF);

	cur_font = hu_fonts.Lookup(DEF);
	SYS_ASSERT(cur_font);

	return 0;
}


// hud.text_color(name)
//
static int HD_text_color(lua_State *L)
{
	//... FIXME

	return 0;
}


// hud.set_scale(value)
//
static int HD_set_scale(lua_State *L)
{
	cur_scale = luaL_checknumber(L, 1);

	if (cur_scale <= 0)
		I_Error("hud.set_scale: Bad scale value: %1.3f\n", cur_scale);

	return 0;
}


// hud.set_alpha(value)
//
static int HD_set_alpha(lua_State *L)
{
	cur_alpha = luaL_checknumber(L, 1);

	return 0;
}


// hud.solid_box(x, y, w, h, color [,alpha])
//
static int HD_solid_box(lua_State *L)
{
	float x = luaL_checknumber(L, 1);
	float y = luaL_checknumber(L, 2);
	float w = luaL_checknumber(L, 3);
	float h = luaL_checknumber(L, 4);

	w = FROM_320(w); h = FROM_200(h);
	x = FROM_320(x); y = SCREENHEIGHT - FROM_200(y) - h;

	rgbcol_t rgb = ParseColor(L, 5);

	RGL_SolidBox((int)x, (int)y, I_ROUND(w), I_ROUND(h), rgb, cur_alpha);

	return 0;
}


// hud.solid_line(x1, y1, x2, y2, color [,alpha])
//
static int HD_solid_line(lua_State *L)
{
	float x1 = luaL_checknumber(L, 1);
	float y1 = luaL_checknumber(L, 2);
	float x2 = luaL_checknumber(L, 3);
	float y2 = luaL_checknumber(L, 4);

	x1 = FROM_320(x1); y1 = SCREENHEIGHT - FROM_200(y1);
	x2 = FROM_320(x2); y2 = SCREENHEIGHT - FROM_200(y2);

	rgbcol_t rgb = ParseColor(L, 5);

	RGL_SolidLine((int)x1, (int)y1, (int)x2, (int)y2, rgb, cur_alpha);

	return 0;
}


// hud.thin_box(x, y, w, h, color)
//
static int HD_thin_box(lua_State *L)
{
	float x = luaL_checknumber(L, 1);
	float y = luaL_checknumber(L, 2);
	float w = luaL_checknumber(L, 3);
	float h = luaL_checknumber(L, 4);

	w = FROM_320(w); h = FROM_200(h);
	x = FROM_320(x); y = SCREENHEIGHT - FROM_200(y) - h;

	rgbcol_t rgb = ParseColor(L, 5);

	RGL_ThinBox((int)x, (int)y, I_ROUND(w), I_ROUND(h), rgb, cur_alpha);

	return 0;
}


// hud.gradient_box(x, y, w, h, TL, BL, TR, BR)
//
static int HD_gradient_box(lua_State *L)
{
	float x = luaL_checknumber(L, 1);
	float y = luaL_checknumber(L, 2);
	float w = luaL_checknumber(L, 3);
	float h = luaL_checknumber(L, 4);

	w = FROM_320(w); h = FROM_200(h);
	x = FROM_320(x); y = SCREENHEIGHT - FROM_200(y) - h;

	rgbcol_t cols[4];

	cols[0] = ParseColor(L, 5);
	cols[1] = ParseColor(L, 6);
	cols[2] = ParseColor(L, 7);
	cols[3] = ParseColor(L, 8);

	RGL_GradientBox((int)x, (int)y, I_ROUND(w), I_ROUND(h), cols, cur_alpha);

	return 0;
}


// hud.draw_image(x, y, name)
//
static int HD_draw_image(lua_State *L)
{
	float x = luaL_checknumber(L, 1);
	float y = luaL_checknumber(L, 2);

	const char *name = luaL_checkstring(L, 3);

	const image_c *img = W_ImageLookup(name, INS_Graphic);
	if (img)
	{
		x -= IM_OFFSETX(img);
		y -= IM_OFFSETY(img);

		float w = IM_WIDTH(img);
		float h = IM_HEIGHT(img);

		w *= cur_scale;
		h *= cur_scale;

		w = FROM_320(w); h = FROM_200(h);
		x = FROM_320(x); y = SCREENHEIGHT - FROM_200(y) - h;

		RGL_DrawImage(x, y, w, h, img,
		              0, 0, IM_RIGHT(img), IM_TOP(img),
					  NULL, cur_alpha);
	}

	return 0;
}


// hud.stretch_image(x, y, w, h, name)
//
static int HD_stretch_image(lua_State *L)
{
	float x = luaL_checknumber(L, 1);
	float y = luaL_checknumber(L, 2);
	float w = luaL_checknumber(L, 3);
	float h = luaL_checknumber(L, 4);

	const char *name = luaL_checkstring(L, 5);

	const image_c *img = W_ImageLookup(name, INS_Graphic);
	if (img)
	{
		w = FROM_320(w); h = FROM_200(h);
		x = FROM_320(x); y = SCREENHEIGHT - FROM_200(y) - h;

		RGL_DrawImage(x, y, w, h, img,
		              0, 0, IM_RIGHT(img), IM_TOP(img),
					  NULL, cur_alpha);
	}

	return 0;
}


// hud.tile_image(x, y, w, h, name, [x_offset, y_offset])
//
static int HD_tile_image(lua_State *L)
{
	float x = luaL_checknumber(L, 1);
	float y = luaL_checknumber(L, 2);
	float w = luaL_checknumber(L, 3);
	float h = luaL_checknumber(L, 4);

	const char *name = luaL_checkstring(L, 5);

	float offset_x = 0;
	float offset_y = 0;

	if (lua_isnumber(L, 6))
		offset_x = lua_tonumber(L, 6);

	if (lua_isnumber(L, 7))
		offset_y = lua_tonumber(L, 7);

	const image_c *img = W_ImageLookup(name, INS_Texture);
	if (img)
	{
		offset_x /=  w;
		offset_y /= -h;

		float tx_scale = w / IM_TOTAL_WIDTH(img)  / cur_scale;
		float ty_scale = h / IM_TOTAL_HEIGHT(img) / cur_scale;

		w = FROM_320(w); h = FROM_200(h);
		x = FROM_320(x); y = SCREENHEIGHT - FROM_200(y) - h;

		RGL_DrawImage(x, y, w, h, img,
		              (offset_x) * tx_scale,
					  (offset_y) * ty_scale,
					  (offset_x + 1) * tx_scale,
					  (offset_y + 1) * ty_scale,
					  NULL, cur_alpha);
	}

	return 0;
}


// hud.draw_text(x, y, str)
//
static int HD_draw_text(lua_State *L)
{
	float x = luaL_checknumber(L, 1);
	float y = luaL_checknumber(L, 2);

	const char *str = luaL_checkstring(L, 3);

	DoWriteText(x, y, str);

	return 0;
}


// hud.draw_num2(x, y, len, num)
//
static int HD_draw_num2(lua_State *L)
{
	float x = luaL_checknumber(L, 1);
	float y = luaL_checknumber(L, 2);

	int len = luaL_checkint(L, 3);
	int num = luaL_checkint(L, 4);

	if (len < 1 || len > 20)
		I_Error("hud.draw_number: bad field length: %d\n", len);

	bool is_neg = false;

	if (num < 0 && len > 1)
	{
		is_neg = true; len--;
	}

	// build the integer backwards

	char buffer[200];
	char *pos = &buffer[sizeof(buffer)-4];

	*--pos = 0;

	if (num == 0)
	{
		*--pos = '0';
	}
	else
	{
		for (; num > 0 && len > 0; num /= 10, len--)
			*--pos = '0' + (num % 10);

		if (is_neg)
			*--pos = '-';
	}

	DoWriteText_RightAlign(x, y, pos);

	return 0;
}


// hud.render_world(x, y, w, h)
//
static int HD_render_world(lua_State *L)
{
	float x = luaL_checknumber(L, 1);
	float y = luaL_checknumber(L, 2);
	float w = luaL_checknumber(L, 3);
	float h = luaL_checknumber(L, 4);

	x = FROM_320(x); y = FROM_200(y);
	w = FROM_320(w); h = FROM_200(h);

 	R_Render((int)x, SCREENHEIGHT-(int)(y+h), I_ROUND(w), I_ROUND(h));

	return 0;
}


// hud.render_automap(x, y, w, h)
//
static int HD_render_automap(lua_State *L)
{
	float x = luaL_checknumber(L, 1);
	float y = luaL_checknumber(L, 2);
	float w = luaL_checknumber(L, 3);
	float h = luaL_checknumber(L, 4);

	x = FROM_320(x); y = FROM_200(y);
	w = FROM_320(w); h = FROM_200(h);

 	AM_Drawer((int)x, SCREENHEIGHT-(int)(y+h), I_ROUND(w), I_ROUND(h));

	return 0;
}


static const char * am_color_names[AM_NUM_COLORS] =
{
    "grid",     // AMCOL_Grid

    "wall",     // AMCOL_Wall
    "step",     // AMCOL_Step
    "ledge",    // AMCOL_Ledge
    "ceil",     // AMCOL_Ceil
    "secret",   // AMCOL_Secret
    "allmap",   // AMCOL_Allmap

    "player",   // AMCOL_Player
    "monster",  // AMCOL_Monster
    "corpse",   // AMCOL_Corpse
    "item",     // AMCOL_Item
    "missile",  // AMCOL_Missile
    "scenery"   // AMCOL_Scenery
};


// hud.automap_colors(table)
//
static int HD_automap_colors(lua_State *L)
{
	if (! lua_istable(L, 1))
		I_Error("hud.automap_colors() requires a table!\n");

	for (int which = 0; which < AM_NUM_COLORS; which++)
	{
		lua_getfield(L, 1, am_color_names[which]);
		
		if (! lua_isnil(L, -1))
		{
			AM_SetColor(which, ParseColor(L, -1));
		}

		lua_pop(L, 1);
	}

	return 0;
}


static const luaL_Reg hud_module[] =
{
	{ "raw_debug_print", HD_raw_debug_print },

	// query functions
    { "game_mode",       HD_game_mode },
    { "map_title",  	 HD_map_title },

	// set-state functions
    { "coord_sys",       HD_coord_sys   },
    { "text_font",       HD_text_font   },
    { "text_color",      HD_text_color  },
    { "set_scale",       HD_set_scale   },
    { "set_alpha",       HD_set_alpha   },
    { "automap_colors",  HD_automap_colors },

	// drawing functions
    { "solid_box",       HD_solid_box   },
    { "solid_line",      HD_solid_line  },
    { "thin_box",        HD_thin_box    },
    { "gradient_box",    HD_gradient_box },

    { "draw_image",      HD_draw_image  },
    { "stretch_image",   HD_stretch_image },
    { "tile_image",      HD_tile_image  },
    { "draw_text",       HD_draw_text   },
    { "draw_num2",       HD_draw_num2   },

    { "render_world",    HD_render_world   },
    { "render_automap",  HD_render_automap },

	{ NULL, NULL } // the end
};


//------------------------------------------------------------------------
//  PLAYER MODULE
//------------------------------------------------------------------------


// player.set_who(pnum)
//
static int PL_set_who(lua_State *L)
{
	int pnum = luaL_checkint(L, 1);

	//... FIXME

	return 0;
}


// player.health()
//
static int PL_health(lua_State *L)
{
	lua_pushinteger(L, (int)floor(cur_player->health + 0.99));
	return 1;
}


// player.armor(type)
//
static int PL_armor(lua_State *L)
{
	int kind = luaL_checkint(L, 1);

	if (kind < 1 || kind > NUMARMOUR)
		I_Error("player.armor: bad armor index: %d\n", kind);

	lua_pushinteger(L, (int)floor(cur_player->armours[kind] + 0.99));
	return 1;
}


// player.frags()
//
static int PL_frags(lua_State *L)
{
	lua_pushinteger(L, cur_player->frags);
	return 1;
}


// player.has_key(key)
//
static int PL_has_key(lua_State *L)
{
	int key = luaL_checkint(L, 1);

	if (key < 1 || key > 16)
		I_Error("player.has_key: bad key number: %d\n", key);

	int value = (cur_player->cards & (1 << (key-1))) ? 1 : 0;

	lua_pushboolean(L, value);
	return 1;
}


// player.has_weapon_slot(slot)
//
static int PL_has_weapon_slot(lua_State *L)
{
	int slot = luaL_checkint(L, 1);

	if (slot < 0 || slot > 9)
		I_Error("player.has_weapon_slot: bad slot number: %d\n", slot);

	int value = cur_player->avail_weapons[slot] ? 1 : 0;

	lua_pushboolean(L, value);
	return 1;
}


// player.ammo(type)
//
static int PL_ammo(lua_State *L)
{
	int ammo = luaL_checkint(L, 1);

	if (ammo < 1 || ammo > NUMAMMO)
		I_Error("player.ammo: bad ammo number: %d\n", ammo);

	lua_pushinteger(L, cur_player->ammo[ammo-1].num);
	return 1;
}


// player.ammomax(type)
//
static int PL_ammomax(lua_State *L)
{
	int ammo = luaL_checkint(L, 1);

	if (ammo < 1 || ammo > NUMAMMO)
		I_Error("player.ammomax: bad ammo number: %d\n", ammo);

	lua_pushinteger(L, cur_player->ammo[ammo-1].max);
	return 1;
}


// player.cur_ammo(clip)
//
static int PL_cur_ammo(lua_State *L)
{
	int clip = luaL_checkint(L, 1);

	if (clip < 1 || clip > 2)
		I_Error("player.cur_ammo: bad clip number: %d\n", clip);

	clip--;

	int value = 0;

	if (cur_player->ready_wp >= 0)
	{
		playerweapon_t *pw = &cur_player->weapons[cur_player->ready_wp];

		if (pw->info->ammo[clip] != AM_NoAmmo)
		{
			if (pw->info->show_clip)
			{
				SYS_ASSERT(pw->info->ammopershot[clip] > 0);

				value = pw->clip_size[clip] / pw->info->ammopershot[clip];
			}
			else
			{
				value = cur_player->ammo[pw->info->ammo[clip]].num;

				if (pw->info->clip_size[clip] > 0)
					value += pw->clip_size[clip];
			}
		}
	}

	lua_pushinteger(L, value);
	return 1;
}


static const luaL_Reg player_module[] =
{
	{ "set_who",   PL_set_who },
    { "health",    PL_health  },
    { "armor",     PL_armor   },
    { "frags",     PL_frags   },

    { "has_key",      PL_has_key  },
    { "has_weapon_slot", PL_has_weapon_slot },
    { "ammo",         PL_ammo     },
    { "ammomax",      PL_ammomax  },
    { "cur_ammo",     PL_cur_ammo },

	{ NULL, NULL } // the end
};


//------------------------------------------------------------------------


static int p_init_lua(lua_State *L)
{
	/* stop collector during initialization */
	lua_gc(L, LUA_GCSTOP, 0);
	{
		/* open libraries */

		luaL_openlibs(L);

		/* register our own modules */

		luaL_register(HUD_ST, "hud",    hud_module);
		luaL_register(HUD_ST, "player", player_module);

		// remove the tables which luaL_register created
		lua_pop(L, 2);
	}
	lua_gc(L, LUA_GCRESTART, 0);

	return 0;
}


void LU_Init(void)
{
	HUD_ST = lua_open();

	if (! HUD_ST)
		I_Error("LUA Init failed: cannot create new state");

	int status = lua_cpcall(HUD_ST, &p_init_lua, NULL);

	if (status != 0)
		I_Error("LUA Init failed (status %d)", status);

	// FIXME !!!! make sure script cannot load any DLLs or other scripts
}


void LU_Close(void)
{
	if (HUD_ST)
	{
		lua_close(HUD_ST);
		HUD_ST = NULL;
	}
}


void LU_LoadScripts(void)
{
	static const char *script_lumps[] =
	{
		"LUAHUD0",  // edge.wad - base
		"LUAHUD1",  // edge.wad - hud

		"LUAHUD2",  // user - in a TC
		"LUAHUD3",  // user - for a add-on

		NULL  // end of list
	};

	for (int i = 0; script_lumps[i]; i++)
	{
		int lump = W_CheckNumForName(script_lumps[i]);

		if (lump < 0)
			continue;
	
		I_Printf("Loading LUA lump: %s...\n", script_lumps[i]);

		char *buffer = (char *)W_LoadLumpNum(lump);
		int length   = W_LumpLength(lump);

		int status = luaL_loadbuffer(HUD_ST, buffer, length, script_lumps[i]);

		if (status == 0)
			status = lua_pcall(HUD_ST, 0, 0, 0);

		if (status != 0)
		{
			const char *msg = lua_tolstring(HUD_ST, -1, NULL);

			I_Error("LUA scripts: failure loading '%s' lump (%d)\n%s",
			        script_lumps[i], status, msg);
		}

		has_loaded = true;

		Z_Free(buffer);
	}

	if (! has_loaded)
		I_Error("Missing required LUAHUDx lumps!\n");
}


void LU_BeginLevel(void)
{
	hud_last_time = -1;
}


void LU_RunHud(void)
{
	FrameSetup();


	lua_getglobal(HUD_ST, "stack_trace");

	if (lua_type(HUD_ST, -1) == LUA_TNIL)
		I_Error("LUA scripts: missing function '%s'\n", "stack_trace");

	lua_getglobal(HUD_ST, "hud");
	lua_getfield(HUD_ST, -1, "draw_all");

	if (lua_type(HUD_ST, -1) == LUA_TNIL)
		I_Error("LUA scripts: missing function 'hud.%s'\n", "draw_all");

	int nargs = 0;

	int status = lua_pcall(HUD_ST, nargs, 0, -3-nargs);
	if (status != 0)
	{
		const char *msg = lua_tolstring(HUD_ST, -1, NULL);

		I_Error("LUA script error:\n%s", msg);
	}

	// remove the traceback function and 'hud' table
	lua_pop(HUD_ST, 2);
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
