//------------------------------------------------------------------------
//  COAL HUD module
//------------------------------------------------------------------------
//
//  Copyright (c) 2006-2009  The EDGE Team.
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

#include "coal/coal.h"

#include "ddf/main.h"
#include "ddf/font.h"
//#include "ddf/level.h"

#include "vm_coal.h"
#include "dm_state.h"
#include "e_main.h"
#include "g_game.h"
#include "w_wad.h"

#include "e_player.h"
#include "hu_font.h"
#include "hu_draw.h"
#include "r_modes.h"
#include "am_map.h"     // AM_Drawer
#include "r_colormap.h"
#include "s_sound.h"


extern coal::vm_c *ui_vm;

extern void VM_SetFloat(coal::vm_c *vm, const char *name, double value);
extern void VM_CallFunction(coal::vm_c *vm, const char *name);


player_t *ui_hud_who = NULL;

extern player_t *ui_player_who;


static int hud_last_time = -1;

extern std::string w_map_title;


//------------------------------------------------------------------------


rgbcol_t VM_VectorToColor(double * v)
{
	if (v[0] < 0)
		return RGB_NO_VALUE;

	int r = CLAMP(0, (int)v[0], 255);
	int g = CLAMP(0, (int)v[1], 255);
	int b = CLAMP(0, (int)v[2], 255);

	rgbcol_t rgb = RGB_MAKE(r, g, b);

	// ensure we don't get the "no color" value by mistake
	if (rgb == RGB_NO_VALUE)
		rgb ^= 0x000101;

	return rgb;
}


//------------------------------------------------------------------------
//  HUD MODULE
//------------------------------------------------------------------------


// hud.coord_sys(w, h)
//
static void HD_coord_sys(coal::vm_c *vm, int argc)
{
	int w = (int) *vm->AccessParam(0);
	int h = (int) *vm->AccessParam(1);

	if (w < 64 || h < 64)
		I_Error("Bad hud.coord_sys size: %dx%d\n", w, h);

	HUD_SetCoordSys(w, h);
}


// hud.game_mode()
//
static void HD_game_mode(coal::vm_c *vm, int argc)
{
	if (DEATHMATCH())
		vm->ReturnString("dm");
	else if (COOP_MATCH())
		vm->ReturnString("coop");
	else
		vm->ReturnString("sp");
}


// hud.game_name()
//
static void HD_game_name(coal::vm_c *vm, int argc)
{
	gamedef_c *g = currmap->episode;
	SYS_ASSERT(g);

	vm->ReturnString(g->ddf.name.c_str());
}


// hud.map_name()
//
static void HD_map_name(coal::vm_c *vm, int argc)
{
	vm->ReturnString(currmap->ddf.name.c_str());
}


// hud.map_title()
//
static void HD_map_title(coal::vm_c *vm, int argc)
{
	vm->ReturnString(w_map_title.c_str());
}


// hud.which_hud()
//
static void HD_which_hud(coal::vm_c *vm, int argc)
{
	vm->ReturnFloat((double)screen_hud);
}


// hud.check_automap()
//
static void HD_check_automap(coal::vm_c *vm, int argc)
{
	vm->ReturnFloat(automapactive ? 1 : 0);
}


// hud.get_time()
//
static void HD_get_time(coal::vm_c *vm, int argc)
{
	vm->ReturnFloat((double) I_GetTime());
}


// hud.text_font(name)
//
static void HD_text_font(coal::vm_c *vm, int argc)
{
	const char *font_name = vm->AccessParamString(0);

	fontdef_c *DEF = fontdefs.Lookup(font_name);
	SYS_ASSERT(DEF);

	font_c *font = hu_fonts.Lookup(DEF);
	SYS_ASSERT(font);

	HUD_SetFont(font);
}


// hud.text_color(rgb)
//
static void HD_text_color(coal::vm_c *vm, int argc)
{
	double * v = vm->AccessParam(0);

	rgbcol_t color = VM_VectorToColor(v);

	HUD_SetTextColor(color);
}


// hud.set_scale(value)
//
static void HD_set_scale(coal::vm_c *vm, int argc)
{
	float scale = *vm->AccessParam(0);

	if (scale <= 0)
		I_Error("hud.set_scale: Bad scale value: %1.3f\n", scale);

	HUD_SetScale(scale);
}


// hud.set_alpha(value)
//
static void HD_set_alpha(coal::vm_c *vm, int argc)
{
	float alpha = *vm->AccessParam(0);

	HUD_SetAlpha(alpha);
}


// hud.solid_box(x, y, w, h, color)
//
static void HD_solid_box(coal::vm_c *vm, int argc)
{
	float x = *vm->AccessParam(0);
	float y = *vm->AccessParam(1);
	float w = *vm->AccessParam(2);
	float h = *vm->AccessParam(3);

	rgbcol_t rgb = VM_VectorToColor(vm->AccessParam(4));

	HUD_SolidBox(x, y, x+w, y+h, rgb);
}


