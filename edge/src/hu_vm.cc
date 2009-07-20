//------------------------------------------------------------------------
//  LUA HUD module
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

#include "ddf/font.h"
#include "ddf/game.h"
#include "ddf/level.h"

#include "l_lua.h"
#include "hu_vm.h"
#include "p_vm.h"

#include "g_state.h"
#include "e_main.h"
#include "g_game.h"
#include "w_wad.h"

#include "e_player.h"
#include "hu_font.h"
#include "hu_draw.h"
#include "r_modes.h"
#include "r_misc.h"     //  R_Render
#include "am_map.h"     // AM_Drawer
#include "r_colormap.h"
#include "s_sound.h"


static font_c *cur_font = NULL;
static rgbcol_t cur_color = RGB_NO_VALUE;
static float cur_scale;
static float cur_alpha;

static float cur_coord_W;
static float cur_coord_H;

static player_t *render_player = NULL;

static int hud_last_time = -1;

extern std::string w_map_title;


#define COORD_X(x)  ((x) * SCREENWIDTH  / cur_coord_W)
#define COORD_Y(y)  ((y) * SCREENHEIGHT / cur_coord_H)


//------------------------------------------------------------------------

static void FrameSetup(void)
{
	fontdef_c *DEF = fontdefs.Lookup("DOOM");  // FIXME allow other default
	SYS_ASSERT(DEF);

	cur_font = HU_LookupFont(DEF);
	SYS_ASSERT(cur_font);

	cur_color  = RGB_NO_VALUE;
	cur_scale  = 1.0f;
	cur_alpha  = 1.0f;

	cur_coord_W = 320;
	cur_coord_H = 200;

	render_player = players[displayplayer];


	int now_time = I_GetTime();
	int passed_time = 0;

	if (hud_last_time > 0 && hud_last_time <= now_time)
	{
		passed_time = MIN(now_time - hud_last_time, TICRATE);
	}

	hud_last_time = now_time;


	// setup some fields in 'hud' module

	vm->SetIntVar("hud", "which", m_screenhud.d);
	vm->SetIntVar("hud", "now_time", now_time);
	vm->SetIntVar("hud", "passed_time", passed_time);

	vm->SetBoolVar("hud", "automap", automapactive);
}


static void DoDrawChar(float cx, float cy, char ch)
{
	const image_c *image = cur_font->CharImage(ch);

	if (! image)
		return;
	
	float sc_x = cur_scale;  // * aspect
	float sc_y = cur_scale;

	cx -= IM_OFFSETX(image) * sc_x;
	cy -= IM_OFFSETY(image) * sc_y;

	RGL_DrawImage(
	    COORD_X(cx),
		SCREENHEIGHT - COORD_Y(cy + IM_HEIGHT(image) * sc_y),
		COORD_X(IM_WIDTH(image))  * sc_x,
		COORD_Y(IM_HEIGHT(image)) * sc_y,
		image, 0.0f, 0.0f,
		IM_RIGHT(image), IM_TOP(image),
		cur_alpha, cur_color);
}

static void DoWriteText(float x, float y, const char *str)
{
	float cx = x;
	float cy = y;

	float f_h = cur_font->NominalHeight() * 1.5;

	for (; *str; str++)
	{
		char ch = *str;

		if (ch == '\n')
		{
			cx = x;
			cy += f_h * cur_scale;
			continue;
		}

		if (COORD_X(cx) >= SCREENWIDTH)
			continue;

		DoDrawChar(cx, cy, ch);

		cx += cur_font->CharWidth(ch) * cur_scale;
	}
}

static void DoWriteText_RightAlign(float x, float y, const char *str)
{
	float cx = x;
	float cy = y;

	float f_h = cur_font->NominalHeight() * 1.5;

	for (int pos = strlen(str)-1; pos >= 0; pos--)
	{
		char ch = str[pos];

		if (ch == '\n')
		{
			cx = x;
			cy += f_h * cur_scale;
			continue;
		}

		if (COORD_X(cx) >= SCREENWIDTH)
			continue;

		cx -= cur_font->CharWidth(ch) * cur_scale;

		DoDrawChar(cx, cy, ch);
	}
}

