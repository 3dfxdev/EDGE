//----------------------------------------------------------------------------
//  EDGE OpenGL Rendering (Things)
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2005  The EDGE Team.
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

// this conditional applies to the whole file
#include "i_defs.h"

#include "dm_defs.h"
#include "dm_state.h"
#include "e_search.h"
#include "g_game.h"
#include "m_bbox.h"
#include "m_misc.h"
#include "m_random.h"
#include "p_local.h"
#include "p_mobj.h"
#include "r_defs.h"
#include "r_main.h"
#include "r_sky.h"
#include "r_state.h"
#include "r_things.h"
#include "r2_defs.h"
#include "rgl_defs.h"
#include "st_stuff.h"
#include "v_colour.h"
#include "v_res.h"
#include "w_textur.h"
#include "w_wad.h"
#include "v_ctx.h"
#include "z_zone.h"


#define DEBUG  0

#define HALOS    0


float sprite_skew;

angle_t fuzz_ang_tl;
angle_t fuzz_ang_tr;
angle_t fuzz_ang_bl;
angle_t fuzz_ang_br;

// colour of the player's weapon
int rgl_weapon_r;
int rgl_weapon_g;
int rgl_weapon_b;


// The minimum distance between player and a visible sprite.
#define MINZ        (4.0f)


void RGL_UpdateTheFuzz(void)
{
	// fuzzy warping effect

	fuzz_ang_tl += FLOAT_2_ANG(90.0f / 17.0f);
	fuzz_ang_tr += FLOAT_2_ANG(90.0f / 11.0f);
	fuzz_ang_bl += FLOAT_2_ANG(90.0f /  8.0f);
	fuzz_ang_br += FLOAT_2_ANG(90.0f / 21.0f);
}

//
// RGL_DrawPSprite
//
static void RGL_DrawPSprite(pspdef_t * psp, int which,
							player_t * player, region_properties_t *props,
							const state_t *state)
{
	// determine sprite patch
	bool flip;
	const image_t *image = R2_GetOtherSprite(state->sprite, state->frame, &flip);

	if (!image)
		return;

	const cached_image_t *cim = W_ImageCache(image);

	GLuint tex_id = W_ImageGetOGL(cim);

	float w = IM_WIDTH(image);
	float h = IM_HEIGHT(image);
	float right = IM_RIGHT(image);
	float bottom = IM_BOTTOM(image);

	int fuzzy = (player->mo->flags & MF_FUZZY);

	float trans = fuzzy ? FUZZY_TRANS : player->mo->visibility;

	if (which == ps_crosshair)
	{
		fuzzy = false;
		trans = 1.0f;
	}

	trans *= psp->visibility;

	if (trans < 0.04f)
		return;

	// psprites are never totally solid and opaque
	glEnable(GL_ALPHA_TEST);
	if (trans <= 0.99 || use_smoothing)
		glEnable(GL_BLEND);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, tex_id);

	float tex_top_h = 1.00f; // 0.98;
	float tex_bot_h = 1.00f - bottom;  // 1.02 - bottom;

	float tex_x1 = 0.01f;
	float tex_x2 = right - 0.01f;

	if (flip)
	{
		tex_x1 = right - tex_x1;
		tex_x2 = right - tex_x2;
	}

	float tx1 = (160.0f - IM_WIDTH(image) / 2.0) + psp->sx - IM_OFFSETX(image);
	float tx2 = tx1 + w;

	float ty1 = WEAPONTOP - psp->sy + IM_OFFSETY(image);
	float ty2 = ty1 + h;

	// compute lighting
	int L_r, L_g, L_b;

	if (which == ps_weapon || which == ps_flash)
	{
		int L = state->bright ? 255 : props->lightlevel;

		float c_r, c_g, c_b;
		V_GetColmapRGB(props->colourmap, &c_r, &c_g, &c_b, false);

		if (doom_fading)
		{
			L = EMU_LIGHT(L, 160.0f);
			L = RGL_LightEmu((L < 0) ? 0 : (L > 255) ? 255 : L);
		}
		else
			L = RGL_Light(L);

		L_r = (int)(rgl_weapon_r + L * c_r);
		L_g = (int)(rgl_weapon_g + L * c_g);
		L_b = (int)(rgl_weapon_b + L * c_b);
	}
	else
	{
		L_r = L_g = L_b = 255;
	}

	float x1b, y1b, x1t, y1t, x2b, y2b, x2t, y2t;  // screen coords

	x1b = x1t = viewwindowwidth  * tx1 / 320.0f;
	x2b = x2t = viewwindowwidth  * tx2 / 320.0f;

	y1b = y2b = viewwindowheight * ty1 / 200.0f;
	y1t = y2t = viewwindowheight * ty2 / 200.0f;

	if (fuzzy)
	{
		float range_x = (float)fabs(x2b - x1b) / 12.0f;
		float range_y = (float)fabs(y1t - y1b) / 12.0f;

		float tl_x = M_Sin(fuzz_ang_tl);
		float tr_x = M_Sin(fuzz_ang_tr);
		float bl_x = M_Sin(fuzz_ang_bl);
		float br_x = M_Sin(fuzz_ang_br);

		float tl_y = M_Cos(fuzz_ang_tl);
		float tr_y = M_Cos(fuzz_ang_tr);
    
		// don't adjust the bottom Y positions
    
		x1b += bl_x * range_x;
		x1t += tl_x * range_x; y1t += tl_y * range_y;
		x2t += tr_x * range_x; y2t += tr_y * range_y;
		x2b += br_x * range_x;

		L_r = L_g = L_b = 50;
	}

	// clip psprite to view window
	glEnable(GL_SCISSOR_TEST);

	glScissor(viewwindowx, SCREENHEIGHT-viewwindowheight-viewwindowy,
			  viewwindowwidth, viewwindowheight);

	x1b = (float)viewwindowx + x1b;
	x1t = (float)viewwindowx + x1t;
	x2t = (float)viewwindowx + x2t;
	x2b = (float)viewwindowx + x2b;

	y1b = (float)(SCREENHEIGHT - viewwindowy - viewwindowheight) + y1b - 1;
	y1t = (float)(SCREENHEIGHT - viewwindowy - viewwindowheight) + y1t - 1;
	y2t = (float)(SCREENHEIGHT - viewwindowy - viewwindowheight) + y2t - 1;
	y2b = (float)(SCREENHEIGHT - viewwindowy - viewwindowheight) + y2b - 1;
 
	glColor4f(LT_RED(L_r), LT_GRN(L_g), LT_BLU(L_b), trans);

	glBegin(GL_QUADS);
  
	glTexCoord2f(tex_x1, tex_bot_h);
	glVertex2f(x1b, y1b);
  
	glTexCoord2f(tex_x1, tex_top_h);
	glVertex2f(x1t, y1t);
  
	glTexCoord2f(tex_x2, tex_top_h);
	glVertex2f(x2t, y1t);
  
	glTexCoord2f(tex_x2, tex_bot_h);
	glVertex2f(x2b, y2b);
  
	glEnd();

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_BLEND);
	glDisable(GL_SCISSOR_TEST);

	W_ImageDone(cim);
}