// hud.solid_line(x1, y1, x2, y2, color)
//
static void HD_solid_line(coal::vm_c *vm, int argc)
{
	float x1 = *vm->AccessParam(0);
	float y1 = *vm->AccessParam(1);
	float x2 = *vm->AccessParam(2);
	float y2 = *vm->AccessParam(3);

	rgbcol_t rgb = VM_VectorToColor(vm->AccessParam(4));

	HUD_SolidLine(x1, y1, x2, y2, rgb);
}


// hud.thin_box(x, y, w, h, color)
//
static void HD_thin_box(coal::vm_c *vm, int argc)
{
	float x = *vm->AccessParam(0);
	float y = *vm->AccessParam(1);
	float w = *vm->AccessParam(2);
	float h = *vm->AccessParam(3);

	rgbcol_t rgb = VM_VectorToColor(vm->AccessParam(4));

	HUD_ThinBox(x, y, x+w, y+h, rgb);
}


// hud.gradient_box(x, y, w, h, TL, BL, TR, BR)
//
static void HD_gradient_box(coal::vm_c *vm, int argc)
{
	float x = *vm->AccessParam(0);
	float y = *vm->AccessParam(1);
	float w = *vm->AccessParam(2);
	float h = *vm->AccessParam(3);

	rgbcol_t cols[4];

	cols[0] = VM_VectorToColor(vm->AccessParam(4));
	cols[1] = VM_VectorToColor(vm->AccessParam(5));
	cols[2] = VM_VectorToColor(vm->AccessParam(6));
	cols[3] = VM_VectorToColor(vm->AccessParam(7));

	HUD_GradientBox(x, y, x+w, y+h, cols);
}


// hud.draw_image(x, y, name)
//
static void HD_draw_image(coal::vm_c *vm, int argc)
{
	float x = *vm->AccessParam(0);
	float y = *vm->AccessParam(1);

	const char *name = vm->AccessParamString(2);

	const image_c *img = W_ImageLookup(name, INS_Graphic);

	if (img)
	{
		HUD_DrawImage(x, y, img);
	}
}


// hud.stretch_image(x, y, w, h, name)
//
static void HD_stretch_image(coal::vm_c *vm, int argc)
{
	float x = *vm->AccessParam(0);
	float y = *vm->AccessParam(1);
	float w = *vm->AccessParam(2);
	float h = *vm->AccessParam(3);

	const char *name = vm->AccessParamString(4);

	const image_c *img = W_ImageLookup(name, INS_Graphic);

	if (img)
	{
		HUD_StretchImage(x, y, w, h, img);
	}
}


// hud.tile_image(x, y, w, h, name, [offset_x, offset_y])
//
static void HD_tile_image(coal::vm_c *vm, int argc)
{
	float x = *vm->AccessParam(0);
	float y = *vm->AccessParam(1);
	float w = *vm->AccessParam(2);
	float h = *vm->AccessParam(3);

	const char *name = vm->AccessParamString(4);

	float offset_x = 0;
	float offset_y = 0;

//???	if (lua_isnumber(L, 6))
//???		offset_x = lua_tonumber(L, 6);
//???
//???	if (lua_isnumber(L, 7))
//???		offset_y = lua_tonumber(L, 7);

	const image_c *img = W_ImageLookup(name, INS_Texture);

	if (img)
	{
		HUD_TileImage(x, y, w, h, img, offset_x, offset_y);
	}
}


// hud.draw_text(x, y, str)
//
static void HD_draw_text(coal::vm_c *vm, int argc)
{
	float x = *vm->AccessParam(0);
	float y = *vm->AccessParam(1);

	const char *str = vm->AccessParamString(2);

	HUD_DrawText(x, y, str);
}


// hud.draw_num2(x, y, len, num)
//
static void HD_draw_num2(coal::vm_c *vm, int argc)
{
	float x = *vm->AccessParam(0);
	float y = *vm->AccessParam(1);

	int len = (int) *vm->AccessParam(2);
	int num = (int) *vm->AccessParam(3);

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

	HUD_SetAlignment(+1, -1);
	HUD_DrawText(x, y, pos);
	HUD_SetAlignment();
}


// hud.render_world(x, y, w, h, [options])
//
static void HD_render_world(coal::vm_c *vm, int argc)
{
	float x = *vm->AccessParam(0);
	float y = *vm->AccessParam(1);
	float w = *vm->AccessParam(2);
	float h = *vm->AccessParam(3);

 	HUD_RenderWorld(x, y, x+w, y+h, ui_hud_who->mo);
}


#if 0  // FIXME
static void ParseAutomapOptions(coal::vm_c *vm, int argc, int IDX, int *state, float *zoom)
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
#endif


