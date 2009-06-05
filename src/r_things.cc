//----------------------------------------------------------------------------
//  EDGE OpenGL Rendering (Things)
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

#include "i_defs.h"
#include "i_defs_gl.h"

#include <math.h>

#include "epi/image_data.h"
#include "epi/image_jpeg.h"

#include "dm_data.h"
#include "dm_defs.h"
#include "dm_state.h"
#include "p_local.h"
#include "r_colormap.h"
#include "r_defs.h"
#include "r_draw.h"
#include "r_effects.h"
#include "r_gldefs.h"
#include "r_image.h"
#include "r_md2.h"
#include "r_misc.h"
#include "r_modes.h"
#include "r_shader.h"
#include "r_texgl.h"
#include "r_units.h"
#include "w_model.h"
#include "w_sprite.h"

#include "m_misc.h"  // !!!! model test


#define DEBUG  0


float sprite_skew;



// colour of the player's weapon
int rgl_weapon_r;
int rgl_weapon_g;
int rgl_weapon_b;


extern mobj_t * view_cam_mo;


// The minimum distance between player and a visible sprite.
#define MINZ        (4.0f)


static float GetHoverDZ(mobj_t *mo)
{
	// compute a different phase for different objects
	angle_t phase = (angle_t)(long)mo;
	phase ^= (angle_t)(phase << 19);
	phase += (angle_t)(leveltime << (ANGLEBITS-6));

	return M_Sin(phase) * 4.0f;
}


typedef struct
{
	vec3_t vert[4];
	vec2_t texc[4];
	vec3_t lit_pos;

	multi_color_c col[4];
}
psprite_coord_data_t;

static void DLIT_PSprite(mobj_t *mo, void *dataptr)
{
	psprite_coord_data_t *data = (psprite_coord_data_t *)dataptr;

	SYS_ASSERT(mo->dlight.shader);

	mo->dlight.shader->Sample(data->col + 0,
			data->lit_pos.x, data->lit_pos.y, data->lit_pos.z);
}

static int GetMulticolMaxRGB(multi_color_c *cols, int num, bool additive)
{
	int result = 0;

	for (; num > 0; num--, cols++)
	{
		int mx = additive ? cols->add_MAX() : cols->mod_MAX();

		result = MAX(result, mx);
	}
	
	return result;
}