static rgbcol_t ParseColor(lua_State *L, int index)
{
	if (lua_isstring(L, index))
	{
		const char *name = lua_tostring(L, index);

		return V_ParseFontColor(name, true);
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

		rgbcol_t rgb = RGB_MAKE(r, g, b);

		if (rgb == RGB_NO_VALUE)
			rgb ^= 0x000101;

		return rgb;
	}

	I_Error("Bad color value in lua script!\n");
	return 0; /* NOT REACHED */
}


//------------------------------------------------------------------------
//  HUD MODULE
//------------------------------------------------------------------------

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

	if (w < 64 || h < 64)
		I_Error("Bad hud.coord_sys size: %dx%d\n", w, h);

	cur_coord_W = w;
	cur_coord_H = h;

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


// hud.game_name()
//
static int HD_game_name(lua_State *L)
{
	gamedef_c *g = currmap->episode;
	SYS_ASSERT(g);

	lua_pushstring(L, g->ddf.name.c_str());
	return 1;
}


// hud.map_name()
//
static int HD_map_name(lua_State *L)
{
	lua_pushstring(L, currmap->ddf.name.c_str());
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

	cur_font = HU_LookupFont(DEF);
	SYS_ASSERT(cur_font);

	HUD_SetFont(cur_font);
	return 0;
}


// hud.text_color(name)
//
static int HD_text_color(lua_State *L)
{
	rgbcol_t color = RGB_NO_VALUE;

	int nargs = lua_gettop(L);
	if (nargs >= 1)
		color = ParseColor(L, 1);

	HUD_SetTextColor(color);
	return 0;
}


// hud.set_scale(value)
//
static int HD_set_scale(lua_State *L)
{
	float scale = luaL_checknumber(L, 1);

	if (scale <= 0)
		I_Error("hud.set_scale: Bad scale value: %1.3f\n", scale);

	HUD_SetScale(scale);
	return 0;
}


// hud.set_alpha(value)
//
static int HD_set_alpha(lua_State *L)
{
	float alpha = luaL_checknumber(L, 1);

	HUD_SetAlpha(alpha);
	return 0;
}


// hud.solid_box(x, y, w, h, color)
//
static int HD_solid_box(lua_State *L)
{
	float x = luaL_checknumber(L, 1);
	float y = luaL_checknumber(L, 2);
	float w = luaL_checknumber(L, 3);
	float h = luaL_checknumber(L, 4);

	rgbcol_t rgb = ParseColor(L, 5);

	HUD_SolidBox(x, y, w, h, rgb);
	return 0;
}


// hud.solid_line(x1, y1, x2, y2, color)
//
static int HD_solid_line(lua_State *L)
{
	float x1 = luaL_checknumber(L, 1);
	float y1 = luaL_checknumber(L, 2);
	float x2 = luaL_checknumber(L, 3);
	float y2 = luaL_checknumber(L, 4);

	rgbcol_t rgb = ParseColor(L, 5);

	HUD_SolidLine(x1, y1, x2, y2, rgb);
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

	rgbcol_t rgb = ParseColor(L, 5);

	HUD_ThinBox(x, y, w, h, rgb);
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

	rgbcol_t cols[4];

	cols[0] = ParseColor(L, 5);
	cols[1] = ParseColor(L, 6);
	cols[2] = ParseColor(L, 7);
	cols[3] = ParseColor(L, 8);

	HUD_GradientBox(x, y, w, h, cols);
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
		HUD_DrawImage(x, y, img);
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
		HUD_StretchImage(x, y, w, h, img);
	}

	return 0;
}


// hud.tile_image(x, y, w, h, name, [offset_x, offset_y])
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
		HUD_TileImage(x, y, w, h, img, offset_x, offset_y);
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


// hud.render_world(x, y, w, h, [options])
//
static int HD_render_world(lua_State *L)
{
	float x = luaL_checknumber(L, 1);
	float y = luaL_checknumber(L, 2);
	float w = luaL_checknumber(L, 3);
	float h = luaL_checknumber(L, 4);

	// FIXME !!!!!!
	x = COORD_X(x); y = COORD_Y(y);
	w = COORD_X(w); h = COORD_Y(h);

 	R_Render((int)x, SCREENHEIGHT-(int)(y+h), I_ROUND(w), I_ROUND(h),
			 render_player->mo);

	return 0;
}