static void DrawStdCrossHair(int sbarheight)
{
	static int crhcount = 0;
	static int crhdir = 1;   // -ACB- 1999/09/19 change from ch * to crh *. chdir is a function.

	// -jc- Pulsating
	if (crhcount == 31)
		crhdir = -1;
	else if (crhcount == 0)
		crhdir = 1;

	crhcount += crhdir;

	int col = RED + crhcount / 4;  /* FIXME: configurable colour ? */
	int mul = 1 + (SCREENWIDTH / 300);

	int x = SCREENWIDTH / 2;
	int y = (SCREENHEIGHT - sbarheight) / 2;

	switch (crosshair)
	{
		case 1:
			RGL_SolidLine(x - 3*mul, y, x - 2*mul, y, col);
			RGL_SolidLine(x + 2*mul, y, x + 3*mul, y, col);
			RGL_SolidLine(x, y - 3*mul, x, y - 2*mul, col);
			RGL_SolidLine(x, y + 2*mul, x, y + 3*mul, col);
			break;
	
		case 2:
			RGL_SolidLine(x, y, x + 1, y, col);
			break;
	
		case 3:
			RGL_SolidLine(x, y, x + 2*mul, y, col);
			RGL_SolidLine(x, y + 1, x, y + 2*mul, col);
			break;
	
		default:
			break;
	}
}


//
// RGL_DrawPlayerSprites
//
void RGL_DrawPlayerSprites(player_t * p)
{
	bool got_cross = false;

	// special handling for zoom: show viewfinder
	if (viewiszoomed)
	{
		weapondef_c *w;
		pspdef_t *psp = &p->psprites[ps_weapon];

		if ((p->ready_wp < 0) || (psp->state == S_NULL))
			return;

		w = p->weapons[p->ready_wp].info;

		if (w->zoom_state <= 0)
			return;

		RGL_DrawPSprite(psp, ps_weapon, p, view_props,
						states + w->zoom_state);
	}
	else
	{
		// add all active psprites
		// Note: order is significant

		for (int i = 0; i < NUMPSPRITES; i++)
		{
			pspdef_t *psp = &p->psprites[i];

			if (psp->state == S_NULL)
				continue;

			RGL_DrawPSprite(psp, i, p, view_props, psp->state);

			if (i == ps_crosshair)
				got_cross = true;
		}
	}

	if (!got_cross && !automapactive && p->health > 0)
		DrawStdCrossHair((screen_hud != HUD_Full) ? 0 : FROM_200(ST_HEIGHT));
}