static void RGL_DrawPSprite(pspdef_t * psp, int which,
							player_t * player, region_properties_t *props,
							const state_t *state)
{
	
	if (state->flags & SFF_Model)
		return;

	// determine sprite patch
	bool flip;
	const image_c *image = R2_GetOtherSprite(state->sprite, state->frame, &flip);

	if (!image)
		return;

	GLuint tex_id = W_ImageCache(image, false,
					  (which == ps_crosshair) ? NULL : ren_fx_colmap);

	float w = IM_WIDTH(image);
	float h = IM_HEIGHT(image);
	float right = IM_RIGHT(image);
	float top = IM_TOP(image);

	bool is_fuzzy = (player->mo->flags & MF_FUZZY) ? true : false;

	float trans = player->mo->visibility;

	if (which == ps_crosshair)
	{
		is_fuzzy = false;
		trans = 1.0f;
	}

	trans *= psp->visibility;

	if (trans <= 0)
		return;


	float tex_top_h = top;  ///## 1.00f; // 0.98;
	float tex_bot_h = 0.0f; ///## 1.00f - top;  // 1.02 - bottom;

	float tex_x1 = 0.002f;
	float tex_x2 = right - 0.002f;

	if (flip)
	{
		tex_x1 = right - tex_x1;
		tex_x2 = right - tex_x2;
	}

	float tx1 = (161.0f - IM_WIDTH(image) / 2.0) + psp->sx - IM_OFFSETX(image);
	float tx2 = tx1 + w;

	float ty1 = - psp->sy + IM_OFFSETY(image);
	float ty2 = ty1 + h;


	float x1b, y1b, x1t, y1t, x2b, y2b, x2t, y2t;  // screen coords

	x1b = x1t = viewwindow_w  * tx1 / 320.0f;
	x2b = x2t = viewwindow_w  * tx2 / 320.0f;

	y1b = y2b = viewwindow_h * ty1 / 200.0f;
	y1t = y2t = viewwindow_h * ty2 / 200.0f;


	// clip psprite to view window
	glEnable(GL_SCISSOR_TEST);

	glScissor(viewwindow_x, viewwindow_y, viewwindow_w, viewwindow_h);

	x1b = (float)viewwindow_x + x1b;
	x1t = (float)viewwindow_x + x1t;
	x2t = (float)viewwindow_x + x2t;
	x2b = (float)viewwindow_x + x2b;

	y1b = (float)viewwindow_y + y1b - 1;
	y1t = (float)viewwindow_y + y1t - 1;
	y2t = (float)viewwindow_y + y2t - 1;
	y2b = (float)viewwindow_y + y2b - 1;


	psprite_coord_data_t data;

	data.vert[0].Set(x1b, y1b, 0);
	data.vert[1].Set(x1t, y1t, 0);
	data.vert[2].Set(x2t, y1t, 0);
	data.vert[3].Set(x2b, y2b, 0);
		
	data.texc[0].Set(tex_x1, tex_bot_h);
	data.texc[1].Set(tex_x1, tex_top_h);
	data.texc[2].Set(tex_x2, tex_top_h);
	data.texc[3].Set(tex_x2, tex_bot_h);

	float away = 120.0;

	data.lit_pos.x = player->mo->x + viewcos * away;
	data.lit_pos.y = player->mo->y + viewsin * away;
	data.lit_pos.z = player->mo->z + player->mo->height *
		PERCENT_2_FLOAT(player->mo->info->shotheight);

	data.col[0].Clear();


	int blending = BL_Masked;

	if (trans >= 0.11f && image->opacity != OPAC_Complex)
		blending = BL_Less;

	if (trans < 0.99 || image->opacity == OPAC_Complex)
		blending |= BL_Alpha;


	if (is_fuzzy)
	{
		blending = BL_Masked | BL_Alpha;
		trans = 1.0f;
	}

	if (! is_fuzzy)
	{
		abstract_shader_c *shader = R_GetColormapShader(props, state->bright);

		shader->Sample(data.col + 0, data.lit_pos.x, data.lit_pos.y, data.lit_pos.z);

		if (use_dlights && ren_extralight < 250)
		{
			data.lit_pos.x = player->mo->x + viewcos * 24;
			data.lit_pos.y = player->mo->y + viewsin * 24;

			float r = 96;

			P_DynamicLightIterator(
				data.lit_pos.x - r, data.lit_pos.y - r, player->mo->z,
				data.lit_pos.x + r, data.lit_pos.y + r, player->mo->z + player->mo->height,
				DLIT_PSprite, &data);

			P_SectorGlowIterator(player->mo->subsector->sector,
				data.lit_pos.x - r, data.lit_pos.y - r, player->mo->z,
				data.lit_pos.x + r, data.lit_pos.y + r, player->mo->z + player->mo->height,
				DLIT_PSprite, &data);
		}
	}

	// FIXME: sample at least TWO points (left and right edges)
	data.col[1] = data.col[0];
	data.col[2] = data.col[0];
	data.col[3] = data.col[0];


	/* draw the weapon */

	RGL_StartUnits(false);

	int num_pass = is_fuzzy ? 1 : (4 + detail_level * 2);

	for (int pass = 0; pass < num_pass; pass++)
	{
		if (pass == 1)
		{
			blending &= ~BL_Alpha;
			blending |=  BL_Add;
		}

		bool is_additive = (pass > 0 && pass == num_pass-1);

		if (pass > 0 && pass < num_pass-1)
		{
			if (GetMulticolMaxRGB(data.col, 4, false) <= 0)
				continue;
		}
		else if (is_additive)
		{
			if (GetMulticolMaxRGB(data.col, 4, true) <= 0)
				continue;
		}

		GLuint fuzz_tex = is_fuzzy ? W_ImageCache(fuzz_image, false) : 0;

		local_gl_vert_t * glvert = RGL_BeginUnit(GL_POLYGON, 4,
				 is_additive ? ENV_SKIP_RGB : GL_MODULATE, tex_id,
				 is_fuzzy ? GL_MODULATE : ENV_NONE, fuzz_tex,
				 pass, blending);

		for (int v_idx=0; v_idx < 4; v_idx++)
		{
			local_gl_vert_t *dest = glvert + v_idx;

			dest->pos     = data.vert[v_idx];
			dest->texc[0] = data.texc[v_idx];

			dest->normal.Set(0, 0, 1);

			if (is_fuzzy)
			{
				dest->texc[1].x = dest->pos.x / (float)SCREENWIDTH;
				dest->texc[1].y = dest->pos.y / (float)SCREENHEIGHT;

				FUZZ_Adjust(& dest->texc[1], player->mo);

				dest->rgba[0] = dest->rgba[1] = dest->rgba[2] = 0;
			}
			else if (! is_additive)
			{
				dest->rgba[0] = data.col[v_idx].mod_R / 255.0;
				dest->rgba[1] = data.col[v_idx].mod_G / 255.0;
				dest->rgba[2] = data.col[v_idx].mod_B / 255.0;

				data.col[v_idx].mod_R -= 256;
				data.col[v_idx].mod_G -= 256;
				data.col[v_idx].mod_B -= 256;
			}
			else
			{
				dest->rgba[0] = data.col[v_idx].add_R / 255.0;
				dest->rgba[1] = data.col[v_idx].add_G / 255.0;
				dest->rgba[2] = data.col[v_idx].add_B / 255.0;
			}

			dest->rgba[3] = trans;
		}

		RGL_EndUnit(4);
	}

	RGL_FinishUnits();

#if 0  // OLD WAY (KEEP FOR CROSSHAIRS !!!)
	if (fuzzy)
		L_r = L_g = L_b = 50;
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
#endif
	glDisable(GL_SCISSOR_TEST);
}