// hud.render_automap(x, y, w, h, [options])
//
static void HD_render_automap(coal::vm_c *vm, int argc)
{
	float x = *vm->AccessParam(0);
	float y = *vm->AccessParam(1);
	float w = *vm->AccessParam(2);
	float h = *vm->AccessParam(3);

	int   old_state;
	float old_zoom;

	AM_GetState(&old_state, &old_zoom);

//???	if (lua_istable(L, 5))
//???	{
//???		int   new_state = old_state;
//???		float new_zoom  = old_zoom;
//???
//???		ParseAutomapOptions(L, 5, &new_state, &new_zoom);
//???
//???		AM_SetState(new_state, new_zoom);
//???	}

 	AM_Drawer(x, y, w, h, ui_hud_who->mo);

	AM_SetState(old_state, old_zoom);
}


#if 0  // FIXME
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
static void HD_automap_colors(coal::vm_c *vm, int argc)
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
#endif


// hud.set_render_who(index)
//
static void HD_set_render_who(coal::vm_c *vm, int argc)
{
	int index = (int) *vm->AccessParam(0);

	if (index < 0 || index >= numplayers)
		I_Error("hud.set_render_who: bad index value: %d (numplayers=%d)\n", index, numplayers);

	if (index == 0)
	{
		ui_hud_who = players[consoleplayer];
		return;
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

	ui_hud_who = players[who];
}


// hud.play_sound(name)
//
static void HD_play_sound(coal::vm_c *vm, int argc)
{
	const char *name = vm->AccessParamString(0);

	sfx_t *fx = sfxdefs.GetEffect(name);

	if (! fx)
		I_Error("Lua script problem: unknown sound '%s'\n", name);

	// FIXME: support 'volume' parameter

	S_StartFX(fx);
}


//------------------------------------------------------------------------
// HUD Functions
//------------------------------------------------------------------------

void VM_RegisterHUD()
{
	// query functions
    ui_vm->AddNativeFunction("hud.game_mode",       HD_game_mode);
    ui_vm->AddNativeFunction("hud.game_name",       HD_game_name);
    ui_vm->AddNativeFunction("hud.map_name",  	    HD_map_name);
    ui_vm->AddNativeFunction("hud.map_title",  	    HD_map_title);

    ui_vm->AddNativeFunction("hud.which_hud",  	    HD_which_hud);
    ui_vm->AddNativeFunction("hud.check_automap",  	HD_check_automap);
    ui_vm->AddNativeFunction("hud.get_time",  	    HD_get_time);

	// set-state functions
    ui_vm->AddNativeFunction("hud.coord_sys",       HD_coord_sys);
    ui_vm->AddNativeFunction("hud.text_font",       HD_text_font);
    ui_vm->AddNativeFunction("hud.text_color",      HD_text_color);
    ui_vm->AddNativeFunction("hud.set_scale",       HD_set_scale);
    ui_vm->AddNativeFunction("hud.set_alpha",       HD_set_alpha);
/// ui_vm->AddNativeFunction("hud.automap_colors",  HD_automap_colors);

	// drawing functions
    ui_vm->AddNativeFunction("hud.solid_box",       HD_solid_box);
    ui_vm->AddNativeFunction("hud.solid_line",      HD_solid_line);
    ui_vm->AddNativeFunction("hud.thin_box",        HD_thin_box);
    ui_vm->AddNativeFunction("hud.gradient_box",    HD_gradient_box);

    ui_vm->AddNativeFunction("hud.draw_image",      HD_draw_image);
    ui_vm->AddNativeFunction("hud.stretch_image",   HD_stretch_image);
    ui_vm->AddNativeFunction("hud.tile_image",      HD_tile_image);
    ui_vm->AddNativeFunction("hud.draw_text",       HD_draw_text);
    ui_vm->AddNativeFunction("hud.draw_num2",       HD_draw_num2);

    ui_vm->AddNativeFunction("hud.render_world",    HD_render_world);
    ui_vm->AddNativeFunction("hud.render_automap",  HD_render_automap);
	ui_vm->AddNativeFunction("hud.set_render_who",  HD_set_render_who);

	// sound functions
	ui_vm->AddNativeFunction("hud.play_sound",      HD_play_sound);
}

void VM_BeginLevel(void)
{
	hud_last_time = -1;
}

void VM_RunHud(void)
{ 
	HUD_Reset();

	ui_hud_who    = players[displayplayer];
	ui_player_who = players[displayplayer];

	// TODO: remove this
	int now_time = I_GetTime();
	int passed_time = 0;

	if (hud_last_time > 0 && hud_last_time <= now_time)
	{
		passed_time = MIN(now_time - hud_last_time, TICRATE);
	}

	hud_last_time = now_time;


	// setup some fields in 'hud' module

//??	VM_SetFloat(ui_vm, "hud.which", m_screenhud.d);
//??	VM_SetFloat(ui_vm, "hud.automap", automapactive ? 1 : 0);
//??	VM_SetFloat(ui_vm, "hud.now_time", now_time);
//??	VM_SetFloat(ui_vm, "hud.passed_time", passed_time);

    VM_CallFunction(ui_vm, "draw_all");
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