void RGL_LightUpPlayerWeapon(player_t *p, drawfloor_t *dfloor)
{
	drawthing_t *dl;

	float x, y, z;

	x = p->mo->x;
	y = p->mo->y;
	z = p->mo->z + p->mo->height *
		PERCENT_2_FLOAT(p->mo->info->shotheight);

	for (dl=dfloor->dlights; dl; dl=dl->next)
	{
		R2_AddColourDLights(1, &rgl_weapon_r, &rgl_weapon_g, &rgl_weapon_b, 
			&x, &y, &z, dl->mo);
	}
}


// ============================================================================
// R2_BSP START
// ============================================================================

int sprite_kludge = 0;

static INLINE void LinkDrawthingIntoDrawfloor(drawthing_t *dthing,
											  drawfloor_t *dfloor)
{
	dthing->props = dfloor->props;
	dthing->next  = dfloor->things;
	dthing->prev  = NULL;

	if (dfloor->things)
		dfloor->things->prev = dthing;

	dfloor->things = dthing;

	//
	// Dynamic Lighting
	//
	dthing->extra_light = 0;

}

//
// R2_GetThingSprite
//
// Can return NULL, for no image.
//
const image_t * R2_GetThingSprite(mobj_t *mo, bool *flip)
{
	// decide which patch to use for sprite relative to player

	if (mo->sprite == SPR_NULL)
		return NULL;

#ifdef DEVELOPERS
	// this shouldn't happen
	if ((unsigned int)mo->sprite >= (unsigned int)numsprites)
		I_Error("R2_GetThingSprite: invalid sprite number %i.\n", mo->sprite);
#endif

	spritedef_c *sprite = sprites[mo->sprite];

	if (mo->frame >= sprite->numframes ||
		!sprite->frames[mo->frame].finished)
	{
#if 1
		// -AJA- 2001/08/04: allow the patch to be missing
		(*flip) = false;
		return W_ImageForDummySprite();
#else
		// -ACB- 1998/06/29 More Informative Error Message
		I_Error("R2_GetThingSprite: Invalid sprite frame %s:%c",
			sprite->name, 'A' + mo->frame);
#endif
	}

	spriteframe_c *frame = sprite->frames + mo->frame;

	int rot = 0;

	if (frame->rotated)
		rot = frame->CalcRot(mo->angle,
			R_PointToAngle(viewx, viewy, mo->x, mo->y));

	DEV_ASSERT2(0 <= rot && rot < 16);

	(*flip) = frame->flip[rot] ? true : false;

	return frame->images[rot];
}

//
// R2_GetOtherSprite
//
// Used for non-object stuff, like weapons and finale.
//
const image_t * R2_GetOtherSprite(int spritenum, int framenum, bool *flip)
{
	spritedef_c *sprite;
	spriteframe_c *frame;

	if (spritenum == SPR_NULL)
		return NULL;

#ifdef DEVELOPERS
	// this shouldn't happen
	if ((unsigned int)spritenum >= (unsigned int)numsprites)
		I_Error("R2_GetThingSprite: invalid sprite number %i.\n", spritenum);
#endif

	sprite = sprites[spritenum];

	if (framenum >= sprite->numframes ||
		!sprite->frames[framenum].finished)
	{
#if 1
		return NULL;
#else
		// -ACB- 1998/06/29 More Informative Error Message
		I_Error("R2_GetOtherSprite: Invalid sprite frame %s:%c",
			sprite->name, 'A' + framenum);
#endif
	}

	frame = sprite->frames + framenum;

	*flip = frame->flip[0] ? true : false;

	return frame->images[0];
}

//
// R2_ClipSpriteVertically
//
#define SY_FUDGE  2