static void DrawStdCrossHair(void)
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
	int mul = 1 + (viewwindow_w / 300);

	int x = viewwindow_x + viewwindow_w / 2;
	int y = viewwindow_y + viewwindow_h / 2;

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


void RGL_DrawWeaponSprites(player_t * p)
{
	bool got_cross = false;

	// special handling for zoom: show viewfinder
	if (view_zoom > 0)
	{
		pspdef_t *psp = &p->psprites[ps_weapon];

		if ((p->ready_wp < 0) || (psp->state == S_NULL))
			return;

		weapondef_c *w = p->weapons[p->ready_wp].info;

		if (w->zoom_state <= 0)
			return;

		RGL_DrawPSprite(psp, ps_weapon, p, view_props,
						&w->states[w->zoom_state]);
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

			if (p->ready_wp < 0)
				continue;
			weapondef_c *w = p->weapons[p->ready_wp].info;

			RGL_DrawPSprite(psp, i, p, view_props, &w->states[psp->state]);

			if (i == ps_crosshair)
				got_cross = true;
		}
	}

	if (!got_cross && p->health > 0)
		DrawStdCrossHair();
}


void RGL_DrawWeaponModel(player_t * p)
{
	if (view_zoom > 0)
		return;

	pspdef_t *psp = &p->psprites[ps_weapon];

	if (p->ready_wp < 0)
		return;

	if (psp->state == S_NULL)
		return;

	weapondef_c *w = p->weapons[p->ready_wp].info;

	const state_t *st = &w->states[psp->state];

	if (! (st->flags & SFF_Model))
		return;

	modeldef_c *md = W_GetModel(st->sprite, w->states);

	int skin_num = p->weapons[p->ready_wp].model_skin;

	const image_c *skin_img = md->skins[skin_num];

	if (! skin_img)  // FIXME: use a dummy image
	{
I_Debugf("Render model: no skin %d\n", skin_num);
		skin_img = W_ImageForDummySkin();
	}


// I_Debugf("Rendering weapon model!\n");
	
	float x = viewx + viewright.x * psp->sx / 8.0;
	float y = viewy + viewright.y * psp->sx / 8.0;
	float z = viewz + viewright.z * psp->sx / 8.0;

	x -= viewup.x * psp->sy / 10.0;
	y -= viewup.y * psp->sy / 10.0;
	z -= viewup.z * psp->sy / 10.0;

	x += viewforward.x * w->model_forward;
	y += viewforward.y * w->model_forward;
	z += viewforward.z * w->model_forward;

	x += viewright.x * w->model_side;
	y += viewright.y * w->model_side;
	z += viewright.z * w->model_side;

	int last_frame = st->frame;
	float lerp = 0.0;

	if (p->weapon_last_frame >= 0)
	{
		SYS_ASSERT(st);
		SYS_ASSERT(st->tics > 1);

		last_frame = p->weapon_last_frame;

		lerp = (st->tics - psp->tics + 1) / (float)(st->tics);
		lerp = CLAMP(0, lerp, 1);
	}

	MD2_RenderModel(md->model, skin_img, true,
			        last_frame, st->frame, lerp,
			        x, y, z, p->mo, view_props,
					1.0f /* scale */, w->model_aspect, w->model_bias);
}

void RGL_DrawCrosshair(player_t * p)
{
	// !!!!! FIXME : RGL_DrawCrosshair
}


// ============================================================================
// R2_BSP START
// ============================================================================

int sprite_kludge = 0;

static inline void LinkDrawthingIntoDrawfloor(
		drawfloor_t *dfloor, drawthing_t *dthing)
{
	dthing->props = dfloor->props;
	dthing->next  = dfloor->things;
	dthing->prev  = NULL;

	if (dfloor->things)
		dfloor->things->prev = dthing;

	dfloor->things = dthing;
}