static void ParseAutomapOptions(lua_State *L, int IDX, int *state, float *zoom)
{
	lua_getfield(L, IDX, "zoom");

	if (! lua_isnil(L, -1))
	{
		(*zoom) = luaL_checknumber(L, -1);

		// impose a very broad limit
		(*zoom) = CLAMP(0.2f, *zoom, 100.0f);
	}

	lua_pop(L, 1);


	static const char * st_names[6] =
	{
		"grid",   "rotate", "follow",
		"things", "walls",  "allmap"
	};
	static const int st_flags[6] =
	{
		AMST_Grid,   AMST_Rotate, AMST_Follow,
		AMST_Things, AMST_Walls,  AMST_Allmap
	};

	for (int k = 0; k < 6; k++)
	{
		lua_getfield(L, IDX, st_names[k]);

		if (! lua_isnil(L, -1))
		{
			if (lua_toboolean(L, -1))
				(*state) |= st_flags[k];
			else
				(*state) &= ~st_flags[k];
		}

		lua_pop(L, 1);
	}
}


// hud.render_automap(x, y, w, h, [options])
//
static int HD_render_automap(lua_State *L)
{
	float x = luaL_checknumber(L, 1);
	float y = luaL_checknumber(L, 2);
	float w = luaL_checknumber(L, 3);
	float h = luaL_checknumber(L, 4);

	int   old_state;
	float old_zoom;

	AM_GetState(&old_state, &old_zoom);

	if (lua_istable(L, 5))
	{
		int   new_state = old_state;
		float new_zoom  = old_zoom;

		ParseAutomapOptions(L, 5, &new_state, &new_zoom);

		AM_SetState(new_state, new_zoom);
	}

 	AM_Drawer(x, y, w, h, render_player->mo);

	AM_SetState(old_state, old_zoom);

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

// hud.set_render_who(index)
//
static int HD_set_render_who(lua_State *L)
{
	int index = luaL_checkint(L, 1);

	if (index < 0 || index >= numplayers)
		I_Error("hud.set_render_who: bad index value: %d (numplayers=%d)\n", index, numplayers);

	if (index == 0)
	{
		render_player = players[consoleplayer];
		return 0;
	}

	int who = displayplayer;

	for (; index > 1; index--)
	{
		do
		{
			who = (who + 1) % MAXPLAYERS;
		}
		while (players[who] == NULL);
	}

	render_player = players[who];
	return 0;
}


// hud.play_sound(name, [volume])
//
static int HD_play_sound(lua_State *L)
{
	const char *name = luaL_checkstring(L, 1);

	float volume = 1.0f;

	int nargs = lua_gettop(L);
	if (nargs >= 2)
		volume = luaL_checknumber(L, 2) / 100.0f;

	if (volume <= 0)
		return 0;

	sfx_t *fx = sfxdefs.GetEffect(name);

	if (! fx)
		I_Error("Lua script problem: unknown sound '%s'\n", name);

	// FIXME: support 'volume' parameter

	S_StartFX(fx);
	return 0;
}


const luaL_Reg hud_module[] =
{
	{ "raw_debug_print", HD_raw_debug_print },

	// query functions
    { "game_mode",       HD_game_mode },
    { "game_name",       HD_game_name },
    { "map_name",  	     HD_map_name },
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
	{ "set_render_who",  HD_set_render_who },

	// sound functions
	{ "play_sound",      HD_play_sound },

	{ NULL, NULL } // the end
};


//------------------------------------------------------------------------
// HUD Functions
//------------------------------------------------------------------------

void HU_RegisterModules()
{
    vm->LoadModule("hud", hud_module);
}

void HU_BeginLevel(void)
{
	hud_last_time = -1;
}

void HU_RunHud(void)
{ 
    P_SetupPlayerModule(displayplayer);

	FrameSetup();

    vm->CallFunction("hud", "draw_all");
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