void R2_ClipSpriteVertically(subsector_t *dsub, drawthing_t *dthing)
{
	drawfloor_t *dfloor, *df_orig;
	drawthing_t *dnew, *dt_orig;

	float z;
	float f1, c1;
	float f1_orig, c1_orig;

	// find the thing's nominal region.  This section is equivalent to
	// the R_PointInVertRegion() code (but using drawfloors).

	if (dthing->is_shadow)
		z = dthing->bottom + 0.5f;
	else
		z = (dthing->top + dthing->bottom) / 2.0f;

	for (dfloor = dsub->z_floors; dfloor->higher; dfloor = dfloor->higher)
	{
		if (z <= dfloor->top_h)
			break;
	}

	DEV_ASSERT2(dfloor);

	// link in sprite.  We'll shrink it if it gets clipped.
	LinkDrawthingIntoDrawfloor(dthing, dfloor);

	// handle never-clip things 
	if (dthing->clip_vert < 0)
		return;

	// Note that sprites are not clipped by the lowest floor or
	// highest ceiling, OR by *solid* extrafloors (even translucent
	// ones) -- UNLESS clip_vert is > 0.

	f1 = dfloor->f_h;
	c1 = dfloor->c_h;

	// handle TRANSLUCENT + THICK floors (a bit of a hack)
	if (dfloor->ef && dfloor->ef->ef_info && dfloor->higher &&
		(dfloor->ef->ef_info->type & EXFL_Thick) &&
		(dfloor->ef->top->translucency <= 0.99f))
	{
		c1 = dfloor->top_h;
	}

	df_orig = dfloor;
	dt_orig = dthing;
	f1_orig = f1;
	c1_orig = c1;

	// Two sections here: Downward clipping (for sprite's bottom) and
	// Upward clipping (for sprite's top).  Both use the exact same
	// algorithm:
	//     
	//    WHILE (current must be clipped)
	//    {
	//       new := COPY OF current
	//       clip both current and new to the clip height
	//       current := new
	//       floor := NEXT floor
	//       link current into floor
	//    }

	// ---- downward section ----

	for (;;)
	{
		if (!dfloor->lower)
			break;

		if ((dthing->bottom >= f1 - SY_FUDGE) ||
			(dthing->top    <  f1 + SY_FUDGE))
			break;

		DEV_ASSERT2(dfloor->lower->ef && dfloor->lower->ef->ef_info);

		if (! (dfloor->lower->ef->ef_info->type & EXFL_Liquid))
			break;

		// sprite's bottom must be clipped.  Make a copy.

		dnew = drawthings.GetNew();
		drawthings.Commit();

		dnew[0] = dthing[0];

		// shorten current sprite

		dthing->bottom = f1;

		DEV_ASSERT2(dthing->bottom < dthing->top);

		// shorten new sprite

		dnew->y_offset += (dnew->top - f1);
		dnew->top = f1;

		DEV_ASSERT2(dnew->bottom < dnew->top);

		// time to move on...

		dthing = dnew;
		dfloor = dfloor->lower;

		f1 = dfloor->f_h;
		c1 = dfloor->c_h;

		// handle TRANSLUCENT + THICK floors (a bit of a hack)
		if (dfloor->ef && dfloor->ef->ef_info && dfloor->higher &&
			(dfloor->ef->ef_info->type & EXFL_Thick) &&
			(dfloor->ef->top->translucency <= 0.99f))
		{
			c1 = dfloor->top_h;
		}

		// link new piece in
		LinkDrawthingIntoDrawfloor(dthing, dfloor);
	}

	// when clip_vert is > 0, we must clip to solids
	if (dthing->clip_vert > 0 &&
		dthing->bottom <  f1 - SY_FUDGE &&
		dthing->top    >= f1 + SY_FUDGE)
	{
		// shorten current sprite

		dthing->bottom = f1;

		DEV_ASSERT2(dthing->bottom < dthing->top);
	}

	dfloor = df_orig;
	dthing = dt_orig;
	f1 = f1_orig;
	c1 = c1_orig;

	// ---- upward section ----

	for (;;)
	{
		if (!dfloor->higher)
			break;

		if ((dthing->bottom >= c1 - SY_FUDGE) ||
			(dthing->top    <  c1 + SY_FUDGE))
			break;

		DEV_ASSERT2(dfloor->ef && dfloor->ef->ef_info);

		if (! (dfloor->ef->ef_info->type & EXFL_Liquid))
			break;

		// sprite's top must be clipped.  Make a copy.

		dnew = drawthings.GetNew();
		drawthings.Commit();

		dnew[0] = dthing[0];

		// shorten current sprite

		dthing->y_offset += (dthing->top - c1);
		dthing->top = c1;

		DEV_ASSERT2(dthing->bottom < dthing->top);

		// shorten new sprite

		dnew->bottom = c1;

		DEV_ASSERT2(dnew->bottom < dnew->top);

		// time to move on...

		dthing = dnew;
		dfloor = dfloor->higher;

		f1 = dfloor->f_h;
		c1 = dfloor->c_h;

		// handle TRANSLUCENT + THICK floors (a bit of a hack)
		if (dfloor->ef && dfloor->ef->ef_info && dfloor->higher &&
			(dfloor->ef->ef_info->type & EXFL_Thick) &&
			(dfloor->ef->top->translucency <= 0.99f))
		{
			c1 = dfloor->top_h;
		}

		// link new piece in
		LinkDrawthingIntoDrawfloor(dthing, dfloor);
	}

	// when clip_vert is > 0, we must clip to solids
	if (dthing->clip_vert > 0 &&
		dthing->bottom <  c1 - SY_FUDGE &&
		dthing->top    >= c1 + SY_FUDGE)
	{
		// shorten current sprite

		dthing->y_offset += dthing->top - c1;
		dthing->top = c1;

		DEV_ASSERT2(dthing->bottom < dthing->top);
	}
}