static const image_c * R2_GetThingSprite2(mobj_t *mo, float mx, float my, bool *flip)
{
	// Note: can return NULL for no image.

	// decide which patch to use for sprite relative to player
	SYS_ASSERT(mo->state != S_NULL);

	const state_t *st = &mo->info->states[mo->state];

	if (st->sprite == SPR_NULL)
		return NULL;

	spriteframe_c *frame = W_GetSpriteFrame(st->sprite, st->frame);

	if (! frame)
	{
		// show dummy sprite for missing frame
		(*flip) = false;
		return W_ImageForDummySprite();
	}

	int rot = 0;

	if (frame->rots >= 8)
	{
		angle_t ang = mo->angle;

		MIR_Angle(ang);

		angle_t from_view = R_PointToAngle(viewx, viewy, mx, my);

		ang = from_view - ang + ANG180;

		if (MIR_Reflective())
			ang = (angle_t)0 - ang;

		if (frame->rots == 16)
			rot = (ang + (ANG45 / 4)) >> (ANGLEBITS - 4);
		else
			rot = (ang + (ANG45 / 2)) >> (ANGLEBITS - 3);
	}

	SYS_ASSERT(0 <= rot && rot < 16);

	(*flip) = frame->flip[rot] ? true : false;

	if (MIR_Reflective())
		(*flip) = !(*flip);

	if (! frame->images[rot])
	{
		// show dummy sprite for missing rotation
		(*flip) = false;
		return W_ImageForDummySprite();
	}

	return frame->images[rot];
}


const image_c * R2_GetOtherSprite(int spritenum, int framenum, bool *flip)
{
	/* Used for non-object stuff, like weapons and finale */

	if (spritenum == SPR_NULL)
		return NULL;

	spriteframe_c *frame = W_GetSpriteFrame(spritenum, framenum);

	if (! frame || ! frame->images[0])
	{
		(*flip) = false;
		return W_ImageForDummySprite();
	}

	*flip = frame->flip[0] ? true : false;

	return frame->images[0];
}


#define SY_FUDGE  2

static void R2_ClipSpriteVertically(drawsub_c *dsub, drawthing_t *dthing)
{
	drawfloor_t *dfloor = NULL;

	// find the thing's nominal region.  This section is equivalent to
	// the R_PointInVertRegion() code (but using drawfloors).

	float z = (dthing->top + dthing->bottom) / 2.0f;

	std::vector<drawfloor_t *>::iterator DFI;

	for (DFI = dsub->floors.begin(); DFI != dsub->floors.end(); DFI++)
	{
		dfloor = *DFI;

		if (z <= dfloor->top_h)
			break;
	}

	SYS_ASSERT(dfloor);

	// link in sprite.  We'll shrink it if it gets clipped.
	LinkDrawthingIntoDrawfloor(dfloor, dthing);

	// HACK: the code below cannot handle Portals
	if (num_active_mirrors > 0)
		return;

	// handle never-clip things 
	if (dthing->y_clipping == YCLIP_Never)
		return;

#if 0  // DISABLED FOR NOW : FIX LATER !!
	drawfloor_t *df_orig;
	drawthing_t *dnew, *dt_orig;

	float f1, f1_orig;
	float c1, c1_orig;

	// Note that sprites are not clipped by the lowest floor or
	// highest ceiling, OR by *solid* extrafloors (even translucent
	// ones) -- UNLESS y_clipping == YCLIP_Hard.

	f1 = dfloor->f_h;
	c1 = dfloor->c_h;

	// handle TRANSLUCENT + THICK floors (a bit of a hack)
	if (dfloor->ef && dfloor->ef->ef_info && !dfloor->is_highest &&
		(dfloor->ef->ef_info->type & EXFL_Thick) &&
		(dfloor->ef->top->translucency < 0.99f))
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
		if (dfloor->is_lowest)
			break;

		if ((dthing->bottom >= f1 - SY_FUDGE) ||
			(dthing->top    <  f1 + SY_FUDGE))
			break;

		SYS_ASSERT(dfloor->lower->ef && dfloor->lower->ef->ef_info);

		if (! (dfloor->lower->ef->ef_info->type & EXFL_Liquid))
			break;

		// sprite must be split (bottom), make a copy.

		dnew = R_GetDrawThing();

		dnew[0] = dthing[0];

		// shorten current sprite

		dthing->bottom = f1;

		SYS_ASSERT(dthing->bottom < dthing->top);

		// shorten new sprite

///----		dnew->y_offset += (dnew->top - f1);
		dnew->top = f1;

		SYS_ASSERT(dnew->bottom < dnew->top);

		// time to move on...

		dthing = dnew;
		dfloor = dfloor->lower;

		f1 = dfloor->f_h;
		c1 = dfloor->c_h;

		// handle TRANSLUCENT + THICK floors (a bit of a hack)
		if (dfloor->ef && dfloor->ef->ef_info && !dfloor->is_highest &&
			(dfloor->ef->ef_info->type & EXFL_Thick) &&
			(dfloor->ef->top->translucency < 0.99f))
		{
			c1 = dfloor->top_h;
		}

		// link new piece in
		LinkDrawthingIntoDrawfloor(dfloor, dthing);
	}

	if (dthing->y_clipping == YCLIP_Hard &&
		dthing->bottom <  f1 - SY_FUDGE &&
		dthing->top    >= f1 + SY_FUDGE)
	{
		// shorten current sprite

		dthing->bottom = f1;

		SYS_ASSERT(dthing->bottom < dthing->top);
	}

	dfloor = df_orig;
	dthing = dt_orig;
	f1 = f1_orig;
	c1 = c1_orig;

	// ---- upward section ----

	for (;;)
	{
		if (dfloor->is_highest)
			break;

		if ((dthing->bottom >= c1 - SY_FUDGE) ||
			(dthing->top    <  c1 + SY_FUDGE))
			break;

		SYS_ASSERT(dfloor->ef && dfloor->ef->ef_info);

		if (! (dfloor->ef->ef_info->type & EXFL_Liquid))
			break;

		// sprite must be split (top), make a copy.

		dnew = R_GetDrawThing();

		dnew[0] = dthing[0];

		// shorten current sprite

///----		dthing->y_offset += (dthing->top - c1);
		dthing->top = c1;

		SYS_ASSERT(dthing->bottom < dthing->top);

		// shorten new sprite

		dnew->bottom = c1;

		SYS_ASSERT(dnew->bottom < dnew->top);

		// time to move on...

		dthing = dnew;
		dfloor = dfloor->higher;

		f1 = dfloor->f_h;
		c1 = dfloor->c_h;

		// handle TRANSLUCENT + THICK floors (a bit of a hack)
		if (dfloor->ef && dfloor->ef->ef_info && !dfloor->is_highest &&
			(dfloor->ef->ef_info->type & EXFL_Thick) &&
			(dfloor->ef->top->translucency < 0.99f))
		{
			c1 = dfloor->top_h;
		}

		// link new piece in
		LinkDrawthingIntoDrawfloor(dfloor, dthing);
	}

	if (dthing->y_clipping == YCLIP_Hard &&
		dthing->bottom <  c1 - SY_FUDGE &&
		dthing->top    >= c1 + SY_FUDGE)
	{
		// shorten current sprite

///----		dthing->y_offset += dthing->top - c1;
		dthing->top = c1;

		SYS_ASSERT(dthing->bottom < dthing->top);
	}

#endif
}