//
// RGL_WalkThing
//
// Visit a single thing that exists in the current subsector.
//
void RGL_WalkThing(mobj_t *mo, subsector_t *cur_sub)
{
	// ignore the player him/herself
	if (mo == players[displayplayer]->mo)
		return;

	// ignore invisible things
	if (mo->visibility == INVISIBLE)
		return;

	// transform the origin point
	float tr_x = mo->x - viewx;
	float tr_y = mo->y - viewy;

	float tz = tr_x * viewcos + tr_y * viewsin;

	// thing is behind view plane?
	if (oned_side_angle != ANG180 && tz <= 0)
		return;

	float tx = tr_x * viewsin - tr_y * viewcos;

	// too far off the side?
	// -ES- 1999/03/13 Fixed clipping to work with large FOVs (up to 176 deg)
	// rejects all sprites where angle>176 deg (arctan 32), since those
	// sprites would result in overflow in future calculations
	if (tz >= MINZ && fabs(tx) / 32 > tz)
		return;

	bool spr_flip;
	const image_t *image = R2_GetThingSprite(mo, &spr_flip);

	if (!image)
		return;

	// calculate edges of the shape
	float sprite_width  = IM_WIDTH(image);
	float sprite_height = IM_HEIGHT(image);
	float side_offset   = IM_OFFSETX(image);
	float top_offset    = IM_OFFSETY(image);

	if (spr_flip)
		side_offset *= -1.0f;

	float pos1 = (sprite_width/-2.0f - side_offset) * mo->info->xscale;
	float pos2 = (sprite_width/+2.0f - side_offset) * mo->info->xscale;

	float tx1 = tx + pos1;
	float tx2 = tx + pos2;

	float gzt = mo->z + (sprite_height + top_offset) * mo->info->yscale;
	float gzb = mo->z + top_offset * mo->info->yscale;

	if (mo->hyperflags & HF_HOVER)
	{
		// compute a different phase for different objects
		angle_t phase = (angle_t) mo;
		phase ^= (angle_t)(phase << 19);
		phase += (angle_t)(leveltime << (ANGLEBITS-6));

		float hover_dz = M_Sin(phase) * 4.0f;

		gzt += hover_dz;
		gzb += hover_dz;
	}

	// fix for sprites that sit wrongly into the floor/ceiling
	int clip_vert = 0;

	if ((mo->flags & MF_FUZZY) || (mo->hyperflags & HF_HOVER))
	{
		clip_vert = -1;
	}
	else if (sprite_kludge==0 && gzb < mo->floorz)
	{
		// explosion ?
		if (mo->info->flags & MF_MISSILE)
		{
			clip_vert = +1;
		}
		else
		{
			gzt += mo->floorz - gzb;
			gzb = mo->floorz;
		}
	}
	else if (sprite_kludge==0 && gzt > mo->ceilingz)
	{
		// explosion ?
		if (mo->info->flags & MF_MISSILE)
		{
			clip_vert = +1;
		}
		else
		{
			gzb -= gzt - mo->ceilingz;
			gzt = mo->ceilingz;
		}
	}

	if (gzb >= gzt)
		return;

	// create new draw thing

	drawthing_t *dthing = drawthings.GetNew();
	drawthings.Commit();

	dthing->mo = mo;
	dthing->props = cur_sub->floors->props;
	dthing->clip_vert = clip_vert;
	dthing->clipped_left = dthing->clipped_right = false;

	dthing->image  = image;
	dthing->flip   = spr_flip;
	dthing->bright = mo->bright ? true : false;
	dthing->is_shadow = false;
	dthing->is_halo   = false;

	dthing->iyscale = 1.0f / mo->info->yscale;

	dthing->tx = tx;
	dthing->tz = tz;
	dthing->tx1 = tx1;
	dthing->tx2 = tx2;

	dthing->top = dthing->orig_top = gzt;
	dthing->bottom = dthing->orig_bottom = gzb;
	dthing->y_offset = 0;

	dthing->left_dx  = pos1 *  viewsin;
	dthing->left_dy  = pos1 * -viewcos;
	dthing->right_dx = pos2 *  viewsin;
	dthing->right_dy = pos2 * -viewcos;

	// create shadow
	if (level_flags.shadows && mo->info->shadow_trans > 0 &&
		mo->floorz < viewz && ! IS_SKY(mo->subsector->sector->floor))
	{
		drawthing_t *dshadow = drawthings.GetNew();
		drawthings.Commit();

		dshadow[0] = dthing[0];

		dshadow->is_shadow = true;
		dshadow->clip_vert = -1;
		dshadow->tz += 1.5f;

		// shadows are 1/4 the height
		dshadow->iyscale *= 4.0f;

		gzb = mo->floorz;
		gzt = gzb + sprite_height / 4.0f * mo->info->yscale;

		dshadow->top = gzt;
		dshadow->bottom = gzb;

		R2_ClipSpriteVertically(cur_sub, dshadow);
	}

#if 0  // DISABLED
	// create halo
	if (level_flags.halos && mo->info->halo.height > 0)
	{
		drawthing_t *dhalo = drawthings.GetNew();
		drawthings.Commit();

		dhalo[0] = dthing[0];

		dhalo->is_halo = true;
		dhalo->image = W_ImageFromHalo(mo->info->halo.graphic);
		dhalo->clip_vert = -1;
		dhalo->tz -= 7.5f;

		gzb = mo->z + mo->height * 0.75f - mo->info->halo.height / 2;
		gzt = mo->z + mo->height * 0.75f + mo->info->halo.height / 2;

		dhalo->top = gzt;
		dhalo->bottom = gzb;

		pos1 = - mo->info->halo.height / 2;
		pos2 = + mo->info->halo.height / 2;

		dhalo->tx1 = tx + pos1;
		dhalo->tx2 = tx + pos2;

		dhalo->left_dx  = pos1 *  viewsin;  // FIXME: move forward
		dhalo->left_dy  = pos1 * -viewcos;
		dhalo->right_dx = pos2 *  viewsin;
		dhalo->right_dy = pos2 * -viewcos;

		dhalo->iyscale = mo->info->halo.height / IM_HEIGHT(dhalo->image);

		R2_ClipSpriteVertically(cur_sub, dhalo);
	}
#endif

	R2_ClipSpriteVertically(cur_sub, dthing);
}