void RGL_WalkThing(drawsub_c *dsub, mobj_t *mo)
{
	/* Visit a single thing that exists in the current subsector */

	SYS_ASSERT(mo->state != S_NULL);

	// ignore the camera itself
	if (mo == view_cam_mo && num_active_mirrors == 0)
		return;

	// ignore invisible things
	if (mo->visibility == INVISIBLE)
		return;

	bool is_model = (mo->info->states[mo->state].flags & SFF_Model) ? true:false;

	// transform the origin point
	float mx = mo->x, my = mo->y, mz = mo->z;

	// position interpolation
	if (mo->lerp_num > 1)
	{
		float along = mo->lerp_pos / (float)mo->lerp_num;

		mx = mo->lerp_from.x + (mx - mo->lerp_from.x) * along;
		my = mo->lerp_from.y + (my - mo->lerp_from.y) * along;
		mz = mo->lerp_from.z + (mz - mo->lerp_from.z) * along;
	}

	MIR_Coordinate(mx, my);

	float tr_x = mx - viewx;
	float tr_y = my - viewy;

	float tz = tr_x * viewcos + tr_y * viewsin;

	// thing is behind view plane?
	if (clip_scope != ANG180 && tz <= 0 && !is_model)
		return;

	float tx = tr_x * viewsin - tr_y * viewcos;

	// too far off the side?
	// -ES- 1999/03/13 Fixed clipping to work with large FOVs (up to 176 deg)
	// rejects all sprites where angle>176 deg (arctan 32), since those
	// sprites would result in overflow in future calculations
	if (tz >= MINZ && fabs(tx) / 32 > tz)
		return;

	float hover_dz = 0;

	if (mo->hyperflags & HF_HOVER)
		hover_dz = GetHoverDZ(mo);

	bool spr_flip = false;
	const image_c *image = NULL;

	float gzt  = 0, gzb  = 0;
	float pos1 = 0, pos2 = 0;

	if (! is_model)
	{
		image = R2_GetThingSprite2(mo, mx, my, &spr_flip);

		if (!image)
			return;

		// calculate edges of the shape
		float sprite_width  = IM_WIDTH(image);
		float sprite_height = IM_HEIGHT(image);
		float side_offset   = IM_OFFSETX(image);
		float top_offset    = IM_OFFSETY(image);

		if (spr_flip)
			side_offset = -side_offset;

		float xscale = mo->info->scale * mo->info->aspect;

		pos1 = (sprite_width/-2.0f - side_offset) * xscale;
		pos2 = (sprite_width/+2.0f - side_offset) * xscale;

		switch (mo->info->yalign)
		{
			case SPYA_TopDown:
				gzt = mo->z + mo->height + top_offset * mo->info->scale;
				gzb = gzt - sprite_height * mo->info->scale;
				break;

			case SPYA_Middle:
			{
				float mz = mo->z + mo->height * 0.5 + top_offset * mo->info->scale;
				float dz = sprite_height * 0.5 * mo->info->scale;

				gzt = mz + dz;
				gzb = mz - dz;
				break;
			}

			case SPYA_BottomUp: default:
				gzb = mo->z +  top_offset * mo->info->scale;
				gzt = gzb + sprite_height * mo->info->scale;
				break;
		}

		if (mo->hyperflags & HF_HOVER)
		{
			gzt += hover_dz;
			gzb += hover_dz;
		}
	} // if (! is_model)


	// fix for sprites that sit wrongly into the floor/ceiling
	int y_clipping = YCLIP_Soft;

	if (is_model || (mo->flags & MF_FUZZY) || (mo->hyperflags & HF_HOVER))
	{
		y_clipping = YCLIP_Never;
	}
	else if (sprite_kludge==0 && gzb < mo->floorz)
	{
		// explosion ?
		if (mo->info->flags & MF_MISSILE)
		{
			y_clipping = YCLIP_Hard;
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
			y_clipping = YCLIP_Hard;
		}
		else
		{
			gzb -= gzt - mo->ceilingz;
			gzt = mo->ceilingz;
		}
	}

	if (!is_model)
	{
		if (gzb >= gzt)
			return;

		MIR_Height(gzb);
		MIR_Height(gzt);
	}

	// create new draw thing

	drawthing_t *dthing = R_GetDrawThing();
	dthing->Clear();

	dthing->mo = mo;
	dthing->mx = mx;
	dthing->my = my;
	dthing->mz = mz;

	dthing->props = dsub->floors[0]->props;
	dthing->y_clipping = y_clipping;
	dthing->is_model = is_model;

	dthing->image = image;
	dthing->flip  = spr_flip;

	dthing->tx = tx;
	dthing->tz = tz;

	dthing->top    = dthing->orig_top    = gzt;
	dthing->bottom = dthing->orig_bottom = gzb;
///----	dthing->y_offset = 0;

	float mir_scale = MIR_XYScale();

	dthing->left_dx  = pos1 *  viewsin * mir_scale;
	dthing->left_dy  = pos1 * -viewcos * mir_scale;
	dthing->right_dx = pos2 *  viewsin * mir_scale;
	dthing->right_dy = pos2 * -viewcos * mir_scale;

	// create shadow
#if 0
	if (level_flags.shadows && mo->info->shadow_trans > 0 &&
		mo->floorz < viewz && ! IS_SKY(mo->subsector->sector->floor))
	{
		drawthing_t *dshadow = R_GetDrawThing();

		dshadow[0] = dthing[0];

		dshadow->is_shadow = true;
		dshadow->y_clipping = -1;
		dshadow->tz += 1.5f;

		// shadows are 1/4 the height
		dshadow->iyscale *= 4.0f;

		gzb = mo->floorz;
		gzt = gzb + sprite_height / 4.0f * mo->info->yscale;

		dshadow->top = gzt;
		dshadow->bottom = gzb;

		R2_ClipSpriteVertically(dsub, dshadow);
	}
#endif

	R2_ClipSpriteVertically(dsub, dthing);
}


static void RGL_DrawModel(drawthing_t *dthing)
{
	mobj_t *mo = dthing->mo;

	const state_t *st = &mo->info->states[mo->state];

	modeldef_c *md = W_GetModel(st->sprite, ((mobjtype_c*)mo->info)->states);

	const image_c *skin_img = md->skins[mo->model_skin];

	if (! skin_img)
	{
I_Debugf("Render model: no skin %d\n", mo->model_skin);
		skin_img = W_ImageForDummySkin();
	}


	float z = dthing->mz;

	MIR_Height(z);

	if (mo->hyperflags & HF_HOVER)
		z += GetHoverDZ(mo);

	int last_frame = st->frame;
	float lerp = 0.0;

	if (mo->model_last_frame >= 0)
	{
		last_frame = mo->model_last_frame;

		SYS_ASSERT(st->tics > 1);

		lerp = (st->tics - mo->tics + 1) / (float)(st->tics);
		lerp = CLAMP(0, lerp, 1);
	}

	MD2_RenderModel(md->model, skin_img, false,
			        last_frame, st->frame, lerp,
					dthing->mx, dthing->my, z, mo, mo->props,
					mo->info->model_scale, mo->info->model_aspect,
					mo->info->model_bias);
}


typedef struct
{
	mobj_t *mo;

	vec3_t vert[4];
	vec2_t texc[4];
	vec3_t normal;

	multi_color_c col[4];
}
thing_coord_data_t;


static void DLIT_Thing(mobj_t *mo, void *dataptr)
{
	thing_coord_data_t *data = (thing_coord_data_t *)dataptr;

	// dynamic lights do not light themselves up!
	if (mo == data->mo)
		return;
	
	SYS_ASSERT(mo->dlight.shader);

	for (int v = 0; v < 4; v++)
	{
		mo->dlight.shader->Sample(data->col + v,
				data->vert[v].x, data->vert[v].y, data->vert[v].z);
	}
}