//
// RGL_DrawThing
//
void RGL_DrawThing(drawfloor_t *dfloor, drawthing_t *dthing)
{
	int fuzzy = (dthing->mo->flags & MF_FUZZY);
	float trans = fuzzy ? FUZZY_TRANS : dthing->mo->visibility;

	const colourmap_c *colmap = dthing->props->colourmap;

	int L_r, L_g, L_b;

	float dx = 0, dy = 0;

	if (dthing->is_shadow)
	{
		L_r = L_g = L_b = 0;

		trans = dthing->mo->visibility * 
			PERCENT_2_FLOAT(dthing->mo->info->shadow_trans);

		dx = viewcos * 2;
		dy = viewsin * 2;
	}
#if 0  // HALO disabled
	else if (dthing->is_halo)
	{
		L_r = L_g = L_b = 255;  //!!

		trans = 0.5f;  //!!

		dx = -viewcos * 5;
		dy = -viewsin * 5;
	}
#endif
	else
	{
		int L = dthing->bright ? 255 : dthing->props->lightlevel;

		float c_r, c_g, c_b;
		V_GetColmapRGB(colmap, &c_r, &c_g, &c_b, false);

		if (doom_fading)
		{
			float dx = fabs(dthing->mo->x - viewx);
			float dy = fabs(dthing->mo->y - viewy);
			float dz = fabs(MO_MIDZ(dthing->mo) - viewz);

			float dist = APPROX_DIST3(dx, dy, dz);

			L = EMU_LIGHT(L, dist);
			L = RGL_LightEmu((L < 0) ? 0 : (L > 255) ? 255 : L);
		}
		else
			L = RGL_Light(L);

		L_r = (int)(L * c_r);
		L_g = (int)(L * c_g);
		L_b = (int)(L * c_b);
	}

	if (trans < 0.04f)
		return;

	const image_t *image = dthing->image;

//	float w = IM_WIDTH(image);
	float h = IM_HEIGHT(image);
	float right  = IM_RIGHT(image);
	float bottom = IM_BOTTOM(image);

	float x1b, y1b, z1b, x1t, y1t, z1t;
	float x2b, y2b, z2b, x2t, y2t, z2t;

	x1b = x1t = dthing->mo->x + dthing->left_dx;
	y1b = y1t = dthing->mo->y + dthing->left_dy;
	x2b = x2t = dthing->mo->x + dthing->right_dx;
	y2b = y2t = dthing->mo->y + dthing->right_dy;

	z1b = z2b = dthing->bottom;
	z1t = z2t = dthing->top;

	// Mlook in GL: tilt sprites so they look better
	{
		float h = dthing->orig_top - dthing->orig_bottom;
		float skew2 = h;

		if (dthing->mo->radius >= 1.0f && h > dthing->mo->radius)
			skew2 = dthing->mo->radius;

		float dx = viewcos * sprite_skew * skew2;
		float dy = viewsin * sprite_skew * skew2;

		float top_q    = ((dthing->top - dthing->orig_bottom)  / h) - 0.5f;
		float bottom_q = ((dthing->orig_top - dthing->bottom)  / h) - 0.5f;

		x1t += top_q * dx; y1t += top_q * dy;
		x2t += top_q * dx; y2t += top_q * dy;

		x1b -= bottom_q * dx; y1b -= bottom_q * dy;
		x2b -= bottom_q * dx; y2b -= bottom_q * dy;
	}

	float tex_x1 = 0.01f;
	float tex_x2 = right - 0.01f;

	float tex_y1 = dthing->y_offset * dthing->iyscale;
	float tex_y2 = tex_y1 + (z1t - z1b) * dthing->iyscale;

	CHECKVAL(h);
	tex_y1 = 1.0f - tex_y1 / h * bottom;
	tex_y2 = 1.0f - tex_y2 / h * bottom;

	if (dthing->flip)
	{
		float temp = tex_x2;
		tex_x1 = right - tex_x1;
		tex_x2 = right - temp;
	}

	//
	//  Dynamic Lighting Stuff
	//

	if (use_dlights && !dthing->is_shadow && !dthing->is_halo && !fuzzy)
	{
		drawthing_t *dl;
		float wx[4], wy[4], wz[4];

		wx[0] = x1b;  wy[0] = y1b;  wz[0] = z1b;
		wx[1] = x1t;  wy[1] = y1t;  wz[1] = z1t;
		wx[2] = x2t;  wy[2] = y2t;  wz[2] = z2t;
		wx[3] = x2b;  wy[3] = y2b;  wz[3] = z2b;

		for (dl=dfloor->dlights; dl; dl=dl->next)
		{
			if (dl->mo == dthing->mo)
				continue;

			R2_AddColourDLights(1, &L_r, &L_g, &L_b, wx, wy, wz, dl->mo);
		}
	}

	//
	// Special FUZZY effect
	//
	if (fuzzy && !dthing->is_shadow && !dthing->is_halo)
	{
		float range_x = fabs(dthing->right_dx - dthing->left_dx) / 12.0f;
		float range_y = fabs(dthing->right_dy - dthing->left_dy) / 12.0f;
		float range_z = fabs(z1t - z1b) / 24.0f / 2;

		angle_t adjust = (angle_t)(dthing->mo) << (ANGLEBITS - 14);

		float tl = M_Sin(fuzz_ang_tl + adjust);
		float tr = M_Sin(fuzz_ang_tr + adjust);
		float bl = M_Sin(fuzz_ang_bl + adjust);
		float br = M_Sin(fuzz_ang_br + adjust);

		float ztl = 1.0f + M_Cos(fuzz_ang_tl + adjust);
		float ztr = 1.0f + M_Cos(fuzz_ang_tr + adjust);
		float zbl = 1.0f + M_Cos(fuzz_ang_bl + adjust);
		float zbr = 1.0f + M_Cos(fuzz_ang_br + adjust);

		x1b += bl * range_x; y1b += bl * range_y; z1b += zbl * range_z;
		x1t += tl * range_x; y1t += tl * range_y; z1t += ztl * range_z;
		x2t += tr * range_x; y2t += tr * range_y; z2t += ztr * range_z;
		x2b += br * range_x; y2b += br * range_y; z2b += zbr * range_z;

		L_r = L_g = L_b = 0;
	}

	const cached_image_t *cim = W_ImageCache(image, false, dthing->mo->info->palremap);

	GLuint tex_id = W_ImageGetOGL(cim);

	// Blended sprites, even if opaque (trans > 0.99), have nicer edges
	int blending = BL_Masked;
	if (trans <= 0.99 || use_smoothing)
		blending |= BL_Alpha;

	if (dthing->mo->hyperflags & HF_NOZBUFFER)
		blending |= BL_NoZBuf;
	
	local_gl_vert_t *vert, *orig;

	vert = orig = RGL_BeginUnit(GL_QUADS, 4, tex_id,0, /* pass */ 0, blending);

	SET_COLOR(LT_RED(L_r), LT_GRN(L_g), LT_BLU(L_b), trans);
	SET_TEXCOORD(tex_x1, tex_y2);
	SET_NORMAL(-viewcos, -viewsin, 0.0f);
	SET_EDGE_FLAG(GL_TRUE);
	SET_VERTEX(x1b+dx, y1b+dy, z1b);
	vert++;

	SET_COLOR(LT_RED(L_r), LT_GRN(L_g), LT_BLU(L_b), trans);
	SET_TEXCOORD(tex_x1, tex_y1);
	SET_NORMAL(-viewcos, -viewsin, 0.0f);
	SET_EDGE_FLAG(GL_TRUE);
	SET_VERTEX(x1t+dx, y1t+dy, z1t);
	vert++;

	SET_COLOR(LT_RED(L_r), LT_GRN(L_g), LT_BLU(L_b), trans);
	SET_TEXCOORD(tex_x2, tex_y1);
	SET_NORMAL(-viewcos, -viewsin, 0.0f);
	SET_EDGE_FLAG(GL_TRUE);
	SET_VERTEX(x2t+dx, y2t+dy, z2t);
	vert++;

	SET_COLOR(LT_RED(L_r), LT_GRN(L_g), LT_BLU(L_b), trans);
	SET_TEXCOORD(tex_x2, tex_y2);
	SET_NORMAL(-viewcos, -viewsin, 0.0f);
	SET_EDGE_FLAG(GL_TRUE);
	SET_VERTEX(x2b+dx, y2b+dy, z2b);
	vert++;

	RGL_EndUnit(vert - orig);

	// Note: normally this would be wrong, since we're using the GL
	// texture ID later on (after W_ImageDone).  The W_LockImagesOGL
	// call saves us though.
	//
	W_ImageDone(cim);
}