void RGL_DrawThing(drawfloor_t *dfloor, drawthing_t *dthing)
{
	if (dthing->is_model)
	{
		RGL_DrawModel(dthing);
		return;
	}

	mobj_t *mo = dthing->mo;

	bool is_fuzzy = (mo->flags & MF_FUZZY) ? true : false;

	float trans = mo->visibility;

	float dx = 0, dy = 0;

	if (trans <= 0)
		return;


	const image_c *image = dthing->image;

	GLuint tex_id = W_ImageCache(image, false,
	                  ren_fx_colmap ? ren_fx_colmap : dthing->mo->info->palremap);

//	float w = IM_WIDTH(image);
	float h = IM_HEIGHT(image);
	float right = IM_RIGHT(image);
	float top   = IM_TOP(image);

	float x1b, y1b, z1b, x1t, y1t, z1t;
	float x2b, y2b, z2b, x2t, y2t, z2t;

	x1b = x1t = dthing->mx + dthing->left_dx;
	y1b = y1t = dthing->my + dthing->left_dy;
	x2b = x2t = dthing->mx + dthing->right_dx;
	y2b = y2t = dthing->my + dthing->right_dy;

	z1b = z2b = dthing->bottom;
	z1t = z2t = dthing->top;

	// MLook: tilt sprites so they look better
	if (MIR_XYScale() >= 0.99)
	{
		float h = dthing->orig_top - dthing->orig_bottom;
		float skew2 = h;

		if (mo->radius >= 1.0f && h > mo->radius)
			skew2 = mo->radius;

		float dx = viewcos * sprite_skew * skew2;
		float dy = viewsin * sprite_skew * skew2;

		float top_q    = ((dthing->top - dthing->orig_bottom)  / h) - 0.5f;
		float bottom_q = ((dthing->orig_top - dthing->bottom)  / h) - 0.5f;

		x1t += top_q * dx; y1t += top_q * dy;
		x2t += top_q * dx; y2t += top_q * dy;

		x1b -= bottom_q * dx; y1b -= bottom_q * dy;
		x2b -= bottom_q * dx; y2b -= bottom_q * dy;
	}

	float tex_x1 = 0.001f;
	float tex_x2 = right - 0.001f;

	float tex_y1 = dthing->bottom - dthing->orig_bottom;
	float tex_y2 = tex_y1 + (z1t - z1b);

	float yscale = mo->info->scale * MIR_ZScale();

	SYS_ASSERT(h > 0);
	tex_y1 = top * tex_y1 / (h * yscale);
	tex_y2 = top * tex_y2 / (h * yscale);

	if (dthing->flip)
	{
		float temp = tex_x2;
		tex_x1 = right - tex_x1;
		tex_x2 = right - temp;
	}


	thing_coord_data_t data;

	data.mo = mo;

	data.vert[0].Set(x1b+dx, y1b+dy, z1b);
	data.vert[1].Set(x1t+dx, y1t+dy, z1t);
	data.vert[2].Set(x2t+dx, y2t+dy, z2t);
	data.vert[3].Set(x2b+dx, y2b+dy, z2b);

	data.texc[0].Set(tex_x1, tex_y1);
	data.texc[1].Set(tex_x1, tex_y2);
	data.texc[2].Set(tex_x2, tex_y2);
	data.texc[3].Set(tex_x2, tex_y1);

	data.normal.Set(-viewcos, -viewsin, 0);


	data.col[0].Clear();
	data.col[1].Clear();
	data.col[2].Clear();
	data.col[3].Clear();


	int blending = BL_Masked;

	if (trans >= 0.11f && image->opacity != OPAC_Complex)
		blending = BL_Less;

	if (trans < 0.99 || image->opacity == OPAC_Complex)
		blending |= BL_Alpha;

	if (mo->hyperflags & HF_NOZBUFFER)
		blending |= BL_NoZBuf;

	
	float  fuzz_mul = 0;
	vec2_t fuzz_add;

	fuzz_add.Set(0, 0);

	if (is_fuzzy)
	{
		blending = BL_Masked | BL_Alpha;
		trans = 1.0f;

		float dist = P_ApproxDistance(mo->x - viewx, mo->y - viewy, mo->z - viewz);

		fuzz_mul = 0.8 / CLAMP(20, dist, 700);

		FUZZ_Adjust(&fuzz_add, mo);
	}

	if (! is_fuzzy)
	{
		abstract_shader_c *shader = R_GetColormapShader(dthing->props, mo->info->states[mo->state].bright);

		for (int v=0; v < 4; v++)
		{
			shader->Sample(data.col + v, data.vert[v].x, data.vert[v].y, data.vert[v].z);
		}

		if (use_dlights && ren_extralight < 250)
		{
			float r = mo->radius + 32;

			P_DynamicLightIterator(
					mo->x - r, mo->y - r, mo->z,
					mo->x + r, mo->y + r, mo->z + mo->height,
					DLIT_Thing, &data);

			P_SectorGlowIterator(mo->subsector->sector,
					mo->x - r, mo->y - r, mo->z,
					mo->x + r, mo->y + r, mo->z + mo->height,
					DLIT_Thing, &data);
		}
	}


	/* draw the sprite */

	int num_pass = is_fuzzy ? 1 : (3 + detail_level * 2);

	for (int pass = 0; pass < num_pass; pass++)
	{
		if (pass == 1)
		{
			blending &= ~BL_Alpha;
			blending |=  BL_Add;
		}

		bool is_additive = (pass > 0 && pass == num_pass-1);

		if (pass > 0 && pass < num_pass-1) 
		{
			if (GetMulticolMaxRGB(data.col, 4, false) <= 0)
				continue;
		}
		else if (is_additive)
		{
			if (GetMulticolMaxRGB(data.col, 4, true) <= 0)
				continue;
		}

		GLuint fuzz_tex = is_fuzzy ? W_ImageCache(fuzz_image, false) : 0;

		local_gl_vert_t * glvert = RGL_BeginUnit(GL_POLYGON, 4,
				 is_additive ? ENV_SKIP_RGB : GL_MODULATE, tex_id,
				 is_fuzzy ? GL_MODULATE : ENV_NONE, fuzz_tex,
				 pass, blending);

		for (int v_idx=0; v_idx < 4; v_idx++)
		{
			local_gl_vert_t *dest = glvert + v_idx;

			dest->pos     = data.vert[v_idx];
			dest->texc[0] = data.texc[v_idx];
			dest->normal  = data.normal;

			if (is_fuzzy)
			{
				float ftx = (v_idx >= 2) ? (mo->radius * 2) : 0;
				float fty = (v_idx == 1 || v_idx == 2) ? (mo->height) : 0;

				dest->texc[1].x = ftx * fuzz_mul + fuzz_add.x;
				dest->texc[1].y = fty * fuzz_mul + fuzz_add.y;;

				dest->rgba[0] = dest->rgba[1] = dest->rgba[2] = 0;
			}
			else if (! is_additive)
			{
				dest->rgba[0] = data.col[v_idx].mod_R / 255.0;
				dest->rgba[1] = data.col[v_idx].mod_G / 255.0;
				dest->rgba[2] = data.col[v_idx].mod_B / 255.0;

				data.col[v_idx].mod_R -= 256;
				data.col[v_idx].mod_G -= 256;
				data.col[v_idx].mod_B -= 256;
			}
			else
			{
				dest->rgba[0] = data.col[v_idx].add_R / 255.0;
				dest->rgba[1] = data.col[v_idx].add_G / 255.0;
				dest->rgba[2] = data.col[v_idx].add_B / 255.0;
			}

			dest->rgba[3] = trans;
		}

		RGL_EndUnit(4);
	}
}


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


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