//
// RGL_DrawSortThings
//
void RGL_DrawSortThings(drawfloor_t *dfloor)
{
	//
	// As part my move to strip out Z_Zone usage and replace 
	// it with array classes and more standard new and delete 
	// calls, I've removed the QSORT() here and the array. 
	// My main reason for doing that is that since I have to 
	// modify the code here anyway, it is prudent to re-evaluate
	// their usage. 
	//
	// The QSORT() mechanism used does an
	// allocation each time it is used and this is called
	// for each floor drawn in each subsector drawn, it is
	// reasonable to assume that removing it will give
	// something of speed improvement. 
	//
	// This comes at a cost since optimisation is always 
	// a balance between speed and size: drawthing_t now has 
	// to hold 4 additional pointers. Two for the binary tree 
	// (order building) and two for the the final linked list 
	// (avoiding recursive function calls that the parsing the 
	// binary tree would require).
	//
	// -ACB- 2004/08/17
	//
	
	drawthing_t *head_dt;
	
	// Check we have something to draw
	head_dt = dfloor->things;
	if (!head_dt)
		return;

	drawthing_t *curr_dt, *dt, *next_dt;
	float cmp_val;
		
	head_dt->rd_l = head_dt->rd_r = head_dt->rd_prev = head_dt->rd_next = NULL;
	
	dt = NULL;	// Warning removal: This will always have been set
	
	curr_dt = head_dt->next;
	while(curr_dt)
	{
		curr_dt->rd_l = curr_dt->rd_r = NULL;
		
		// Parse the tree to find our place
		next_dt = head_dt;
		do
		{
			dt = next_dt;
			
			cmp_val = dt->tz - curr_dt->tz;
			if (cmp_val == 0.0f)
			{
				// Resolve Z fight by letting the mobj pointer values settle it
				int offset = dt->mo - curr_dt->mo;
				cmp_val = (float)offset;
			}
						
			if (cmp_val < 0.0f)
				next_dt = dt->rd_l;				
			else
				next_dt = dt->rd_r;
		}
		while (next_dt);
		
		// Update our place 
		if (cmp_val < 0.0f)
		{ 
			// Update the binary tree
			dt->rd_l = curr_dt;

			// Update the linked list (Insert behind node)
			if (dt->rd_prev) 
				dt->rd_prev->rd_next = curr_dt; 
			
			curr_dt->rd_prev = dt->rd_prev;
			curr_dt->rd_next = dt; 

			dt->rd_prev = curr_dt;		
		}
		else
		{
			// Update the binary tree
			dt->rd_r = curr_dt; 

			// Update the linked list (Insert infront of node)
			if (dt->rd_next) 
				dt->rd_next->rd_prev = curr_dt; 
			
			curr_dt->rd_next = dt->rd_next;
			curr_dt->rd_prev = dt; 

			dt->rd_next = curr_dt;	
		}
		
		curr_dt = curr_dt->next;
	}
	
	// Find the first to draw
	while(head_dt->rd_prev)
		head_dt = head_dt->rd_prev;

	// Draw...
	for (dt=head_dt; dt; dt=dt->rd_next)
		RGL_DrawThing(dfloor, dt);
}

